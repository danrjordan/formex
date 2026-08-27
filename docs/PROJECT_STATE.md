# formex — Project State

_Audience: an AI assistant implementing the next milestone. Describes what the
code actually does. Where docs and code disagree, the code wins._

---

## 0. Migration note

formex was originally written in C++20 (ftxui TUI). It was ported to Python
1:1 (same lexer/parser/AST/simplifier/differentiator rules, same known
bugs/gaps) on 2026-08-27. The C++ implementation is preserved on the
`cpp-version-backup` git branch and is no longer maintained. Everything below
describes the current Python code.

---

## 1. What formex is and does today

formex is a terminal-based symbolic algebra engine written in Python. The
user types an arithmetic expression (polynomials, powers, products,
quotients) into a full-screen `curses` TUI; the engine lexes and parses it
into an AST, symbolically differentiates with respect to `x`, simplifies the
derivative, and displays the result alongside a numbered proof-step panel.
The only operation the UI exposes is differentiation; there is no way
through the UI to just simplify or evaluate a standalone expression, even
though the simplifier subsystem is exposed as a library function.

---

## 2. Directory tree

```
formex/
├── pyproject.toml          — packaging (setuptools, src layout), `formex` console script
├── README.md
├── src/
│   └── formex/
│       ├── __init__.py
│       ├── tokens.py       — TokenType enum + Token dataclass
│       ├── expr.py         — AST node hierarchy (Number, Symbol, BinOp, UnaryOp)
│       ├── lexer.py        — Lexer class
│       ├── parser.py       — Parser class
│       ├── printer.py      — pretty_print()
│       ├── simplifier.py   — simplify()
│       ├── differentiator.py — DiffResult, Step dataclasses + differentiate()/clone()
│       ├── ui.py           — curses TUI: layout, event loop
│       └── __main__.py     — `python -m formex` / `formex` entry point
└── tests/
    └── test_lexer.py       — pytest
```

---

## 3. Core data model

### Node types (`src/formex/expr.py`)

```python
class Expr: ...

@dataclass
class Number(Expr): value: float

@dataclass
class Symbol(Expr): name: str

@dataclass
class BinOp(Expr): op: TokenType; left: Expr; right: Expr

@dataclass
class UnaryOp(Expr): op: TokenType; operand: Expr
```

### Ownership

Plain Python object references (no manual memory management, unlike the
original `std::unique_ptr` tree). Subtrees are still explicitly deep-copied
with `clone()` before appearing in two places — the differentiator does this
heavily, matching the original's structure even though Python doesn't
require it for correctness the way the C++ ownership model did.

### Token type (`src/formex/tokens.py`)

```python
class TokenType(Enum):
    NUMBER, IDENT, PLUS, MINUS, STAR, SLASH, CARET, LPAREN, RPAREN, END

@dataclass
class Token: type: TokenType; value: str
```

---

## 4. Public API of each subsystem

### Lexer (`lexer.py`)

```python
Lexer(input_str).tokenise() -> list[Token]
```

Converts an input string to a flat token list. Handles integer/decimal
literals, identifiers (alphanumeric + underscore), and the seven operator/
paren characters. Silently ignores unknown characters. Always appends a
terminal `END` token.

### Parser (`parser.py`)

```python
Parser(token_list).construct_tree() -> Expr
```

Pratt parser with binding powers `{+,- -> 1, *,/ -> 2, ^ -> 3}`. Handles
prefix unary minus and parenthesised groups, plus implicit multiplication
(`2x`, `x(x+1)`, `2 x^2`). Does **not** verify the full token list was
consumed; silently stops at the first token it doesn't recognise as a
continuation.

### Printer (`printer.py`)

```python
pretty_print(expr: Expr) -> str
```

Recursively serialises the AST to an infix string, adding parentheses only
where operator precedence requires it. `Number` values print as integers
when they have no fractional part, otherwise as a 6-decimal fixed string
(matching `std::to_string(double)` from the original C++). Returns `"?"` for
unrecognised node types.

### Simplifier (`simplifier.py`)

```python
simplify(expr: Expr) -> Expr
```

Runs a one-pass structural rewrite repeatedly until `pretty_print` of
consecutive passes is identical (fixed-point via string comparison, not
structural comparison — a known fragility carried over from the original).
Implements: `x+0->x`, `0+x->x`, `x*1->x`, `1*x->x`, `x*0->0`, `0*x->0`,
`x^1->x`, `x^0->1`, and `sym+sym->2*sym` for identical symbols. No constant
folding (`2+3` stays as-is). No rules for subtraction, division, or
coefficient merging (`2*x+3*x` stays as-is).

### Differentiator (`differentiator.py`)

```python
clone(expr: Expr | None) -> Expr | None
differentiate(expr: Expr, var: str) -> DiffResult  # DiffResult(result, steps)
```

Implements: constant rule, variable rule, negation rule, sum rule, difference
rule, constant-multiple rule (checked via `isinstance(expr.left/right, Number)`
on the *original* operands, not the differentiated ones), product rule,
power rule for constant exponent (`f^n -> n*f^(n-1)*f'` with chain rule on
the inner expression), quotient rule. Returns `DiffResult(None, [Step(...)])`
for non-constant exponents; falls through to `DiffResult(None, [])` for any
unhandled combination (shouldn't currently be reachable given the five
`BinOp` operator kinds).

`DiffResult` carries both the result tree and a `list[Step]` of human
readable proof steps shown in the Working panel.

### TUI (`ui.py`)

```python
run_ui() -> None
```

`curses.wrapper`-based full-screen UI. Layout: WORKING panel (proof steps,
left, flexible width) | RESULT panel (fixed 30 cols, right); below that a
history bar; below that the input line. On Enter: lex -> parse ->
differentiate -> simplify -> append history entry. Left/Right arrow keys
always cycle history (they are handled before any text-input logic, matching
the original's `CatchEvent`-intercepts-first behavior — arrow keys never
move a cursor within the input line). Esc quits. Colors are approximated
with curses' 8-color palette (the original's RGB orange becomes yellow)
since curses doesn't portably support arbitrary RGB.

---

## 5. Key invariants and conventions

**Naming:** snake_case for functions/variables/modules, PascalCase for
classes, matching PEP 8 (the original used camelCase/PascalCase per C++
convention).

**Error handling:** the Parser raises `RuntimeError` on unexpected tokens.
The UI catches all `Exception` and displays the message. Subsystems outside
the parser signal failure by returning `None` where the original returned
`nullptr` — callers must check for `None` before use. The UI only checks
`diff_result.result is None`, matching the original's fragility (a future
subsystem returning a non-None but malformed tree will not be caught).

**Fixed-point detection in Simplifier** uses string equality of
`pretty_print` output rather than structural comparison — carried over
verbatim from the C++ version, including its fragility.

**AST/token coupling:** `BinOp.op` stores a `TokenType` directly, same as
the original. Any extension that adds operators must extend `TokenType`,
`Lexer`, `Parser`, `Printer`, `Simplifier`, and `Differentiator` consistently
— there is no single dispatch table.

---

## 6. Build & test setup

**Packaging:** `pyproject.toml`, setuptools `src/` layout, `formex` console
script via `project.scripts`.

**Dependencies:** none beyond the stdlib (`curses`) on macOS/Linux;
`windows-curses` is pulled in automatically on Windows via an environment
marker.

**Run:**
```
pip install -e ".[test]"
formex            # or: python -m formex
```

**Tests:** `pytest` — currently only `tests/test_lexer.py` (ported from the
original `test_lexer.cpp`). No coverage yet for parser, printer, simplifier,
or differentiator.

---

## 7. Milestone status

| Milestone | Status | Notes |
|---|---|---|
| M1 Foundation | **Done** | Lexer, Parser, AST, Printer all functional |
| M2 Simplification | **Partial** | Basic identity rules work; no constant folding, no subtraction/division rules, no coefficient merging |
| M3 Differentiation | **Partial** | Sum, difference, product, quotient, power (constant exp), constant-multiple, negation rules work; no trig (parser can't tokenise function calls), non-constant exponents unsupported |
| M4 REPL / TUI | **Done** | curses TUI, history, working panel |
| M5 Equation solving | **Untouched** | |
| M6 Matrix algebra | **Untouched** | |
| M7 JIT/codegen | **Untouched** — not applicable in the same form now that the project is Python; would likely mean a numeric fast-path (e.g. via `numpy`) rather than LLVM | |

**M2 gaps:** no constant folding (`2+3` stays `BinOp(+, 2, 3)`); no rule for
`x-0`, `x-x->0`, `x/1`, `x/x`; no merging of `2*x+3*x->5*x`.

**M3 gaps:** the parser cannot parse function calls (`sin(x)` lexes fine but
the parser returns `Symbol("sin")` and silently discards `(x)`, since `(`
after an identifier isn't consumed as a call — it's simply not a
continuation token the Pratt loop recognises). Trig derivatives and symbolic
exponent differentiation are absent.

---

## 8. Known issues and fragile code

1. **Parser silently truncates on unknown continuation tokens.** If a
   sub-expression is followed by a token the Pratt loop doesn't recognise as
   a continuation (and it isn't `END`), the loop just stops; `construct_tree`
   never verifies the full token list was consumed.

2. **Parser cannot handle function-call syntax.** `sin(x)` tokenises fine but
   parses as `Symbol("sin")` with the trailing `(x)` silently dropped.

3. **Simplifier fixed-point via string comparison**, not structural
   comparison. If two structurally different trees print identically, the
   loop exits too early.

4. **`clone()` returns `None` for unrecognised node types and `None` input**
   with no diagnostic. A `None` clone silently produces a malformed tree
   further up the call chain.

5. **The UI hardwires differentiation as the only operation.** `simplify()`
   is used internally but not exposed as a standalone command; there's no
   evaluator at all currently.

6. **curses color approximation.** The original ftxui UI used RGB orange
   (`Color::RGB(255,165,0)`); curses' 8-color palette has no orange, so
   yellow is used instead. Terminals with a custom yellow may render
   differently than intended.

---

## 9. Three things to clarify before extending

1. **Function-call syntax scope for M5.** M5 (equation solving) operates on
   polynomials. Should the parser learn function-call syntax now (enabling
   `sin`, `log`, `exp` in M5/M6), or stay expression-only and defer function
   support? The answer determines whether M5 needs a parser overhaul or just
   algebraic manipulation of the existing node types.

2. **Operator representation strategy.** Currently `BinOp.op` reuses
   `TokenType` directly. Adding operators (equality `=` for equations,
   matrix ops) will require either extending `TokenType` (coupling lexer and
   AST further) or introducing a separate `OpKind` enum. Which direction is
   preferred?

3. **Number representation.** `Number.value` is a Python `float`, which
   means `1/3` evaluates to an irrational approximation and constant-folding
   `1/3 * 3` won't simplify to `1`. M5 (polynomial solving) will need exact
   rational arithmetic for correct symbolic results — Python's stdlib
   `fractions.Fraction` is a natural fit here, more so than in the original
   C++ version. Should `Number` be extended to hold a `Fraction` now, or is
   floating-point acceptable for M5?
