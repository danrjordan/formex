#pragma once

#include "formex/Expr.h"
#include <string>
#include <vector>

struct DiffResult {
  ExprPtr result;
  std::vector<std::string> steps;
};

DiffResult differentiate(const ExprPtr &, const std::string &);