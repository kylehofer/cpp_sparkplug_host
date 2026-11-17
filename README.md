
# cpp_sparkplug_host - A C++ Sparkplug Primary Host library

A Sparkplug B (3.0) Host Application for tracking data from Sparkplug Edge Nodes and Devices.
The host connects to an MQTT broker, keeps the latest state and metrics of every Node and Device it sees,
validates sequence numbers, and asks Nodes to rebirth when its view of them falls out of date.
Your application polls the host for what changed, or for a full snapshot.

## WIP
This project is designed as a testing platform for my other Sparkplug Projects.

## Software Goals
- [X] Basic Metric Support
- [X] Basic Property Support
- [X] State Management
- [X] Sequence Management
- [X] Basic Commands
- [X] Rebirth Control
- [X] Metric Aliases
- [X] Automated Tests
- [ ] Complex Metric Support (DataSet, Template, Bytes, File, UUID values are reported as null)
- [ ] Complex Property Support (PropertySetList values are reported as null)

## Usage

```cpp
#include "SparkplugHost.h"
#include <thread>

int main()
{
    // The third argument is the Sparkplug Host Id. With it, the host publishes
    // spBv1.0/STATE/my-host and acts as the Primary Host. Leave it out for a passive host.
    SparkplugHost host("tcp://localhost:1883", "my-host-client", "my-host");
    host.credentials("user", "password"); // optional

    // run() blocks until stop() is called, so give it a thread
    std::thread loop([&host] { host.run(); });

    while (keepRunning)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        for (const PublishableUpdate &update : host.getPayloads())
        {
            // update.id is "group/node" or "group/node/device"
            // update.type is BIRTH, PUBLISH or DEATH
            // update.payload holds the metrics (null for DEATH)
        }
        // Updates own their payloads and free them when destroyed
    }

    host.stop();
    loop.join();
}
```

A complete program that prints every change is in [examples/basic_host.cpp](examples/basic_host.cpp).

### Reading updates

`getPayloads()` returns one `PublishableUpdate` for each Node or Device that changed since the previous call:

| Type | When | Payload |
| --- | --- | --- |
| `BIRTH` | The Node or Device sent an NBIRTH/DBIRTH | Every metric |
| `PUBLISH` | Metrics changed through NDATA/DDATA | Only the changed metrics |
| `DEATH` | The Node or Device went offline | None (`payload` is null) |

`getPayloads(true)` returns a full snapshot instead: a `BIRTH` for every online entity and a `DEATH`
for every offline one. A snapshot does not consume pending changes, so the next normal call still reports them.

Metrics in update payloads always carry their name, never just an alias. Metrics with a datatype the
library does not store (DataSet, Template, Bytes, File, UUID) are reported with `is_null` set.

### Sending commands

```cpp
tahu::PayloadPtr payload = tahu::makePayload();
bool value = true;
add_simple_metric(payload.get(), "Output/Enable", false, 0, METRIC_DATA_TYPE_BOOLEAN,
                  false, false, &value, sizeof(value));

host.commandNode("Plant1", "Edge1", std::move(payload));                 // NCMD
host.commandDevice("Plant1", "Edge1", "Pump", std::move(otherPayload));  // DCMD
host.requestRebirth("Plant1", "Edge1");                                  // Node Control/Rebirth
```

Commands are queued and sent on the next loop iteration while connected. Commands queued while
disconnected are kept until the connection returns. The payload timestamp is set when the command is sent.
`command(SparkplugMessage)` is still available for publishing to an arbitrary topic.

### Thread safety

`run()` must only be running on one thread at a time. Every other member function of `SparkplugHost`
can be called from any thread, including while `run()` is executing.

`stop()` returns immediately and `run()` finishes within about 100 ms, after publishing the offline STATE
and disconnecting. Join the thread running `run()` before destroying the host.

`configure()` and `credentials()` take effect on the next loop iteration. The host disconnects, forgets
every Node and Device it was tracking, and reconnects with the new settings.

### Memory ownership

- `PublishableUpdate` owns its payload. Do not call `free_payload()` or `free()` on it.
- `commandNode()`, `commandDevice()` and `command()` take ownership of the payload you pass.
- `tahu::PayloadPtr` is a `std::unique_ptr` that calls `free_payload()` and `free()`. Create empty payloads
  with `tahu::makePayload()`. A `SparkplugMessage` built from a raw `tahu::Payload *` takes ownership of it,
  so the pointer must come from `malloc`/`calloc`.

### Logging

The library logs through [spdlog](https://github.com/gabime/spdlog), using a logger named `cpp_sparkplug_host`.
By default, warnings and errors go to stderr. You don't need to use spdlog yourself: the `Logger` class controls it.

| Where | How | Notes |
| --- | --- | --- |
| Build | `-DCPP_SPARKPLUG_HOST_LOG_LEVEL=debug` | The most verbose level compiled in. More verbose calls are removed from the binary. Default `debug`. |
| Environment | `CPP_SPARKPLUG_HOST_LOG_LEVEL=info ./app` | Read the first time the library logs. |
| Environment | `SPDLOG_LEVEL=cpp_sparkplug_host=info ./app` | spdlog's standard variable; `SPDLOG_LEVEL=info` sets every spdlog logger. |
| Command line | `./app CPP_SPARKPLUG_HOST_LOG_LEVEL=info` | Only if the application calls `Logger::loadArgv(argc, argv)`. |
| Code | `Logger::setLevel(LogLevel::Info)` | |

Levels are `trace`, `debug`, `info`, `warn`, `error`, `critical` and `off`. Later rows override earlier ones,
but nothing can enable a level that was compiled out; `Logger::compiledLevel()` reports what was compiled in.

```cpp
Logger::loadArgv(argc, argv);         // honour CPP_SPARKPLUG_HOST_LOG_LEVEL=... arguments
Logger::setLevel(LogLevel::Debug);    // Trace, Debug, Info, Warning (default), Error, None
Logger::setSink([](LogLevel level, const std::string &message) {
    myLogger.write(level, message);   // route into your own logging; nullptr restores stderr
});
```

spdlog is linked privately. In shared builds its symbols are not exported, so your application can use a
different spdlog version without conflicts.

## Sparkplug behaviour

- **STATE.** With a host id, the host connects with a Will of `{"online":false,...}` on `spBv1.0/STATE/<host_id>`
  (QoS 1, retained), subscribes to that topic at QoS 1, and publishes `{"online":true,...}` once subscribed.
  If anything marks the host offline while it is running, it publishes online again.
  On `stop()` it publishes offline before disconnecting.
- **Sequence numbers.** `seq` is checked across all of a Node's messages, including device messages, and wraps from 255 to 0.
  On a gap the host requests a rebirth and resynchronises to the received `seq`, so one lost message is
  reported once rather than on every following message.
- **Rebirths.** Requested when a Node is out of sequence, sends data while offline or unknown, sends a metric or alias
  that was not in its birth, changes a metric's datatype, or sends data for an unknown device.
  At most one request is sent per Node every 5 seconds until its NBIRTH arrives
  (`SparkplugHost::setRebirthRetryInterval()` changes this).
- **Births and deaths.** An NBIRTH replaces all of the Node's metrics and aliases and takes its Devices offline
  until they send a new DBIRTH. An NDEATH takes the Node and all of its Devices offline. An NDEATH whose
  `bdSeq` does not match the current NBIRTH comes from an earlier session and is ignored.
- **Aliases.** Aliases defined in a birth are resolved in later data messages. An unknown alias triggers a rebirth.
- **Datatypes.** Data messages may omit a metric's datatype; the type from the birth is used.
- **Connection loss.** When the host loses its broker connection it marks every Node and Device offline.
- **MQTT session.** MQTT 5 with Clean Start and a Session Expiry Interval of 0, as Sparkplug requires.
  Reconnects happen automatically, backing off from 1 to 10 seconds.

## Building

This library uses CMake for building and dependency management. It requires a C++20 compiler.

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build        # run the unit tests
./build/basic_host tcp://localhost:1883 my-host CPP_SPARKPLUG_HOST_LOG_LEVEL=debug
```

The following build options are supported:

| Option | Default | Description |
| ------------- | ------------- |  ------------- |
| FETCH_REMOTE | ON | Fetch pico_tahu with CPM. If disabled, pico_tahu must be placed in `{PROJECT_ROOT}/external/pico_tahu`. |
| CPP_SPARKPLUG_HOST_SHARED | ON | Build shared libraries. Set to OFF to build static libraries. |
| CPP_SPARKPLUG_HOST_STATIC | OFF | Deprecated. Same as `CPP_SPARKPLUG_HOST_SHARED=OFF`. |
| CPP_SPARKPLUG_HOST_BUILD_TESTS | ON when top level | Build the unit tests (`cpp_sparkplug_host_tests`). |
| CPP_SPARKPLUG_HOST_BUILD_EXAMPLES | ON when top level | Build `examples/basic_host`. |
| CPP_SPARKPLUG_HOST_LOG_LEVEL | debug | Most verbose log level compiled in: trace, debug, info, warn, error, critical, off. |

### Using the library from another CMake project

```cmake
add_subdirectory(cpp_sparkplug_host)   # or CPMAddPackage(...)
target_link_libraries(my_app PRIVATE cpp_sparkplug_host::cpp_sparkplug_host)
```

`cmake --install` places the library in `lib/` and the headers in `include/cpp_sparkplug_host/`,
keeping their directory layout. pico_tahu installs `tahu.h` into `include/`.

## Dependencies
The following dependencies will be pulled and built by cmake:
- https://github.com/kylehofer/pico_tahu.git (pinned to a commit)
- https://github.com/eclipse/paho.mqtt.cpp.git (v1.5.3)
- https://github.com/gabime/spdlog.git (v1.15.3)

Paho and spdlog are only used internally; the public headers depend on pico_tahu only.

See [CHANGELOG.md](CHANGELOG.md) for changes and migration notes.
