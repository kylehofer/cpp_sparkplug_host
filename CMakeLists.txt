cmake_minimum_required(VERSION 3.17)

include(cmake/CPM.cmake)

SET(BUILD_TARGET LINUX)
SET(PAHO_BUILD_SHARED TRUE)

project(cpp_sparkplug_host C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

SET(FETCH_REMOTE ON CACHE BOOL "")
SET(CPP_SPARKPLUG_HOST_SHARED ON CACHE BOOL "")
SET(CPP_SPARKPLUG_HOST_STATIC OFF CACHE BOOL "")

# Finding all of our source files
file(GLOB_RECURSE SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "src/*.cpp")
file(GLOB_RECURSE HEADERS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "src/*.h*")

# Main library compiling
add_library(cpp_sparkplug_host SHARED ${SOURCES})

IF(FETCH_REMOTE)
    CPMAddPackage(
        pico_tahu
        GIT_REPOSITORY https://github.com/kylehofer/pico_tahu.git
        GIT_TAG main
        OPTIONS
        "BUILD_TARGET ${BUILD_TARGET}"
        "PICO_TAHU_STATIC ${CPP_SPARKPLUG_HOST_STATIC}"
        "PICO_TAHU_SHARED ${CPP_SPARKPLUG_HOST_SHARED}"
        "CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}"
        "CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE}"
    )
ELSE()
    add_subdirectory(./external/pico_tahu)
ENDIF()

CPMAddPackage(
    paho_mqtt_cpp
    GIT_REPOSITORY https://github.com/eclipse/paho.mqtt.cpp.git
    GIT_TAG v1.3.2
    OPTIONS
    "PAHO_BUILD_STATIC ${CPP_SPARKPLUG_HOST_STATIC}"
    "PAHO_BUILD_SHARED ${CPP_SPARKPLUG_HOST_SHARED}"
    "PAHO_WITH_SSL TRUE"
    "PAHO_HIGH_PERFORMANCE TRUE"
    "PAHO_BUILD_SAMPLES FALSE"
    "PAHO_ENABLE_TESTING FALSE"
    "PAHO_WITH_MQTT_C TRUE"
    "PAHO_BUILD_DOCUMENTATION FALSE"
    "CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}"
    "CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE}"
)

target_include_directories(cpp_sparkplug_host PUBLIC "${paho_mqtt_cpp_SOURCE_DIR}/src")

# Linking libraries
target_link_libraries(
    cpp_sparkplug_host
    pico_tahu
    paho-mqttpp3
)

set_target_properties(cpp_sparkplug_host PROPERTIES PUBLIC_HEADER "${HEADERS}")

INSTALL(TARGETS cpp_sparkplug_host
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})