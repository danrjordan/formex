#include "formex/Simplifier.h"
#include "formex/Expr.h"
#include "formex/Printer.h"
#include <cmath>
#include <memory>

static ExprPtr simplifyOnce(const ExprPtr &expr) {
  if (auto *num = dynamic_cast<Number *>(expr.get())) {
    return std::make_unique<Number>(num->value);
  }
  if (auto *symbol = dynamic_cast<Symbol *>(expr.get())) {
    return std::make_unique<Symbol>(symbol->name);
  }
  if (auto *binOp = dynamic_cast<BinOp *>(expr.get())) {
    ExprPtr left = simplifyOnce(binOp->left);
    ExprPtr right = simplifyOnce(binOp->right);

    auto *lNum = dynamic_cast<Number *>(left.get());
    auto *rNum = dynamic_cast<Number *>(right.get());

    // Constant folding: both sides are literals.
    if (lNum && rNum) {
      switch (binOp->op) {
      case TokenType::PLUS:
        return std::make_unique<Number>(lNum->value + rNum->value);
      case TokenType::MINUS:
        return std::make_unique<Number>(lNum->value - rNum->value);
      case TokenType::STAR:
        return std::make_unique<Number>(lNum->value * rNum->value);
      case TokenType::CARET:
        return std::make_unique<Number>(std::pow(lNum->value, rNum->value));
      case TokenType::SLASH:
        // Leave n/0 unfolded rather than producing inf/nan.
        if (rNum->value != 0)
          return std::make_unique<Number>(lNum->value / rNum->value);
        break;
      default:
        break;
      }
    }

    // Two structurally identical operands (cheap check via pretty-printing).
    bool sameOperand = prettyPrint(left) == prettyPrint(right);

    if (binOp->op == TokenType::PLUS) {
      if (rNum && rNum->value == 0)
        return left;
      if (lNum && lNum->value == 0)
        return right;
      if (sameOperand)
        return std::make_unique<BinOp>(TokenType::STAR,
                                       std::make_unique<Number>(2),
                                       std::move(left));
    }

    if (binOp->op == TokenType::MINUS) {
      if (rNum && rNum->value == 0)
        return left;
      if (sameOperand)
        return std::make_unique<Number>(0);
      if (lNum && lNum->value == 0)
        return std::make_unique<UnaryOp>(TokenType::MINUS, std::move(right));
    }

    if (binOp->op == TokenType::STAR) {
      if (rNum && rNum->value == 1)
        return left;
      if (lNum && lNum->value == 1)
        return right;
      if ((rNum && rNum->value == 0) || (lNum && lNum->value == 0))
        return std::make_unique<Number>(0);
    }

    if (binOp->op == TokenType::SLASH) {
      if (rNum && rNum->value == 1)
        return left;
      if (lNum && lNum->value == 0 && !(rNum && rNum->value == 0))
        return std::make_unique<Number>(0);
      if (sameOperand && !(lNum && lNum->value == 0))
        return std::make_unique<Number>(1);
    }

    if (binOp->op == TokenType::CARET) {
      if (rNum && rNum->value == 1)
        return left;
      if (rNum && rNum->value == 0)
        return std::make_unique<Number>(1);
    }

    return std::make_unique<BinOp>(binOp->op, std::move(left),
                                   std::move(right));
  }
  if (auto *unaryOp = dynamic_cast<UnaryOp *>(expr.get())) {
    ExprPtr operand = simplifyOnce(unaryOp->operand);
    if (auto *num = dynamic_cast<Number *>(operand.get()))
      return std::make_unique<Number>(-num->value);
    if (auto *inner = dynamic_cast<UnaryOp *>(operand.get()))
      return std::move(inner->operand); // -(-e) -> e
    return std::make_unique<UnaryOp>(unaryOp->op, std::move(operand));
  }
  return nullptr;
}

ExprPtr simplify(const ExprPtr &expr) {
  ExprPtr current = simplifyOnce(expr);
  while (true) {
    ExprPtr next = simplifyOnce(current);
    if (prettyPrint(next) == prettyPrint(current))
      return current;
    current = std::move(next);
  }
}