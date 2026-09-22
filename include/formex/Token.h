#pragma once
#include <string>

enum class TokenType {
  NUMBER,
  IDENT,
  PLUS,
  MINUS,
  STAR,
  SLASH,
  CARET,
  LPAREN,
  RPAREN,
  END
};

struct Token {
  TokenType type;
  std::string value;
};