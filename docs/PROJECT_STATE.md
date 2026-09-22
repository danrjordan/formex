# formex — Project State

_Audience: an AI assistant implementing the next milestone. Describes what the
code actually does. Where docs and code disagree, the code wins._

---

## 1. What formex is and does today

formex is a terminal-based symbolic algebra engine written in C++20. The user
types an arithmetic expression (polynomials, powers, products) into a full-screen
ftxui TUI; the engine lexes and parses it into an AST, symbolically differentiates
with respect to `x`, simplifies the derivative, and displays the result alongside
a numbered proof-step panel. A second view (Ctrl-G) plots both `f(x)` and `f'(x)`
as ASCII charts in the terminal. The only operation the UI exposes is
differentiation; there is no way through the UI to just simplify or evaluate a
standalone expression, even though those subsystems exist.

---

## 2. Directory tree

```
formex/
├── CMakeLists.txt          — build definition, single target, finds ftxui
├── CLAUDE.md               — project instructions and milestone roadmap
├── README.md               — empty (only "# formex")
├── include/
│   └── formex/
│       ├── Token.h         — TokenType enum + Token struct
│       ├── Expr.h          — AST node hierarchy (Number, Symbol, BinOp, UnaryOp)
│       ├── Lexer.h         — Lexer class declaration
│       ├── Parser.h        — Parser class declaration
│       ├── Printer.h       — prettyPrint() declaration
│       ├── Simplifier.h    — simplify() declaration
│       ├── Differentiator.h — DiffResult, Step structs + differentiate()/clone() decls
│       ├── Evaluator.h     — evaluate() declaration
│       └── Ui.h            — runUi() declaration
└── src/
    ├── main.cpp            — calls runUi(), nothing else
    ├── Lexer.cpp           — tokenises input string
    ├── Parser.cpp          — Pratt (binding-power) recursive parser
    ├── Printer.cpp         — AST → infix string
    ├── Simplifier.cpp      — repeated one-pass rewrite until fixed point
    ├── Differentiator.cpp  — symbolic differentiation + clone()
    ├── Evaluator.cpp       — numerical evaluation at a given x
    └── Ui.cpp              — entire TUI: layout, event handling, graph renderer
```

There is no `tests/` directory on disk (CLAUDE.md says "empty for now").

---

## 3. Core data model

### Node types (`include/formex/Expr.h`)

```cpp
struct Expr { virtual ~Expr() = default; };
using ExprPtr = std::unique_ptr<Expr>;

struct Number  : Expr { double value; };
struct Symbol  : Expr { std::string name; };
struct BinOp   : Expr { TokenType op; ExprPtr left, right; };
struct UnaryOp : Expr { TokenType op; ExprPtr operand; };   // DEAD — see §8
```

`TokenType` is used directly as the operator tag in `BinOp` (and the planned
`UnaryOp`). This couples the AST to the lexer vocabulary; there is no separate
`OpKind` enum.

### Ownership and memory

All nodes are heap-allocated and owned by `std::unique_ptr`. The tree is a DAG
of exclusive ownership — no sharing. Subtrees must be deep-copied with `clone()`
before they can appear in two places (the differentiator does this heavily).
There is no interning, no canonicalisation, and no hash-consing.

### Token type (`include/formex/Token.h`)

```cpp
enum class TokenType {
  NUMBER, IDENT, PLUS, MINUS, STAR, SLASH, CARET, LPAREN, RPAREN, END
};
struct Token { TokenType type; std::string value; };
```

`MINUS` and `LPAREN`/`RPAREN` exist as tokens but the parser cannot currently
use them in prefix position (see §8).

---

## 4. Public API of each subsystem

### Lexer (`Lexer.h / Lexer.cpp`)

```cpp
Lexer(const std::string &input);
std::vector<Token> tokenise();
```

Converts an input string to a flat token list. Handles: integer and decimal
literals, identifiers (alphanumeric + underscore), and the seven operators.
Silently ignores unknown characters. Always appends a terminal `END` token.

### Parser (`Parser.h / Parser.cpp`)

```cpp
Parser(std::vector<Token> &tokenList);
ExprPtr constructTree();
```

Pratt parser with binding powers `{+,- → 1, *,/ → 2, ^ → 3}`. Takes a token
list by reference (copies it internally). `constructTree()` calls `parseExpr(0)`,
which is the only recursive entry point. Does **not** consume the full token list;
silently stops at the first unrecognised prefix token.

### Printer (`Printer.h / Printer.cpp`)

```cpp
std::string prettyPrint(const ExprPtr &expr);
```

Recursively serialises the AST to an infix string with no parentheses added.
`Number` values are printed as integers when the double has no fractional part;
otherwise as `std::to_string` (6 decimal places). Returns `"?"` for unrecognised
nodes (including `UnaryOp`).

### Simplifier (`Simplifier.h / Simplifier.cpp`)

```cpp
ExprPtr simplify(const ExprPtr &expr);
```

Runs `simplifyOnce` in a loop until `prettyPrint` of consecutive passes is
identical. `simplifyOnce` is a structural bottom-up rewrite implementing:
`x+0→x`, `0+x→x`, `x*1→x`, `1*x→x`, `x*0→0`, `0*x→0`, `x^1→x`, `x^0→1`,
and `sym+sym→2*sym` for identical symbols. No constant folding
(e.g. `2+3` stays as-is). No rules for subtraction, division, or coefficient
merging (`2*x+3*x` stays as-is).

### Differentiator (`Differentiator.h / Differentiator.cpp`)

```cpp
ExprPtr clone(const ExprPtr &expr);
DiffResult differentiate(const ExprPtr &expr, const std::string &var);
```

`clone` performs a deep copy; returns `nullptr` for `UnaryOp` and null input
(propagates silently). `differentiate` implements:
- constant rule, variable rule
- sum rule (`+` only, not `-`)
- constant-multiple rule (when one operand is a `Number` literal)
- product rule (when neither operand is a `Number` literal)
- power rule for constant integer exponent (`f^n` → `n*f^(n-1)*f'`, with chain
  rule applied to the inner expression)
- returns `DiffResult{nullptr, {{"unsupported: …", …}}}` for non-constant
  exponents and unhandled node types

`DiffResult` carries both the result tree and a `std::vector<Step>` of
human-readable proof steps shown in the Working panel.

### Evaluator (`Evaluator.h / Evaluator.cpp`)

```cpp
double evaluate(const ExprPtr &expr, double x);
```

Numerically evaluates an expression at a given `x`. Only the symbol `"x"` is
recognised; all other symbols evaluate to `0`. Division by zero returns `0`.
Used exclusively by the graph renderer.

### TUI (`Ui.h / Ui.cpp`)

```cpp
void runUi();
```

Runs the interactive UI until the user exits. All state is local to `runUi`.
On Enter: lex → parse → clone → differentiate → simplify → store history entry.
Ctrl-G toggles between working/result panes and graph pane. Arrow keys navigate
history. Ctrl-F and Ctrl-S are shown in the hint bar but **not handled** in the
event loop — they silently do nothing.

---

## 5. Key invariants and conventions

**Naming:** camelCase for variables and functions; PascalCase for classes and
structs; no prefix on private members.

**No implementation in headers:** all `.cpp` files include only declarations from
`include/formex/`. Exception: `Evaluator.h` uses `#include "Expr.h"` (no
`formex/` prefix) which works only because CMake adds `include/` to the include
path globally.

**Error handling:** the Parser throws `std::runtime_error` on unexpected tokens.
The Ui catches all `std::exception` and displays the message. Subsystems outside
the parser signal failure by returning `nullptr` inside `ExprPtr` — callers must
null-check before dereferencing. The Ui only checks `if (!diffResult)` (where
`diffResult` is the `ExprPtr` member), which is correct but fragile — a future
subsystem returning a non-null but malformed tree will not be caught.

**C++20 features used:** structured bindings (auto `[a,b]`), `if`-init
statements, `std::make_unique`, range-based for. No concepts, no modules, no
coroutines, no `std::ranges`. The `std::format` from C++20 is not used — the
code uses `snprintf` and `std::to_string`.

**Fixed-point detection in Simplifier** uses string equality of `prettyPrint`
output rather than structural comparison. This works in practice but breaks if
`prettyPrint` is non-deterministic or if a future rewrite produces structurally
distinct but print-equal trees.

**AST/token coupling:** `BinOp::op` stores a `TokenType`. Any extension that
adds operators must extend `TokenType`, `Lexer`, `Parser`, `Printer`,
`Simplifier`, `Differentiator`, and `Evaluator` consistently — there is no single
dispatch table.

---

## 6. Build & test setup

**CMake:** `cmake_minimum_required(VERSION 3.20)`, `CMAKE_CXX_STANDARD 20`,
single `add_executable(formex …)` target, `CMAKE_EXPORT_COMPILE_COMMANDS ON`
(clangd index is present in `.cache/`).

**Dependencies:** ftxui is the only external dependency. It is located via
`find_package(ftxui REQUIRED)` — it must be installed system-wide (e.g. via
Homebrew or built and installed manually). It is **not vendored**. Three ftxui
components are linked: `ftxui::screen`, `ftxui::dom`, `ftxui::component`.

**Build:**
```
mkdir build && cd build
cmake ..
make
./formex
```

**Tests:** zero. No `tests/` directory, no CMake test target, no test framework.
No area of the codebase has any automated coverage.

---

## 7. Milestone status

| Milestone | Status | Notes |
|---|---|---|
| M1 Foundation | **Done** | Lexer, Parser, AST, Printer all functional |
| M2 Simplification | **Partial** | Basic identity rules work; no constant folding, no subtraction/division rules, no coefficient merging |
| M3 Differentiation | **Partial** | Sum, product, power (constant exp), constant-multiple rules work; no quotient rule, no trig (parser can't even tokenise function calls), non-constant exponents unsupported |
| M4 REPL / TUI | **Mostly done** | Full ftxui TUI, history, working panel, graph; replxx not used; ^F and ^S hinted but unimplemented; graph x-range hardcoded to [-10,10] |
| M5 Equation solving | **Untouched** | |
| M6 Matrix algebra | **Untouched** | |
| M7 LLVM/JIT | **Untouched** | |

**M2 gaps:** no constant folding (`2+3` stays `BinOp(+, 2, 3)`); no rule for
`x-0`, `x-x→0`, `x/1`, `x/x`; no merging of `2*x+3*x→5*x`.

**M3 gaps:** the parser cannot parse function calls (`sin(x)` lexes fine but the
parser returns `Symbol("sin")` and silently discards `(x)`). The CLAUDE.md lists
trig derivatives as part of M3 but they are not implementable without first
extending the lexer/parser to handle function-call syntax. Quotient rule and
symbolic exponent differentiation are also absent.

---

## 8. Known issues and fragile code

1. **`UnaryOp` is dead code.** The struct is declared in `Expr.h` and appears
   nowhere else — not constructed, not matched in any `dynamic_cast`, not handled
   in Printer, Simplifier, Differentiator, or Evaluator. Parsing unary minus
   (e.g. `-x`, `-3`) throws `std::runtime_error("unexpected token: -")`.

2. **Parser cannot handle parentheses in prefix position.** `(x+1)*2` throws
   because `LPAREN` is not a valid start-of-expression token. The Pratt parser
   loop does correctly stop at `RPAREN` (it's not in `bindingPower`), so
   sub-expressions like `2*(x+1)` — if the inner group were parseable — would
   terminate correctly. But they aren't parseable.

3. **Parser silently truncates on unknown prefix tokens.** If the first token of
   a sub-expression is not `NUMBER` or `IDENT`, the parser throws rather than
   recovering. If parsing a function-call argument like `sin(x)` is attempted,
   `constructTree` returns `Symbol("sin")` and the `(x)` tokens are silently
   abandoned (no exception because `LPAREN` stops the outer `while` loop).

4. **Simplifier fixed-point via string comparison.** `prettyPrint` of floats
   uses `std::to_string` which produces locale-dependent output with trailing
   zeros. If two structurally different trees print identically, the loop exits
   too early.

5. **Printer omits parentheses.** `BinOp(STAR, BinOp(PLUS,x,y), z)` prints as
   `x+y*z`, which is mathematically incorrect (implies lower precedence for `*`).
   Differentiation of products that produce nested sums will display incorrectly.

6. **`clone()` returns `nullptr` for `UnaryOp` and null input** with no
   diagnostic. A null clone silently produces a malformed tree.

7. **Evaluator silently maps unknown symbols to 0.** A multi-variable expression
   like `x+y` evaluates as `x+0`, which corrupts graph output without any
   warning.

8. **Ctrl-F and Ctrl-S event handlers are missing.** The status bar text
   advertises `^F factor  ^S steps` but neither `Event::CtrlF` nor
   `Event::CtrlS` appears in the `CatchEvent` handler.

9. **Graph x-range is hardcoded** to `[-10, 10]` with no UI control.

10. **The Ui hardwires differentiation as the only operation.** The simplify and
    evaluate subsystems exist but are not exposed as standalone commands.

---

## 9. Three things to clarify before extending

1. **Function-call syntax scope for M5.** M5 (equation solving) operates on
   polynomials. Should the parser learn function-call syntax now (enabling
   `sin`, `log`, `exp` in M5/M6), or stay expression-only and defer function
   support? The answer determines whether M5 needs a parser overhaul or just
   algebraic manipulation of the existing node types.

2. **Operator representation strategy.** Currently `BinOp::op` reuses `TokenType`
   directly. Adding operators (unary minus, equality `=` for equations, matrix
   ops) will require either extending `TokenType` (coupling lexer and AST further)
   or introducing a separate `OpKind` enum. Which direction is preferred?

3. **Number representation.** `Number::value` is `double`, which means `1/3`
   evaluates to an irrational approximation and constant-folding `1/3 * 3` won't
   simplify to `1`. M5 (polynomial solving) will need exact rational arithmetic
   for correct symbolic results. Should `Number` be extended to hold a rational
   (`int64_t num, den`) now, or is floating-point acceptable for M5?
