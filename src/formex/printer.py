from .expr import BinOp, Expr, Number, Symbol, UnaryOp
from .tokens import TokenType

_OP_STRINGS = {
    TokenType.PLUS: "+",
    TokenType.MINUS: "-",
    TokenType.STAR: "*",
    TokenType.SLASH: "/",
    TokenType.CARET: "^",
}


def get_op_string(op: TokenType) -> str:
    return _OP_STRINGS.get(op, "?")


def _precedence(op: TokenType) -> int:
    if op in (TokenType.PLUS, TokenType.MINUS):
        return 1
    if op in (TokenType.STAR, TokenType.SLASH):
        return 2
    if op == TokenType.CARET:
        return 3
    return 4


def _print_expr(expr: Expr, parent_prec: int) -> str:
    if isinstance(expr, Number):
        if expr.value == int(expr.value):
            return str(int(expr.value))
        return f"{expr.value:.6f}"
    if isinstance(expr, Symbol):
        return expr.name
    if isinstance(expr, UnaryOp):
        s = "-" + _print_expr(expr.operand, 3)
        return f"({s})" if parent_prec > 2 else s
    if isinstance(expr, BinOp):
        prec = _precedence(expr.op)
        s = _print_expr(expr.left, prec) + get_op_string(expr.op) + _print_expr(expr.right, prec + 1)
        return f"({s})" if prec < parent_prec else s
    return "?"


def pretty_print(expr: Expr) -> str:
    return _print_expr(expr, 0)
