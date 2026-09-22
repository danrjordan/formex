#include "Evaluator.h"
#include "formex/Token.h"
#include <cmath>

static std::optional<double> checkFinite(double v) {
  if (!std::isfinite(v))
    return std::nullopt;
  return v;
}

std::optional<double> evaluate(const ExprPtr &expr, const std::string &var,
                                double value) {
  if (auto *num = dynamic_cast<Number *>(expr.get()))
    return checkFinite(num->value);

  if (auto *sym = dynamic_cast<Symbol *>(expr.get())) {
    if (sym->name == var)
      return checkFinite(value);
    return std::nullopt; // unbound variable
  }

  if (auto *un = dynamic_cast<UnaryOp *>(expr.get())) {
    auto operand = evaluate(un->operand, var, value);
    if (!operand)
      return std::nullopt;
    return checkFinite(-*operand);
  }

  if (auto *bin = dynamic_cast<BinOp *>(expr.get())) {
    auto l = evaluate(bin->left, var, value);
    if (!l)
      return std::nullopt;
    auto r = evaluate(bin->right, var, value);
    if (!r)
      return std::nullopt;

    switch (bin->op) {
    case TokenType::PLUS:
      return checkFinite(*l + *r);
    case TokenType::MINUS:
      return checkFinite(*l - *r);
    case TokenType::STAR:
      return checkFinite(*l * *r);
    case TokenType::SLASH:
      if (*r == 0.0)
        return std::nullopt;
      return checkFinite(*l / *r);
    case TokenType::CARET:
      return checkFinite(std::pow(*l, *r));
    default:
      return std::nullopt;
    }
  }

  return std::nullopt;
}
