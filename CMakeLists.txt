cmake_minimum_required(VERSION 3.20)
project(formex VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(include)

add_executable(formex
    src/Lexer.cpp
    src/Parser.cpp
    src/Printer.cpp
    src/main.cpp
    src/Simplifier.cpp
)