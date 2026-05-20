#include "formex/Expr.h"
#include "formex/Token.h"
#include <cstddef>
#include <string>

std::string getOpString(TokenType op) {
  switch (op) {
  case TokenType::PLUS:
    return "+";
  case TokenType::MINUS:
    return "-";
  case TokenType::STAR:
    return "*";
  case TokenType::SLASH:
    return "/";
  case TokenType::CARET:
    return "^";
  default:
    return "?";
  }
}

std::string prettyPrint(const ExprPtr &expr) {
  if (auto *num = dynamic_cast<Number *>(expr.get())) {
    if (num->value == static_cast<int>(num->value)) {
      return std::to_string(static_cast<int>(num->value));
    }
    return std::to_string(num->value);
  }
  if (auto *symbol = dynamic_cast<Symbol *>(expr.get())) {
    return symbol->name;
  }
  if (auto *binOp = dynamic_cast<BinOp *>(expr.get())) {
    return prettyPrint(binOp->left) + getOpString(binOp->op) +
           prettyPrint(binOp->right);
  } else {
    return "?";
  }
};