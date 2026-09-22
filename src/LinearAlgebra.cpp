#include "formex/LinearAlgebra.h"

LinearSolveResult solveLinear(const std::string &input) {
  return LinearSolveResult{
      false, "linear algebra solver not implemented yet",
      {LinearStep{"not implemented", input, ""}}};
}
