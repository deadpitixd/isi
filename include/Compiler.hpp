#ifndef __PITI_COMPILER_HPP
#define __PITI_COMPILER_HPP
#include <AST.hpp>
#include <iostream>
#include <vector>
#include <Opcode.hpp>
#include <Other.hpp>
#include <map>
#include <cmath>

bool isDigit(const std::string& s) {
    if (s.empty()) return false;
    size_t start = (s[0] == '-') ? 1 : 0;
    return s.find_first_not_of("0123456789.", start) == std::string::npos;
}

template <typename E>
constexpr std::string_view enum_to_string(E value) {
    template for (constexpr auto member : std::define_static_array(std::meta::enumerators_of(^^E))) {
        if (value == [:member:]) {
            return std::meta::display_string_of(member);
        }
    }
    return "<unknown>";
}

struct Symbol {
    int index;
    DataType type;
    bool isConst=false;
    Value constValue;
};

struct Instruction{
    OpCode op;
    Value value;
};

class Compiler{
    private:
    std::map<std::string, Symbol> symbolTable;
    int nextAvailableIndex = 0;
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

    bool isTypeCompatible(DataType expected, DataType given) {
        if (expected == given) return true;
        if (expected == DataType::FLOAT && given == DataType::INT) return true;
        return false;
    }

    public:
        std::unique_ptr<Expr> primary() {
            if (errTokens[current].type == TOKEN_NUMBER || errTokens[current].type == TOKEN_STRING) {
                return std::make_unique<LiteralExpr>(errTokens[current++].lexeme);
            }
            
            if (errTokens[current].type == TOKEN_IDENTIFIER) {
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
                   (errTokens[current].type == TOKEN_NOT_EQUALS))) {
                
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
                if (isDigit(valStr)) {
                    if (valStr.find('.') != std::string::npos) {
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
                if (symbolTable[var->name].isConst){
                    emitInstruction(OP_PUSH, symbolTable[var->name].constValue);
                }
                else {
                    emitInstruction(OP_LOAD, getVariableIndex(var->name, symbolTable[var->name].type));
                }
                return symbolTable[var->name].type;
            }
            else if (auto assign = dynamic_cast<AssignExpr*>(node.get())) {
                DataType rhsType = compileExpression(assign->value);
                DataType lhsType = symbolTable[assign->name].type;
                
                if (!isTypeCompatible(lhsType, rhsType)) {
                    throwError("Cannot assign " + typeToString(rhsType) + 
                            " to variable '" + assign->name + "' of type " + typeToString(lhsType), -1, false, "Type Error");
                }

                emitInstruction(OP_STORE, getVariableIndex(assign->name, lhsType));
                return lhsType;
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
                    default: break;
                }

                if (binary->op.type == TOKEN_EQUALS || binary->op.type == TOKEN_NOT_EQUALS) {
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
                if (isalpha(c)) {
                    std::string id;
                    while (isalnum(src[i])) {
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

                if (c == '(') { tokens.push_back({TOKEN_LPAREN, "("}); i++; continue; }
                if (c == ')') { tokens.push_back({TOKEN_RPAREN, ")"}); i++; continue; }
                if (c == '[') { tokens.push_back({TOKEN_LBRACKET, "["}); i++; continue; }
                if (c == ']') { tokens.push_back({TOKEN_RBRACKET, "]"}); i++; continue; }
                if (c == '{') { tokens.push_back({TOKEN_LBRACE, "{"}); i++; continue; }
                if (c == '}') { tokens.push_back({TOKEN_RBRACE, "}"}); i++; continue; }
                if (c == ';') { tokens.push_back({TOKEN_SEMICOLON, ";"}); i++; continue; }
                if (c == '=') { tokens.push_back({TOKEN_EQUALS, "="}); i++; continue; }
                if (c == ',') { tokens.push_back({TOKEN_COMMA, ","}); i++; continue; }
                if (c == '-') { tokens.push_back({TOKEN_MINUS, "-"}); i++; continue; }
                if (c == '*') { tokens.push_back({TOKEN_STAR, "*"}); i++; continue; }
                if (c == '/') { tokens.push_back({TOKEN_SLASH, "/"}); i++; continue; }
                if (c == '+') { tokens.push_back({TOKEN_PLUS, "+"}); i++; continue; }
                if (c == '!') { tokens.push_back({TOKEN_NOT, "!"}); i++; continue; }

                i++;
            }
            for (int i = 0; i < tokens.size(); i++){
                if (tokens[i].lexeme == "print"){
                    tokens[i].type = TOKEN_PRINT;
                }
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
                if (tokens[current].type == TOKEN_INT || tokens[current].type == TOKEN_FLOAT || tokens[current].type == TOKEN_STRING_T) {
                    DataType type;
                    if (tokens[current].type == TOKEN_INT) type = DataType::INT;
                    else if (tokens[current].type == TOKEN_FLOAT) type = DataType::FLOAT;
                    else type = DataType::STRING;
                    
                    current++;

                    if (tokens[current].type == TOKEN_IDENTIFIER) {
                        std::string name = tokens[current].lexeme;
                        current++;

                        std::unique_ptr<Expr> initializer = nullptr;

                        if (tokens[current].type == TOKEN_EQUALS) {
                            current++;
                            initializer = expression(); 
                        }

                        if (tokens[current].type == TOKEN_SEMICOLON) current++;
                        
                        blockStatements.push_back(std::make_unique<VarDeclStmt>(type, name, std::move(initializer), isConstDecl));
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
                        if (tokens[current].type == TOKEN_NUMBER) {
                            callArgs.push_back(std::make_unique<LiteralExpr>(tokens[current].lexeme));
                        }
                        
                        current++;
                        if (tokens[current].type == TOKEN_COMMA) current++;
                    }
                    
                    blockStatements.push_back(std::make_unique<ExpressionStmt>(
                        std::make_unique<CallExpr>(funcName, std::move(callArgs))
                    ));
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
            if (debug || useDevEnv) std::cout << "AST\n";
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
                                typeToString(varDecl->type) + " with " + typeToString(rhsType), -1, false, "Type Error");
                    }
                }

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
            else if (auto exprStmt = dynamic_cast<ExpressionStmt*>(node.get())) {
                compileExpression(exprStmt->expression);
            }
            else if (auto p = dynamic_cast<PrintStmt*>(node.get())) {
                for (const auto& arg : p->arguments) {
                    compileExpression(arg); 
                    emitByte(OP_PRINT);
                }
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
};

class VirtualMachine {
private:
    std::vector<Value> globals;
    std::map<std::string, Symbol> symbolTable;
    int nextSlot = 0;
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
    int run(const std::vector<Instruction>& code, const std::vector<DataType>& indexTypes) {
        globals.reserve(512);
        stack.reserve(1024);
        const Instruction* rawCode = code.data();
        const size_t codeSize = code.size();
        pc = 0;
        bool endsWithNewline = false;
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
                        int result = valueToInt(a) - valueToInt(b);
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
                        throwError("Stack Underflow PC: " + pc, -1);
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
                    Value val = pop();

                    if (index < indexTypes.size()) {
                        if (indexTypes[index] == DataType::INT && std::holds_alternative<double>(val) || std::holds_alternative<std::string>(val)) {
                            throwError("Cannot store " + std::string(enum_to_string(indexTypes[index])) + " value into " + std::string(enum_to_string(valueToType(val))) + " variable", -1, true);
                        }
                    }

                    if (index >= globals.size()) globals.resize(index + 1);
                    globals[index] = val;
                
                    break;
                }

                case OP_LOAD: {
                    if (std::holds_alternative<int>(instr.value)) {
                        const int index = std::get<int>(instr.value);
                        if (globals.size() < index){
                            throwError("Stack Underflow", -1);
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
                    const Value val1 = pop();
                    const Value val2 = pop();
                    push(val1 > val2 ? 1 : 0);
                    break;
                }
                case OP_LESS:{
                    const Value val1 = pop();
                    const Value val2 = pop();
                    push(val1 < val2 ? 1 : 0);
                    break;
                }
                case OP_HALT:{
                    const Value out = pop();
                    if (!endsWithNewline) std::print("\n");
                    return valueToInt(out);
                }
                default:
                    std::cerr << "Unknown OpCode: " << instr.op << std::endl;
                    return -INT32_MAX;
            }
            pc++;
        }
        return INT32_MAX;
    }
};
#endif