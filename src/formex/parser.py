from .expr import BinOp, Expr, Number, Symbol, UnaryOp
from .tokens import Token, TokenType

_BINDING_POWER = {
    TokenType.PLUS: 1,
    TokenType.MINUS: 1,
    TokenType.STAR: 2,
    TokenType.SLASH: 2,
    TokenType.CARET: 3,
}


class Parser:
    def __init__(self, token_list: list[Token]) -> None:
        self.token_list = list(token_list)
        self.curr_token = 0

    def construct_tree(self) -> Expr:
        return self._parse_expr(0)

    def _parse_expr(self, min_bp: int) -> Expr:
        if self.curr_token >= len(self.token_list):
            raise RuntimeError("unexpected end of input")

        left = self.token_list[self.curr_token]
        self.curr_token += 1
        left_node: Expr

        if left.type == TokenType.NUMBER:
            left_node = Number(float(left.value))
        elif left.type == TokenType.IDENT:
            left_node = Symbol(left.value)
        elif left.type == TokenType.MINUS:
            left_node = UnaryOp(TokenType.MINUS, self._parse_expr(2))
        elif left.type == TokenType.LPAREN:
            left_node = self._parse_expr(0)
            if (
                self.curr_token >= len(self.token_list)
                or self.token_list[self.curr_token].type != TokenType.RPAREN
            ):
                raise RuntimeError("expected closing parenthesis")
            self.curr_token += 1
        else:
            raise RuntimeError(f"unexpected token: {left.value}")

        while self.curr_token < len(self.token_list):
            op = self.token_list[self.curr_token]
            if op.type == TokenType.END:
                break

            implicit_mul = op.type in (TokenType.IDENT, TokenType.NUMBER)
            if implicit_mul:
                if 2 <= min_bp:
                    break
                right_node = self._parse_expr(2)
                left_node = BinOp(TokenType.STAR, left_node, right_node)
                continue

            bp = _BINDING_POWER.get(op.type)
            if bp is None or bp <= min_bp:
                break

            self.curr_token += 1
            right_bp = bp - 1 if op.type == TokenType.CARET else bp
            right_node = self._parse_expr(right_bp)
            left_node = BinOp(op.type, left_node, right_node)

        return left_node
