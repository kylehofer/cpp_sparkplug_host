#!/usr/bin/env python3
"""
Runs the Eclipse Sparkplug TCK host profile against tck_host, without the TCK web console.

The web console is only an MQTT client: it publishes "NEW_TEST <profile> <test> <params>" to
SPARKPLUG_TCK/TEST_CONTROL, reads SPARKPLUG_TCK/RESULT, shows the operator the prompts on
SPARKPLUG_TCK/CONSOLE_PROMPT, and sends their PASS/FAIL answers to SPARKPLUG_TCK/CONSOLE_REPLY.
This script does the same, and answers each prompt by asking tck_host (over its stdin) to act or
to report what the library says.

    pip install -r tck/requirements.txt
    python3 tck/run_tck.py --host-app build/tck/tck_host --docker

Exit code: 0 when no assertion failed (other than those in --known-failures), 1 otherwise.
"""

import argparse
import os
import queue
import re
import signal
import subprocess
import sys
import threading
import time
from dataclasses import dataclass, field
from pathlib import Path

import paho.mqtt.client as mqtt

TOPIC_CONTROL = "SPARKPLUG_TCK/TEST_CONTROL"
TOPIC_RESULT = "SPARKPLUG_TCK/RESULT"
TOPIC_LOG = "SPARKPLUG_TCK/LOG"
TOPIC_PROMPT = "SPARKPLUG_TCK/CONSOLE_PROMPT"
TOPIC_REPLY = "SPARKPLUG_TCK/CONSOLE_REPLY"


@dataclass
class HostTest:
    name: str
    # Extra NEW_TEST parameters after the host ID (the edge node/device ones are added per test)
    edge_params: bool = False
    extra_params: list = field(default_factory=list)
    # Extra tck_host flags
    host_args: list = field(default_factory=list)
    # Start the host before NEW_TEST (the test checks it is online) instead of after it
    host_first: bool = False
    # Seconds to wait for the TCK to end the test by itself before sending END_TEST
    timeout: float = 30.0
    description: str = ""


# The host profile, in the order the TCK user guide lists it
HOST_TESTS = [
    HostTest("SessionEstablishmentTest",
             description="Connect after the test starts: Will, subscriptions and the online STATE"),
    HostTest("SessionTerminationTest", host_args=["--stop-after", "3"],
             description="Come online, then stop intentionally with SparkplugHost::stop()"),
    HostTest("SendCommandTest", edge_params=True, host_first=True, timeout=60.0,
             description="Send node and device rebirths and commands to the TCK's simulated edge node"),
    HostTest("EdgeSessionTerminationTest", edge_params=True, host_first=True, timeout=60.0,
             description="Take the device, then the node, offline on their DDEATH and NDEATH"),
    # The reorder timeout is the TCK's parameter for how long the host waits for a missing message
    HostTest("MessageOrderingTest", edge_params=True, extra_params=["5000"], host_first=True, timeout=60.0,
             description="Ask for a rebirth when a sequence number is missing"),
    HostTest("MultipleBrokerTest", timeout=60.0,
             description="Publish STATE on two brokers, and come back after the TCK disconnects it"),
]

SECOND_BROKER_TESTS = {"MultipleBrokerTest"}

# Second broker for MultipleBrokerTest: a plain broker, which the TCK reaches through its own client.
# With --docker both brokers share a Docker network, so the TCK can reach it by name.
BROKER2_IMAGE = "eclipse-mosquitto:2.0.20"

RESULT_LINE = re.compile(r"^(?P<id>[^:\s][^:]*(?::[^:\s][^:]*)?):\s*(?P<verdict>PASS|FAIL|NOT EXECUTED)(?P<detail>.*?);?\s*$")

# Console prompts, and the tck_host request that answers each. {node} and {device} come from the prompt
# or, for the EdgeSessionTerminationTest prompts that do not name them, from the test's parameters.
PROMPTS = [
    (re.compile(r"Send an edge rebirth to edge node (?P<node>\S+)"), "rebirth {node}"),
    (re.compile(r"Send an edge command to edge node (?P<node>\S+) to update a metric value"), "ncmd {node}"),
    (re.compile(r"Send a device rebirth command to device (?P<device>\S+) at edge node (?P<node>\S+)"),
     "drebirth {node} {device}"),
    (re.compile(r"Send a device command to device (?P<device>\S+) at edge node (?P<node>\S+) to update"),
     "dcmd {node} {device}"),
    # After a DDEATH: the device is offline, and its metrics stale
    (re.compile(r"receiving an DDEATH"), "offline {node}/{device}"),
    # After an NDEATH: the devices are offline (and their metrics stale), or the node is (and its metrics)
    (re.compile(r"receiving an NDEATH.*Sparkplug Devices", re.S), "offline {node}/{device}"),
    (re.compile(r"receiving an NDEATH"), "offline {node}"),
]

# Prompts that ask a question, so their answer goes back to the TCK on CONSOLE_REPLY
QUESTION = re.compile(r"\bMUST\b")


def log(message):
    print(f"[run_tck] {message}", flush=True)


class TckConnection:
    """MQTT connection to the TCK broker. Collects RESULT and CONSOLE_PROMPT messages as events."""

    def __init__(self, host, port):
        self.events = queue.Queue()
        self.lines = []
        self.lines_lock = threading.Lock()
        self.connected = threading.Event()
        self.client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2,
                                  client_id=f"cpp_sparkplug_host_tck_runner_{os.getpid()}")
        self.client.on_connect = self._on_connect
        self.client.on_message = self._on_message
        self.client.connect(host, port, keepalive=30)
        self.client.loop_start()
        if not self.connected.wait(10):
            raise RuntimeError(f"Could not connect to the TCK broker at {host}:{port}")

    def _on_connect(self, client, userdata, flags, reason_code, properties):
        if reason_code == 0:
            client.subscribe([(TOPIC_RESULT, 1), (TOPIC_LOG, 1), (TOPIC_PROMPT, 1)])
            self.connected.set()

    def _on_message(self, client, userdata, message):
        text = message.payload.decode("utf-8", errors="replace")
        if message.topic in (TOPIC_RESULT, TOPIC_PROMPT):
            self.events.put((message.topic, text))
        self.note(f"[{message.topic}] {text}")

    def note(self, line):
        with self.lines_lock:
            self.lines.append(line)

    def take_lines(self):
        with self.lines_lock:
            lines, self.lines = self.lines, []
        return lines

    def drain(self):
        while not self.events.empty():
            self.events.get_nowait()

    def control(self, command):
        log(f"TEST_CONTROL: {command}")
        self.client.publish(TOPIC_CONTROL, command, qos=1).wait_for_publish(5)

    def reply(self, answer):
        log(f"CONSOLE_REPLY: {answer}")
        self.note(f"[{TOPIC_REPLY}] {answer}")
        self.client.publish(TOPIC_REPLY, answer, qos=1).wait_for_publish(5)

    def close(self):
        self.client.loop_stop()
        self.client.disconnect()


class HostProcess:
    """tck_host: requests on its stdin, one reply line each on its stdout, its log (stderr) in a file."""

    def __init__(self, command, output_path):
        self.output = open(output_path, "w")
        self.replies = queue.Queue()
        log("host: " + " ".join(command))
        self.process = subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                        stderr=self.output, text=True, bufsize=1)
        threading.Thread(target=self._read, daemon=True).start()

    def _read(self):
        for line in self.process.stdout:
            self.replies.put(line.strip())

    def request(self, line, timeout=10):
        if self.process.poll() is not None:
            return "ERROR tck_host is not running"
        self.process.stdin.write(line + "\n")
        self.process.stdin.flush()
        try:
            return self.replies.get(timeout=timeout)
        except queue.Empty:
            return "ERROR no reply from tck_host"

    def stop(self):
        if self.process.poll() is None:
            self.process.send_signal(signal.SIGTERM)
            try:
                self.process.wait(10)
            except subprocess.TimeoutExpired:
                self.process.kill()
                self.process.wait()
        self.output.close()
        return self.process.returncode


def answer_prompt(tck, host, prompt, node, device):
    """Turns a console prompt into a tck_host request, and sends the TCK the answer when it asks one."""
    for pattern, template in PROMPTS:
        match = pattern.search(prompt)
        if not match:
            continue
        values = {"node": node, "device": device, **match.groupdict()}
        request = template.format(**values)
        if host is None:
            reply = "ERROR tck_host is not running"
        else:
            reply = host.request(request)
        log(f"prompt -> {request} -> {reply}")
        tck.note(f"[tck_host] {request} -> {reply}")
        if QUESTION.search(prompt):
            tck.reply("PASS" if reply == "PASS" else "FAIL")
        return
    log(f"no answer for prompt: {prompt}")


def parse_result(text):
    """Returns ({assertion id: (verdict, detail)}, overall line) from a RESULT payload."""
    assertions = {}
    overall = ""
    for line in text.splitlines()[1:]:
        line = line.strip()
        if not line:
            continue
        if line.startswith("OVERALL:"):
            overall = line[len("OVERALL:"):].strip().rstrip(";")
            continue
        match = RESULT_LINE.match(line)
        if match:
            assertions[match["id"].strip()] = (match["verdict"], match["detail"].strip())
    return assertions, overall


def load_known_failures(path):
    known = set()
    if path:
        for line in Path(path).read_text().splitlines():
            line = line.split("#", 1)[0].strip()
            if line:
                known.add(line)
    return known


def docker_quiet(*command):
    subprocess.run(["docker", *command], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)


def wait_for_log(container, *needles, timeout=60):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        logs = subprocess.run(["docker", "logs", container], capture_output=True, text=True)
        output = logs.stdout + logs.stderr
        if all(needle in output for needle in needles):
            return
        time.sleep(1)
    raise RuntimeError(f"{container} did not start within {timeout} s")


def broker2_name(args):
    return f"{args.docker_name}-broker2"


def start_brokers(args, second_broker):
    """Starts the TCK broker container (tck/broker), and the second broker when it is needed."""
    stop_brokers(args, None)
    if args.docker_build:
        log(f"building {args.docker_image}")
        subprocess.run(["docker", "build", "-t", args.docker_image, str(Path(__file__).parent / "broker")], check=True)
    subprocess.run(["docker", "network", "create", args.docker_name], check=True, stdout=subprocess.DEVNULL)

    log(f"starting {args.docker_image} on port {args.port}")
    subprocess.run(["docker", "run", "-d", "--name", args.docker_name, "--network", args.docker_name,
                    "-p", f"{args.port}:1883", args.docker_image], check=True, stdout=subprocess.DEVNULL)
    if second_broker:
        log(f"starting second broker {BROKER2_IMAGE} on port {args.port2}")
        subprocess.run(["docker", "run", "-d", "--name", broker2_name(args), "--network", args.docker_name,
                        "-p", f"{args.port2}:1883", BROKER2_IMAGE, "mosquitto", "-c", "/mosquitto-no-auth.conf"],
                       check=True, stdout=subprocess.DEVNULL)

    wait_for_log(args.docker_name, "Sparkplug", "started successfully", "Started TCP Listener")
    if second_broker:
        wait_for_log(broker2_name(args), "running")


def stop_brokers(args, output_dir):
    """Saves the broker logs (when output_dir is given) and removes the containers and network."""
    for container, log_name in ((args.docker_name, "hivemq.log"), (broker2_name(args), "broker2.log")):
        if output_dir:
            logs = subprocess.run(["docker", "logs", container], capture_output=True, text=True)
            if logs.returncode == 0:
                (output_dir / log_name).write_text(logs.stdout + logs.stderr)
        docker_quiet("rm", "-f", container)
    docker_quiet("network", "rm", args.docker_name)


def run_test(tck, test, index, args, output_dir):
    """Runs one TCK test. Returns the RESULT payload, or None if the TCK never reported."""
    # A new edge node ID per test: the TCK tracks bdSeq per edge node for the life of the broker
    node = f"{args.node}{index}"
    params = [args.host_id]
    if test.edge_params:
        params += [args.group, node, args.device]
    params += test.extra_params

    host_command = [args.host_app, "--broker", f"tcp://{args.host}:{args.port}", "--host-id", args.host_id,
                    "--client-id", f"tck_host_{index}", "--group", args.group] + test.host_args
    if test.name in SECOND_BROKER_TESTS:
        # The TCK reaches the second broker at broker2_url; the host at host:port2
        params.append(args.broker2_url)
        host_command += ["--broker", f"tcp://{args.host}:{args.port2}"]

    tck.drain()
    tck.take_lines()
    host = None
    result = None
    try:
        if test.host_first:
            host = HostProcess(host_command, output_dir / f"{test.name}.host.log")
            time.sleep(3)  # connected, and its online STATE published, before the test starts
        tck.control(f"NEW_TEST host {test.name} {' '.join(params)}")
        if not test.host_first:
            time.sleep(1)  # the TCK must be watching for the host's CONNECT
            host = HostProcess(host_command, output_dir / f"{test.name}.host.log")

        deadline = time.monotonic() + test.timeout
        ending = False
        while result is None:
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                if ending:
                    log(f"{test.name}: no result from the TCK")
                    break
                log(f"{test.name} did not end by itself within {test.timeout:.0f} s, sending END_TEST")
                tck.control("END_TEST")
                ending = True
                deadline = time.monotonic() + 15
                continue
            try:
                topic, text = tck.events.get(timeout=remaining)
            except queue.Empty:
                continue
            if topic == TOPIC_RESULT:
                result = text
            elif topic == TOPIC_PROMPT:
                answer_prompt(tck, host, text, node, args.device)
    finally:
        if host:
            host.stop()
        time.sleep(1)
        (output_dir / f"{test.name}.tck.log").write_text("\n".join(tck.take_lines()) + "\n")
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--host-app", required=True, help="Path to tck_host")
    parser.add_argument("--host", default="localhost", help="TCK broker host")
    parser.add_argument("--port", type=int, default=1886, help="TCK broker MQTT port (default 1886)")
    parser.add_argument("--port2", type=int, default=1887,
                        help="Second broker's MQTT port, for MultipleBrokerTest (default 1887)")
    parser.add_argument("--broker2-url",
                        help="Second broker's URL as the TCK broker sees it, for MultipleBrokerTest. "
                             "With --docker it defaults to the second broker container")
    parser.add_argument("--group", default="Test", help="Sparkplug group ID of the TCK's edge node (default Test)")
    parser.add_argument("--node", default="TckHostNode", help="Edge node ID prefix; the test number is appended")
    parser.add_argument("--device", default="TckDevice")
    parser.add_argument("--host-id", default="TckHost", help="Host Application ID of tck_host")
    parser.add_argument("--tests", nargs="+", metavar="TEST", help="Run only these tests")
    parser.add_argument("--output-dir", default="build/tck/results", help="Logs and results go here")
    parser.add_argument("--known-failures", help="File of assertion ids that are expected to fail")
    parser.add_argument("--docker", action="store_true", help="Start and stop the TCK broker container")
    parser.add_argument("--docker-build", action="store_true", help="Build the broker image first (with --docker)")
    parser.add_argument("--docker-image", default="cpp-sparkplug-host-tck")
    parser.add_argument("--docker-name", default="cpp-sparkplug-host-tck",
                        help="Name of the broker container, and of the Docker network it uses")
    args = parser.parse_args()

    tests = HOST_TESTS
    if args.tests:
        known_names = {t.name for t in HOST_TESTS}
        unknown = [name for name in args.tests if name not in known_names]
        if unknown:
            parser.error(f"unknown test(s): {', '.join(unknown)}. Available: {', '.join(sorted(known_names))}")
        tests = [t for t in HOST_TESTS if t.name in args.tests]

    if args.docker and not args.broker2_url:
        args.broker2_url = f"tcp://{broker2_name(args)}:1883"
    skipped = {}
    if not args.broker2_url:
        for test in tests:
            if test.name in SECOND_BROKER_TESTS:
                skipped[test.name] = "needs a second broker: use --docker, or pass --broker2-url"
        tests = [t for t in tests if t.name not in skipped]
    second_broker = any(t.name in SECOND_BROKER_TESTS for t in tests)

    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    results_log = output_dir / "SparkplugTCKresults.log"
    results_log.unlink(missing_ok=True)
    known_failures = load_known_failures(args.known_failures)

    if args.docker:
        start_brokers(args, second_broker)

    summary = []  # (test, overall, {id: (verdict, detail)})
    try:
        tck = TckConnection(args.host, args.port)
        try:
            for test in tests:
                log(f"=== {test.name}: {test.description}")
                result = run_test(tck, test, HOST_TESTS.index(test) + 1, args, output_dir)
                if result is None:
                    summary.append((test.name, "NO RESULT", {}))
                    continue
                # Same format the TCK writes to SparkplugTCKresults.log, so its report.py can read it
                with results_log.open("a") as f:
                    f.write(result + "\n")
                assertions, overall = parse_result(result)
                summary.append((test.name, overall, assertions))
                time.sleep(2)  # let the TCK take its simulated edge node offline before the next test
        finally:
            tck.close()
    finally:
        if args.docker:
            stop_brokers(args, output_dir)

    # ---- Report ----
    new_failures = []  # (test, assertion, detail)
    known_seen = set()
    print(f"\n{'Test':<28} {'Pass':>5} {'Fail':>5} {'N/E':>5}  Overall")
    for name, overall, assertions in summary:
        verdicts = [v for v, _ in assertions.values()]
        print(f"{name:<28} {verdicts.count('PASS'):>5} {verdicts.count('FAIL'):>5} "
              f"{verdicts.count('NOT EXECUTED'):>5}  {overall}")
        if not assertions:
            new_failures.append((name, "(no result)", "the TCK did not report"))
        for assertion, (verdict, detail) in sorted(assertions.items()):
            if verdict != "FAIL":
                continue
            if assertion in known_failures:
                known_seen.add(assertion)
            else:
                new_failures.append((name, assertion, detail))
    for name, reason in skipped.items():
        print(f"{name:<28} {'-':>5} {'-':>5} {'-':>5}  SKIPPED ({reason})")

    if known_seen:
        print(f"\nKnown failures ({len(known_seen)}): {', '.join(sorted(known_seen))}")
    fixed = sorted(known_failures - known_seen)
    if fixed and not args.tests:
        print(f"\nIn --known-failures but not failing any more ({len(fixed)}), remove them: {', '.join(fixed)}")
    if new_failures:
        print(f"\nFailures ({len(new_failures)}):")
        for name, assertion, detail in new_failures:
            print(f"  {name}: {assertion} {detail}")
    print(f"\nLogs and SparkplugTCKresults.log: {output_dir}/")
    return 1 if new_failures else 0


if __name__ == "__main__":
    sys.exit(main())
