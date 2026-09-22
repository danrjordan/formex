include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.7.1
)
FetchContent_MakeAvailable(Catch2)

# --- shared test/benchmark-only helper code (never linked into formex_core) ---
add_library(formex_test_support STATIC
    support/Rng.cpp
    support/ExprGen.cpp
    support/TreeEqual.cpp
    support/NodeCount.cpp
)
target_include_directories(formex_test_support PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/support)
target_link_libraries(formex_test_support PUBLIC formex_core)

add_executable(formex_tests
    test_lexer.cpp
    test_parser.cpp
    test_simplifier.cpp
    test_generator_smoke.cpp
)
target_link_libraries(formex_tests PRIVATE formex_core formex_test_support Catch2::Catch2WithMain)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)
# TEST_SPEC excludes [stats]-tagged cases from ctest/make-test discovery so
# the normal test run stays fast; they're still runnable directly via
# `formex_tests [stats]` for a quick manual smoke check of the generator.
catch_discover_tests(formex_tests TEST_SPEC "~[stats]")

add_subdirectory(stats)
