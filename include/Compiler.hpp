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

        std::unique_ptr<Expr> addition() {
            auto expr = multiplication();

            while (errTokens[current].type == TOKEN_PLUS || errTokens[current].type == TOKEN_MINUS) {
                Token op = errTokens[current++];
                auto right = multiplication();
                expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
            }

            return expr;
        }

        std::unique_ptr<Expr> expression() {
            return addition();
        }
        void compileExpression(const std::unique_ptr<Expr>& node) {
            if (auto literal = dynamic_cast<LiteralExpr*>(node.get())) {
                std::string valStr = valueToString(literal->value);
                if (isDigit(valStr)) {
                    if (valStr.find('.') != std::string::npos) {
                        emitInstruction(OP_PUSH, std::stod(valStr));
                    } else {
                        emitInstruction(OP_PUSH, std::stoi(valStr));
                    }
                } else {
                    emitInstruction(OP_PUSH, literal->value); 
                }
            }
            else if (auto var = dynamic_cast<VariableExpr*>(node.get())) {
                emitInstruction(OP_LOAD, getVariableIndex(var->name, DataType::INT));
            }
            else if (auto binary = dynamic_cast<BinaryExpr*>(node.get())) {
                compileExpression(binary->left);
                compileExpression(binary->right);
                
                switch (binary->op.type) {
                    case TOKEN_PLUS:  emitByte(OP_ADD); break;
                    case TOKEN_MINUS: emitByte(OP_SUB); break;
                    case TOKEN_STAR:  emitByte(OP_MUL); break;
                    case TOKEN_SLASH: emitByte(OP_DIV); break;
                    default: break;
                }
            }
        }
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
                        
                        statements.push_back(std::make_unique<VarDeclStmt>(type, name, std::move(initializer)));
                        continue;
                    }
                }
                if (tokens[current].type == TOKEN_IDENTIFIER && peek(1).type == TOKEN_EQUALS) {
                    std::string name = tokens[current].lexeme;
                    current += 2;

                    std::unique_ptr<Expr> value = expression();
                    
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
                if (tokens[current].type == TOKEN_EQUALS){
                    current++;
                    
                }
                // Print statement, really close to a function
                // THIS CODE BLOCK WILL BE REMOVED WHEN C++ EXTENSION WILL BE ADDED
                if (tokens[current].type == TOKEN_PRINT) {
                    if (expect(TOKEN_LPAREN)) {
                        std::vector<std::unique_ptr<Expr>> astArgs;
                        current += 2; 

                        while (!isAtEnd() && tokens[current].type != TOKEN_RPAREN) {
                            // REPLACE all your manual if/else checks with this one line:
                            astArgs.push_back(expression()); 
                            
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
                        compileExpression(decl->initializer); 
                        emitInstruction(OP_STORE, index);
                    }
                }
                
                else if (auto exprStmt = dynamic_cast<ExpressionStmt*>(node.get())) {
                    if (auto assign = dynamic_cast<AssignExpr*>(exprStmt->expression.get())) {
                        auto it = symbolTable.find(assign->name);
                        
                        if (it == symbolTable.end()) {
                            throwError("Variable '" + assign->name + "' not defined.", -1);
                        }

                        int index = it->second.index;
                        DataType varType = it->second.type; 

                        compileExpression(assign->value);

                        emitInstruction(OP_STORE, index);
                    }
                }
                if (auto p = dynamic_cast<PrintStmt*>(node.get())) {
                    for (const auto& arg : p->arguments) {
                        compileExpression(arg); 
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

    // Pops from stack
    Value pop() {
        Value val = stack.back();
        stack.pop_back();
        return val;
    }

    // Pushes a value to the stack
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
                case OP_ADD:{
                    Value b = pop();
                    Value a = pop();
                    // Does math if both Values are numbers
                    if ((std::holds_alternative<double>(a) || std::holds_alternative<int>(a)) &&
                        (std::holds_alternative<double>(b) || std::holds_alternative<int>(b))) {
                        
                        if (std::holds_alternative<double>(a) || std::holds_alternative<double>(b)) {
                            push(valueToFloat(a) + valueToFloat(b));
                        } else {
                            push((int)(valueToInt(a) + valueToInt(b)));
                        }
                    } 
                    // Does concat if they are not
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
                    if (index >= globals.size()) globals.resize(index + 1);
                    globals[index] = val;
                
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