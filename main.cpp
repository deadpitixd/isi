#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <variant>
#include <stdexcept>

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

using Value = std::variant<int, double, std::string, bool>;

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
        throw std::runtime_error("Unknown value type");
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
        throw std::runtime_error("Type mismatch in declaration of '" + name + "'");
    }
    symbols[name] = {type, initialValue};
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

    Value evaluateExpression(std::vector<std::string> tokens, Environment& env) {
        if (tokens.empty()) return 0;

        Value result = env.exists(tokens[0]) ? env.get(tokens[0]) : env.parseLiteral(tokens[0]);

        size_t i = 1;
        while (i + 1 < tokens.size()) {
            std::string op = tokens[i];
            Value nextVal = env.exists(tokens[i + 1]) ? env.get(tokens[i + 1]) : env.parseLiteral(tokens[i + 1]);

            result = std::visit([&](auto&& l, auto&& r) -> Value {
                using T1 = std::decay_t<decltype(l)>;
                using T2 = std::decay_t<decltype(r)>;

                if constexpr ((std::is_same_v<T1, int> || std::is_same_v<T1, double>) &&
                              (std::is_same_v<T2, int> || std::is_same_v<T2, double>)) {
                            
                    double leftVal = static_cast<double>(l);
                    double rightVal = static_cast<double>(r);
                    double res = 0;

                    if (op == "+") res = leftVal + rightVal;
                    else if (op == "-") res = leftVal - rightVal;
                    else if (op == "*") res = leftVal * rightVal;
                    else if (op == "/") res = (rightVal != 0) ? leftVal / rightVal : 0;

                    if constexpr (std::is_same_v<T1, int> && std::is_same_v<T2, int>) {
                        if (op != "/") return static_cast<int>(res);
                    }
                    return res;
                } 
                else if (op == "+") {
                    return env.stringify(l) + env.stringify(r);
                }
                return 0;
            }, result, nextVal);

            i += 2;
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
                 throw std::runtime_error("Type mismatch...");
            }
            sym.value = newValue;
            return;
        }

        if (enclosing != nullptr) {
            enclosing->assign(name, newValue);
            return;
        }

        throw std::runtime_error("Undefined variable '" + name + "'");
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
        throw std::runtime_error("Undefined variable '" + name + "'");
    }
};
std::unordered_set<std::string> keywords = {"cout", "int"};

std::vector<std::string> Lexer(std::vector<std::string> in){
    bool doSpace = true;
    int i = 0;
    std::vector<std::string> out;
    for (const std::string& l : in){
        out.push_back("");
        for (int c = 0; c < l.size(); c++){
            if (l[c] == '"') {
                doSpace = !doSpace;
                if (doSpace) {
                    out.push_back("");
                    i++;
                    continue;
                } else {
                    continue;
                }
            }
            if (nlChars.find(l[c]) == nlChars.end() || !doSpace){
                out[i] = out[i] + l[c];
            }
            else
            {
                if (!(out[out.size()-1] == "")){
                    out.push_back("");
                    i++;
                }
                if (l[c] != ' '){
                    if (l[c] == '('){
                        out[i]+=l[c];
                        out.push_back("");
                        i++;
                    }
                    else
                        out[i] += l[c];
                }
            }
        }
        i++;
    }
    return out;
}

void run(std::vector<std::string> code){
    Environment env;
    env.define("endl", DataType::STRING, "\n");
    for (int i=0; i<code.size();i++){
        if (keywords.find(code[i]) == keywords.end()){

        }
        if (code[i] == "int"){
            i++;
            if (!env.exists(code[i])){
                env.define(code[i], DataType::INT, 0);
            }
            if (code[i+1] == ";"){
                i+=2;
            }
        }
        // float is the same as double, it's basically a macro pointing to double.
        if (code[i] == "double" || code[i] == "float"){
            i++;
            if (!env.exists(code[i])){
                env.define(code[i], DataType::FLOAT, 0.0);
            }
            if (code[i+1] == ";"){
                i+=2;
            }
        }
        if (code[i] == "string"){
            i++;
            if (!env.exists(code[i])){
                env.define(code[i], DataType::STRING, "");
            }
            if (code[i+1] == ";")
            {
                i+=2;
            }
        }
        if (code[i] == "=") {
            std::string varName = code[i-1];
            std::vector<std::string> expressionTokens;

            int j = i + 1; 

            while (j < code.size() && code[j] != ";") {
                expressionTokens.push_back(code[j]);
                j++;
            }
        
            Value result = env.evaluateExpression(expressionTokens, env);

            env.assign(varName, result);
        
            i = j; 
        }
        if (code[i] == "cout"){
            i++;
            if (code[i]=="("){
                i++;
                while (code[i] != ")"){
                    if(code[i]!=","){
                        if (!env.exists(code[i]))
                            std::cout << code[i];
                        else
                            std::cout << env.valueToString(env.get(code[i]));
                    }
                    i++;
                }
            }
        }
        if (code[i] == "input"){
            i++;
            if(code[i]=="("){
                i++;
                std::string cinInput;
                std::getline(std::cin >> std::ws, cinInput);
                env.assign(code[i], env.parseLiteral(cinInput));
            }
        }
    }
}

int main(int argc, char* argv[]){
    bool debug = true;

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

    input = Lexer(input);

    if (debug){
        for (const auto& l : input) {
            std::cout << l << "\n";
        }
    }

    run(input);

    return 0;
}