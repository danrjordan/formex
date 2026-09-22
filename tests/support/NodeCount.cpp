#include "NodeCount.h"

int countNodes(const ExprPtr &expr) {
  if (!expr)
    return 0;
  if (auto *un = dynamic_cast<UnaryOp *>(expr.get()))
    return 1 + countNodes(un->operand);
  if (auto *bin = dynamic_cast<BinOp *>(expr.get()))
    return 1 + countNodes(bin->left) + countNodes(bin->right);
  return 1; // Number or Symbol
}
