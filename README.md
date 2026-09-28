# formex

A terminal symbolic algebra engine in C++20. Type an expression and formex
parses it, differentiates it with respect to `x`, simplifies the result, and
shows each rule it applied as numbered steps in a full-screen
[FTXUI](https://github.com/ArthurSonzogni/FTXUI) TUI.

## Features

- **Parser**: numbers, identifiers, `+ - * / ^`, unary minus and parentheses (Pratt parser)
- **Differentiation**: constant, sum, constant-multiple, product and power (integer exponent, with chain) rules
- **Simplification**: identity rules such as `x+0`, `x*1`, `x*0`, `x^1` and `x^0`, applied until nothing changes
- **TUI**: working panel, result panel and history bar
- **Linear algebra mode**: the UI is in place, but the solver (`solveLinear`) is not implemented yet

## Build

Requirements: CMake ≥ 3.20, a C++20 compiler and FTXUI installed where CMake can find it
(for example `brew install ftxui`). The build fetches Catch2 automatically for the tests.

```sh
cmake -S . -B build
cmake --build build
./build/formex
```

### Keys

| Key       | Action                              |
|-----------|-------------------------------------|
| `Enter`   | Evaluate the input                  |
| `Tab`     | Toggle symbolic / linear algebra mode |
| `←` / `→` | Browse history                      |

## Tests

```sh
ctest --test-dir build --output-on-failure   # unit tests (Catch2)
./build/tests/formex_tests "[stats]"         # generator smoke test (excluded from ctest)
./build/tests/stats/formex_stats             # randomized property report
```

`formex_stats` generates 500 random expressions from a fixed seed and reports:

- parse → print → re-parse round-trip fidelity
- how much the simplifier shrinks raw derivatives
- symbolic derivatives checked against central-difference estimates

### Coverage

```sh
cmake -S . -B build-cov -DFORMEX_COVERAGE=ON
cmake --build build-cov --target coverage    # requires gcovr
```

## Layout

```
include/formex/   public headers
src/              core library (formex_core) + TUI (Ui.cpp, main.cpp)
tests/            Catch2 unit tests
tests/support/    test-only helpers: RNG, expression generator, tree equality, evaluator
tests/stats/      formex_stats reporting tool
docs/             design notes
```
