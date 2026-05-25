#include "formex/Differentiator.h"
#include "formex/Expr.h"
#include "formex/Printer.h"
#include "formex/Token.h"
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
                      {{"constant rule: d/dx(n) = 0", prettyPrint(expr), "0"}}};
  }
  if (auto *symbol = dynamic_cast<Symbol *>(expr.get())) {
    if (symbol->name == var) {
      return DiffResult{
          std::make_unique<Number>(1),
          {{"variable rule: d/dx(x) = 1", prettyPrint(expr), "1"}}};
    } else {
      return DiffResult{
          std::make_unique<Number>(0),
          {{"constant rule: d/dx(n) = 0", prettyPrint(expr), "0"}}};
    }
  }
  if (auto *binOp = dynamic_cast<BinOp *>(expr.get())) {
    if (binOp->op == TokenType::PLUS) {
      auto lResult = differentiate(binOp->left, var);
      auto rResult = differentiate(binOp->right, var);
      ExprPtr result = std::make_unique<BinOp>(
          TokenType::PLUS, clone(lResult.result), clone(rResult.result));
      std::vector<Step> steps;
      steps.push_back({"sum rule: d/dx(f+g) = f' + g'", prettyPrint(expr),
                       prettyPrint(result)});
      steps.insert(steps.end(), lResult.steps.begin(), lResult.steps.end());
      steps.insert(steps.end(), rResult.steps.begin(), rResult.steps.end());
      return DiffResult{std::move(result), std::move(steps)};
    }
    if (binOp->op == TokenType::STAR) {
      if (auto *l = dynamic_cast<Number *>(binOp->left.get())) {
        auto rResult = differentiate(binOp->right, var);
        ExprPtr result = std::make_unique<BinOp>(
            TokenType::STAR, std::make_unique<Number>(l->value),
            clone(rResult.result));
        std::vector<Step> steps;
        steps.push_back({"constant multiple rule: d/dx(c*f) = c*f'",
                         prettyPrint(expr), prettyPrint(result)});
        steps.insert(steps.end(), rResult.steps.begin(), rResult.steps.end());
        return DiffResult{std::move(result), std::move(steps)};
      }
      if (auto *r = dynamic_cast<Number *>(binOp->right.get())) {
        auto lResult = differentiate(binOp->left, var);
        ExprPtr result = std::make_unique<BinOp>(
            TokenType::STAR, std::make_unique<Number>(r->value),
            clone(lResult.result));
        std::vector<Step> steps;
        steps.push_back({"constant multiple rule: d/dx(f*c) = c*f'",
                         prettyPrint(expr), prettyPrint(result)});
        steps.insert(steps.end(), lResult.steps.begin(), lResult.steps.end());
        return DiffResult{std::move(result), std::move(steps)};
      }
      auto lResult = differentiate(binOp->left, var);
      auto rResult = differentiate(binOp->right, var);
      ExprPtr left = std::make_unique<BinOp>(
          TokenType::STAR, clone(lResult.result), clone(binOp->right));
      ExprPtr right = std::make_unique<BinOp>(
          TokenType::STAR, clone(binOp->left), clone(rResult.result));
      ExprPtr result = std::make_unique<BinOp>(
          TokenType::PLUS, clone(left), clone(right));
      std::vector<Step> steps;
      steps.push_back({"product rule: d/dx(f*g) = f'g + fg'", prettyPrint(expr),
                       prettyPrint(result)});
      steps.insert(steps.end(), lResult.steps.begin(), lResult.steps.end());
      steps.insert(steps.end(), rResult.steps.begin(), rResult.steps.end());
      return DiffResult{std::move(result), std::move(steps)};
    }
    if (binOp->op == TokenType::CARET) {
      auto *n = dynamic_cast<Number *>(binOp->right.get());
      if (n != nullptr) {
        auto innerResult = differentiate(binOp->left, var);
        ExprPtr powerPart = std::make_unique<BinOp>(
            TokenType::STAR, std::make_unique<Number>(n->value),
            std::make_unique<BinOp>(TokenType::CARET, clone(binOp->left),
                                    std::make_unique<Number>(n->value - 1)));
        std::vector<Step> steps;
        steps.push_back({"power rule: d/dx(f^n) = n*f^(n-1)*f'",
                         prettyPrint(expr), prettyPrint(powerPart)});
        steps.insert(steps.end(), innerResult.steps.begin(),
                     innerResult.steps.end());
        ExprPtr result = std::make_unique<BinOp>(
            TokenType::STAR, std::move(powerPart),
            std::move(innerResult.result));
        return DiffResult{std::move(result), std::move(steps)};
      } else {
        return DiffResult{nullptr,
                          {{"unsupported: non-constant exponent", "", ""}}};
      }
    }
  }
  return DiffResult{nullptr, {}};
}
