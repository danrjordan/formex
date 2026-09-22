#pragma once

#include <string>
#include <vector>

struct LinearStep {
  std::string rule;
  std::string expr;
  std::string result;
};

struct LinearSolveResult {
  bool implemented;
  std::string result;
  std::vector<LinearStep> steps;
};

// Solves a linear-algebra input (e.g. a system of equations or a matrix
// expression). Not implemented yet -- this is the seam the linear algebra
// solver will be built behind once it exists.
LinearSolveResult solveLinear(const std::string &input);
