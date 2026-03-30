#ifndef VM_HPP
#define VM_HPP

#include <vector>
#include <stack>
#include <functional>
#include "Opcodes.hpp"
#include "Value.hpp"
#include "Enviroment.hpp"

std::vector<Value> globals;

struct CallFrame {
    size_t returnAddr;
    size_t stackStart; // for local variables
};

// Prototype for C++ functions ISI can call
using NativeFn = Value(*)(int argCount, Value* args);

class ISI_VM {
private:
    std::vector<uint8_t> bytecode;
    std::vector<Value> constants;
    std::vector<Value> globals;
    std::vector<Value> stack;
    std::stack<CallFrame> callStack;
    
    std::vector<NativeFn> nativeTable; // Registered C++ functions
    size_t pc = 0; // Program Counter

    // Helper to read next byte from code
    uint8_t readByte() { return bytecode[pc++]; }
    
    // Helper to read 16-bit addresses
    uint16_t readShort() {
        pc += 2;
        return (uint16_t)((bytecode[pc - 2] << 8) | bytecode[pc - 1]);
    }

public:
    void load(const std::vector<uint8_t>& code, const std::vector<Value>& consts) {
        bytecode = code;
        constants = consts;
        pc = 0;
        if(globals.size() < 256) globals.resize(256); 
    }

    void bindNative(NativeFn fn) {
        nativeTable.push_back(fn);
    }

    bool run() {
        while (pc < bytecode.size()) {
            OpCode instruction = (OpCode)readByte();

            switch (instruction) {
                case OpCode::OP_CONSTANT: {
                    uint8_t index = readByte();
                    stack.push_back(constants[index]);
                    break;
                }

            case OpCode::OP_ADD: {
                Value b = stack.back(); stack.pop_back();
                Value a = stack.back(); stack.pop_back();
            
                if ((std::holds_alternative<int>(a) || std::holds_alternative<double>(a)) &&
                    (std::holds_alternative<int>(b) || std::holds_alternative<double>(b))) {
                    
                    double valA = std::holds_alternative<double>(a) ? std::get<double>(a) : std::get<int>(a);
                    double valB = std::holds_alternative<double>(b) ? std::get<double>(b) : std::get<int>(b);
                    
                    if (std::holds_alternative<int>(a) && std::holds_alternative<int>(b)) {
                        stack.push_back(Value((int)(valA + valB)));
                    } else {
                        stack.push_back(Value(valA + valB));
                    }
                } 
                else if (std::holds_alternative<std::string>(a) || std::holds_alternative<std::string>(b)) {
                    auto stringify = [](const Value& v) -> std::string {
                        return std::visit([](auto&& arg) -> std::string {
                            using T = std::decay_t<decltype(arg)>;
                            if constexpr (std::is_same_v<T, std::monostate>) return "nil";
                            else if constexpr (std::is_same_v<T, bool>) return arg ? "true" : "false";
                            else if constexpr (std::is_same_v<T, std::string>) return arg;
                            else return std::to_string(arg);
                        }, v);
                    };
                    stack.push_back(Value(stringify(a) + stringify(b)));
                }
                break;
            }

                case OpCode::OP_GET_GLOBAL: {
                    uint8_t index = readByte();
                    stack.push_back(globals[index]);
                    break;
                }

                case OpCode::OP_CALL: {
                    int funcAddr = (int)valueToFloat(stack.back()); 
                    stack.pop_back();

                    // Save where we are so we can come back
                    callStack.push({ pc, stack.size() }); 
                    pc = funcAddr; 
                    break;
                }

                case OpCode::OP_CALL_NATIVE: {
                    uint8_t id = readByte();
                    uint8_t argCount = readByte();

                    // Point to the arguments at the top of the stack
                    Value* args = &stack[stack.size() - argCount];

                    Value result = nativeTable[id](argCount, args);

                    // Clean up args and push the C++ result
                    for(int i=0; i < argCount; i++) stack.pop_back();
                    stack.push_back(result);
                    break;
                }
                case OpCode::OP_SET_GLOBAL: {
                        uint8_t index = readByte(); 

                        Value val = stack.back(); 

                        if (index >= globals.size()) {
                            globals.resize(index + 1);
                        }
                    
                        globals[index] = val;

                    stack.pop_back(); 
                    break;
                }

                case OpCode::OP_GET_LOCAL: {
                    uint8_t slot = readByte();
                    size_t currentFrameStart = callStack.top().stackStart;
                    stack.push_back(stack[currentFrameStart + slot]);
                    break;
                }

                case OpCode::OP_RETURN: {
                    if (callStack.empty()) return true; // End of program

                    CallFrame frame = callStack.top();
                    callStack.pop();

                    pc = frame.returnAddr;
                    // Optional: Clear local variables from stack here
                    break;
                }

            case OpCode::OP_PRINT: {
                Value val = stack.back();
                stack.pop_back();
            
                std::visit([](auto&& arg) {
                    using T = std::decay_t<decltype(arg)>;
                    
                    if constexpr (std::is_same_v<T, std::monostate>) {
                        std::cout << "nil"; // Or "undefined"
                    } 
                    else if constexpr (std::is_same_v<T, bool>) {
                        std::cout << (arg ? "true" : "false");
                    } 
                    else {
                        std::cout << arg;
                    }
                }, val);
            
                std::cout << std::endl;
                break;
            }

                case OpCode::OP_HALT:
                    return true;
            }
        }
        return true;
    }
};

#endif