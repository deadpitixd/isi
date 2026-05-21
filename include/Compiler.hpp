#ifndef __PITI_COMPILER_HPP
#define __PITI_COMPILER_HPP
#include <AST.hpp>
#include <iostream>
#include <vector>
#include <Other.hpp>
#include <map>
#include <cmath>
#include <variant>

struct Symbol {
    int index;
    DataType type;
    bool isConst=false;
    Value constValue;
};

class Compiler{
    private:
    DataType currentFunctionReturnType = DataType::INT;
    std::map<std::string, Function> functionTable;
    bool isCompilingFunction = false;
    int nextLocalIndex = 0;
    std::map<std::string, int> localSymbolTable;
    std::map<std::string, DataType> localTypes;
    std::map<std::string, std::string> libraryTable;
    std::map<std::string, Symbol> symbolTable;
    int nextAvailableIndex = 1;
    std::vector<Instruction> Code;
    std::vector<DataType> indexTypes;
    
    void emitByte(uint32_t byte) {
        Code.push_back({static_cast<OpCode>(byte), {}});
    }

    void emitInstruction(OpCode op, Value val) {
        Code.push_back({op, val});
    }
    std::vector<Token> errTokens;
    int current = 0;
    bool isAtEnd() {
        return current >= errTokens.size() || errTokens[current].type == TOKEN_EOF;
    }

    Token peek(int distance) {
        if (current + distance >= errTokens.size()) {
            return errTokens.back();
        }
        return errTokens[current + distance];
    }
    bool expect(int tok) {
        if (isAtEnd()) return false;
        if (errTokens[current + 1].type == tok) {
            return true;
        }
        return false;
    }
    const Token& previous() {
        return errTokens[current - 1];
    }

    public:
        std::unique_ptr<Expr> primary() {
            if (errTokens[current].type == TOKEN_NUMBER || errTokens[current].type == TOKEN_STRING) {
                return std::make_unique<LiteralExpr>(errTokens[current++].lexeme);
            }
            
            if (errTokens[current].type == TOKEN_IDENTIFIER) {
                if (peek(1).type == TOKEN_LPAREN) {
                    std::string funcName = errTokens[current].lexeme;
                    current += 2;
                    std::vector<std::unique_ptr<Expr>> callArgs;
                    while (!isAtEnd() && errTokens[current].type != TOKEN_RPAREN) {
                        callArgs.push_back(expression());
                        if (errTokens[current].type == TOKEN_COMMA) current++;
                    }
                    if (!isAtEnd()) current++;
                    return std::make_unique<CallExpr>(funcName, std::move(callArgs));
                }
                return std::make_unique<VariableExpr>(errTokens[current++].lexeme);
            }

            if (errTokens[current].type == TOKEN_LPAREN) {
                current++;
                auto expr = expression();
                if (errTokens[current].type == TOKEN_RPAREN) {
                    current++;
                }
                return expr;
            }

            return nullptr;
        }

        std::unique_ptr<Expr> multiplication() {
            auto expr = primary();

            while (errTokens[current].type == TOKEN_STAR || errTokens[current].type == TOKEN_SLASH) {
                Token op = errTokens[current++];
                auto right = primary();
                expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
            }

            return expr;
        }

        std::unique_ptr<Expr> equality() {
            auto expr = multiplication();

            while (errTokens[current].type == TOKEN_PLUS || errTokens[current].type == TOKEN_MINUS) {
                Token op = errTokens[current++];
                auto right = multiplication();
                expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
            }

            return expr;
        }

        std::unique_ptr<Expr> addition() {
            auto expr = equality();

            while (current + 1 < errTokens.size() && 
                  ((errTokens[current].type == TOKEN_EQUALS && errTokens[current + 1].type == TOKEN_EQUALS) ||
                   (errTokens[current].type == TOKEN_NOT_EQUALS) || (errTokens[current].type == TOKEN_BIGGER) ||
                   (errTokens[current].type == TOKEN_SMALLER))) {
                
                Token op = errTokens[current];
                if (op.type == TOKEN_EQUALS) {
                    current += 2;
                } else {
                    current += 1;
                }
                
                auto right = equality();
                expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
            }

            return expr;
        }

        std::unique_ptr<Expr> expression() {
            if (current + 1 < errTokens.size() && errTokens[current].type == TOKEN_IDENTIFIER && errTokens[current + 1].type == TOKEN_EQUALS && errTokens[current + 2].type != TOKEN_EQUALS) {
                std::string name = errTokens[current].lexeme;
                current += 2;
                auto value = expression();
                return std::make_unique<AssignExpr>(name, std::move(value));
            }
            return addition();
        }
        DataType compileExpression(const std::unique_ptr<Expr>& node) {
            if (auto literal = dynamic_cast<LiteralExpr*>(node.get())) {
                std::string valStr = valueToString(literal->value);
                
                bool isNumeric = !valStr.empty() && valStr.find_first_not_of("-0123456789.") == std::string::npos;

                if (isNumeric) {
                    DataType litType = getLiteralType(valStr);
                    if (litType == DataType::FLOAT) {
                        emitInstruction(OP_PUSH, std::stod(valStr));
                        return DataType::FLOAT;
                    } else {
                        emitInstruction(OP_PUSH, std::stoi(valStr));
                        return DataType::INT;
                    }
                } else {
                    emitInstruction(OP_PUSH, literal->value);
                    return DataType::STRING;
                }
            }
            else if (auto var = dynamic_cast<VariableExpr*>(node.get())) {
                if (functionTable.contains(var->name)) {
                    throwError("Cannot use function '" + var->name + "' as a variable. Did you mean " + var->name + "()?", -1);
                }
                if (isCompilingFunction && localSymbolTable.contains(var->name)) {
                    emitInstruction(OP_LOAD_LOCAL, localSymbolTable[var->name]);
                    return localTypes[var->name];
                }
                if (symbolTable.contains(var->name)) {
                    if (symbolTable[var->name].isConst){
                        emitInstruction(OP_PUSH, symbolTable[var->name].constValue);
                    }
                    else {
                        emitInstruction(OP_LOAD, symbolTable[var->name].index);
                    }
                    return symbolTable[var->name].type;
                }
                throwError("Undeclared variable: " + var->name, -1);
            }
            else if (auto assign = dynamic_cast<AssignExpr*>(node.get())) {
                DataType rhsType = compileExpression(assign->value);
                
                if (isCompilingFunction && localSymbolTable.contains(assign->name)) {
                    emitInstruction(OP_STORE_LOCAL, localSymbolTable[assign->name]);
                    return rhsType;
                }
                
                if (symbolTable.contains(assign->name)) {
                    DataType lhsType = symbolTable[assign->name].type;
                    if (!isTypeCompatible(lhsType, rhsType)) {
                        throwError("Cannot assign " + std::string(typeToString(rhsType)) + 
                                " to variable '" + assign->name + "' of type " + std::string(typeToString(lhsType)), -1, false, "Type Error");
                    }
                    emitInstruction(OP_STORE, symbolTable[assign->name].index);
                    return lhsType;
                }
                
                if (isCompilingFunction) {
                    localSymbolTable[assign->name] = nextLocalIndex++;
                    localTypes[assign->name] = rhsType;
                    emitInstruction(OP_STORE_LOCAL, localSymbolTable[assign->name]);
                    return rhsType;
                }
                
                throwError("Undeclared variable assignment: " + assign->name, -1);
            }
            else if (auto call = dynamic_cast<CallExpr*>(node.get())) {
                if (!functionTable.contains(call->callee)) {
                    throwError("Undefined function invocation: " + call->callee, -1);
                }
                Function func = functionTable[call->callee];
                
                if (call->arguments.size() != func.params.size()) {
                    throwError("Function '" + call->callee + "' expects " + std::to_string(func.params.size()) + " argument/s, but got " + std::to_string(call->arguments.size()), -1);
                }

                for (size_t i = 0; i < call->arguments.size(); i++) {
                    DataType argType = compileExpression(call->arguments[i]); 
                    DataType paramType = func.params[i].type;
                    
                    if (!compatibleTypes(paramType, argType)) {
                        throwError("Type mismatch in function '" + call->callee + "' at argument " + std::to_string(i + 1) + 
                                   ": Expected '" + std::string(typeToString(paramType)) + 
                                   "', but got '" + std::string(typeToString(argType)) + "'", -1);
                    }
                }
                
                emitInstruction(OP_CALL, call->callee);
                return func.returnType;
            }
            else if (auto binary = dynamic_cast<BinaryExpr*>(node.get())) {
                DataType leftType = compileExpression(binary->left);
                DataType rightType = compileExpression(binary->right);
                
                if (binary->op.type != TOKEN_PLUS && binary->op.type != TOKEN_EQUALS && binary->op.type != TOKEN_NOT_EQUALS) {
                    if ((leftType != DataType::INT && leftType != DataType::FLOAT) ||
                        (rightType != DataType::INT && rightType != DataType::FLOAT)) {
                        throwError("Math operators require numeric types.", -1, false, "Type Error");
                    }
                }
                
                switch (binary->op.type) {
                    case TOKEN_PLUS:  emitByte(OP_ADD); break;
                    case TOKEN_MINUS: emitByte(OP_SUB); break;
                    case TOKEN_STAR:  emitByte(OP_MUL); break;
                    case TOKEN_SLASH: emitByte(OP_DIV); break;
                    case TOKEN_EQUALS: emitByte(OP_EQUALS); break;
                    case TOKEN_NOT_EQUALS: emitByte(OP_NOT_EQUALS); break;
                    case TOKEN_BIGGER: emitByte(OP_GREATER); break;
                    case TOKEN_SMALLER: emitByte(OP_LESS); break;
                    default: break;
                }

                if (binary->op.type == TOKEN_EQUALS || binary->op.type == TOKEN_NOT_EQUALS ||
                binary->op.type == TOKEN_BIGGER || binary->op.type == TOKEN_SMALLER) {
                    return DataType::INT;
                }

                if (leftType == DataType::STRING || rightType == DataType::STRING) return DataType::STRING;
                if (leftType == DataType::FLOAT || rightType == DataType::FLOAT) return DataType::FLOAT;
                return DataType::INT;
            }
            return DataType::INT;
        }

        int getVariableIndex(const std::string& name, DataType type){
            auto it = symbolTable.find(name);
            if (it != symbolTable.end()){
                return it->second.index;
            }
            int index = nextAvailableIndex++;
            symbolTable[name] = { index, type };

            if (index >= indexTypes.size()) {
                indexTypes.resize(index + 1);
            }
            indexTypes[index] = type;
    
            return index;
        }

        std::vector<Token> lex(const std::string& src) {
            std::vector<Token> tokens;
            int i = 0;

            while (i < src.size()) {
                char c = src[i];

                if (c == '#') {
                    i++;
                    while (i < src.size() && src[i] != '#') {
                        i++;
                    }
                    i++;
                    continue;
                }
                if (c == '"'){
                    i++;
                    std::string id;
                    while (i < src.size() && src[i] != '"')
                    {
                        id += src[i++];
                    }
                    if (i < src.size()) i++;
                    tokens.push_back({TOKEN_STRING, id});
                    continue;
                }
                if (isspace(c)) {
                    i++;
                    continue;
                }

                if (c == '-' && i + 1 < src.size() && isdigit(src[i+1])) {
                    std::string num;
                    num += src[i++];
                    bool hasDecimal = false;
                    while (i < src.size() && (isdigit(src[i]) || src[i] == '.')) {
                        if (src[i] == '.') {
                            if (hasDecimal) break;
                            hasDecimal = true;
                        }
                        num += src[i++];
                    }
                    tokens.push_back({TOKEN_NUMBER, num});
                    continue;
                }

                if (isdigit(c)) {
                    std::string num;
                    bool hasDecimal = false;
                    while (i < src.size() && (isdigit(src[i]) || src[i] == '.')) {
                        if (src[i] == '.') {
                            if (hasDecimal) break;
                            hasDecimal = true;
                        }
                        num += src[i++];
                    }
                    tokens.push_back({TOKEN_NUMBER, num});
                    continue;
                }
                if (isalpha(c) || c == '_') {
                    std::string id;
                    while (i < src.size() && isalnum(src[i]) ||  src[i] == '_') {
                        id += src[i++];
                    }
                    tokens.push_back({TOKEN_IDENTIFIER, id});
                    continue;
                }
                if (c == ';'){
                    tokens.push_back({TOKEN_SEMICOLON, ";"});
                    i++;
                    continue;
                }
                switch (c){
                    case '(': { tokens.push_back({TOKEN_LPAREN, "("}); i++; continue; }
                    case ')': { tokens.push_back({TOKEN_RPAREN, ")"}); i++; continue; }
                    case '[': { tokens.push_back({TOKEN_LBRACKET, "["}); i++; continue; }
                    case ']': { tokens.push_back({TOKEN_RBRACKET, "]"}); i++; continue; }
                    case '{': { tokens.push_back({TOKEN_LBRACE, "{"}); i++; continue; }
                    case '}': { tokens.push_back({TOKEN_RBRACE, "}"}); i++; continue; }
                    case ';': { tokens.push_back({TOKEN_SEMICOLON, ";"}); i++; continue; }
                    case '=': { tokens.push_back({TOKEN_EQUALS, "="}); i++; continue; }
                    case ',': { tokens.push_back({TOKEN_COMMA, ","}); i++; continue; }
                    case '-': { tokens.push_back({TOKEN_MINUS, "-"}); i++; continue; }
                    case '*': { tokens.push_back({TOKEN_STAR, "*"}); i++; continue; }
                    case '/': { tokens.push_back({TOKEN_SLASH, "/"}); i++; continue; }
                    case '+': { tokens.push_back({TOKEN_PLUS, "+"}); i++; continue; }
                    case '!': { tokens.push_back({TOKEN_NOT, "!"}); i++; continue; }
                    case '>': { tokens.push_back({TOKEN_BIGGER, ">"}); i++; continue; }
                    case '<': { tokens.push_back({TOKEN_SMALLER, "<"}); i++; continue; }
                    default: break;
                }

                i++;
            }
            for (int i = 0; i < tokens.size(); i++){
                if (tokens[i].lexeme == "print"){ tokens[i].type = TOKEN_PRINT; }
                if (tokens[i].lexeme == "extern"){ tokens[i].type = TOKEN_EXTERN; }
                if (tokens[i].lexeme == "import"){ tokens[i].type = TOKEN_IMPORT; }
                if (tokens[i].lexeme == "lib"){ tokens[i].type = TOKEN_LIB; }
                if (tokens[i].lexeme == "int"){ tokens[i].type = TOKEN_INT; }
                if (tokens[i].lexeme == "string"){ tokens[i].type = TOKEN_STRING_T; }
                if (tokens[i].lexeme == "bool"){ tokens[i].type = TOKEN_BOOL; }
                if (tokens[i].lexeme == "float"){ tokens[i].type = TOKEN_FLOAT; }
                if (tokens[i].lexeme == "char"){ tokens[i].type = TOKEN_CHAR; }
                if (tokens[i].lexeme == "if"){ tokens[i].type = TOKEN_IF; }
                if (tokens[i].lexeme == "else"){ tokens[i].type = TOKEN_ELSE; }
                if (tokens[i].lexeme == "while"){ tokens[i].type = TOKEN_WHILE; }
                if (tokens[i].lexeme == "exit"){ tokens[i].type = TOKEN_EXIT; }
                if (tokens[i].lexeme == "const"){ tokens[i].type = TOKEN_CONST; }
                if (tokens[i].lexeme == "return"){ tokens[i].type = TOKEN_RETURN; }
                if (tokens[i].lexeme == "throw"){ tokens[i].type = TOKEN_THROW; }
                if (tokens[i].type == TOKEN_NOT && tokens[i+1].type == TOKEN_EQUALS) { tokens[i].type = TOKEN_NOT_EQUALS; tokens.erase(tokens.begin() + i+1); i++; }
            }
            tokens.push_back({TOKEN_EOF, "\0"});
            return tokens;
        }

        std::vector<std::unique_ptr<Stmt>> parseExpression(std::vector<Token>& tokens){
            return {};
        }

        std::vector<std::unique_ptr<Stmt>> parseBlock(std::vector<Token>& tokens) {
            std::vector<std::unique_ptr<Stmt>> blockStatements;
            
            bool isConstDecl=false;
            while (!isAtEnd() && tokens[current].type != TOKEN_RBRACE) {
                if (tokens[current].type == TOKEN_CONST){
                    isConstDecl=true;
                    current++;
                }
                if (tokens[current].type == TOKEN_LIB) {
                    current++;
                    if (tokens[current].type != TOKEN_IDENTIFIER) throwError("Expected library handle after 'lib'", -1);
                    std::string libHandle = tokens[current].lexeme;
                    current++;
                    if (tokens[current].type != TOKEN_EQUALS) throwError("Expected '=' after library handle", -1);
                    current++;
                    if (tokens[current].type != TOKEN_IDENTIFIER || tokens[current].lexeme != "loadLibrary") {
                        throwError("Expected loadLibrary() after 'lib <handle> ='",-1);
                    }
                    current++;
                    if (tokens[current].type != TOKEN_LPAREN) throwError("Expected '(' after loadLibrary", -1);
                    current++;
                    if (tokens[current].type != TOKEN_STRING) throwError("Expected string library name inside loadLibrary()", -1);
                    std::string libName = tokens[current].lexeme;
                    current++;
                    if (tokens[current].type != TOKEN_RPAREN) throwError("Expected ')' after loadLibrary argument", -1);
                    current++;
                    if (tokens[current].type == TOKEN_SEMICOLON) current++;
                    blockStatements.push_back(std::make_unique<LibStmt>(libHandle, libName));
                    continue;
                }
                if (tokens[current].type == TOKEN_EXTERN) {
                    current++;
                    if (tokens[current].type != TOKEN_LPAREN) throwError("Expected '(' after extern", -1);
                    current++;
                    if (tokens[current].type != TOKEN_IDENTIFIER && tokens[current].type != TOKEN_STRING) throwError("Expected library handle or string inside extern()", -1);
                    std::string libHandle = tokens[current].lexeme;
                    current++;
                    if (tokens[current].type != TOKEN_RPAREN) throwError("Expected ')' after extern library handle", -1);
                    current++;
                    if (tokens[current].type != TOKEN_INT && tokens[current].type != TOKEN_FLOAT && tokens[current].type != TOKEN_STRING_T && tokens[current].type != TOKEN_CHAR && tokens[current].type != TOKEN_BOOL) {
                        throwError("Expected return type after extern()", -1);
                    }
                    DataType returnType;
                    if (tokens[current].type == TOKEN_INT) returnType = DataType::INT;
                    else if (tokens[current].type == TOKEN_FLOAT) returnType = DataType::FLOAT;
                    else if (tokens[current].type == TOKEN_BOOL) returnType = DataType::BOOL;
                    else if (tokens[current].type == TOKEN_CHAR) returnType = DataType::CHAR;
                    else returnType = DataType::STRING;
                    current++;
                    if (tokens[current].type != TOKEN_IDENTIFIER) throwError("Expected extern function name", -1);
                    std::string funcName = tokens[current].lexeme;
                    current++;
                    if (tokens[current].type != TOKEN_LPAREN) throwError("Expected '(' after extern function name", -1);
                    current++;
                    std::vector<Param> params;
                    while (!isAtEnd() && tokens[current].type != TOKEN_RPAREN) {
                        if (tokens[current].type != TOKEN_INT && tokens[current].type != TOKEN_FLOAT && tokens[current].type != TOKEN_STRING_T && tokens[current].type != TOKEN_CHAR && tokens[current].type != TOKEN_BOOL) {
                            throwError("Expected parameter type in extern declaration", -1);
                        }
                        DataType pType;
                        if (tokens[current].type == TOKEN_INT) pType = DataType::INT;
                        else if (tokens[current].type == TOKEN_FLOAT) pType = DataType::FLOAT;
                        else if (tokens[current].type == TOKEN_BOOL) pType = DataType::BOOL;
                        else if (tokens[current].type == TOKEN_CHAR) pType = DataType::CHAR;
                        else pType = DataType::STRING;
                        current++;
                        std::string paramName = "arg" + std::to_string(params.size());
                        if (tokens[current].type == TOKEN_IDENTIFIER) {
                            paramName = tokens[current].lexeme;
                            current++;
                        }
                        params.push_back({pType, paramName});
                        if (tokens[current].type == TOKEN_COMMA) current++;
                    }
                    if (tokens[current].type != TOKEN_RPAREN) throwError("Expected ')' at end of extern parameter list", -1);
                    current++;
                    if (tokens[current].type == TOKEN_SEMICOLON) current++;
                    blockStatements.push_back(std::make_unique<ExternStmt>(libHandle, funcName, returnType, params));
                    continue;
                }
                if (tokens[current].type == TOKEN_INT || tokens[current].type == TOKEN_FLOAT || tokens[current].type == TOKEN_STRING_T || tokens[current].type == TOKEN_CHAR) {
                    DataType type;
                    if (tokens[current].type == TOKEN_INT) type = DataType::INT;
                    else if (tokens[current].type == TOKEN_FLOAT) type = DataType::FLOAT;
                    else type = DataType::STRING;
                    
                    if (peek(1).type == TOKEN_IDENTIFIER && peek(2).type == TOKEN_LPAREN) {
                        current++;
                        std::string funcName = tokens[current].lexeme;
                        current += 2;

                        std::vector<Parameter> parameters;
                        while (!isAtEnd() && tokens[current].type != TOKEN_RPAREN) {
                            DataType pType;
                            if (tokens[current].type == TOKEN_INT) pType = DataType::INT;
                            else if (tokens[current].type == TOKEN_FLOAT) pType = DataType::FLOAT;
                            else pType = DataType::STRING;
                            current++;

                            std::string pName = tokens[current].lexeme;
                            parameters.push_back({pName, pType});
                            current++;


                            if (tokens[current].type == TOKEN_COMMA) current++;
                        }
                        current++;

                        if (tokens[current].type != TOKEN_LBRACE) throwError("Expected '{' to start function body", -1);
                        current++;

                        int braceCount = 1;
                        std::vector<Token> funcBody;
                        while (!isAtEnd() && braceCount > 0) {
                            if (tokens[current].type == TOKEN_LBRACE) braceCount++;
                            if (tokens[current].type == TOKEN_RBRACE) braceCount--;
                            if (braceCount > 0) {
                                funcBody.push_back(tokens[current++]);
                            }
                        }
                        current++;

                        blockStatements.push_back(std::make_unique<FunctionDeclStmt>(type, funcName, parameters, funcBody, false));
                        continue;
                    }
                    
                    if (peek(1).type == TOKEN_IDENTIFIER) {
                        current++;
                        std::string varName = tokens[current].lexeme;
                        current++;
                        
                        std::unique_ptr<Expr> initializer = nullptr;
                        if (tokens[current].type == TOKEN_EQUALS) {
                            current++;
                            initializer = expression();
                        }
                        
                        if (tokens[current].type == TOKEN_SEMICOLON) current++;
                        
                        blockStatements.push_back(std::make_unique<VarDeclStmt>(type, varName, std::move(initializer), isConstDecl));
                        isConstDecl = false;
                        continue;
                    }
                }
                if (tokens[current].type == TOKEN_IDENTIFIER && peek(1).type == TOKEN_EQUALS && peek(2).type != TOKEN_EQUALS) {
                    std::string name = tokens[current].lexeme;
                    current += 2;

                    if (symbolTable.contains(name) && symbolTable[name].isConst){
                        throwError("Cannot assign a value to an constant variable '" + name + "'", -2);
                    }

                    std::unique_ptr<Expr> value = expression();
                    
                    if (tokens[current].type == TOKEN_SEMICOLON) current++;
                    blockStatements.push_back(std::make_unique<ExpressionStmt>(std::make_unique<AssignExpr>(name, std::move(value))));
                    continue;
                }
                if (tokens[current].type == TOKEN_IDENTIFIER && expect(TOKEN_LPAREN)) {
                    std::string funcName = tokens[current].lexeme;
                    std::vector<std::unique_ptr<Expr>> callArgs;
                    
                    current += 2;

                    while (!isAtEnd() && tokens[current].type != TOKEN_RPAREN) {
                        callArgs.push_back(expression());
                        if (tokens[current].type == TOKEN_COMMA) current++;
                    }
                    if (!isAtEnd()) current++;
                    
                    blockStatements.push_back(std::make_unique<ExpressionStmt>(
                        std::make_unique<CallExpr>(funcName, std::move(callArgs))
                    ));
                    continue;
                }
                if (tokens[current].type == TOKEN_EQUALS) {
                    current++;
                }
                if (tokens[current].type == TOKEN_WHILE){
                    current ++;
                    if (tokens[current].type != TOKEN_LPAREN) throwError("Expected '(' after 'while'", -1);
                    current++;
                    
                    auto condition = expression();
                    
                    if (tokens[current].type != TOKEN_RPAREN) throwError("Expected ')' after condition", -1);
                    current++;
                    
                    if (tokens[current].type != TOKEN_LBRACE) throwError("Expected '{' to start while block", -1);
                    current++;
                    
                    std::vector<std::unique_ptr<Stmt>> thenBranch = parseBlock(tokens);
                    
                    if (tokens[current].type != TOKEN_RBRACE) throwError("Expected '}' at end of while block", -1);
                    current++;

                    blockStatements.push_back(std::make_unique<WhileStmt>(std::move(condition), std::move(thenBranch)));
                    continue;
                }
                if (tokens[current].type == TOKEN_IF) {
                    current++;
                    
                    if (tokens[current].type != TOKEN_LPAREN) throwError("Expected '(' after 'if'", -1);
                    current++;
                    
                    auto condition = expression();
                    
                    if (tokens[current].type != TOKEN_RPAREN) throwError("Expected ')' after condition", -1);
                    current++;
                    
                    if (tokens[current].type != TOKEN_LBRACE) throwError("Expected '{' to start if block", -1);
                    current++;
                    
                    std::vector<std::unique_ptr<Stmt>> thenBranch = parseBlock(tokens);
                    
                    if (tokens[current].type != TOKEN_RBRACE) throwError("Expected '}' at end of if block", -1);
                    current++;
                    
                    std::vector<std::unique_ptr<Stmt>> elseBranch;
                    if (!isAtEnd() && tokens[current].type == TOKEN_ELSE) {
                        current++;
                        if (tokens[current].type != TOKEN_LBRACE) throwError("Expected '{' to start else block", -1);
                        current++;
                        
                        elseBranch = parseBlock(tokens);
                        
                        if (tokens[current].type != TOKEN_RBRACE) throwError("Expected '}' at end of else block", -1);
                        current++;
                    }
                    
                    blockStatements.push_back(std::make_unique<IfStmt>(std::move(condition), std::move(thenBranch), std::move(elseBranch)));
                    continue;
                }
                if (tokens[current].type == TOKEN_RETURN) {
                    current++;
                    std::unique_ptr<Expr> retVal = nullptr;
                    if (current < tokens.size() && tokens[current].type != TOKEN_SEMICOLON) {
                        retVal = expression();
                    }
                    if (tokens[current].type == TOKEN_SEMICOLON) current++;
                    blockStatements.push_back(std::make_unique<ReturnStmt>(std::move(retVal)));
                    continue;
                }
                if (tokens[current].type == TOKEN_PRINT) {
                    if (expect(TOKEN_LPAREN)) {
                        std::vector<std::unique_ptr<Expr>> astArgs;
                        current += 2; 

                        while (!isAtEnd() && tokens[current].type != TOKEN_RPAREN) {
                            astArgs.push_back(expression()); 
                            
                            if (tokens[current].type == TOKEN_COMMA) current++;
                        }
                        blockStatements.push_back(std::make_unique<PrintStmt>(std::move(astArgs)));
                    }
                }
                if (tokens[current].type == TOKEN_THROW) {
                    if (expect(TOKEN_LPAREN)) {
                        std::vector<std::unique_ptr<Expr>> astArgs;
                        current += 2; 

                        while (!isAtEnd() && tokens[current].type != TOKEN_RPAREN) {
                            if (astArgs.size() > 3){ throwError("throw() only takes 3 arguments.", -1); }
                            astArgs.push_back(expression()); 
                            
                            if (tokens[current].type == TOKEN_COMMA) current++;
                        }
                        blockStatements.push_back(std::make_unique<ThrowStmt>(std::move(astArgs[0]),std::move(astArgs[1]),std::move(astArgs[2
                        ])));
                    }
                }
                if (tokens[current].type == TOKEN_EXIT){
                    if (expect(TOKEN_LPAREN)) {
                        std::unique_ptr<Expr> astArg;
                        current += 2; 

                        while (!isAtEnd() && tokens[current].type != TOKEN_RPAREN) {
                            astArg = expression(); 
                    
                            if (tokens[current].type == TOKEN_COMMA) throwError("exit() only takes 1 argument.", -1);
                        }
                        blockStatements.push_back(std::make_unique<ExitStmt>(std::move(astArg)));
                    }
                }
                if (tokens[current].type == TOKEN_SEMICOLON) {
                    current++;
                    continue;
                }
                current++;
            }
            return blockStatements;
        }

        std::vector<std::unique_ptr<Stmt>> makeAST(std::vector<Token>& tokens) {
            if (useDevEnv) std::cout << "AST\n";
            errTokens = tokens;
            current = 0; 
            return parseBlock(tokens); 
        }
        
        void compileSingle(const std::unique_ptr<Stmt>& node) {
            if (auto varDecl = dynamic_cast<VarDeclStmt*>(node.get())) {
                if (varDecl->initializer) {
                    DataType rhsType = compileExpression(varDecl->initializer);
                    if (!isTypeCompatible(varDecl->type, rhsType)) {
                        throwError("Cannot initialize '" + varDecl->name + "' of type " + 
                                std::string(typeToString(varDecl->type)) + " with " + std::string(typeToString(rhsType)), -1, false, "Type Error");
                    }
                }
                else
                {
                    emitInstruction(OP_PUSH, defaultValueOfType(varDecl->type));
                }

                if (isCompilingFunction) {
                    localSymbolTable[varDecl->name] = nextLocalIndex++;
                    localTypes[varDecl->name] = varDecl->type;
                    emitInstruction(OP_STORE_LOCAL, localSymbolTable[varDecl->name]);
                } else {
                    Value compileTimeValue = 0;
                    if (varDecl->isConstant) { 
                        if (!Code.empty() && Code.back().op == OP_PUSH) {
                            compileTimeValue = Code.back().value;
                            Code.pop_back(); 
                        }
                    }

                    Symbol newSymbol;
                    newSymbol.index = nextAvailableIndex++;
                    newSymbol.type = varDecl->type;
                    newSymbol.isConst = varDecl->isConstant; 
                    newSymbol.constValue = compileTimeValue;
                    symbolTable[varDecl->name] = newSymbol;

                    if (newSymbol.index >= indexTypes.size()) {
                        indexTypes.resize(newSymbol.index + 1);
                    }
                    indexTypes[newSymbol.index] = varDecl->type;

                    if (!newSymbol.isConst) {
                        emitInstruction(OP_STORE, getVariableIndex(varDecl->name, varDecl->type));
                    }
                }
            }
            else if (auto libDecl = dynamic_cast<LibStmt*>(node.get())) {
                registerNativeLibrary(libDecl->handle, libDecl->libPath);
            }
            else if (auto externDecl = dynamic_cast<ExternStmt*>(node.get())) {
                Function function;
                function.name = externDecl->funcName;
                function.returnType = externDecl->returnType;
                function.isVoid = (externDecl->returnType == DataType::VOID);
                function.isNative = true;
                function.nativeHandler = [libHandle = externDecl->libHandle,
                                          symbol = externDecl->funcName,
                                          params = externDecl->params,
                                          returnType = externDecl->returnType](std::vector<Value> args) -> Value {
                    if (args.size() != params.size()) {
                        throwError("External function '" + symbol + "' called with wrong argument count", -1);
                    }
                    void* sym = resolveNativeSymbol(libHandle, symbol);
                    if (!sym) {
                        throwError("Failed to resolve symbol '" + symbol + "'", -1);
                    }

                    auto argToInt = [&](const Value& v) {
                        return static_cast<int>(valueToInt(v));
                    };
                    auto argToFloat = [&](const Value& v) {
                        return valueToFloat(v);
                    };

                    if (params.size() == 0) {
                        if (returnType == DataType::VOID) return std::monostate{};
                        if (returnType == DataType::INT) {
                            using Fn = int(*)();
                            return reinterpret_cast<Fn>(sym)();
                        }
                        if (returnType == DataType::FLOAT) {
                            using Fn = double(*)();
                            return reinterpret_cast<Fn>(sym)();
                        }
                        if (returnType == DataType::BOOL) {
                            using Fn = bool(*)();
                            return reinterpret_cast<Fn>(sym)();
                        }
                    }

                    if (params.size() == 1) {
                        if (params[0].type == DataType::FLOAT) {
                            auto fn = reinterpret_cast<double(*)(double)>(sym);
                            double result = fn(argToFloat(args[0]));
                            if (returnType == DataType::INT) return static_cast<int>(result);
                            if (returnType == DataType::FLOAT) return result;
                            if (returnType == DataType::BOOL) return result != 0.0;
                        }
                        if (params[0].type == DataType::INT) {
                            auto fn = reinterpret_cast<int(*)(int)>(sym);
                            int result = fn(argToInt(args[0]));
                            if (returnType == DataType::INT) return result;
                            if (returnType == DataType::FLOAT) return static_cast<double>(result);
                            if (returnType == DataType::BOOL) return result != 0;
                        }
                    }

                    if (params.size() == 2) {
                        if (params[0].type == DataType::FLOAT && params[1].type == DataType::FLOAT) {
                            auto fn = reinterpret_cast<double(*)(double, double)>(sym);
                            double result = fn(argToFloat(args[0]), argToFloat(args[1]));
                            if (returnType == DataType::INT) return static_cast<int>(result);
                            if (returnType == DataType::FLOAT) return result;
                            if (returnType == DataType::BOOL) return result != 0.0;
                        }
                        if (params[0].type == DataType::INT && params[1].type == DataType::INT) {
                            auto fn = reinterpret_cast<int(*)(int, int)>(sym);
                            int result = fn(argToInt(args[0]), argToInt(args[1]));
                            if (returnType == DataType::INT) return result;
                            if (returnType == DataType::FLOAT) return static_cast<double>(result);
                            if (returnType == DataType::BOOL) return result != 0;
                        }
                    }

                    throwError("External function signature not supported for '" + symbol + "'", -1);
                    return std::monostate{};
                };
                for (const auto& param : externDecl->params) {
                    function.params.push_back({param.name, param.type});
                }
                functionTable[function.name] = function;
            }
            else if (auto funcDecl = dynamic_cast<FunctionDeclStmt*>(node.get())) {
                Function function;
                function.name = funcDecl->name;
                function.returnType = funcDecl->returnType;
                function.params = funcDecl->params;
                function.body = funcDecl->body;
                function.isVoid = funcDecl->isVoid;
                function.isNative = false;
                function.nativeHandler = nullptr;
                
                int jumpAroundFunc = Code.size();
                emitInstruction(OP_JMP, 0);
                
                function.address = Code.size();
                functionTable[function.name] = function;

                bool oldIsCompilingFunction = isCompilingFunction;
                int oldLocalIndex = nextLocalIndex;
                auto oldLocalSymbols = localSymbolTable;
                auto oldLocalTypes = localTypes;
                DataType oldReturnType = currentFunctionReturnType;

                isCompilingFunction = true;
                currentFunctionReturnType = function.returnType;
                localSymbolTable.clear();
                localTypes.clear();
                
                nextLocalIndex = 0;

                for (const auto& param : function.params) {
                    localSymbolTable[param.name] = nextLocalIndex++;
                    localTypes[param.name] = param.type;
                }

                Compiler localCompiler;
                localCompiler.functionTable = this->functionTable;
                localCompiler.isCompilingFunction = true;
                localCompiler.currentFunctionReturnType = function.returnType;
                
                std::vector<Token> localTokens = function.body;
                localTokens.push_back({TOKEN_EOF, "\0"});
                auto bodyAST = localCompiler.makeAST(localTokens);
                
                for (const auto& stmt : bodyAST) {
                    compileSingle(stmt);
                }

                emitInstruction(OP_PUSH, std::monostate{});
                emitByte(OP_RETURN);

                std::vector<DataType> functionLocalTypeList(nextLocalIndex);
                for (const auto& [name, index] : localSymbolTable) {
                    functionLocalTypeList[index] = localTypes[name];
                }
                function.localTypes = std::move(functionLocalTypeList);
                functionTable[function.name] = function;

                Code[jumpAroundFunc].value = (int)(Code.size() - jumpAroundFunc - 1);

                isCompilingFunction = oldIsCompilingFunction;
                nextLocalIndex = oldLocalIndex;
                localSymbolTable = oldLocalSymbols;
                localTypes = oldLocalTypes;
                currentFunctionReturnType = oldReturnType;
            }
            else if (auto retStmt = dynamic_cast<ReturnStmt*>(node.get())) {
                if (!isCompilingFunction) {
                    throwError("Return statement used outside of a function scope", -1);
                }

                if (retStmt->value) {
                    DataType actualReturnType = compileExpression(retStmt->value);
                    
                    if (!compatibleTypes(currentFunctionReturnType, actualReturnType)) {
                        throwError("Type mismatch: Function expects to return '" + 
                                   std::string(typeToString(currentFunctionReturnType)) + 
                                   "', but expression returns '" + 
                                   std::string(typeToString(actualReturnType)) + "'", -1);
                    }
                } else {
                    if (currentFunctionReturnType != DataType::VOID) {
                        throwError("Empty return statement in a function expecting a value type", -1);
                    }
                    emitInstruction(OP_PUSH, std::monostate{});
                }
                emitByte(OP_RETURN);
            }
            else if (auto exprStmt = dynamic_cast<ExpressionStmt*>(node.get())) {
                compileExpression(exprStmt->expression);
            }
            else if (auto p = dynamic_cast<PrintStmt*>(node.get())) {
                for (const auto& arg : p->arguments) {
                    compileExpression(arg); 
                    emitByte(OP_PRINT);
                }
            }
            else if (auto t = dynamic_cast<ThrowStmt*>(node.get())){
                compileExpression(t->message);
                compileExpression(t->ident);
                compileExpression(t->errorCode);
                
                emitByte(OP_THROW);
            }
            else if (auto whileStmt = dynamic_cast<WhileStmt*>(node.get())) {
                int loopStartHead = Code.size();

                compileExpression(whileStmt->condition);

                int jumpFalseIndex = Code.size();
                emitInstruction(OP_JMP_IF_FALSE, 0);

                for (const auto& stmt : whileStmt->body) {
                    compileSingle(stmt);
                }

                int jumpBackIndex = Code.size();
                int offsetBack = loopStartHead - jumpBackIndex - 1;
                emitInstruction(OP_JMP, offsetBack);

                Code[jumpFalseIndex].value = (int)(Code.size() - jumpFalseIndex - 1);
            }
            else if (auto ifStmt = dynamic_cast<IfStmt*>(node.get())) {
                compileExpression(ifStmt->condition);
                
                int jumpInstIndex = Code.size();
                emitInstruction(OP_JMP_IF_FALSE, 0);
                
                for (const auto& stmt : ifStmt->thenBranch) {
                    compileSingle(stmt);
                }
                
                if (!ifStmt->elseBranch.empty()) {
                    int elseJumpIndex = Code.size();
                    emitInstruction(OP_JMP, 0);
                    
                    Code[jumpInstIndex].value = (int)(Code.size() - jumpInstIndex - 1);
                    
                    for (const auto& stmt : ifStmt->elseBranch) {
                        compileSingle(stmt);
                    }
                    Code[elseJumpIndex].value = (int)(Code.size() - elseJumpIndex - 1);
                } else {
                    Code[jumpInstIndex].value = (int)(Code.size() - jumpInstIndex - 1);
                }
            }
            else if (auto exitStmt = dynamic_cast<ExitStmt*>(node.get())){
                compileExpression(exitStmt->argument);
                emitByte(OP_HALT);
            }
        }

        void compile(const std::vector<std::unique_ptr<Stmt>>& nodes)
        {
            for (const std::unique_ptr<Stmt> &node : nodes){
                compileSingle(node);
            }
            if (Code.empty() || Code.back().op != OP_HALT){
                emitInstruction(OP_PUSH,0);
                emitByte(OP_HALT);
            }
            if (debug){
                int address = 0;
                for (Instruction i : Code){
                    if (valueToString(i.value) != "")
                        std::println("{:04d}: {:<16} {}",address, enum_to_string(i.op), valueToString(i.value));
                    else
                        std::println("{:04d}: {:<16}", address , enum_to_string(i.op));
                    address++;
                }
            }
        }
        constexpr std::vector<Instruction> getCode(){
            return Code;
        }
        const std::vector<DataType>& getIndexTypes() const {
            return indexTypes;
        }
        const std::map<std::string, Function>& getFunctionTable() const {
            return functionTable;
        }
};

class VirtualMachine {
private:
    std::map<std::string, Function> vmFunctions;
    std::vector<Value> globals;
    std::map<std::string, Symbol> symbolTable;
    int nextSlot = 0;
    struct CallFrame {
        int returnAddress;
        int stackBase;
        std::vector<DataType> localTypes;
    };
    std::vector<CallFrame> frames;
    void handleEscapes(std::string& str) {
        size_t pos = 0;
        while ((pos = str.find("\\n", pos)) != std::string::npos) {
            str.replace(pos, 2, "\n");
            pos += 1;
        }
    }
    std::vector<Value> stack;
    int pc = 0;

    Value pop() {
        Value val = stack.back();
        stack.pop_back();
        return val;
    }

    void push(Value val) {
        stack.push_back(val);
    }

public: 
    void setFunctions(const std::map<std::string, Function>& functions) {
        vmFunctions = functions;
    }
    int run(const std::vector<Instruction>& code, const std::vector<DataType>& indexTypes) {
        globals.clear();
        stack.clear();
        frames.clear();
        globals.reserve(512);
        stack.reserve(1024);
        globals.push_back(std::monostate{});
        const Instruction* rawCode = code.data();
        const size_t codeSize = code.size();
        pc = 0;
        bool endsWithNewline = false;
        setErrParam(code, &pc);
        while (pc < codeSize) {
            const Instruction& instr = rawCode[pc];

            switch (instr.op) {
                case OP_PUSH:
                    push(instr.value);
                    break;
                case OP_ADD:{
                    Value b = pop();
                    Value a = pop();
                    if ((std::holds_alternative<double>(a) || std::holds_alternative<int>(a)) &&
                        (std::holds_alternative<double>(b) || std::holds_alternative<int>(b))) {
                        
                        if (std::holds_alternative<double>(a) || std::holds_alternative<double>(b)) {
                            push(valueToFloat(a) + valueToFloat(b));
                        } else {
                            push((int)(valueToInt(a) + valueToInt(b)));
                        }
                    } 
                    else {
                        push(valueToString(a) + valueToString(b));
                    }
                    break;
                }
                case OP_SUB:{
                    if (stack.size()==1){
                        const Value val = pop();
                        if (std::holds_alternative<double>(val)){
                            push(-valueToFloat(val));
                        }
                        else
                        {
                            push((int)-valueToInt(val));
                        }
                        break;
                    }
                    Value b = pop();
                    Value a = pop();
                    if (std::holds_alternative<double>(a) || std::holds_alternative<double>(b)) {
                        double result = (double)(valueToFloat(a) - valueToFloat(b));
                        push(result);
                    } else {
                        int result = valueToInt(a) - valueToInt(b);
                        push(result);
                    }
                    break;
                }
                case OP_MUL:{
                    Value b = pop();
                    Value a = pop();
                    if (std::holds_alternative<double>(a) || std::holds_alternative<double>(b)) {
                        double result = (double)(valueToFloat(a) * valueToFloat(b));
                        push(result);
                    } else {
                        int result = valueToInt(a) * valueToInt(b);
                        push(result);
                    }
                    break;
                }
                case OP_DIV:{
                    Value b = pop();
                    Value a = pop();
                    if (valueToFloat(a) == 0 || valueToFloat(b) == 0){
                        push(0);
                        break;
                    }
                    double out = (double)(valueToFloat(a) / valueToFloat(b));
                    if (out != std::floor(out)){
                        push(Value(out));
                    }
                    else
                    {
                        push(Value((int)out));
                    }
                    break;
                }
                case OP_PRINT: {
                    if (stack.size() < 1){
                        throwError("Stack Underflow PC: " + std::to_string(pc), -1);
                    }
                    Value val = pop();
                    std::string text = valueToString(val);
                    if (text.empty()) break;
                    handleEscapes(text);
                    std::print("{}",text);
                    if (text[text.size()-1]=='\n'){
                        endsWithNewline=true;
                    }
                    break;
                }
                case OP_STORE: {
                    int index = (int)valueToInt(instr.value);
                    if (stack.empty()){
                        throwError("The value cannot be null\n",-1,true,"Variable Error");
                    }
                    Value val = pop();
                    if (index == 0){ throwError("Cannot store on index 0", -1, true, "Forbidden Access"); }
                    if (index < indexTypes.size()) {
                        if (!isTypeCompatible(indexTypes[index], valueToType(val)))
                            throwError("Cannot store " + std::string(typeToString(indexTypes[index])) + " value into " + std::string(typeToString(valueToType(val))) + " variable", -1, true);
                    }
                    if (index >= globals.size()) globals.resize(index + 1);
                    globals[index] = convertValSafely(val, indexTypes[index], valueToType(val));
                
                    break;
                }

                case OP_LOAD: {
                    if (std::holds_alternative<int>(instr.value)) {
                        const int index = std::get<int>(instr.value);
                        if (globals.size() <= index){
                            throwError("Stack Underflow", -1);
                        }
                        if (isNull(globals[index])){
                            throwError("No variable declared or variable is null.", -1);
                        }
                        push(globals[index]);
                    }
                    break;
                }
                case OP_JMP: {
                    pc += valueToInt(instr.value);
                    break;
                }
                case OP_JMP_IF_FALSE: {
                    const int result = valueToInt(pop(), -1);
                    if (result == -1) throwError("Condition is not a number",-1,1);
                    if (result == 0){
                        pc += valueToInt(instr.value);
                    }
                    break;
                }
                case OP_EQUALS:{
                    const Value val1 = pop();
                    const Value val2 = pop();
                    push(val1 == val2 ? 1 : 0);
                    break;
                }
                case OP_NOT_EQUALS:{
                    const Value val1 = pop();
                    const Value val2 = pop();
                    push(val1 != val2 ? 1 : 0);
                    break;
                }
                case OP_GREATER:{
                    const Value b = pop();
                    const Value a = pop();
                    push(a > b ? 1 : 0);
                    break;
                }
                case OP_LESS:{
                    const Value b = pop();
                    const Value a = pop();
                    push(a < b ? 1 : 0);
                    break;
                }
                case OP_NOT:{
                    const Value val = pop();
                    if (std::holds_alternative<int>(val)){
                        const int intVal = valueToInt(val);
                        if (intVal == 1 || intVal == 0){
                            push(!intVal);
                        }
                    }
                    else if (std::holds_alternative<bool>(val)){
                        const bool boolVal = valueToBool(val);
                        push(!boolVal);
                    }
                    break;
                }
                case OP_CALL: {
                    std::string funcName = valueToString(instr.value);
                    if (!vmFunctions.contains(funcName)) {
                        throwError("Runtime Error: Invocation targeting missing function routine: " + funcName, -1);
                    }
                    Function& func = vmFunctions[funcName];
                    if (func.isNative) {
                        int argCount = func.params.size();
                        std::vector<Value> args(argCount);
                        for (int i = argCount - 1; i >= 0; --i) {
                            args[i] = pop();
                        }
                        Value result = func.nativeHandler(args);
                        push(result);
                    } else {
                        int targetAddress = func.address; 
                        CallFrame frame;
                        frame.returnAddress = pc + 1;
                        frame.stackBase = stack.size() - func.params.size();
                        frame.localTypes = func.localTypes;
                        frames.push_back(frame);
                        pc = targetAddress - 1;
                    }
                    break;
                }
                case OP_RETURN: {
                    CallFrame currentFrame = frames.back();
                    frames.pop_back();
                    Value retVal = std::monostate{};
                    if (!stack.empty()) {
                        retVal = pop();
                    }
                    while (stack.size() > currentFrame.stackBase) {
                        pop();
                    }
                    
                    push(retVal);
                    pc = currentFrame.returnAddress - 1;
                    break;
                }
                case OP_LOAD_LOCAL: {
                    int slot = valueToInt(instr.value);
                    int actualIndex = frames.back().stackBase + slot;
                    push(stack[actualIndex]);
                    break;
                }
                case OP_STORE_LOCAL: {
                    int slot = valueToInt(instr.value);
                    Value val = pop();
                    CallFrame& frame = frames.back();
                    if (slot < (int)frame.localTypes.size()) {
                        DataType expected = frame.localTypes[slot];
                        DataType given = valueToType(val);
                        if (!isTypeCompatible(expected, given)) {
                            throwError("Cannot store " + std::string(typeToString(expected)) + " value into " + std::string(typeToString(given)) + " variable", -1, true);
                        }
                        val = convertValSafely(val, expected, given);
                    }
                    int actualIndex = frame.stackBase + slot;
                    if (actualIndex >= stack.size()) {
                        stack.resize(actualIndex + 1);
                    }
                    stack[actualIndex] = val;
                    break;
                }
                case OP_THROW:{
                    const int errCode = valueToInt(pop());
                    const int iden = valueToInt(pop());
                    const std::string msg = stringify(pop());
                    throwError(msg, errCode, true, "Uncaught Exception of id " + std::to_string(iden));
                    
                    return errCode;
                    break;
                }
                case OP_HALT:{
                    const Value out = pop();
                    if (!endsWithNewline) std::print("\n");
                    return valueToInt(out);
                }
                default:
                    throwError("Unknown OpCode met during runtime", -1, true, "OpCode Error");
                    return -INT32_MAX;
            }
            pc++;
        }
        return INT32_MAX;
    }
};
#endif