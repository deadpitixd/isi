#include <cstdint>
#define uint uint8_t

enum OpCode : uint {
    OP_PUSH,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,

    OP_STORE, // 5
    OP_LOAD,

    OP_JMP, // 7
    OP_JMP_IF_FALSE,

    OP_EQUALS, // 9
    OP_NOT_EQUALS,
    OP_GREATER,
    OP_LESS,

    OP_HALT, // 13
    // print will be deprecated later, and added in the API
    OP_PRINT
};