#include "formex/Expr.h"
#include "formex/Token.h"
#include <cmath>

double evaluate(const ExprPtr &expr, double x) {
  if (auto *num = dynamic_cast<Number *>(expr.get())) {
    return num->value;
  } else if (auto *symbol = dynamic_cast<Symbol *>(expr.get())) {
    if (symbol->name == "x") {
      return x;
    }
    return 0;
  } else if (auto *binOp = dynamic_cast<BinOp *>(expr.get())) {
    double l = evaluate(binOp->left, x);
    double r = evaluate(binOp->right, x);
    switch (binOp->op) {
    case TokenType::PLUS:
      return l + r;
    case TokenType::MINUS:
      return l - r;
    case TokenType::STAR:
      return l * r;
    case TokenType::SLASH:
      return r != 0 ? l / r : 0;
    case TokenType::CARET:
      return std::pow(l, r);
    default:
      return 0;
    }
  }
  return 0;
}