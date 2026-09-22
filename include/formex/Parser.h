#pragma once

#include "formex/Expr.h"
#include "formex/Token.h"
#include <vector>

class Parser {
private:
  std::vector<Token> tokenList;
  size_t currToken;
  ExprPtr parseExpr(int minBP);

public:
  Parser(std::vector<Token> &tokenList);
  ExprPtr constructTree();
};