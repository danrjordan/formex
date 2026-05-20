# CLAUDE.md

# formex

A symbolic algebra engine / CAS (Computer Algebra System) built in C++20.
Inspired by SymPy / Mathematica. AST-based symbolic rewriting.

## Build

```bash
mkdir build && cd build
cmake ..
make
./formex
```

## Project structure

- `include/formex/` — public headers
- `src/` — implementation files
- `tests/` — tests (empty for now)

## Milestones

M1 — Foundation
Lexer, Parser, AST, pretty printer. The full pipeline from raw string to tree to readable output. Everything else builds on this.

M2 — Simplification
Rewrite rules engine. x + 0 → x, x * 1 → x, x * 0 → 0, x + x → 2x. Walk the AST and replace nodes with simpler equivalents.

M3 — Symbolic Differentiation
Differentiate expressions symbolically. Power rule, product rule, chain rule, trig derivatives. Takes an AST, returns a new differentiated AST.

M4 — REPL
Interactive shell. User types diff(sin(x^2)), engine prints 2x*cos(x^2). Ties together the full pipeline. Add replxx for history and tab completion.

M5 — Equation Solving
Polynomial manipulation. Solve simple equations symbolically. Linear and quadratic forms to start.

M6 — Matrix Algebra
Symbolic matrix type. New AST node for matrices. Multiplication, determinant, inverse.

M7 — LLVM/JIT Backend
Emit LLVM IR from the AST. ORC JIT for fast numerical evaluation. The big stretch goal — study the Kaleidoscope tutorial before touching this.


## Style

- C++20
- camelCase for variables and functions
- PascalCase for classes and structs
- Private members prefixed with nothing, just kept in private section
- No implementation in headers
- No or very limited comments
