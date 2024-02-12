cmake_minimum_required(VERSION 3.17)

include(FetchContent)
include(ExternalProject)

SET(BUILD_TARGET LINUX)
SET(PAHO_BUILD_SHARED TRUE)

project(cpp_sparkplug_host C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

# Finding all of our source files
file(GLOB_RECURSE SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "src/*.cpp")

# Main library compiling
add_library(cpp_sparkplug_host SHARED ${SOURCES})

IF(FETCH_REMOTE)
    FetchContent_Declare(
        pico_tahu
        GIT_REPOSITORY https://github.com/kylehofer/pico_tahu.git
        GIT_TAG propertySetSupport
    )
    FetchContent_MakeAvailable(pico_tahu)
ELSE()
    add_subdirectory(./external/pico_tahu)
ENDIF()

set(PAHO_BUILD_STATIC FALSE)
set(PAHO_BUILD_SHARED TRUE)
set(PAHO_WITH_SSL TRUE)
set(PAHO_HIGH_PERFORMANCE TRUE)
set(PAHO_BUILD_SAMPLES FALSE)
set(PAHO_ENABLE_TESTING FALSE)
set(PAHO_BUILD_DOCUMENTATION FALSE)
set(PAHO_WITH_MQTT_C TRUE)

# Grabbing paho MQTT
FetchContent_Declare(
    paho_mqtt_cpp
    GIT_REPOSITORY https://github.com/eclipse/paho.mqtt.cpp.git
    GIT_TAG v1.3.2 # release-1.4
)

FetchContent_MakeAvailable(paho_mqtt_cpp)

target_include_directories(cpp_sparkplug_host PUBLIC "${paho_mqtt_cpp_SOURCE_DIR}/src")

# Linking libraries
target_link_libraries(
    cpp_sparkplug_host
    pico_tahu
    paho-mqttpp3
)