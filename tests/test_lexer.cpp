#include <catch2/catch_test_macros.hpp>
#include "formex/Lexer.h"

TEST_CASE("lexer produces expected token types", "[lexer]") {
    Lexer lex("2*x^3");
    auto toks = lex.tokenise();

    REQUIRE(toks.size() == 6);          // 2 * x ^ 3 END
    CHECK(toks[0].type == TokenType::NUMBER);
    CHECK(toks[0].value == "2");
    CHECK(toks[1].type == TokenType::STAR);
    CHECK(toks.back().type == TokenType::END);
}

TEST_CASE("lexer handles decimals", "[lexer]") {
    Lexer lex("3.5");
    auto toks = lex.tokenise();
    REQUIRE(toks.size() == 2);
    CHECK(toks[0].value == "3.5");
}
