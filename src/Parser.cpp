#include "formex/Parser.h"
#include <stdexcept>
#include <unordered_map>

const std::unordered_map<TokenType, int> bindingPower = {{TokenType::PLUS, 1},
                                                         {TokenType::MINUS, 1},
                                                         {TokenType::STAR, 2},
                                                         {TokenType::SLASH, 2},
                                                         {TokenType::CARET, 3}};

Parser::Parser(std::vector<Token> &tokenList) {
  this->tokenList = tokenList;
  currToken = 0;
}

ExprPtr Parser::constructTree() { return parseExpr(0); }

ExprPtr Parser::parseExpr(int minBP) {
  if (currToken >= tokenList.size())
    throw std::runtime_error("unexpected end of input");

  Token left = tokenList[currToken++];
  ExprPtr leftNode;

  if (left.type == TokenType::NUMBER) {
    leftNode = std::make_unique<Number>(std::stod(left.value));
  } else if (left.type == TokenType::IDENT) {
    leftNode = std::make_unique<Symbol>(left.value);
  } else if (left.type == TokenType::MINUS) {
    leftNode = std::make_unique<UnaryOp>(TokenType::MINUS, parseExpr(2));
  } else if (left.type == TokenType::LPAREN) {
    leftNode = parseExpr(0);
    if (currToken >= tokenList.size() ||
        tokenList[currToken].type != TokenType::RPAREN)
      throw std::runtime_error("expected closing parenthesis");
    currToken++;
  } else {
    throw std::runtime_error("unexpected token: " + left.value);
  }

  while (currToken < tokenList.size()) {
    Token op = tokenList[currToken];
    if (op.type == TokenType::END)
      break;

    bool implicitMul = (op.type == TokenType::IDENT || op.type == TokenType::NUMBER);
    if (implicitMul) {
      if (2 <= minBP)
        break;
      ExprPtr rightNode = parseExpr(2);
      leftNode = std::make_unique<BinOp>(TokenType::STAR, std::move(leftNode),
                                         std::move(rightNode));
      continue;
    }

    auto it = bindingPower.find(op.type);
    if (it == bindingPower.end() || it->second <= minBP)
      break;

    currToken++;
    int rightBP = op.type == TokenType::CARET ? it->second - 1 : it->second;
    ExprPtr rightNode = parseExpr(rightBP);
    leftNode = std::make_unique<BinOp>(op.type, std::move(leftNode),
                                       std::move(rightNode));
  }

  return leftNode;
}