# Sparkplug TCK harness. A standalone project so it never touches the library's own build:
#   cmake -S tck -B build/tck && cmake --build build/tck -j
# Pass the same dependency options as the main build, e.g. -DFETCH_REMOTE=OFF for a local pico_tahu.
cmake_minimum_required(VERSION 3.17)

# pico_tahu enables ASM in its own project(); enabling it here too keeps every directory consistent
project(cpp_sparkplug_host_tck C CXX ASM)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Only the library; the host application needs nothing else
set(CPP_SPARKPLUG_HOST_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(CPP_SPARKPLUG_HOST_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/.. cpp_sparkplug_host)

find_package(Threads REQUIRED)
add_executable(tck_host host_app/tck_host.cpp)
target_link_libraries(tck_host PRIVATE cpp_sparkplug_host Threads::Threads)
