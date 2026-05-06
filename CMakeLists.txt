cmake_minimum_required(VERSION 3.10)

project(TomasuloSimulator)

set(CMAKE_CXX_STANDARD 17)

include_directories(include)

add_executable(simulator
    src/main.cpp
    src/Parser.cpp
    src/RegisterFile.cpp
    src/Memory.cpp
)