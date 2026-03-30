#pragma once
#ifndef ENVIRONMENT_HPP
#define ENVIRONMENT_HPP

#include <map>
#include <string>
#include <vector>
#include <functional>
#include <variant>
#include <unordered_map>
#include "Value.hpp"
#include "Other.hpp"

class Environment;
void execute(const std::vector<Token>& code, Environment& env);

class Environment {
private:
    std::unordered_map<std::string, Symbol> symbols;
    Environment* enclosing = nullptr;

public:
    std::unordered_map<std::string, Function> functionTable;
    Environment(Environment* parent = nullptr) : enclosing(parent) {}

    static DataType getValType(const Value& val) {
        if (std::holds_alternative<int>(val)) return DataType::INT;
        if (std::holds_alternative<double>(val)) return DataType::FLOAT;
        if (std::holds_alternative<std::string>(val)) return DataType::STRING;
        if (std::holds_alternative<bool>(val)) return DataType::BOOL;

        if (val.valueless_by_exception()) {
            throwError("Internal Error: Value is in an invalid state", -4);
        }

        throwError("Unknown value type during type check", -4);
        return DataType::INT;
    }

    void addNativeFunction(std::string name, NativeFunction handler) {
        Function func;
        func.name = name;
        func.isNative = true;
        func.nativeHandler = handler;

        functionTable[name] = func;
    }

    Value callFunction(std::string name, std::vector<Value> args, Environment& parentEnv) {
        if (!functionTable.contains(name)) {
            throwError("Function " + name + " not found", -11);
        }

        Function& func = functionTable[name];

        if (func.isNative) {
            if (!func.nativeHandler) throwError("Native handler is null", -11);
            return func.nativeHandler(args);
        }

        Environment localEnv(&parentEnv);
        if (args.size() != func.params.size()) {
            throwError("Argument mismatch for " + name, -11);
        }

        for (size_t j = 0; j < func.params.size(); j++) {
            localEnv.define(func.params[j].name, func.params[j].type, args[j]);
        }

        try {
            execute(func.body, localEnv);
        } catch (ReturnSignal& sig) {
            return sig.value;
        }

        return 0;
    }

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

    static std::string typeToString(DataType type) {
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
            } else if constexpr (std::is_same_v<T, std::monostate>) { return "null"; } 
            else {
                return std::to_string(arg);
            }
        }, v);
    }

    Value evaluateExpression(std::vector<Token> tokens, Environment& env) {
        if (tokens.empty()) return 0;

    auto getAtomValue = [&](size_t& index) -> Value {
        if (index >= tokens.size()) return 0;
        Token t = tokens[index];

        if (t.type == isiTokenType::IDENTIFIER && index + 1 < tokens.size() && tokens[index + 1].text == "(") {
            std::string funcName = t.text;
            index += 2;

            std::vector<Value> args;
            while (index < tokens.size() && tokens[index].text != ")") {
                std::vector<Token> argExpr;
                int parenCount = 0;
                while (index < tokens.size() && (tokens[index].text != "," || parenCount > 0) && tokens[index].text != ")") {
                    if (tokens[index].text == "(") parenCount++;
                    if (tokens[index].text == ")") parenCount--;
                    argExpr.push_back(tokens[index]);
                    index++;
                }
                args.push_back(evaluateExpression(argExpr, env));
                if (index < tokens.size() && tokens[index].text == ",") index++;
            }
            index++;

            return callFunction(funcName, args, env);
        }

        if (t.text == "input" && index + 1 < tokens.size() && tokens[index + 1].text == "(") {
                index += 2;

                std::vector<Token> promptExpr;
                while (index < tokens.size() && tokens[index].text != ")") {
                    promptExpr.push_back(tokens[index]);
                    index++;
                }

                if (!promptExpr.empty()) {
                    std::cout << env.stringify(env.evaluateExpression(promptExpr, env));
                }
                index++;
            
                std::string userInput;
                if (std::getline(std::cin >> std::ws, userInput)) {
                    return env.parseLiteral(userInput);
                }
                return std::string("");
        }

        if (t.type == isiTokenType::IDENTIFIER && env.exists(t.text)) {
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

    static Value parseLiteral(const std::string& token) {
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
            } else if constexpr (std::is_same_v<T, std::monostate>) { return "null"; } else {
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

double valueToFloat(const Value& val) {
    if (std::holds_alternative<double>(val)) {
        return std::get<double>(val);
    } else if (std::holds_alternative<int>(val)) {
        return static_cast<double>(std::get<int>(val));
    } else if (std::holds_alternative<bool>(val)) {
        return std::get<bool>(val) ? 1.0 : 0.0;
    }
    return 0.0;
}

bool valueToBool(const Value& val) {
    if (std::holds_alternative<bool>(val)) {
        return std::get<bool>(val);
    } 
    else if (std::holds_alternative<int>(val)) {
        return std::get<int>(val) != 0;
    } 
    else if (std::holds_alternative<double>(val)) {
        return std::get<double>(val) != 0.0;
    } 
    else if (std::holds_alternative<std::string>(val)) {
        return !std::get<std::string>(val).empty();
    }
    return false;
}

std::string valueToString(const Value& val) {
    if (std::holds_alternative<std::string>(val)) {
        return std::get<std::string>(val);
    } else if (std::holds_alternative<int>(val)) {
        return std::to_string(std::get<int>(val));
    } else if (std::holds_alternative<double>(val)) {
        return std::to_string(std::get<double>(val));
    } else if (std::holds_alternative<bool>(val)) {
        return std::get<bool>(val) ? "true" : "false";
    }
    return "";
}

std::string valueTypeToStr(const Value& val){
    return Environment::typeToString(Environment::getValType(val));
}

#endif