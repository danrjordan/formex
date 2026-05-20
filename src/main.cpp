#include "formex/Lexer.h"
#include "formex/Parser.h"
#include "formex/Printer.h"
#include "formex/Simplifier.h"
#include <iostream>

int main() {
  std::vector<std::string> tests = {
      "x + 0",
      "x * 0 + 3",
      "x^2 + 3*x",
      "x + y * z",
  };

  for (const auto &input : tests) {
    Lexer lexer(input);
    auto tokens = lexer.tokenise();
    Parser parser(tokens);
    auto tree = parser.constructTree();
    std::cout << input << " => " << prettyPrint(simplify(tree)) << "\n";
  }

  return 0;
}