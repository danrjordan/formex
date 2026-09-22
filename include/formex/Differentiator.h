#pragma once

#include "formex/Expr.h"
#include <string>
#include <vector>

struct Step {
  std::string rule;
  std::string expr;
  std::string result;
};

struct DiffResult {
  ExprPtr result;
  std::vector<Step> steps;
};

ExprPtr clone(const ExprPtr &expr);
DiffResult differentiate(const ExprPtr &, const std::string &);