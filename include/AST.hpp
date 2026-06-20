#pragma once
#include <string>
#include <vector>
#include <memory>
#include "Value.hpp"

struct Node {
    virtual ~Node() = default;
};

struct Expr : public Node {};

struct Stmt : public Node {};


struct LiteralExpr : public Expr {
    Value value;
    LiteralExpr(Value v) : value(std::move(v)) {}
};

struct VariableExpr : public Expr {
    std::string name;
    VariableExpr(std::string n) : name(std::move(n)) {}
};

struct BinaryExpr : public Expr {
    std::unique_ptr<Expr> left;
    Token op;
    std::unique_ptr<Expr> right;
    BinaryExpr(std::unique_ptr<Expr> l, Token o, std::unique_ptr<Expr> r)
        : left(std::move(l)), op(std::move(o)), right(std::move(r)) {}
};

struct AssignExpr : public Expr {
    std::string name;
    std::unique_ptr<Expr> value;
    AssignExpr(std::string n, std::unique_ptr<Expr> v)
        : name(std::move(n)), value(std::move(v)) {}
};

struct IndexExpr : public Expr {
    std::unique_ptr<Expr> object;
    std::unique_ptr<Expr> index;
    IndexExpr(std::unique_ptr<Expr> obj, std::unique_ptr<Expr> idx)
        : object(std::move(obj)), index(std::move(idx)) {}
};

struct LoadLibExpr : public Expr {
    std::string path;
    LoadLibExpr(std::string fname) : path(std::move(fname)) {}
};

struct ExpressionStmt : public Stmt {
    std::unique_ptr<Expr> expression;
    ExpressionStmt(std::unique_ptr<Expr> e) : expression(std::move(e)) {}
};


struct VarDeclStmt : public Stmt {
    DataType type;
    std::string name;
    std::unique_ptr<Expr> initializer;
    bool isConstant;
    VarDeclStmt(DataType t, std::string n, std::unique_ptr<Expr> init, bool isCon)
        : type(t), name(std::move(n)), initializer(std::move(init)), isConstant(isCon) {}
};

struct PrintStmt : public Stmt {
    std::vector<std::unique_ptr<Expr>> arguments;
    PrintStmt(std::vector<std::unique_ptr<Expr>> args) : arguments(std::move(args)) {}
};

struct ThrowStmt : public Stmt {
    // 3 arguments
    std::unique_ptr<Expr> message;
    std::unique_ptr<Expr> ident;
    std::unique_ptr<Expr> errorCode;
    ThrowStmt(std::unique_ptr<Expr> msg, std::unique_ptr<Expr> id, std::unique_ptr<Expr> errc) : message(std::move(msg)), ident(std::move(id)) , errorCode(std::move(errc)) {}
};

struct ExitStmt : public Stmt {
    std::unique_ptr<Expr> argument;
    ExitStmt(std::unique_ptr<Expr> args) : argument(std::move(args)) {}
};

struct BlockStmt : public Stmt {
    std::vector<std::unique_ptr<Stmt>> statements;
    BlockStmt(std::vector<std::unique_ptr<Stmt>> stmts) : statements(std::move(stmts)) {}
};


struct ImportStmt : public Stmt {
    std::string path;
    ImportStmt(std::string p) : path(std::move(p)) {}
};

struct LibStmt : public Stmt {
    std::string handle;
    std::string libPath;
    LibStmt(std::string h, std::string p) : handle(std::move(h)), libPath(std::move(p)) {}
};

struct Param {
    DataType type;
    std::string name;
};

struct Annotation {
    std::string name;
    std::vector<std::string> arguments;

    Annotation(std::string n, std::vector<std::string> args) : name(std::move(n)), arguments(std::move(args)) {}

    Annotation(std::string n) : name(std::move(n)){}
};

struct ExternStmt : public Stmt {
    std::string libHandle;
    std::string funcName;
    DataType returnType;
    std::vector<Param> params;
    ExternStmt(std::string h, std::string f, DataType r, std::vector<Param> p)
        : libHandle(std::move(h)), funcName(std::move(f)), returnType(r), params(std::move(p)) {}
};

struct IfStmt : public Stmt {
    std::unique_ptr<Expr> condition;
    std::vector<std::unique_ptr<Stmt>> thenBranch;
    // else can be empty
    std::vector<std::unique_ptr<Stmt>> elseBranch;
    IfStmt(std::unique_ptr<Expr> cond, 
            std::vector<std::unique_ptr<Stmt>> thenBr, 
            std::vector<std::unique_ptr<Stmt>> elseBr)
            : condition(std::move(cond)), 
            thenBranch(std::move(thenBr)), 
            elseBranch(std::move(elseBr)) {}
};

struct WhileStmt : public Stmt {
    std::unique_ptr<Expr> condition;
    std::vector<std::unique_ptr<Stmt>> body;
    WhileStmt(std::unique_ptr<Expr> cond,
            std::vector<std::unique_ptr<Stmt>> b) : condition(std::move(cond)), body(std::move(b)) {}
};

class FunctionDeclStmt : public Stmt {
public:
    DataType returnType;
    std::string name;
    std::vector<Parameter> params;
    std::vector<Token> body;
    std::vector<Annotation> annotations;
    bool isVoid;

    FunctionDeclStmt(DataType retType, std::string n, std::vector<Parameter> p, std::vector<Token> b, bool v, std::vector<Annotation> an)
        : returnType(retType), name(std::move(n)), params(std::move(p)), body(std::move(b)), isVoid(v), annotations(std::move(an)) {}
};

class CallExpr : public Expr {
public:
    std::string callee;
    std::vector<std::unique_ptr<Expr>> arguments;

    CallExpr(std::string c, std::vector<std::unique_ptr<Expr>> args)
        : callee(std::move(c)), arguments(std::move(args)) {}
};

class ReturnStmt : public Stmt {
public:
    std::unique_ptr<Expr> value;
    ReturnStmt(std::unique_ptr<Expr> val) : value(std::move(val)) {}
};

class StaticCastExpr : public Expr {
public:
    std::unique_ptr<Expr> value;
    DataType targetType;
    StaticCastExpr(DataType t, std::unique_ptr<Expr> val) : value(std::move(val)), targetType(std::move(t)) {};
};

class StructDeclStmt : public Stmt {
    public:
    std::string name;
    std::vector<std::unique_ptr<Stmt>> members;
    StructDeclStmt(std::string n, std::vector<std::unique_ptr<Stmt>> m)
        : name(std::move(n)), members(std::move(m)) {}
};

class MemberAccessExpr : public Expr {
    public:
    std::unique_ptr<Expr> object;
    std::string member;
    MemberAccessExpr(std::unique_ptr<Expr> obj, std::string mem)
        : object(std::move(obj)), member(std::move(mem)) {}
};

class MethodCallExpr : public Expr {
    public:
    std::unique_ptr<Expr> object;
    std::string methodName;
    std::vector<std::unique_ptr<Expr>> arguments;
    MethodCallExpr(std::unique_ptr<Expr> obj, std::string name, std::vector<std::unique_ptr<Expr>> args)
        : object(std::move(obj)), methodName(std::move(name)), arguments(std::move(args)) {}
};

class OverloadDeclStmt : public Stmt {
    public:
    std::string opName;
    std::unique_ptr<FunctionDeclStmt> method;
    OverloadDeclStmt(std::string op, std::unique_ptr<FunctionDeclStmt> m)
        : opName(std::move(op)), method(std::move(m)) {}
};
