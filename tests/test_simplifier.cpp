#include <catch2/catch_test_macros.hpp>
#include "formex/Lexer.h"
#include "formex/Parser.h"
#include "formex/Printer.h"
#include "formex/Simplifier.h"

static std::string simplifyStr(const std::string &in) {
  Lexer lexer(in);
  auto tokens = lexer.tokenise();
  Parser parser(tokens);
  auto tree = parser.constructTree();
  return prettyPrint(simplify(tree));
}

TEST_CASE("simplifier folds constants", "[simplifier]") {
  CHECK(simplifyStr("2+3") == "5");
  CHECK(simplifyStr("2*3") == "6");
  CHECK(simplifyStr("2^3") == "8");
}

TEST_CASE("simplifier reduces identical operands", "[simplifier]") {
  CHECK(simplifyStr("x-x") == "0");
  CHECK(simplifyStr("x/x") == "1");
}

TEST_CASE("simplifier formats numbers without trailing zeros",
          "[simplifier]") {
  CHECK(simplifyStr("x^0.5") == "x^0.5");
}
