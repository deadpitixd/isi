#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include "Value.hpp"
#include "Opcodes.hpp"
#include "Enviroment.hpp"

class Compiler {
public:
    std::vector<uint8_t> bytecode;
    std::vector<Value> constants;
    
    std::unordered_map<std::string, uint8_t> globalsMap;

    void compile(const std::vector<Token>& tokens) {
        for (size_t i = 0; i < tokens.size(); i++) {
            const Token& t = tokens[i];

            if (tokens[i].text == "cout") {
                i++;
                if (tokens[i].text == "(") i++; 

                while (i < tokens.size() && tokens[i].text != ")") {
                    if (tokens[i].text == ",") {
                        i++;
                        continue;
                    }

                    emitValue(tokens[i]);
                    emitOp(OpCode::OP_PRINT);
                    i++;
                }
            }
            
            else if (t.type == isiTokenType::IDENTIFIER && i + 1 < tokens.size() && tokens[i+1].text == "=") {
                std::string varName = t.text;
                i += 2;
                
                emitValue(tokens[i]);
                
                if (globalsMap.contains(varName)) {
                    globalsMap[varName] = (uint8_t)globalsMap.size();
                }
                
                emitOp(OpCode::OP_SET_GLOBAL);
                emitByte(globalsMap[varName]);
            }
        }
        emitOp(OpCode::OP_HALT);
    }

private:
    void emitByte(uint8_t byte) { bytecode.push_back(byte); }
    void emitOp(OpCode op) { emitByte(static_cast<uint8_t>(op)); }

    void handleValue(const Token& t) {
        if (t.type == isiTokenType::LITERAL) {
            emitOp(OpCode::OP_CONSTANT);
            emitByte(addConstant(Environment::parseLiteral(t.text))); 
        } 
        else if (t.type == isiTokenType::IDENTIFIER) {
            emitOp(OpCode::OP_GET_GLOBAL);
            emitByte(globalsMap[t.text]);
        }
    }

    void emitValue(const Token& t) {
        if (t.type == isiTokenType::LITERAL) {
            Value val;
            if (isdigit(t.text[0])) {
                val = std::stoi(t.text);
            } else {
                std::string s = t.text;
                if (s.front() == '"') s.erase(0, 1);
                if (s.back() == '"') s.pop_back();
                val = s;
            }
            
            uint8_t idx = addConstant(val);
            emitOp(OpCode::OP_CONSTANT);
            emitByte(idx);
        } 
        else if (t.type == isiTokenType::IDENTIFIER) {
            if (globalsMap.count(t.text)) {
                emitOp(OpCode::OP_GET_GLOBAL);
                emitByte(globalsMap[t.text]);
            }
        }
    }

    uint8_t addConstant(Value val) {
        constants.push_back(val);
        return static_cast<uint8_t>(constants.size() - 1);
    }
};