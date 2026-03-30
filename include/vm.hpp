#pragma once
#include <vector>
#include <iostream>
#include "Value.hpp"
#include "Opcodes.hpp"
#include "Enviroment.hpp"

class VM {
public:
    std::vector<Value> constants;
    std::vector<Value> globals;
    std::vector<uint> bytecode;
    size_t pc = 0;

    void load(const std::vector<uint>& code, const std::vector<Value>& consts) {
        bytecode = code;
        constants = consts;
        globals.resize(256); // for example
        pc = 0;
    }

    void run() {
        while (pc < bytecode.size()) {
            OpCode op = (OpCode)bytecode[pc++];
            switch (op) {
                case OpCode::OP_CONSTANT: {
                    uint idx = bytecode[pc++];
                    stack.push_back(constants[idx]);
                    break;
                }
                case OpCode::OP_PRINT: {
                    Value val = stack.back(); stack.pop_back();
                    printValue(val);
                    break;
                }
                case OpCode::OP_SET_GLOBAL: {
                    uint idx = bytecode[pc++];
                    globals[idx] = stack.back(); stack.pop_back();
                    break;
                }
                default: break;
            }
        }
    }

private:
    std::vector<Value> stack;

    void printValue(const Value& v) {
        std::visit([](auto&& arg) {
            if constexpr (std::is_same_v<decltype(arg), std::monostate>) std::cout << "nil";
            else std::cout << valueTypeToStr(arg);
        }, v);
        std::cout << "\n";
    }
};