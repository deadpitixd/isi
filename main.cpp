#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <variant>
#include <stdexcept>
#include <string.h>
#include <cctype>

std::vector<bool> boolSettings = {false};

enum class TokenType {
    KEYWORD,
    IDENTIFIER,
    LITERAL,
    OPERATOR,
    SYMBOL
};


struct Token {
    std::string text;
    TokenType type;
};
bool isKeyword(const std::string& s) {
    static const std::unordered_set<std::string> kws = {
        "int", "float", "double", "string", "bool", "cout", "input", "while", "if", "endl", "return"
    };
    return kws.contains(s);
}

const char newLine = '`';
std::unordered_set<char> nlChars = {';', ' ','(',')','{','}',','};

enum class DataType { INT, FLOAT, STRING, BOOL };

std::string typeToString(DataType type) {
    switch (type) {
        case DataType::INT: return "int";
        case DataType::FLOAT: return "float";
        case DataType::STRING: return "string";
        case DataType::BOOL: return "bool";
        default: return "unknown";
    }
}

std::vector<Token> *cCode;
int *cI;
void setErrParam(std::vector<Token> *code, int *i){
    cCode = code;
    cI = i;
}
void throwError(std::string msg, int errCode, bool runtimeErr=true){
    std::cerr << msg << ", at token line: " << *cI << " (token: '" << (*cCode)[*cI].text << "')" <<  "\n";
    std::exit(errCode);
}

using Value = std::variant<int, double, std::string, bool>;

struct ReturnSignal {
    Value value;
};

struct Symbol {
    DataType type;
    Value value;
};

class Environment {
private:
    std::unordered_map<std::string, Symbol> symbols;
    Environment* enclosing = nullptr;

    DataType getValType(const Value& val) {
        if (std::holds_alternative<int>(val)) return DataType::INT;
        if (std::holds_alternative<double>(val)) return DataType::FLOAT;
        if (std::holds_alternative<std::string>(val)) return DataType::STRING;
        if (std::holds_alternative<bool>(val)) return DataType::BOOL;
        throwError("Unknown value type", -4);
        return DataType::BOOL;
    }

public:
    Environment(Environment* parent = nullptr) : enclosing(parent) {}

    void define(const std::string& name, DataType type, Value initialValue) {
        if (type == DataType::FLOAT && std::holds_alternative<int>(initialValue)) {
            initialValue = static_cast<double>(std::get<int>(initialValue));
        }
        else if (type == DataType::INT && std::holds_alternative<double>(initialValue)) {
            initialValue = static_cast<int>(std::get<double>(initialValue));
        }

        if (getValType(initialValue) != type) {
            throwError("Type mismatch in declaration of '" + name + "'", -4);
        }
        symbols[name] = {type, initialValue};
    }

    bool asBool(const Value& val) {
        if (std::holds_alternative<bool>(val)) return std::get<bool>(val);
        if (std::holds_alternative<int>(val)) return std::get<int>(val) != 0;
        if (std::holds_alternative<double>(val)) return std::get<double>(val) != 0.0;
        return false;
    }

    std::string typeToString(DataType type) {
        switch (type) {
            case DataType::INT:    return "int";
            case DataType::FLOAT:  return "float";
            case DataType::STRING: return "string";
            case DataType::BOOL:   return "bool";
            default:               return "unknown";
        }
    }
    std::string stringify(const Value& v) {
        return std::visit([](auto&& arg) -> std::string {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, std::string>) {
                return arg;
            } else if constexpr (std::is_same_v<T, bool>) {
                return arg ? "true" : "false";
            } else {
                return std::to_string(arg);
            }
        }, v);
    }

    Value evaluateExpression(std::vector<Token> tokens, Environment& env) {
        if (tokens.empty()) return 0;

        auto getAtomValue = [&](size_t& index) -> Value {
            if (index >= tokens.size()) return 0;
            Token t = tokens[index];

            if (t.text == "input" && index + 1 < tokens.size() && tokens[index + 1].text == "(") {
                index += 2;
                while (index < tokens.size() && tokens[index].text != ")") {
                    index++;
                }
                index++; 

                std::string cinInput;
                if (std::getline(std::cin >> std::ws, cinInput)) {
                    return env.parseLiteral(cinInput);
                }
                return std::string("");
            }

            if (t.type == TokenType::IDENTIFIER && env.exists(t.text)) {
                index++;
                return env.get(t.text);
            }

            index++;
            return env.parseLiteral(t.text);
        };

        size_t i = 0;
        Value result = getAtomValue(i);

        while (i + 1 < tokens.size()) {
            std::string op = tokens[i].text;
            i++; 

            Value nextVal = getAtomValue(i);

            result = std::visit([&](auto&& l, auto&& r) -> Value {
                using T1 = std::decay_t<decltype(l)>;
                using T2 = std::decay_t<decltype(r)>;

                if constexpr ((std::is_same_v<T1, int> || std::is_same_v<T1, double>) &&
                              (std::is_same_v<T2, int> || std::is_same_v<T2, double>)) {
                    double leftVal = static_cast<double>(l);
                    double rightVal = static_cast<double>(r);

                    if (op == "+") return leftVal + rightVal;
                    if (op == "-") return leftVal - rightVal;
                    if (op == "*") return leftVal * rightVal;
                    if (op == "/") return (rightVal != 0) ? leftVal / rightVal : 0.0;
                    if (op == "==") return leftVal == rightVal;
                    if (op == "!=") return leftVal != rightVal;
                    if (op == "<")  return leftVal < rightVal;
                    if (op == ">")  return leftVal > rightVal;
                    if (op == "<=") return leftVal <= rightVal;
                    if (op == ">=") return leftVal >= rightVal;
                } 
                else if (op == "+") {
                    return env.stringify(l) + env.stringify(r);
                }
                else if (op == "==") {
                    return env.stringify(l) == env.stringify(r);
                }
                return 0;
            }, result, nextVal);
        }

        return result;
    }

    Value parseLiteral(const std::string& token) {
        if (token == "true") return true;
        if (token == "false") return false;

        if (token.size() >= 2 && token.front() == '"' && token.back() == '"') {
            return token.substr(1, token.size() - 2);
        }

        try {
            size_t pos;
            if (token.find('.') != std::string::npos) {
                double d = std::stod(token, &pos);
                if (pos == token.size()) return d;
            } else {
                int i = std::stoi(token, &pos);
                if (pos == token.size()) return i;
            }
        } catch (...) {
        }

        return token; 
    }

    std::string valueToString(const Value& val) {
        return std::visit([](auto&& arg) -> std::string {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, bool>) {
                return arg ? "true" : "false";
            } else if constexpr (std::is_same_v<T, std::string>) {
                return arg;
            } else {
                // handles int and double
                return std::to_string(arg);
            }
        }, val);
    }

    void assign(const std::string& name, Value newValue) {
        if (symbols.contains(name)) {
            Symbol& sym = symbols[name];
    
            if (sym.type == DataType::FLOAT && std::holds_alternative<int>(newValue)) {
                newValue = static_cast<double>(std::get<int>(newValue));
            } else if (sym.type == DataType::INT && std::holds_alternative<double>(newValue)) {
                newValue = static_cast<int>(std::get<double>(newValue));
            }
        
            if (getValType(newValue) != sym.type && sym.type != DataType::STRING) {
                throwError("Type mismatch: " + typeToString(sym.type) + " and " + typeToString(getValType(newValue)), -4);
            }
            sym.value = newValue;
            return;
        }

        if (enclosing != nullptr) {
            enclosing->assign(name, newValue);
            return;
        }

        throwError("Undefined variable '" + name + "'", -4);
    }

    void printVariable(const std::string& name, Environment& env) {
        Value val = env.get(name);

        std::visit([](auto&& arg) {
            std::cout << arg;
        }, val);

        std::cout << std::endl;
    }

    bool exists(const std::string& name) {
        if (symbols.contains(name)) {
            return true;
        }

        if (enclosing != nullptr) {
            return enclosing->exists(name);
        }

        return false;
    }

    Value get(const std::string& name) {
        if (symbols.contains(name)) {
            return symbols.at(name).value;
        }
        if (enclosing != nullptr) {
            return enclosing->get(name);
        }
        throwError("Undefined variable '" + name + "'", -4);
        return enclosing->get(name);
    }
};
std::unordered_set<std::string> keywords = {"cout", "int"};

std::vector<Token> Lexer(std::vector<std::string> lines) {
    std::vector<Token> tokens;
    for (const std::string& line : lines) {
        int i = 0;
        while (i < line.length()) {
            char c = line[i];
            if (std::isspace(c)) { i++; continue; }

            if (c == '"') {
                std::string str = "";
                i++;
                while (i < line.length() && line[i] != '"') {
                    str += line[i];
                    i++;
                }
                tokens.push_back({str, TokenType::LITERAL});
                i++; continue;
            }
            
            if (c == '{' || c == '}') {
                tokens.push_back({std::string(1, c), TokenType::SYMBOL});
                i++;
                continue;
            }

            if (std::isalpha(c) || c == '_') {
                std::string word;
                while (i < line.length() && (std::isalnum(line[i]) || line[i] == '_')) {
                    word += line[i]; i++;
                }
                TokenType t = isKeyword(word) ? TokenType::KEYWORD : TokenType::IDENTIFIER;
                tokens.push_back({word, t});
                continue;
            }

            if (std::isdigit(c)) {
                std::string num;
                while (i < line.length() && (std::isdigit(line[i]) || line[i] == '.')) {
                    num += line[i]; i++;
                }
                tokens.push_back({num, TokenType::LITERAL});
                continue;
            }

            std::string op(1, c);
            if (i + 1 < line.length()) {
                std::string twoChar = line.substr(i, 2);
                if (twoChar == "==" || twoChar == "!=" || twoChar == "<=" || twoChar == ">=") {
                    tokens.push_back({twoChar, TokenType::OPERATOR});
                    i += 2; continue;
                }
            }
            
            TokenType t = (ispunct(c) && std::string("+-*/=< >").find(c) != std::string::npos) 
                        ? TokenType::OPERATOR : TokenType::SYMBOL;
            tokens.push_back({op, t});
            i++;
        }
    }
    return tokens;
}

void execute(const std::vector<Token>& code, Environment& env) {
    for (int i = 0; i < code.size(); i++) {
        Token t = code[i];

        if (t.type == TokenType::KEYWORD && (t.text == "int" || t.text == "float" || t.text == "double" || t.text == "string" || t.text == "bool")) {
            DataType type = (t.text == "int") ? DataType::INT : 
                            (t.text == "bool") ? DataType::BOOL : 
                            (t.text == "string") ? DataType::STRING : DataType::FLOAT;
            i++;
            std::string name = code[i].text;
            
            Value initialVal;
            if (type == DataType::INT) initialVal = 0;
            else if (type == DataType::FLOAT) initialVal = 0.0;
            else if (type == DataType::BOOL) initialVal = false;
            else initialVal = std::string("");

            env.define(name, type, initialVal);

            if (i + 1 < code.size() && code[i+1].text == "=") {
                i += 2;
                std::vector<Token> expr;
                while (i < code.size() && code[i].text != ";") {
                    expr.push_back(code[i]);
                    i++;
                }
                env.assign(name, env.evaluateExpression(expr, env));
            }
        }

        else if (t.type == TokenType::IDENTIFIER) {
            if (i + 1 < code.size() && code[i+1].text == "=") {
                std::string varName = t.text;
                i += 2;
                std::vector<Token> expr;
                while (i < code.size() && code[i].text != ";") {
                    expr.push_back(code[i]);
                    i++;
                }
                env.assign(varName, env.evaluateExpression(expr, env));
            }
        }

        else if (t.text == "cout") {
            i++;
            if (i < code.size() && code[i].text == "(") {
                i++;
                while (i < code.size() && code[i].text != ")") {
                    std::vector<Token> expr;
                    while (i < code.size() && code[i].text != "," && code[i].text != ")") {
                        expr.push_back(code[i]);
                        i++;
                    }
                    if (!expr.empty()) {
                        std::cout << env.stringify(env.evaluateExpression(expr, env));
                    }
                    if (code[i].text == ",") i++; 
                }
            }
        }

        else if (t.text == "if") {
            i++; 
            if (code[i].text == "(") i++;
            std::vector<Token> conditionTokens;
            while (i < code.size() && code[i].text != ")") {
                conditionTokens.push_back(code[i]);
                i++;
            }
            i++;

            bool shouldExecute = env.asBool(env.evaluateExpression(conditionTokens, env));

            if (i < code.size() && code[i].text == "{") {
                i++;
                int start = i;
                int braceCount = 1;
                while (i < code.size() && braceCount > 0) {
                    if (code[i].text == "{") braceCount++;
                    else if (code[i].text == "}") braceCount--;
                    i++;
                }
                int end = i - 1;

                if (shouldExecute) {
                    std::vector<Token> block(code.begin() + start, code.begin() + end);
                    execute(block, env);
                }
                i--;
            }
        }
        
        if (t.text == "return") {
            i++;
            std::vector<Token> expr;
            while (i < code.size() && code[i].text != ";") {
                expr.push_back(code[i]);
                i++;
            }
            throw ReturnSignal{ env.evaluateExpression(expr, env) };
        }
    }
}

void run(std::vector<Token> code) {
    int i = 0;
    setErrParam(&code, &i);
    Environment env;
    env.define("endl", DataType::STRING, "\n");
    execute(code, env);
}
int main(int argc, char* argv[]){
    bool debug = false;
    bool *ignUnkFlags = new bool;
    *ignUnkFlags = false;
    if (argc > 0){
        for (int i = 2; i < argc; i++){
            if (strcmp(argv[i], "--ignore-unk-flags") == 0){
                *ignUnkFlags = true;
            }
            else if (strcmp(argv[i], "--debug") == 0){
                debug = true;
            }
            else
            {
                if (!*ignUnkFlags){
                    std::cerr << "Unknown flag: '" << argv[i] << "'!\n";
                    return -2;
                }
            }
        }
    }
    delete ignUnkFlags;

    std::vector<std::string> input;
    std::ifstream inputFile(argv[1]);
    if (!inputFile.is_open()){
        std::cerr << "Failed to open file: '" << argv[1] << "'.\n";
        return 1;
    }
    std::string line;

    while (std::getline(inputFile, line)){
        input.push_back(line);
    }
    inputFile.close();

    std::vector<Token> lexed = Lexer(input);

    if (debug){
        int *i = new int;
        *i = 0;
        for (const Token& l : lexed) {
            std::cout << "[" << *i << "] " << l.text << "\n";
            *i = *i + 1;
        }
        delete i;
    }

    run(lexed);

    return 0;
}