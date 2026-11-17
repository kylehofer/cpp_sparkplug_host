cmake_minimum_required(VERSION 3.17)

project(cpp_sparkplug_host VERSION 0.2.0 LANGUAGES C CXX)

include(GNUInstallDirs)
include(cmake/CPM.cmake)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(CPP_SPARKPLUG_HOST_TOP_LEVEL ON)
else()
    set(CPP_SPARKPLUG_HOST_TOP_LEVEL OFF)
endif()

option(FETCH_REMOTE "Fetch pico_tahu with CPM. When OFF, pico_tahu must be placed in external/pico_tahu." ON)
option(CPP_SPARKPLUG_HOST_SHARED "Build shared libraries. OFF builds static libraries." ON)
option(CPP_SPARKPLUG_HOST_STATIC "Deprecated: equivalent to CPP_SPARKPLUG_HOST_SHARED=OFF." OFF)
option(CPP_SPARKPLUG_HOST_BUILD_TESTS "Build the unit tests." ${CPP_SPARKPLUG_HOST_TOP_LEVEL})
option(CPP_SPARKPLUG_HOST_BUILD_EXAMPLES "Build the examples." ${CPP_SPARKPLUG_HOST_TOP_LEVEL})
set(BUILD_TARGET LINUX CACHE STRING "Target platform passed to pico_tahu.")
set(CPP_SPARKPLUG_HOST_LOG_LEVEL debug CACHE STRING
    "Most verbose log level compiled into the library: trace, debug, info, warn, error, critical or off.")
set_property(CACHE CPP_SPARKPLUG_HOST_LOG_LEVEL PROPERTY STRINGS trace debug info warn error critical off)

string(TOUPPER "${CPP_SPARKPLUG_HOST_LOG_LEVEL}" SPH_LOG_LEVEL_UPPER)
if(NOT SPH_LOG_LEVEL_UPPER MATCHES "^(TRACE|DEBUG|INFO|WARN|ERROR|CRITICAL|OFF)$")
    message(FATAL_ERROR "CPP_SPARKPLUG_HOST_LOG_LEVEL must be one of trace, debug, info, warn, error, critical, off")
endif()

if(CPP_SPARKPLUG_HOST_STATIC)
    set(CPP_SPARKPLUG_HOST_SHARED OFF)
endif()

if(CPP_SPARKPLUG_HOST_SHARED)
    set(LIBRARY_TYPE SHARED)
    set(BUILD_STATIC OFF)
else()
    set(LIBRARY_TYPE STATIC)
    set(BUILD_STATIC ON)
endif()

# --- Dependencies ---

if(FETCH_REMOTE)
    CPMAddPackage(
        NAME pico_tahu
        GIT_REPOSITORY https://github.com/kylehofer/pico_tahu.git
        GIT_TAG e6acaeb699a0a62e66297dfb19bcfa7496990cf2
        OPTIONS
        "BUILD_TARGET ${BUILD_TARGET}"
        "PICO_TAHU_STATIC ${BUILD_STATIC}"
        "PICO_TAHU_SHARED ${CPP_SPARKPLUG_HOST_SHARED}"
    )
else()
    set(PICO_TAHU_STATIC ${BUILD_STATIC})
    set(PICO_TAHU_SHARED ${CPP_SPARKPLUG_HOST_SHARED})
    add_subdirectory(external/pico_tahu)
endif()

CPMAddPackage(
    NAME paho_mqtt_cpp
    GIT_REPOSITORY https://github.com/eclipse/paho.mqtt.cpp.git
    GIT_TAG v1.5.3
    OPTIONS
    "PAHO_BUILD_STATIC ${BUILD_STATIC}"
    "PAHO_BUILD_SHARED ${CPP_SPARKPLUG_HOST_SHARED}"
    "PAHO_WITH_SSL TRUE"
    "PAHO_HIGH_PERFORMANCE TRUE"
    "PAHO_BUILD_SAMPLES FALSE"
    "PAHO_ENABLE_TESTING FALSE"
    "PAHO_WITH_MQTT_C TRUE"
    "PAHO_BUILD_DOCUMENTATION FALSE"
)

# Built as a static, position independent library and linked privately. In shared builds its
# symbols are hidden, so applications can use their own spdlog version without conflicts.
CPMAddPackage(
    NAME spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.15.3
    OPTIONS
    "SPDLOG_BUILD_SHARED OFF"
    "SPDLOG_BUILD_PIC ON"
    "SPDLOG_INSTALL ${BUILD_STATIC}"
)

# --- Library ---

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp")

add_library(cpp_sparkplug_host ${LIBRARY_TYPE} ${SOURCES})
add_library(cpp_sparkplug_host::cpp_sparkplug_host ALIAS cpp_sparkplug_host)

target_include_directories(cpp_sparkplug_host PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/cpp_sparkplug_host>
)

# The public headers include tahu.h; paho and spdlog are only used internally
target_link_libraries(cpp_sparkplug_host
    PUBLIC pico_tahu
    PRIVATE PahoMqttCpp::paho-mqttpp3 spdlog::spdlog
)

target_compile_definitions(cpp_sparkplug_host PRIVATE SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_${SPH_LOG_LEVEL_UPPER})

# Keep spdlog and fmt out of the shared library's exported symbols, so applications can use their own
# spdlog version alongside this library.
if(CPP_SPARKPLUG_HOST_SHARED AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_options(cpp_sparkplug_host PRIVATE
        "LINKER:--exclude-libs,ALL"
        "LINKER:--version-script=${CMAKE_CURRENT_SOURCE_DIR}/cmake/hide_spdlog.map")
    set_property(TARGET cpp_sparkplug_host APPEND PROPERTY LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/cmake/hide_spdlog.map)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(cpp_sparkplug_host PRIVATE -Wall -Wextra)
endif()

set_target_properties(cpp_sparkplug_host PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    POSITION_INDEPENDENT_CODE ON
)

# --- Install ---

install(TARGETS cpp_sparkplug_host
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# Headers keep their directory layout, since they include each other by relative path.
# SparkplugReceiver.h and LogMacros.h are internal and need paho/spdlog, so they are not installed.
install(DIRECTORY src/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/cpp_sparkplug_host
    FILES_MATCHING PATTERN "*.h"
    PATTERN "SparkplugReceiver.h" EXCLUDE
    PATTERN "LogMacros.h" EXCLUDE)

# The public headers include tahu.h. pico_tahu's own install rule uses CMAKE_SOURCE_DIR, so as a
# subproject it installs this project's src/ instead of its own headers. Install them here.
get_target_property(PICO_TAHU_SOURCE_DIR pico_tahu SOURCE_DIR)
install(DIRECTORY ${PICO_TAHU_SOURCE_DIR}/src/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h")

# --- Tests and examples ---

if(CPP_SPARKPLUG_HOST_BUILD_TESTS)
    enable_testing()
    file(GLOB TEST_SOURCES CONFIGURE_DEPENDS "tests/*.cpp")
    add_executable(cpp_sparkplug_host_tests ${TEST_SOURCES})
    target_link_libraries(cpp_sparkplug_host_tests PRIVATE cpp_sparkplug_host)
    add_test(NAME cpp_sparkplug_host_tests COMMAND cpp_sparkplug_host_tests)
endif()

if(CPP_SPARKPLUG_HOST_BUILD_EXAMPLES)
    find_package(Threads REQUIRED)
    add_executable(basic_host examples/basic_host.cpp)
    target_link_libraries(basic_host PRIVATE cpp_sparkplug_host Threads::Threads)
endif()
