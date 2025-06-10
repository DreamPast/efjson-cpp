cmake_minimum_required(VERSION 3.10)
project(efjson)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(efjson-example ./example/example.cpp)
target_include_directories(efjson-example PRIVATE ./)

add_executable(efjson-perf ./perf/perf.cpp)
target_include_directories(efjson-perf PRIVATE ./)
