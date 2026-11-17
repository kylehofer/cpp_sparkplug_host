# Sparkplug TCK

Runs the [Eclipse Sparkplug TCK](https://github.com/eclipse-sparkplug/sparkplug/tree/master/tck) host
profile against cpp_sparkplug_host, with no browser. CI runs it on every push (`.github/workflows/ci.yml`).

The TCK is a HiveMQ extension. Its web console is only an MQTT client: it publishes
`NEW_TEST host <test> <params>` to `SPARKPLUG_TCK/TEST_CONTROL`, shows the operator what arrives on
`SPARKPLUG_TCK/CONSOLE_PROMPT`, sends their answers to `SPARKPLUG_TCK/CONSOLE_REPLY`, and reads the
result from `SPARKPLUG_TCK/RESULT`. `run_tck.py` does the same, and answers each prompt by asking the
host application.

| Path | What it is |
|---|---|
| `host_app/tck_host.cpp` | A Host Application built from the public API: one `SparkplugHost` per `--broker`, following Node and Device state through `getPayloads()`. Takes requests from the runner on stdin (send a rebirth or command, report whether something went offline) |
| `broker/` | Docker image: HiveMQ CE with the TCK extension, both pinned (`HIVEMQ_VERSION`, `TCK_REF` build args). The same image as cpp_sparkplug's edge TCK |
| `run_tck.py` | Runs each host test, answers its prompts, collects results, exits 1 on any failure not in `--known-failures` |
| `known_failures.txt` | Assertions expected to fail (empty) |

## Running locally

Needs Docker, CMake and Python 3.

```sh
# Host application (a standalone CMake project, so the library's own build is untouched)
cmake -S tck -B build/tck
cmake --build build/tck -j --target tck_host

# Broker image (the first build takes a few minutes: it builds the TCK from source)
docker build -t cpp-sparkplug-host-tck tck/broker

# Runner
python3 -m venv build/tck/venv
build/tck/venv/bin/pip install -r tck/requirements.txt
build/tck/venv/bin/python tck/run_tck.py --host-app build/tck/tck_host --docker
```

`--docker` starts the broker on port 1886, and for MultipleBrokerTest a plain `eclipse-mosquitto` on
1887, both on a Docker network so the TCK can reach the second broker by name. These are not the edge
TCK's ports (1884/1885), so both suites can run at once. `--tests SendCommandTest ...` runs a subset.

Results go to `build/tck/results/`:
- `SparkplugTCKresults.log`: every test's result, in the TCK's own format (its `report.py` reads it)
- `<Test>.tck.log`: what the TCK logged, the prompts, and how each was answered
- `<Test>.host.log`: tck_host's output (the library logs at debug level)
- `hivemq.log`, `broker2.log`: the brokers' logs

The TCK's simulated edge node uses group `Test` and edge node `TckHostNode<n>`, a new one per test,
because the TCK tracks bdSeq per edge node for as long as the broker runs.

## Tests

| Test | What tck_host does |
|---|---|
| SessionEstablishmentTest | Connects after the test starts: Will, subscriptions, online STATE |
| SessionTerminationTest | Comes online, then stops with `SparkplugHost::stop()` after 3 s |
| SendCommandTest | Running before the test. Answers the prompts with `requestRebirth()`, `commandNode()` and `commandDevice()`, writing a metric from the NBIRTH/DBIRTH |
| EdgeSessionTerminationTest | Running before the test. The TCK asks whether the device and node are offline (and their metrics stale) after DDEATH and NDEATH; tck_host answers from its `DEATH` updates |
| MessageOrderingTest | Running before the test. Asks for a rebirth when a DDATA sequence number is missing |
| MultipleBrokerTest | Connects to both brokers after the test starts, and reconnects when the TCK disconnects it |

## How prompts are answered

The prompts are written for a person, so `run_tck.py` matches their text (`PROMPTS`) and turns each
into a request to tck_host. For the "MUST mark ... offline / STALE" questions, it answers PASS only if
the library delivered a `DEATH` update for that Node or Device within 5 s. The library has no separate
stale flag: after a `DEATH`, none of the entity's metrics are current until its next `BIRTH`. The
"using the timestamp in the DDEATH payload" part of those questions is not checked, since `DEATH`
updates carry no payload.

## Expected NOT EXECUTED assertions

- `message-flow-phid-sparkplug-clean-session-311`: for MQTT 3.1.1 hosts; this one uses MQTT 5.
- `host-topic-phid-death-payload-timestamp-disconnect-with-no-disconnect-packet`: needs the host to
  drop without a DISCONNECT; tck_host always stops cleanly.

These are reported as PASS but INCOMPLETE, and do not fail the run.
