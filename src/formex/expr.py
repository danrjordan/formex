from __future__ import annotations

from dataclasses import dataclass

from .tokens import TokenType


class Expr:
    """Base class for AST nodes."""


@dataclass
class Number(Expr):
    value: float


@dataclass
class Symbol(Expr):
    name: str


@dataclass
class BinOp(Expr):
    op: TokenType
    left: Expr
    right: Expr


@dataclass
class UnaryOp(Expr):
    op: TokenType
    operand: Expr
