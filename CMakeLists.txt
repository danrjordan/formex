cmake_minimum_required(VERSION 3.20)
project(formex LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- core library: no UI, no ftxui ---
add_library(formex_core STATIC
    src/Lexer.cpp
    src/Parser.cpp
    src/Printer.cpp
    src/Simplifier.cpp
    src/Differentiator.cpp
)
target_include_directories(formex_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# --- executable ---
find_package(ftxui REQUIRED)
add_executable(formex src/main.cpp src/Ui.cpp)
target_link_libraries(formex PRIVATE
    formex_core ftxui::screen ftxui::dom ftxui::component)

# --- tests ---
option(FORMEX_BUILD_TESTS "Build tests" ON)
if(FORMEX_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
