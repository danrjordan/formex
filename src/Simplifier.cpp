#include "formex/Simplifier.h"
#include "formex/Expr.h"
#include <memory>

ExprPtr simplify(const ExprPtr &expr) {
  if (auto *num = dynamic_cast<Number *>(expr.get())) {
    return std::make_unique<Number>(num->value);
  }
  if (auto *symbol = dynamic_cast<Symbol *>(expr.get())) {
    return std::make_unique<Symbol>(symbol->name);
  }
  if (auto *binOp = dynamic_cast<BinOp *>(expr.get())) {
    ExprPtr left = simplify(binOp->left);
    ExprPtr right = simplify(binOp->right);
    if (binOp->op == TokenType::PLUS) {
      if (auto *r = dynamic_cast<Number *>(right.get()); r && r->value == 0)
        return left;
      if (auto *l = dynamic_cast<Number *>(left.get()); l && l->value == 0)
        return right;
      if (auto *l = dynamic_cast<Symbol *>(left.get()),
          *r = dynamic_cast<Symbol *>(right.get());
          l && r && l->name == r->name)
        return std::make_unique<BinOp>(TokenType::STAR,
                                       std::make_unique<Number>(2),
                                       std::make_unique<Symbol>(l->name));
    }
    if (binOp->op == TokenType::STAR) {
      if (auto *r = dynamic_cast<Number *>(right.get()); r && r->value == 1)
        return left;
      if (auto *l = dynamic_cast<Number *>(left.get()); l && l->value == 1)
        return right;
      if (auto *r = dynamic_cast<Number *>(right.get()); r && r->value == 0)
        return std::make_unique<Number>(0);
      if (auto *l = dynamic_cast<Number *>(left.get()); l && l->value == 0)
        return std::make_unique<Number>(0);
    }
    return std::make_unique<BinOp>(binOp->op, std::move(left),
                                   std::move(right));
  }
  return nullptr;
}
