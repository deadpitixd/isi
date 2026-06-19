#include <cstdint>
#define uint uint8_t

enum OpCode : uint
{
    OP_PUSH,
    OP_IMPORT,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,

    OP_INDEX,

    OP_STORE, // 6
    OP_LOAD,

    OP_JMP, // 8
    OP_JMP_IF_FALSE,

    OP_EQUALS, // 10
    OP_NOT_EQUALS,
    OP_GREATER,
    OP_LESS,

    OP_CALL, // 14
    OP_LOAD_LOCAL,
    OP_STORE_LOCAL,
    OP_RETURN,

    OP_GET_MEMBER,

    OP_NOT, // 18 the ! symbol, which basically inverts 1 -> 0, 0 -> 1

    OP_HALT, // 19
    OP_PRINT,
    // Throw works like throw, i'll add catch later
    OP_THROW
};