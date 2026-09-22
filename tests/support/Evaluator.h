#pragma once
#include "formex/Expr.h"
#include <optional>
#include <string>

// Numeric evaluation for test/benchmark purposes only (e.g. central-difference
// derivative verification in formex_stats). Not part of the shipped engine:
// no UI code currently evaluates expressions numerically.
//
// Returns std::nullopt if evaluation hits an undefined operation (division
// by zero, 0^negative, negative base to a fractional-looking result via a
// non-integer exponent, or a non-finite intermediate value) rather than
// silently producing inf/nan, so callers can distinguish "undefined at this
// point" from a real numeric result.
std::optional<double> evaluate(const ExprPtr &expr, const std::string &var,
                                double value);
