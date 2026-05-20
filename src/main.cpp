#include "formex/Differentiator.h"
#include "formex/Lexer.h"
#include "formex/Parser.h"
#include "formex/Printer.h"
#include "formex/Simplifier.h"
#include <iostream>

int main() {
  std::vector<std::string> tests = {
      "x + 0", "x * 1", "x * 0", "x + x", "x^2", "x^3 + x", "x * x",
  };

  for (const auto &input : tests) {
    Lexer lexer(input);
    auto tokens = lexer.tokenise();
    Parser parser(tokens);
    auto tree = parser.constructTree();
    auto simplified = simplify(tree);
    auto [result, steps] = differentiate(simplified, "x");

    std::cout << "d/dx(" << input << ") =>\n";
    for (const auto &step : steps)
      std::cout << "  " << step << "\n";
    std::cout << "  = " << prettyPrint(result) << "\n\n";
  }

  return 0;
}