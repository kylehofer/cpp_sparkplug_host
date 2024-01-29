
# cpp_sparkplug - A C++ Sparkplug library
A basic Sparkplug Primary Host application for managing data from Sparkplug Clients. Tracks the data from multiple clients managing their data and state. Data from all activate clients can be either retrieved as complete data, or only the changes from the last call.

## WIP
This project is designed as a testing platform for my other Sparkplug Projects. 

## Software Goals
- [X] Basic Metric Support
- [X] Basic Property Support
- [X] State Management
- [X] Sequence Management
- [X] Basic Commands
- [X] Rebirth Control
- [ ] Complex Metric Support
- [ ] Complex Property Support
- [ ] Automated Tests

## Building
This library uses cmake for building and dependency management.
The following build flags are supported:
| Flag  | Default | Description |
| ------------- | ------------- |  ------------- |
| FETCH_REMOTE | ON | Whether to fetch remote dependencies through cmake. If disabled, the remote dependencies can be put within {PROJECT_ROOT}/external. |

## Dependencies
The following dependencies will be pulled and built by cmake:
- https://github.com/kylehofer/pico_tahu.git
- https://github.com/eclipse/paho.mqtt.cpp.git
