#include "ExprGen.h"
#include "formex/Token.h"
#include <memory>

static ExprPtr generateLeaf(Rng &rng, const ExprGenConfig &cfg) {
  if (rng.nextBool(cfg.symbolLeafProbability))
    return std::make_unique<Symbol>("x");
  return std::make_unique<Number>(
      static_cast<double>(rng.nextInt(cfg.minLeafNumber, cfg.maxLeafNumber)));
}

ExprPtr generateRandomExpr(Rng &rng, int depth, const ExprGenConfig &cfg) {
  if (depth <= 0 || rng.nextBool(cfg.leafProbability))
    return generateLeaf(rng, cfg);

  int choice = rng.nextInt(0, 5); // 6 kinds: + - * / ^ unary-minus
  switch (choice) {
  case 0:
    return std::make_unique<BinOp>(TokenType::PLUS,
                                    generateRandomExpr(rng, depth - 1, cfg),
                                    generateRandomExpr(rng, depth - 1, cfg));
  case 1:
    return std::make_unique<BinOp>(TokenType::MINUS,
                                    generateRandomExpr(rng, depth - 1, cfg),
                                    generateRandomExpr(rng, depth - 1, cfg));
  case 2:
    return std::make_unique<BinOp>(TokenType::STAR,
                                    generateRandomExpr(rng, depth - 1, cfg),
                                    generateRandomExpr(rng, depth - 1, cfg));
  case 3:
    return std::make_unique<BinOp>(TokenType::SLASH,
                                    generateRandomExpr(rng, depth - 1, cfg),
                                    generateRandomExpr(rng, depth - 1, cfg));
  case 4: {
    int exponent = rng.nextInt(cfg.minExponent, cfg.maxExponent);
    return std::make_unique<BinOp>(
        TokenType::CARET, generateRandomExpr(rng, depth - 1, cfg),
        std::make_unique<Number>(static_cast<double>(exponent)));
  }
  default:
    return std::make_unique<UnaryOp>(TokenType::MINUS,
                                      generateRandomExpr(rng, depth - 1, cfg));
  }
}
