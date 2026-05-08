#include <cstdint>
#define uint uint32_t

enum OpCode : uint {
    OP_PUSH,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,

    OP_STORE,
    OP_LOAD,

    OP_JMP,
    OP_JMP_IF_FALSE,

    OP_HALT,
    // print will be deprecated later, and added in the API
    OP_PRINT
};