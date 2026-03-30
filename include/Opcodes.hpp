#ifndef OPCODES_HPP
#define OPCODES_HPP
#define uint uint32_t
#include <cstdint>

enum class OpCode : uint
{
    OP_CONSTANT,      // [Op, Index] - Push value from constants to stack
    OP_POP,           // [Op] - Remove top value from stack

    OP_GET_GLOBAL,    // [Op, Index] - Read from globals array
    OP_SET_GLOBAL,    // [Op, Index] - Write to globals array
    OP_GET_LOCAL,     // [Op, Index] - Read from stack frame (relative)
    OP_SET_LOCAL,     // [Op, Index] - Write to stack frame (relative)
    
    OP_ADD, OP_SUB, OP_MUL, OP_DIV,

    OP_JUMP,          // [Op, Offset] - Unconditional jump
    OP_JUMP_IF_FALSE, // [Op, Offset] - Jump if stack top is falsy
    
    OP_CALL,          // [Op, ArgCount] - Call ISI function (Address on stack)
    OP_CALL_NATIVE,   // [Op, NativeID, ArgCount] - Call C++ function
    OP_RETURN,        // [Op] - Return from function

    OP_PRINT,         // [Op] - Specialized cout for debugging
    OP_HALT,           // [Op] - Stop execution
};

#endif