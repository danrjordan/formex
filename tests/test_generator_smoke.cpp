#include "ExprGen.h"
#include "Rng.h"
#include "TreeEqual.h"
#include "formex/Lexer.h"
#include "formex/Parser.h"
#include "formex/Printer.h"
#include <catch2/catch_test_macros.hpp>

// Tagged [stats] and excluded from ctest discovery (see tests/CMakeLists.txt)
// -- this is a quick manual sanity check for the generator/round-trip
// machinery, not the full statistical run. That lives in formex_stats.

TEST_CASE("generated expressions round-trip through print/parse",
          "[stats]") {
  Rng rng(12345);
  ExprGenConfig cfg;
  for (int i = 0; i < 20; i++) {
    ExprPtr expr = generateRandomExpr(rng, cfg.maxDepth, cfg);
    std::string printed = prettyPrint(expr);

    Lexer lexer(printed);
    auto tokens = lexer.tokenise();
    Parser parser(tokens);
    ExprPtr reparsed = parser.constructTree();

    CHECK(treesEqual(expr, reparsed));
  }
}
