#pragma once
#include <string>
#include <vector>
#include <memory>
#include "Value.hpp"

// expression template
struct Expr {
    virtual ~Expr() = default;
};

// expresses any kind of literal: 123, "Hello", 5.5
struct LiteralExpr : Expr {
    Value value;
    LiteralExpr(Value v) : value(v) {}
};

// expresses a variable
struct VariableExpr : Expr {
    std::string name;
    VariableExpr(const std::string& n) : name(n) {}
};

// binary expr -> stuff with operators
struct BinaryExpr : Expr {
    std::unique_ptr<Expr> left;
    std::unique_ptr<Expr> right;
    std::string op;
    BinaryExpr(std::unique_ptr<Expr> l, std::string o, std::unique_ptr<Expr> r)
        : left(std::move(l)), op(o), right(std::move(r)) {}
};

// statement template
struct Stmt {
    virtual ~Stmt() = default;
};

// variable declaration
struct VarDeclStmt : Stmt {
    std::string name;
    std::string type;
    std::unique_ptr<Expr> initializer; // can be nullptr
};

// variable assignment
struct AssignStmt : Stmt {
    std::string name;
    std::unique_ptr<Expr> value;
};

// prints
struct PrintStmt : Stmt {
    std::unique_ptr<Expr> expr;
};

// expression statement
struct ExprStmt : Stmt {
    std::unique_ptr<Expr> expr;
};