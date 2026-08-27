from __future__ import annotations

from dataclasses import dataclass, field

from .expr import BinOp, Expr, Number, Symbol, UnaryOp
from .printer import pretty_print
from .tokens import TokenType


@dataclass
class Step:
    rule: str
    expr: str
    result: str


@dataclass
class DiffResult:
    result: Expr | None
    steps: list[Step] = field(default_factory=list)


def clone(expr: Expr | None) -> Expr | None:
    if expr is None:
        return None
    if isinstance(expr, Number):
        return Number(expr.value)
    if isinstance(expr, Symbol):
        return Symbol(expr.name)
    if isinstance(expr, BinOp):
        return BinOp(expr.op, clone(expr.left), clone(expr.right))
    if isinstance(expr, UnaryOp):
        return UnaryOp(expr.op, clone(expr.operand))
    return None


def differentiate(expr: Expr, var: str) -> DiffResult:
    if isinstance(expr, Number):
        return DiffResult(Number(0), [Step("constant rule: d/dx(n) = 0", pretty_print(expr), "0")])

    if isinstance(expr, Symbol):
        if expr.name == var:
            return DiffResult(Number(1), [Step("variable rule: d/dx(x) = 1", pretty_print(expr), "1")])
        return DiffResult(Number(0), [Step("constant rule: d/dx(n) = 0", pretty_print(expr), "0")])

    if isinstance(expr, UnaryOp):
        operand_result = differentiate(expr.operand, var)
        result = UnaryOp(TokenType.MINUS, clone(operand_result.result))
        steps = [Step("negation rule: d/dx(-f) = -f'", pretty_print(expr), pretty_print(result))]
        steps.extend(operand_result.steps)
        return DiffResult(result, steps)

    if isinstance(expr, BinOp):
        if expr.op == TokenType.PLUS:
            l_result = differentiate(expr.left, var)
            r_result = differentiate(expr.right, var)
            result = BinOp(TokenType.PLUS, clone(l_result.result), clone(r_result.result))
            steps = [Step("sum rule: d/dx(f+g) = f' + g'", pretty_print(expr), pretty_print(result))]
            steps.extend(l_result.steps)
            steps.extend(r_result.steps)
            return DiffResult(result, steps)

        if expr.op == TokenType.MINUS:
            l_result = differentiate(expr.left, var)
            r_result = differentiate(expr.right, var)
            result = BinOp(TokenType.MINUS, clone(l_result.result), clone(r_result.result))
            steps = [Step("difference rule: d/dx(f-g) = f' - g'", pretty_print(expr), pretty_print(result))]
            steps.extend(l_result.steps)
            steps.extend(r_result.steps)
            return DiffResult(result, steps)

        if expr.op == TokenType.STAR:
            if isinstance(expr.left, Number):
                r_result = differentiate(expr.right, var)
                result = BinOp(TokenType.STAR, Number(expr.left.value), clone(r_result.result))
                steps = [
                    Step(
                        "constant multiple rule: d/dx(c*f) = c*f'",
                        pretty_print(expr),
                        pretty_print(result),
                    )
                ]
                steps.extend(r_result.steps)
                return DiffResult(result, steps)

            if isinstance(expr.right, Number):
                l_result = differentiate(expr.left, var)
                result = BinOp(TokenType.STAR, Number(expr.right.value), clone(l_result.result))
                steps = [
                    Step(
                        "constant multiple rule: d/dx(f*c) = c*f'",
                        pretty_print(expr),
                        pretty_print(result),
                    )
                ]
                steps.extend(l_result.steps)
                return DiffResult(result, steps)

            l_result = differentiate(expr.left, var)
            r_result = differentiate(expr.right, var)
            left = BinOp(TokenType.STAR, clone(l_result.result), clone(expr.right))
            right = BinOp(TokenType.STAR, clone(expr.left), clone(r_result.result))
            result = BinOp(TokenType.PLUS, clone(left), clone(right))
            steps = [
                Step("product rule: d/dx(f*g) = f'g + fg'", pretty_print(expr), pretty_print(result))
            ]
            steps.extend(l_result.steps)
            steps.extend(r_result.steps)
            return DiffResult(result, steps)

        if expr.op == TokenType.CARET:
            n = expr.right if isinstance(expr.right, Number) else None
            if n is not None:
                inner_result = differentiate(expr.left, var)
                power_part = BinOp(
                    TokenType.STAR,
                    Number(n.value),
                    BinOp(TokenType.CARET, clone(expr.left), Number(n.value - 1)),
                )
                steps = [
                    Step(
                        "power rule: d/dx(f^n) = n*f^(n-1)*f'",
                        pretty_print(expr),
                        pretty_print(power_part),
                    )
                ]
                steps.extend(inner_result.steps)
                result = BinOp(TokenType.STAR, power_part, inner_result.result)
                return DiffResult(result, steps)
            return DiffResult(None, [Step("unsupported: non-constant exponent", "", "")])

        if expr.op == TokenType.SLASH:
            l_result = differentiate(expr.left, var)
            r_result = differentiate(expr.right, var)
            numerator = BinOp(
                TokenType.MINUS,
                BinOp(TokenType.STAR, clone(l_result.result), clone(expr.right)),
                BinOp(TokenType.STAR, clone(expr.left), clone(r_result.result)),
            )
            denominator = BinOp(TokenType.CARET, clone(expr.right), Number(2))
            result = BinOp(TokenType.SLASH, numerator, denominator)
            steps = [
                Step(
                    "quotient rule: d/dx(f/g) = (f'g - fg')/g^2",
                    pretty_print(expr),
                    pretty_print(result),
                )
            ]
            steps.extend(l_result.steps)
            steps.extend(r_result.steps)
            return DiffResult(result, steps)

    return DiffResult(None, [])
