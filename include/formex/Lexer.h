#pragma once
#include "Token.h"
#include <string>
#include <vector>

class Lexer {
public:
  Lexer(const std::string &inputVal);
  std::vector<Token> tokenise();

private:
  std::string inputStr;
  std::vector<Token> tokens;
};