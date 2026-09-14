from .tokens import Token, TokenType

_SINGLE_CHAR_TOKENS = {
    "^": TokenType.CARET,
    "+": TokenType.PLUS,
    "-": TokenType.MINUS,
    "*": TokenType.STAR,
    "/": TokenType.SLASH,
    "(": TokenType.LPAREN,
    ")": TokenType.RPAREN,
}


class Lexer:
    def __init__(self, input_str: str) -> None:
        self.input_str = input_str

    def tokenise(self) -> list[Token]:
        tokens: list[Token] = []
        i = 0
        n = len(self.input_str)

        while i < n:
            curr = self.input_str[i]

            if curr.isspace():
                i += 1
                continue

            if curr.isdigit():
                start = i
                while i < n and (self.input_str[i].isdigit() or self.input_str[i] == "."):
                    i += 1
                tokens.append(Token(TokenType.NUMBER, self.input_str[start:i]))
                continue

            if curr.isalpha() or curr == "_":
                start = i
                while i < n and (self.input_str[i].isalnum() or self.input_str[i] == "_"):
                    i += 1
                tokens.append(Token(TokenType.IDENT, self.input_str[start:i]))
                continue

            op = _SINGLE_CHAR_TOKENS.get(curr)
            if op is not None:
                tokens.append(Token(op, curr))

            i += 1

        tokens.append(Token(TokenType.END, ""))
        return tokens
