#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include "Value.hpp"
#include "Opcodes.hpp"
#include "AST.hpp"

class Compiler {
public:
    std::vector<uint> bytecode;
    std::vector<Value> constants;
    std::unordered_map<std::string, uint> globalsMap;

    void compileStmt(const Stmt* stmt) {
        if (auto decl = dynamic_cast<const VarDeclStmt*>(stmt)) {
            compileVarDecl(decl);
        } else if (auto assign = dynamic_cast<const AssignStmt*>(stmt)) {
            compileAssign(assign);
        } else if (auto print = dynamic_cast<const PrintStmt*>(stmt)) {
            compilePrint(print);
        } else if (auto expr = dynamic_cast<const ExprStmt*>(stmt)) {
            compileExpr(expr->expr.get());
        }
    }

private:
    void emitByte(uint byte) { bytecode.push_back(byte); }
    void emitOp(OpCode op) { emitByte(static_cast<uint>(op)); }
    uint addConstant(Value val) {
        constants.push_back(val);
        return (uint)(constants.size() - 1);
    }

    void compileVarDecl(const VarDeclStmt* decl) {
        Value val = decl->initializer ? compileExpr(decl->initializer.get()) : Value{};
        uint constIdx = addConstant(val);
        emitOp(OpCode::OP_CONSTANT);
        emitByte(constIdx);

        uint globalIdx = globalsMap.size();
        globalsMap[decl->name] = globalIdx;
        emitOp(OpCode::OP_SET_GLOBAL);
        emitByte(globalIdx);
    }

    void compileAssign(const AssignStmt* assign) {
        Value val = compileExpr(assign->value.get());
        uint constIdx = addConstant(val);
        emitOp(OpCode::OP_CONSTANT);
        emitByte(constIdx);

        uint globalIdx = globalsMap[assign->name];
        emitOp(OpCode::OP_SET_GLOBAL);
        emitByte(globalIdx);
    }

    void compilePrint(const PrintStmt* print) {
        Value val = compileExpr(print->expr.get());
        uint constIdx = addConstant(val);
        emitOp(OpCode::OP_CONSTANT);
        emitByte(constIdx);

        emitOp(OpCode::OP_PRINT);
    }

    Value compileExpr(const Expr* expr) {
        if (auto lit = dynamic_cast<const LiteralExpr*>(expr)) {
            uint constIdx = addConstant(lit->value);
            emitOp(OpCode::OP_CONSTANT);
            emitByte(constIdx);
            return lit->value;
        }
        // TODO: handle VariableExpr and BinaryExpr
        return {};
    }
};