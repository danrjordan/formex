#pragma once
#include "Rng.h"
#include "formex/Expr.h"

struct ExprGenConfig {
  int maxDepth = 4;
  int minLeafNumber = 1;
  int maxLeafNumber = 9;
  int minExponent = 0;
  int maxExponent = 4;
  double leafProbability = 0.35;      // chance of stopping early, depth > 0
  double symbolLeafProbability = 0.6; // chance a leaf is 'x' vs a number
};

// Depth-limited random expression tree. Leaves are small positive integer
// literals or the symbol x. Internal nodes are +, -, *, /, ^ and unary
// minus. The right-hand side of ^ is always forced to a small constant
// integer, never a subtree, because Differentiator's power rule only
// supports constant integer exponents (see Differentiator.cpp).
ExprPtr generateRandomExpr(Rng &rng, int depth, const ExprGenConfig &cfg = {});
