#pragma once
#include "formex/Expr.h"

// Total number of nodes in the tree (Number/Symbol leaves count as 1).
int countNodes(const ExprPtr &expr);
