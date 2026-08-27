from .expr import BinOp, Expr, Number, Symbol, UnaryOp
from .printer import pretty_print
from .tokens import TokenType


def _simplify_once(expr: Expr) -> Expr:
    if isinstance(expr, Number):
        return Number(expr.value)
    if isinstance(expr, Symbol):
        return Symbol(expr.name)
    if isinstance(expr, BinOp):
        left = _simplify_once(expr.left)
        right = _simplify_once(expr.right)

        if expr.op == TokenType.PLUS:
            if isinstance(right, Number) and right.value == 0:
                return left
            if isinstance(left, Number) and left.value == 0:
                return right
            if isinstance(left, Symbol) and isinstance(right, Symbol) and left.name == right.name:
                return BinOp(TokenType.STAR, Number(2), Symbol(left.name))

        if expr.op == TokenType.STAR:
            if isinstance(right, Number) and right.value == 1:
                return left
            if isinstance(left, Number) and left.value == 1:
                return right
            if isinstance(right, Number) and right.value == 0:
                return Number(0)
            if isinstance(left, Number) and left.value == 0:
                return Number(0)

        if expr.op == TokenType.CARET:
            if isinstance(right, Number) and right.value == 1:
                return left
            if isinstance(right, Number) and right.value == 0:
                return Number(1)

        return BinOp(expr.op, left, right)
    if isinstance(expr, UnaryOp):
        operand = _simplify_once(expr.operand)
        if isinstance(operand, Number):
            return Number(-operand.value)
        return UnaryOp(expr.op, operand)
    raise TypeError(f"unhandled expr type: {type(expr)!r}")


def simplify(expr: Expr) -> Expr:
    current = _simplify_once(expr)
    while True:
        nxt = _simplify_once(current)
        if pretty_print(nxt) == pretty_print(current):
            return current
        current = nxt
