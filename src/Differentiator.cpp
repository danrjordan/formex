#include "formex/Differentiator.h"
#include "formex/Expr.h"
#include "formex/Token.h"
#include <algorithm>
#include <cstddef>
#include <memory>

ExprPtr clone(const ExprPtr &expr) {
  if (auto *num = dynamic_cast<Number *>(expr.get()))
    return std::make_unique<Number>(num->value);
  if (auto *sym = dynamic_cast<Symbol *>(expr.get()))
    return std::make_unique<Symbol>(sym->name);
  if (auto *bin = dynamic_cast<BinOp *>(expr.get()))
    return std::make_unique<BinOp>(bin->op, clone(bin->left),
                                   clone(bin->right));
  return nullptr;
}

DiffResult differentiate(const ExprPtr &expr, const std::string &var) {
  if (auto *num = dynamic_cast<Number *>(expr.get())) {
    return DiffResult{std::make_unique<Number>(0),
                      {"constant rule: d/dx(n) = 0"}};
  }
  if (auto *symbol = dynamic_cast<Symbol *>(expr.get())) {
    if (symbol->name == var) {
      return DiffResult{std::make_unique<Number>(1),
                        {"variable rule: d/dx(n) = 1"}};
    }
    if (symbol->name != var) {
      return DiffResult{std::make_unique<Number>(0),
                        {"constant rule: d/dx(n) = 0"}};
    }
  }
  if (auto *binOp = dynamic_cast<BinOp *>(expr.get())) {
    if (binOp->op == TokenType::PLUS) {
      auto lResult = differentiate(binOp->left, var);
      auto rResult = differentiate(binOp->right, var);
      std::vector<std::string> steps;
      steps.insert(steps.end(), lResult.steps.begin(), lResult.steps.end());
      steps.insert(steps.end(), rResult.steps.begin(), rResult.steps.end());
      steps.push_back("sum rule: d/dx(f+g) = f' + g'");
      return DiffResult{std::make_unique<BinOp>(TokenType::PLUS,
                                                std::move(lResult.result),
                                                std::move(rResult.result)),
                        std::move(steps)};
    }
    if (binOp->op == TokenType::STAR) {
      auto lResult = differentiate(binOp->left, var);
      auto rResult = differentiate(binOp->right, var);
      std::vector<std::string> steps;
      steps.insert(steps.end(), lResult.steps.begin(), lResult.steps.end());
      steps.insert(steps.end(), rResult.steps.begin(), rResult.steps.end());
      steps.push_back("product rule: d/dx(f*g) = f'g + fg'");

      ExprPtr left = std::make_unique<BinOp>(
          TokenType::STAR, std::move(lResult.result), clone(binOp->right));
      ExprPtr right = std::make_unique<BinOp>(
          TokenType::STAR, clone(binOp->left), std::move(rResult.result));
      return DiffResult{std::make_unique<BinOp>(
                            TokenType::PLUS, std::move(left), std::move(right)),
                        std::move(steps)};
    }
    if (binOp->op == TokenType::CARET) {
      auto *n = dynamic_cast<Number *>(binOp->right.get());
      if (n != nullptr) {
        std::vector<std::string> steps;
        steps.push_back("power rule: d/dx(x^n) = n*x^(n-1)");
        return DiffResult{std::make_unique<BinOp>(
                              TokenType::STAR,
                              std::make_unique<Number>(n->value),
                              std::make_unique<BinOp>(
                                  TokenType::CARET, clone(binOp->left),
                                  std::make_unique<Number>(n->value - 1))),
                          std::move(steps)};
      } else {
        return DiffResult{nullptr, {"unsupported: non-constant exponent"}};
      }
    }
  }
  return DiffResult{nullptr, {}};
}
