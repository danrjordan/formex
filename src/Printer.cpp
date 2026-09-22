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

static int precedence(TokenType op) {
  switch (op) {
  case TokenType::PLUS:
  case TokenType::MINUS:
    return 1;
  case TokenType::STAR:
  case TokenType::SLASH:
    return 2;
  case TokenType::CARET:
    return 3;
  default:
    return 4;
  }
}

static std::string printExpr(const ExprPtr &expr, int parentPrec) {
  if (auto *num = dynamic_cast<Number *>(expr.get())) {
    if (num->value == static_cast<int>(num->value)) {
      return std::to_string(static_cast<int>(num->value));
    }
    return std::to_string(num->value);
  }
  if (auto *symbol = dynamic_cast<Symbol *>(expr.get())) {
    return symbol->name;
  }
  if (auto *unaryOp = dynamic_cast<UnaryOp *>(expr.get())) {
    std::string s = "-" + printExpr(unaryOp->operand, 3);
    return parentPrec > 2 ? "(" + s + ")" : s;
  }
  if (auto *binOp = dynamic_cast<BinOp *>(expr.get())) {
    int prec = precedence(binOp->op);
    std::string s = printExpr(binOp->left, prec) + getOpString(binOp->op) +
                     printExpr(binOp->right, prec + 1);
    return prec < parentPrec ? "(" + s + ")" : s;
  } else {
    return "?";
  }
};

std::string prettyPrint(const ExprPtr &expr) { return printExpr(expr, 0); }