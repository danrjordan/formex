cmake_minimum_required(VERSION 3.20)
project(formex LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- coverage instrumentation (opt-in) ---
option(FORMEX_COVERAGE "Build with --coverage instrumentation (gcov/gcovr)" OFF)
if(FORMEX_COVERAGE)
    add_compile_options(--coverage -O0 -g)
    add_link_options(--coverage)
endif()

# --- core library: no UI, no ftxui ---
add_library(formex_core STATIC
    src/Lexer.cpp
    src/Parser.cpp
    src/Printer.cpp
    src/Simplifier.cpp
    src/Differentiator.cpp
    src/LinearAlgebra.cpp
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

# --- coverage report target (needs FORMEX_TESTS' targets to exist) ---
if(FORMEX_COVERAGE)
    find_program(GCOVR_EXECUTABLE gcovr)
    if(CMAKE_CXX_COMPILER_ID MATCHES "AppleClang")
        set(GCOVR_GCOV_EXECUTABLE "xcrun llvm-cov gcov")
    else()
        set(GCOVR_GCOV_EXECUTABLE "gcov")
    endif()
    if(GCOVR_EXECUTABLE)
        add_custom_target(coverage
            COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
            COMMAND ${GCOVR_EXECUTABLE}
                --root ${CMAKE_SOURCE_DIR}
                --filter "${CMAKE_SOURCE_DIR}/src/.*"
                --filter "${CMAKE_SOURCE_DIR}/include/formex/.*"
                --exclude-directories ".*_deps.*"
                --gcov-executable "${GCOVR_GCOV_EXECUTABLE}"
                --gcov-ignore-errors=no_working_dir_found
                ${CMAKE_BINARY_DIR}
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Running tests and generating gcovr line-coverage report"
            VERBATIM
        )
        add_dependencies(coverage formex_tests formex)
    else()
        message(STATUS "FORMEX_COVERAGE is ON but gcovr was not found on PATH; 'coverage' target not created")
    endif()
endif()
