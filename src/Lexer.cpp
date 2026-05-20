#include "formex/Lexer.h"
#include <cctype>

Lexer::Lexer(const std::string &input) { this->inputStr = input; }

std::vector<Token> Lexer::tokenise() {
  std::vector<Token> tokens;
  size_t i = 0;

  while (i < inputStr.size()) {
    char curr = inputStr[i];

    if (std::isspace(static_cast<unsigned char>(curr))) {
      ++i;
      continue;
    }

    if (std::isdigit(static_cast<unsigned char>(curr))) {
      size_t start = i;
      while (i < inputStr.size() &&
             (std::isdigit(static_cast<unsigned char>(inputStr[i])) ||
              inputStr[i] == '.'))
        ++i;
      tokens.push_back(
          Token{TokenType::NUMBER, inputStr.substr(start, i - start)});
      continue;
    }

    if (std::isalpha(static_cast<unsigned char>(curr)) || curr == '_') {
      size_t start = i;
      while (i < inputStr.size() &&
             (std::isalnum(static_cast<unsigned char>(inputStr[i])) ||
              inputStr[i] == '_'))
        ++i;
      tokens.push_back(
          Token{TokenType::IDENT, inputStr.substr(start, i - start)});
      continue;
    }

    switch (curr) {
    case '^':
      tokens.push_back(Token{TokenType::CARET, "^"});
      break;
    case '+':
      tokens.push_back(Token{TokenType::PLUS, "+"});
      break;
    case '-':
      tokens.push_back(Token{TokenType::MINUS, "-"});
      break;
    case '*':
      tokens.push_back(Token{TokenType::STAR, "*"});
      break;
    case '/':
      tokens.push_back(Token{TokenType::SLASH, "/"});
      break;
    case '(':
      tokens.push_back(Token{TokenType::LPAREN, "("});
      break;
    case ')':
      tokens.push_back(Token{TokenType::RPAREN, ")"});
      break;
    default:
      break;
    }
    i++;
  }

  tokens.push_back(Token{TokenType::END, ""});
  return tokens;
}