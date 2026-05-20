#include "formex/Parser.h"
#include <unordered_map>

const std::unordered_map<TokenType, int> bindingPower = {{TokenType::PLUS, 1},
                                                         {TokenType::MINUS, 1},
                                                         {TokenType::STAR, 2},
                                                         {TokenType::SLASH, 2},
                                                         {TokenType::CARET, 3}};

Parser::Parser(std::vector<Token> &tokenList) { this->tokenList = tokenList; currToken = 0; }

ExprPtr Parser::constructTree() { return parseExpr(0); }

ExprPtr Parser::parseExpr(int minBP) {
  Token left = tokenList[currToken++];
  ExprPtr leftNode;

  if (left.type == TokenType::NUMBER) {
    leftNode = std::make_unique<Number>(std::stod(left.value));
  } else if (left.type == TokenType::IDENT) {
    leftNode = std::make_unique<Symbol>(left.value);
  }

  while (currToken < tokenList.size()) {
    Token op = tokenList[currToken];
    if (op.type == TokenType::END)
      break;

    auto it = bindingPower.find(op.type);
    if (it == bindingPower.end() || it->second <= minBP)
      break;

    currToken++;
    ExprPtr rightNode = parseExpr(it->second);
    leftNode = std::make_unique<BinOp>(op.type, std::move(leftNode),
                                       std::move(rightNode));
  }

  return leftNode;
}