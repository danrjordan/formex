#pragma once
#include "Token.h"
#include <memory>
#include <string>

struct Expr {
  virtual ~Expr() = default;
};

using ExprPtr = std::unique_ptr<Expr>;

struct Number : Expr {
  double value;
  explicit Number(double v) : value(v) {}
};

struct Symbol : Expr {
  std::string name;
  explicit Symbol(std::string n) : name(std::move(n)) {}
};

struct BinOp : Expr {
  TokenType op;
  ExprPtr left, right;
  BinOp(TokenType op, ExprPtr l, ExprPtr r)
      : op(op), left(std::move(l)), right(std::move(r)) {}
};

struct UnaryOp : Expr {
  TokenType op;
  ExprPtr operand;
  UnaryOp(TokenType op, ExprPtr e) : op(op), operand(std::move(e)) {}
};
