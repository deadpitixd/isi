#include <AST.hpp>
#include <iostream>
#include <vector>
#include <Opcode.hpp>
#include <Other.hpp>

struct Instruction{
    OpCode op;
    Value value;
};

class Compiler{
    private:
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
            std::cout << "AST\n";
            std::vector<std::unique_ptr<Stmt>> statements;
            errTokens = tokens;
            while (current < tokens.size() && tokens[current].type != TOKEN_EOF) {
                // Functions
                if (tokens[current].type == TOKEN_IDENTIFIER && expect(TOKEN_LPAREN)) {
                    std::string funcName = tokens[current].lexeme;
                    std::cout << "Calling function '" << funcName << "'\n";
                    std::vector<std::unique_ptr<Expr>> callArgs;
                    
                    current+=2;

                    while (!isAtEnd() && tokens[current].type != TOKEN_RPAREN) {
                        if (tokens[current].type == TOKEN_NUMBER) {
                            std::cout << "Pushed back number '" << tokens[current].lexeme << "'\n";
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
                        std::cout << "Calling function 'cout'\n";
                        std::vector<std::unique_ptr<Expr>> astArgs;
                        current += 2; 

                        while (!isAtEnd() && tokens[current].type != TOKEN_RPAREN) {
                            if (tokens[current].type == TOKEN_STRING) {
                                std::cout << "Pushed back string '" << tokens[current].lexeme << "'\n";
                                astArgs.push_back(std::make_unique<LiteralExpr>(tokens[current].lexeme));
                            } 
                            else if (tokens[current].type == TOKEN_IDENTIFIER) {
                                std::cout << "Pushed back identifier '" << tokens[current].lexeme << "'\n";
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
        void compile(std::vector<std::unique_ptr<Stmt>> nodes)
        {
            for (const std::unique_ptr<Stmt> &node : nodes){
                if (auto p = dynamic_cast<PrintStmt*>(node.get())) {
                    for (const auto& arg : p->arguments) {
                        if (auto lit = dynamic_cast<LiteralExpr*>(arg.get())) {
                            emitInstruction(OP_PUSH, lit->value); 
                        } 
                        else if (auto var = dynamic_cast<VariableExpr*>(arg.get())) {
                            //emitInstruction(OP_LOAD, getVariableIndex(var->name));
                        }
                        emitByte(OP_PRINT);
                    }
                }
            }
            emitByte(OP_HALT);
            for (Instruction i : Code){
                std::cout << i.op << ", " << valueToString(i.value) << "\n";
            }
        }
        constexpr std::vector<Instruction> getCode(){
            return Code;
        }
};

class VirtualMachine {
private:
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
    void run(const std::vector<Instruction>& code) {
        pc = 0;
        while (pc < code.size()) {
            const Instruction& instr = code[pc];

            switch (instr.op) {
                case OP_PUSH:
                    push(instr.value);
                    break;

                case OP_PRINT: {
                    Value val = pop();
                    std::string text = valueToString(val);
                    handleEscapes(text);
                    std::cout << text;
                    break;
                }

                case OP_HALT:
                    return;

                default:
                    std::cerr << "Unknown OpCode: " << instr.op << std::endl;
                    return;
            }
            pc++;
        }
    }
};