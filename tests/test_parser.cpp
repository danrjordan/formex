#include <catch2/catch_test_macros.hpp>
#include "formex/Lexer.h"
#include "formex/Parser.h"
#include "formex/Printer.h"

static std::string parseAndPrint(const std::string &in) {
  Lexer lexer(in);
  auto tokens = lexer.tokenise();
  Parser parser(tokens);
  return prettyPrint(parser.constructTree());
}

TEST_CASE("parser rejects trailing tokens instead of silently truncating",
          "[parser]") {
  CHECK_THROWS_AS(parseAndPrint("sin(x)"), std::runtime_error);
  CHECK_THROWS_AS(parseAndPrint("(x+1))"), std::runtime_error);
}

TEST_CASE("parser accepts valid expressions", "[parser]") {
  CHECK(parseAndPrint("2*x^3") == "2*x^3");
  CHECK(parseAndPrint("(x+1)*2") == "(x+1)*2");
}

TEST_CASE("lexer rejects unrecognised characters instead of dropping them",
          "[lexer]") {
  Lexer lexer("x&1");
  CHECK_THROWS_AS(lexer.tokenise(), std::runtime_error);
}
