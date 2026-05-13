#include <AST.hpp>
#include <iostream>
#include <vector>
#include <Opcode.hpp>
#include <Other.hpp>
#include <map>

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
        int getVariableIndex(const std::string& name, DataType type){
            auto it = symbolTable.find(name);
            if (it != symbolTable.end()){
                return it->second.index;
            }
            int index = nextAvailableIndex++;
            symbolTable[name] = { index, type };
    
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
                // skip whitespace
                if (isspace(c)) {
                    i++;
                    continue;
                }

                // numbers
                if (isdigit(c)) {
                    std::string num;
                    while (i < src.size() && (isdigit(src[i]) || src[i] == '.')) {
                        num += src[i++];
                    }
                    tokens.push_back({TOKEN_NUMBER, num});
                    continue;
                }
                // identifiers
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
                // operators
                if (c == '+') {
                    if (src[i+1] == '=') {
                        tokens.push_back({TOKEN_PLUS_EQUALS, "+="});
                        i += 2;
                    } else {
                        tokens.push_back({TOKEN_PLUS, "+"});
                        i++;
                    }
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


                i++;
            }
            for (int i = 0; i < tokens.size(); i++){
                if (tokens[i].lexeme == "cout"){
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
            }
            tokens.push_back({TOKEN_EOF, "\0"});
            return tokens;
        }
        std::vector<std::unique_ptr<Stmt>> parseExpression(std::vector<Token>& tokens){
            return {};
        }
        std::vector<std::unique_ptr<Stmt>> makeAST(std::vector<Token>& tokens){
            if (debug || useDevEnv) std::cout << "AST\n";
            std::vector<std::unique_ptr<Stmt>> statements;
            errTokens = tokens;
            while (current < tokens.size() && tokens[current].type != TOKEN_EOF) {
                // Data types
                if (tokens[current].type == TOKEN_INT || tokens[current].type == TOKEN_FLOAT || tokens[current].type == TOKEN_STRING_T) {
                    DataType type = (tokens[current].type == TOKEN_INT) ? DataType::INT : DataType::STRING;
                    current++;

                    if (tokens[current].type == TOKEN_IDENTIFIER) {
                        std::string name = tokens[current].lexeme;
                        current++;

                        std::unique_ptr<Expr> initializer = nullptr;
                        if (tokens[current].type == TOKEN_EQUALS) {
                            current++;
                            if (tokens[current].type == TOKEN_NUMBER || tokens[current].type == TOKEN_STRING) {
                                initializer = std::make_unique<LiteralExpr>(tokens[current].lexeme);
                                current++;
                            }
                        }
                        
                if (tokens[current].type == TOKEN_SEMICOLON) current++;
                statements.push_back(std::make_unique<VarDeclStmt>(type, name, std::move(initializer)));
                continue;
                    }
                }
                if (tokens[current].type == TOKEN_IDENTIFIER && peek(1).type == TOKEN_EQUALS) {
                    std::string name = tokens[current].lexeme;
                    current += 2;

                    std::unique_ptr<Expr> value;
                    if (tokens[current].type == TOKEN_NUMBER || tokens[current].type == TOKEN_STRING) {
                        value = std::make_unique<LiteralExpr>(tokens[current].lexeme);
                        current++;
                    }
                    
                    if (tokens[current].type == TOKEN_SEMICOLON) current++;
                    statements.push_back(std::make_unique<ExpressionStmt>(std::make_unique<AssignExpr>(name, std::move(value))));
                    continue;
                }
                // Functions
                if (tokens[current].type == TOKEN_IDENTIFIER && expect(TOKEN_LPAREN)) {
                    std::string funcName = tokens[current].lexeme;
                    if (debug && !useDevEnv) std::cout << "Calling function '" << funcName << "'\n";
                    std::vector<std::unique_ptr<Expr>> callArgs;
                    
                    current+=2;

                    while (!isAtEnd() && tokens[current].type != TOKEN_RPAREN) {
                        if (tokens[current].type == TOKEN_NUMBER) {
                            if (debug && !useDevEnv) std::cout << "Pushed back number '" << tokens[current].lexeme << "'\n";
                            callArgs.push_back(std::make_unique<LiteralExpr>(tokens[current].lexeme));
                        }
                        
                        current++;
                        if (tokens[current].type == TOKEN_COMMA) current++;
                    }
                    
                    statements.push_back(std::make_unique<ExpressionStmt>(
                        std::make_unique<CallExpr>(funcName, std::move(callArgs))
                    ));
                }
                // Print statement, really close to a function
                // THIS CODE BLOCK WILL BE REMOVED WHEN C++ EXTENSION WILL BE ADDED
                if (tokens[current].type == TOKEN_PRINT){
                    if (expect(TOKEN_LPAREN)) {
                        if (debug && !useDevEnv) std::cout << "Calling function 'cout'\n";
                        std::vector<std::unique_ptr<Expr>> astArgs;
                        current += 2; 

                        while (!isAtEnd() && tokens[current].type != TOKEN_RPAREN) {
                            if (tokens[current].type == TOKEN_STRING) {
                                if (debug && !useDevEnv) std::cout << "Pushed back string '" << tokens[current].lexeme << "'\n";
                                astArgs.push_back(std::make_unique<LiteralExpr>(tokens[current].lexeme));
                            } 
                            else if (tokens[current].type == TOKEN_IDENTIFIER) {
                                if (debug && !useDevEnv) std::cout << "Pushed back identifier '" << tokens[current].lexeme << "'\n";
                                astArgs.push_back(std::make_unique<VariableExpr>(tokens[current].lexeme));
                            }
                            
                            current++;
                            if (tokens[current].type == TOKEN_COMMA) current++;
                        }
                        
                        statements.push_back(std::make_unique<PrintStmt>(std::move(astArgs)));
                    }
                }
                current++;
            }
            return statements;
        }  
        void compile(const std::vector<std::unique_ptr<Stmt>>& nodes)
        {
            for (const std::unique_ptr<Stmt> &node : nodes){
                if (auto decl = dynamic_cast<VarDeclStmt*>(node.get())) {
                    int index = getVariableIndex(decl->name, decl->type);
                    if (decl->initializer) {
                        if (auto lit = dynamic_cast<LiteralExpr*>(decl->initializer.get())) {
                            emitInstruction(OP_PUSH, lit->value);
                        }
                        emitInstruction(OP_STORE, index);
                    }
                }
                
                else if (auto exprStmt = dynamic_cast<ExpressionStmt*>(node.get())) {
                    if (auto assign = dynamic_cast<AssignExpr*>(exprStmt->expression.get())) {
                        int index = getVariableIndex(assign->name, DataType::INT);
                        if (auto lit = dynamic_cast<LiteralExpr*>(assign->value.get())) {
                            emitInstruction(OP_PUSH, lit->value);
                        }
                        emitInstruction(OP_STORE, index);
                    }
                }
                if (auto p = dynamic_cast<PrintStmt*>(node.get())) {
                    for (const auto& arg : p->arguments) {
                        if (auto lit = dynamic_cast<LiteralExpr*>(arg.get())) {
                            emitInstruction(OP_PUSH, lit->value); 
                        } 
                        else if (auto var = dynamic_cast<VariableExpr*>(arg.get())) {
                            emitInstruction(OP_LOAD, getVariableIndex(var->name, DataType::INT));
                        }
                        emitByte(OP_PRINT);
                    }
                }
            }
            emitInstruction(OP_HALT,0);
            if (debug){
                for (Instruction i : Code){
                    if (valueToString(i.value) != "")
                        std::println("Op: {}, Val: {}", enum_to_string(i.op), valueToString(i.value));
                    else
                        std::println("Op: {}", enum_to_string(i.op));
                }
            }
        }
        constexpr std::vector<Instruction> getCode(){
            return Code;
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
    int run(const std::vector<Instruction>& code) {
        pc = 0;
        bool endsWithNewline = false;
        while (pc < code.size()) {
            const Instruction& instr = code[pc];

            switch (instr.op) {
                case OP_PUSH:
                    push(instr.value);
                    break;

                case OP_PRINT: {
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
                    if (std::holds_alternative<int>(instr.value)) {
                        int index = std::get<int>(instr.value);
                        Value val = pop();
                        if (index >= globals.size()) globals.resize(index + 1);
                        globals[index] = val;
                    }
                    break;
                }

                case OP_LOAD: {
                    if (std::holds_alternative<int>(instr.value)) {
                        int index = std::get<int>(instr.value);
                        push(globals[index]);
                    }
                    break;
                }
                case OP_HALT:{
                    if (!endsWithNewline) std::print("\n");
                    return valueToInt(instr.value);
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