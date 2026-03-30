#include <iostream>
#include <memory>
#include "include/Compiler.hpp"
#include "include/vm.hpp"

// Fake AST nodes for testing
struct Expr { virtual ~Expr() = default; };
struct LiteralExpr : Expr { 
    Value value; 
    LiteralExpr(Value v) : value(v) {}
};
struct VariableExpr : Expr {
    std::string name;
    VariableExpr(std::string n) : name(n) {}
};
struct BinaryExpr : Expr {
    std::unique_ptr<Expr> left, right;
    std::string op;
    BinaryExpr(std::unique_ptr<Expr> l, std::string o, std::unique_ptr<Expr> r)
        : left(std::move(l)), op(o), right(std::move(r)) {}
};

// Fake statements
struct Stmt { virtual ~Stmt() = default; };
struct VarDeclStmt : Stmt {
    std::string name;
    std::string type;
    std::unique_ptr<Expr> initializer;
    VarDeclStmt(std::string n, std::string t, std::unique_ptr<Expr> init = nullptr)
        : name(n), type(t), initializer(std::move(init)) {}
};
struct PrintStmt : Stmt {
    std::unique_ptr<Expr> expr;
    PrintStmt(std::unique_ptr<Expr> e) : expr(std::move(e)) {}
};

int main() {
    AssignmentExpr assignX;
    assignX.name = "x";
    BinaryExpr binExpr;
    binExpr.left = new LiteralExpr{5};
    binExpr.right = new LiteralExpr{3};
    binExpr.op = "+";
    assignX.value = &binExpr;
    
    // print(x)
    PrintStmt printX;
    printX.expression = new VariableExpr{"x"};
    
    // compile
    Compiler compiler;
    compiler.compileStmt(&assignX);
    compiler.compileStmt(&printX);
    compiler.emitOp(OpCode::OP_HALT);
    
    // run VM
    ISI_VM vm;
    vm.load(compiler.bytecode, compiler.constants);
    vm.run();
    return 0;
}