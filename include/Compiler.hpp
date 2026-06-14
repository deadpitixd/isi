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
    int nextLibraryId = 1;
    std::map<int, std::string> loadedLibraries;
    int nextAvailableIndex = 1;
    std::vector<Instruction> Code;
    std::vector<DataType> indexTypes;
    std::string currentSourceDir;

    std::vector<Token> *globTokens;

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
            if (errTokens[current].type == TOKEN_F_STR) {
                current++;
                
                if (errTokens[current].type != TOKEN_STRING) {
                    throwError("String expected after f string", errors::syntaxError, true, "Syntax Error");
                }

                std::string format = errTokens[current].lexeme;
                current++;

                std::unique_ptr<Expr> rootExpr = nullptr;
                std::string currentLiteral = "";

                for (size_t i = 0; i < format.size(); i++) {
                    if (format[i] == '{') {
                        if (!currentLiteral.empty() || rootExpr == nullptr) {
                            auto litNode = std::make_unique<LiteralExpr>(currentLiteral);
                            if (rootExpr == nullptr) {
                                rootExpr = std::move(litNode);
                            } else {
                                rootExpr = std::make_unique<BinaryExpr>(
                                    std::move(rootExpr), 
                                    Token{TOKEN_PLUS, "+"}, 
                                    std::move(litNode)
                                );
                            }
                            currentLiteral = "";
                        }

                        std::string varName = "";
                        i++;
                        while (i < format.size() && format[i] != '}') {
                            varName += format[i];
                            i++;
                        }

                        if (!varName.empty()) {
                            auto varNode = std::make_unique<VariableExpr>(varName);
                            if (rootExpr == nullptr) {
                                rootExpr = std::move(varNode);
                            } else {
                                rootExpr = std::make_unique<BinaryExpr>(
                                    std::move(rootExpr), 
                                    Token{TOKEN_PLUS, "+"}, 
                                    std::move(varNode)
                                );
                            }
                        }
                        continue;
                    }
                    currentLiteral += format[i];
                }

                if (!currentLiteral.empty()) {
                    auto litNode = std::make_unique<LiteralExpr>(currentLiteral);
                    if (rootExpr == nullptr) {
                        rootExpr = std::move(litNode);
                    } else {
                        rootExpr = std::make_unique<BinaryExpr>(
                            std::move(rootExpr), 
                            Token{TOKEN_PLUS, "+"}, 
                            std::move(litNode)
                        );
                    }
                }

                if (rootExpr == nullptr) {
                    return std::make_unique<LiteralExpr>("");
                }

                return rootExpr;
            }
            if (errTokens[current].type == TOKEN_NUMBER || errTokens[current].type == TOKEN_STRING) {
                return std::make_unique<LiteralExpr>(errTokens[current++].lexeme);
            }
            
            if (errTokens[current].type == TOKEN_CHAR) {
                char cVal = errTokens[current].lexeme[0];
                current++;
                return std::make_unique<LiteralExpr>(cVal);
            }
            
            if (errTokens[current].type == TOKEN_IDENTIFIER) {
                std::unique_ptr<Expr> expr = nullptr;
                
                if (peek(1).type == TOKEN_LPAREN) {
                    std::string funcName = errTokens[current].lexeme;
                    current += 2;
                    std::vector<std::unique_ptr<Expr>> callArgs;
                    while (!isAtEnd() && errTokens[current].type != TOKEN_RPAREN) {
                        callArgs.push_back(expression());
                        if (errTokens[current].type == TOKEN_COMMA) current++;
                    }
                    if (!isAtEnd()) current++;
                    expr = std::make_unique<CallExpr>(funcName, std::move(callArgs));
                } else {
                    expr = std::make_unique<VariableExpr>(errTokens[current++].lexeme);
                }

                while (!isAtEnd() && errTokens[current].type == TOKEN_LBRACKET) {
                    current++;
                    auto indexExpr = expression();
                    if (errTokens[current].type != TOKEN_RBRACKET) {
                        throwError("Expected ']' after string index", errors::syntaxError, true, "Syntax Error");
                    }
                    current++;
                    expr = std::make_unique<IndexExpr>(std::move(expr), std::move(indexExpr));
                }

                return expr;
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
            if (errTokens[current].type == TOKEN_LOADLIB) {
                current++;
                if (errTokens[current].type != TOKEN_LPAREN) {
                    throwError("Expected '(' after loadLibrary keyword", errors::syntaxError, true, "Syntax Error");
                }
                current++;
                
                if (errTokens[current].type != TOKEN_STRING) {
                    throwError("Expected static string argument in loadLibrary()", errors::syntaxError, true, "Syntax Error");
                }
                std::string libPath = errTokens[current].lexeme;
                current++;
                
                if (errTokens[current].type != TOKEN_RPAREN) {
                    throwError("Expected ')' matching loadLibrary function opening bracket", errors::syntaxError, true, "Syntax Error");
                }
                current++;
                
                return std::make_unique<LoadLibExpr>(libPath);
            }
            
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
                
                if (std::holds_alternative<char>(literal->value)) {
                    emitInstruction(OP_PUSH, literal->value);
                    return DataType::CHAR;
                }

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
            else if (auto indexExpr = dynamic_cast<IndexExpr*>(node.get())) {
                DataType objType = compileExpression(indexExpr->object);
                DataType idxType = compileExpression(indexExpr->index);
                
                if (objType != DataType::STRING) {
                    throwError("Only strings can be indexed.", errors::typeError, false, "Type Error");
                }
                if (idxType != DataType::INT) {
                    throwError("Index must be an integer.", errors::typeError, false, "Type Error");
                }
                
                emitByte(OP_INDEX);
                return DataType::CHAR;
            }
            else if (auto var = dynamic_cast<VariableExpr*>(node.get())) {
                std::string varName = var->name;

                bool exists = (localSymbolTable.find(varName) != localSymbolTable.end()) || 
                            (symbolTable.find(varName) != symbolTable.end());

                if (!exists) {
                    std::vector<std::string> availableNames;
                    for (const auto& [name, _] : localSymbolTable) availableNames.push_back(name);
                    for (const auto& [name, _] : symbolTable) availableNames.push_back(name);

                    std::string suggestion = nearestString(availableNames, varName);
                    
                    std::string errorMsg = "Undefined variable '" + varName + "'";
                    if (!suggestion.empty()) {
                        errorMsg += ". Did you mean '" + suggestion + "'?";
                    }

                    throwError(errorMsg, errors::undefinedError, true, "Name Error");
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
            else if (auto ll = dynamic_cast<LoadLibExpr*>(node.get())){
                std::string resolvedLibPath = ll->path;
                if (!currentSourceDir.empty() && ll->path.find('/') == std::string::npos) {
                    resolvedLibPath = currentSourceDir + "/" + ll->path;
                }
                registerNativeLibrary(ll->path, resolvedLibPath);
                
                int currentLibId = nextLibraryId++;
                loadedLibraries[currentLibId] = ll->path;
                
                emitInstruction(OP_PUSH, currentLibId);
                return DataType::INT;
            }
            else if (auto assign = dynamic_cast<AssignExpr*>(node.get())) {
                DataType rhsType = compileExpression(assign->value);
                
                if (isCompilingFunction && localSymbolTable.contains(assign->name)) {
                    emitInstruction(OP_STORE_LOCAL, localSymbolTable[assign->name]);
                    return rhsType;
                }
                
                if (symbolTable.contains(assign->name)) {
                    if(symbolTable[assign->name].isConst){
                        throwError("Cannot assign a value to a constant variable '"+assign->name+"'.",-2);
                    }
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
                    
                    if (!isTypeCompatible(paramType, argType)) {
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
                    size_t commentStart = i;
                    while (i < src.size() && src[i] != '#') {
                        i++;
                    }
                    if (i > commentStart) {
                        std::string commentContent = src.substr(commentStart, i - commentStart);
                        if (commentContent.find("__SOURCE_DIR__") != std::string::npos) {
                            size_t dirStart = commentContent.find('"');
                            size_t dirEnd = commentContent.rfind('"');
                            if (dirStart != std::string::npos && dirEnd != std::string::npos && dirStart < dirEnd) {
                                currentSourceDir = commentContent.substr(dirStart + 1, dirEnd - dirStart - 1);
                            }
                        }
                    }
                    if (i < src.size()) i++;
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
                if (c == '\''){
                    i++;
                    std::string id;
                    if (src[i] != '\''){
                        if (src[i+1] != '\''){
                            throwError("A char can only have one character", errors::syntaxError, true, "Syntax Error");
                        }
                        id = std::string(1, src[i]);
                        tokens.push_back({TOKEN_CHAR, id});
                        i+=2;
                    }
                    else
                    {
                        tokens.push_back({TOKEN_CHAR, stringify(defaultValueOfType(DataType::CHAR))});
                        i++;
                    }
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
            for (size_t i = 0; i < tokens.size(); i++){
                if (tokens[i].type == TOKEN_IDENTIFIER){
                    if (tokens[i].lexeme == "print"){ tokens[i].type = TOKEN_PRINT; }
                    if (tokens[i].lexeme == "extern"){ tokens[i].type = TOKEN_EXTERN; }
                    if (tokens[i].lexeme == "import"){ tokens[i].type = TOKEN_IMPORT; }
                    if (tokens[i].lexeme == "lib"){ tokens[i].type = TOKEN_LIB; }
                    if (tokens[i].lexeme == "int"){ tokens[i].type = TOKEN_INT; }
                    if (tokens[i].lexeme == "string"){ tokens[i].type = TOKEN_STRING_T; }
                    if (tokens[i].lexeme == "bool"){ tokens[i].type = TOKEN_BOOL; }
                    if (tokens[i].lexeme == "float"){ tokens[i].type = TOKEN_FLOAT; }
                    if (tokens[i].lexeme == "char"){ tokens[i].type = TOKEN_CHAR_T; }
                    if (tokens[i].lexeme == "if"){ tokens[i].type = TOKEN_IF; }
                    if (tokens[i].lexeme == "else"){ tokens[i].type = TOKEN_ELSE; }
                    if (tokens[i].lexeme == "while"){ tokens[i].type = TOKEN_WHILE; }
                    if (tokens[i].lexeme == "exit"){ tokens[i].type = TOKEN_EXIT; }
                    if (tokens[i].lexeme == "const"){ tokens[i].type = TOKEN_CONST; }
                    if (tokens[i].lexeme == "return"){ tokens[i].type = TOKEN_RETURN; }
                    if (tokens[i].lexeme == "throw"){ tokens[i].type = TOKEN_THROW; }
                    if (tokens[i].lexeme == "loadLibrary"){ tokens[i].type = TOKEN_LOADLIB; }
                    if (tokens[i].lexeme == "true"){ tokens[i].type = TOKEN_NUMBER; tokens[i].lexeme = std::to_string(1); }
                    if (tokens[i].lexeme == "false"){ tokens[i].type = TOKEN_NUMBER; tokens[i].lexeme = std::to_string(0); }
                    if (tokens[i].lexeme == "f" && i + 1 < tokens.size() && tokens[i+1].type == TOKEN_STRING){ tokens[i].type = TOKEN_F_STR; } 
                }

                if (i + 1 < tokens.size() && tokens[i].type == TOKEN_NOT && tokens[i+1].type == TOKEN_EQUALS) { 
                    tokens[i].type = TOKEN_NOT_EQUALS; 
                    tokens.erase(tokens.begin() + i + 1); 
                }
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
                    if (!tokenIsDecl(tokens[current].type)){
                        throwError(std::string("Stray 'const' keyword met, expected identifier (Requested: ") + enum_to_string(TOKEN_IDENTIFIER) + ", but got: " + enum_to_string(tokens[current].type)
                        ,errors::strayKeywordError, true , "Stray Keyword Error");
                    }
                }
                if (tokens[current].type == TOKEN_IMPORT) {
                    current++;
                    if (tokens[current].type != TOKEN_STRING) throwError("Expected string path after 'import'", -1);
                    std::string importPath = tokens[current].lexeme;
                    current++;
                    if (tokens[current].type == TOKEN_SEMICOLON) current++;
                    blockStatements.push_back(std::make_unique<ImportStmt>(importPath));
                    continue;
                }
                if (tokens[current].type == TOKEN_EXTERN) {
                    current++;
                    if (tokens[current].type != TOKEN_LPAREN) throwError("Expected '(' after extern", errors::syntaxError, true, "Syntax Error");
                    current++;
                    if (tokens[current].type != TOKEN_IDENTIFIER && tokens[current].type != TOKEN_NUMBER) throwError("Expected library id", errors::syntaxError, true, "Syntax Error");
                    
                    std::string libHandle = tokens[current].lexeme;
                    current++;
                    if (tokens[current].type != TOKEN_RPAREN) throwError("Expected ')' after extern library handle", errors::syntaxError, true, "Syntax Error");
                    current++;
                    
                    if (tokens[current].type != TOKEN_INT && tokens[current].type != TOKEN_FLOAT && tokens[current].type != TOKEN_STRING_T && tokens[current].type != TOKEN_CHAR_T && tokens[current].type != TOKEN_BOOL) {
                        throwError("Expected return type after extern()", errors::syntaxError, true, "Syntax Error");
                    }
                    DataType returnType;
                    if (tokens[current].type == TOKEN_INT) returnType = DataType::INT;
                    else if (tokens[current].type == TOKEN_FLOAT) returnType = DataType::FLOAT;
                    else if (tokens[current].type == TOKEN_BOOL) returnType = DataType::BOOL;
                    else if (tokens[current].type == TOKEN_CHAR_T) returnType = DataType::CHAR;
                    else returnType = DataType::STRING;
                    current++;
                    
                    if (tokens[current].type != TOKEN_IDENTIFIER) throwError("Expected extern function name", errors::syntaxError, true, "Syntax Error");
                    std::string funcName = tokens[current].lexeme;
                    current++;
                    
                    if (tokens[current].type != TOKEN_LPAREN) throwError("Expected '(' after extern function name", errors::syntaxError, true, "Syntax Error");
                    current++;
                    
                    std::vector<Param> params;
                    while (!isAtEnd() && tokens[current].type != TOKEN_RPAREN) {
                        if (tokens[current].type != TOKEN_INT && tokens[current].type != TOKEN_FLOAT && tokens[current].type != TOKEN_STRING_T && tokens[current].type != TOKEN_CHAR_T && tokens[current].type != TOKEN_BOOL) {
                            throwError("Expected parameter type in extern declaration", errors::syntaxError, true, "Syntax Error");
                        }
                        DataType pType;
                        if (tokens[current].type == TOKEN_INT) pType = DataType::INT;
                        else if (tokens[current].type == TOKEN_FLOAT) pType = DataType::FLOAT;
                        else if (tokens[current].type == TOKEN_BOOL) pType = DataType::BOOL;
                        else if (tokens[current].type == TOKEN_CHAR_T) pType = DataType::CHAR;
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
                    if (tokens[current].type != TOKEN_RPAREN) throwError("Expected ')' at end of extern parameter list", errors::syntaxError, true, "Syntax Error");
                    current++;
                    if (tokens[current].type == TOKEN_SEMICOLON) current++;
                    
                    blockStatements.push_back(std::make_unique<ExternStmt>(libHandle, funcName, returnType, params));
                    continue;
                }
                if (tokens[current].type == TOKEN_INT || tokens[current].type == TOKEN_FLOAT ||
                     tokens[current].type == TOKEN_STRING_T || tokens[current].type == TOKEN_CHAR_T) {
                    DataType type;
                    if (tokens[current].type == TOKEN_INT) type = DataType::INT;
                    else if (tokens[current].type == TOKEN_FLOAT) type = DataType::FLOAT;
                    else if (tokens[current].type == TOKEN_CHAR_T) type = DataType::CHAR;
                    else type = DataType::STRING;
                    
                    if (peek(1).type == TOKEN_NUMBER){
                        throwError("Variable cannot start with a number", errors::syntaxError, true, "Syntax Error");
                    }
                    if (peek(1).type == TOKEN_IDENTIFIER && peek(2).type == TOKEN_LPAREN) {
                        current++;
                        std::string funcName = tokens[current].lexeme;
                        if (isdigit(funcName[0])){
                            throwError("Variable cannot start with a number", errors::syntaxError, true, "Syntax Error");
                        }
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
            globTokens = &tokens;
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

                    if (varDecl->initializer && dynamic_cast<LoadLibExpr*>(varDecl->initializer.get())) {
                                            newSymbol.constValue = nextLibraryId - 1;
                                            newSymbol.isConst = true; 
                    }
                    if (newSymbol.index >= indexTypes.size()) {
                        indexTypes.resize(newSymbol.index + 1);
                    }
                    indexTypes[newSymbol.index] = varDecl->type;

                    if (!newSymbol.isConst) {
                        emitInstruction(OP_STORE, getVariableIndex(varDecl->name, varDecl->type));
                    }
                }
            }
            else if (auto importDecl = dynamic_cast<ImportStmt*>(node.get())) {
                emitInstruction(OP_IMPORT, importDecl->path);
            }
            else if (auto externDecl = dynamic_cast<ExternStmt*>(node.get())) {
                Function function;
                function.name = externDecl->funcName;
                function.returnType = externDecl->returnType;
                function.isVoid = (externDecl->returnType == DataType::VOID);
                function.isNative = true;
                
                std::string targetLibPath = "";
                
                if (symbolTable.contains(externDecl->libHandle)) {
                    int libId = valueToInt(symbolTable[externDecl->libHandle].constValue);
                    if (loadedLibraries.contains(libId)) {
                        targetLibPath = loadedLibraries[libId];
                    } else {
                        throwError("Invalid library ID referenced by '" + externDecl->libHandle + "'", -1);
                    }
                } else if (isdigit(externDecl->libHandle[0])) {
                    int libId = std::stoi(externDecl->libHandle);
                    if (loadedLibraries.contains(libId)) {
                        targetLibPath = loadedLibraries[libId];
                    } else {
                        throwError("Invalid library ID: " + externDecl->libHandle, -1);
                    }
                } else {
                    throwError("Undefined library variable '" + externDecl->libHandle + "'", -1);
                }
                function.nativeHandler = [libHandle = targetLibPath,
                                        symbol = externDecl->funcName](std::vector<Value> args) -> Value {
                    void* sym = resolveNativeSymbol(libHandle, symbol);
                    if (!sym) {
                        throwError("Failed to resolve symbol '" + symbol + "'", -1);
                    }

                    using VecRefFn = Value(*)(const std::vector<Value>&);
                    using VecValFn = Value(*)(std::vector<Value>);

                    {
                        auto fn = reinterpret_cast<VecRefFn>(sym);
                        if (fn) return fn(args);
                    }

                    {
                        auto fn = reinterpret_cast<VecValFn>(sym);
                        if (fn) return fn(args);
                    }

                    throwError("External function does not match expected vector<Value> signature for '" + symbol + "'", -1);
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
                    
                    if (!isTypeCompatible(currentFunctionReturnType, actualReturnType)) {
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
        std::string result;
        result.reserve(str.size());

        for (size_t i = 0; i < str.size(); ++i) {
            if (str[i] == '\\' && i + 1 < str.size()) {
                if (str[i + 1] == 'x' && i + 3 < str.size()) {
                    if (str[i + 2] == '1' && str[i + 3] == 'B') {
                        result += '\x1B'; 
                        i += 3;
                    } else {
                        result += str[i];
                    }
                } else {
                    switch (str[i + 1]) {
                        case 'n':  result += '\n'; break;
                        case 't':  result += '\t'; break;
                        case 'r':  result += '\r'; break;
                        case '\\': result += '\\'; break;
                        default:   result += str[i + 1]; break;
                    }
                    i++;
                }
            } else {
                result += str[i];
            }
        }
        str = result;
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
                case OP_IMPORT: {
                    break;
                }
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
                case OP_INDEX: {
                    Value indexVal = pop();
                    Value objVal = pop();
                    
                    int idx = valueToInt(indexVal);
                    std::string str = valueToString(objVal);
                    
                    if (idx < 0 || idx > str.length()) {
                        throwError("String index out of bounds", -1, true, "Runtime Error");
                    }
                    
                    push(str[idx]);
                    break;
                }
                case OP_HALT:{
                    const Value out = pop();
                    if (!endsWithNewline) std::print("\n");
                    return valueToInt(out);
                }
                default:
                    throwError("Unknown OpCode met during runtime\nRecompiling the program might fix the issue", -1, true, "OpCode Error");
                    return -INT32_MAX;
            }
            pc++;
        }
        return INT32_MAX;
    }
};
#endif