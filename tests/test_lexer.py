from formex.lexer import Lexer
from formex.tokens import TokenType


def test_lexer_produces_expected_token_types():
    toks = Lexer("2*x^3").tokenise()

    assert len(toks) == 6  # 2 * x ^ 3 END
    assert toks[0].type == TokenType.NUMBER
    assert toks[0].value == "2"
    assert toks[1].type == TokenType.STAR
    assert toks[-1].type == TokenType.END


def test_lexer_handles_decimals():
    toks = Lexer("3.5").tokenise()
    assert len(toks) == 2
    assert toks[0].value == "3.5"
