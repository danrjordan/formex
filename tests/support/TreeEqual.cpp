#include "TreeEqual.h"

bool treesEqual(const ExprPtr &a, const ExprPtr &b) {
  if (!a || !b)
    return !a && !b;

  if (auto *an = dynamic_cast<Number *>(a.get())) {
    auto *bn = dynamic_cast<Number *>(b.get());
    return bn && an->value == bn->value;
  }
  if (auto *as = dynamic_cast<Symbol *>(a.get())) {
    auto *bs = dynamic_cast<Symbol *>(b.get());
    return bs && as->name == bs->name;
  }
  if (auto *au = dynamic_cast<UnaryOp *>(a.get())) {
    auto *bu = dynamic_cast<UnaryOp *>(b.get());
    return bu && au->op == bu->op && treesEqual(au->operand, bu->operand);
  }
  if (auto *ab = dynamic_cast<BinOp *>(a.get())) {
    auto *bb = dynamic_cast<BinOp *>(b.get());
    return bb && ab->op == bb->op && treesEqual(ab->left, bb->left) &&
           treesEqual(ab->right, bb->right);
  }
  return false;
}
