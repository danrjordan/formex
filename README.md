# formex

A small terminal-based symbolic differentiation engine, written in Python.

Type an arithmetic expression (polynomials, powers, products, quotients) and
formex lexes/parses it into an AST, differentiates with respect to `x`,
simplifies the result, and shows a step-by-step proof alongside the answer.

## Install

```
pip install -e ".[test]"
```

## Run

```
formex
# or
python -m formex
```

Arrow keys cycle through history; Esc quits.

## Test

```
pytest
```

---

This project was originally written in C++20 (lexer/parser/simplifier/
differentiator + an ftxui terminal UI). That version is preserved on the
`cpp-version-backup` branch.
