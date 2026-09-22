#pragma once
#include "formex/Expr.h"

// Structural equality: same shape, same operators, same symbol names, same
// numeric values. Used to check that a tree survives a print/re-parse
// round trip, independent of prettyPrint's string formatting.
bool treesEqual(const ExprPtr &a, const ExprPtr &b);
