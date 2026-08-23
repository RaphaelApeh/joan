#ifndef JN_OP_CODE_H
#define JN_OP_CODE_H
#include <stdint.h>
#include "object.h"

typedef struct Chuck Chuck;

typedef enum{
    OP_CONSTANT,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_POW,
    OP_NOT,
    OP_IN,
    OP_NOT_IN,
    OP_IS,
    OP_OR,
    OP_AND,
    OP_EQUAL,
    OP_NEQ,
    OP_GT,
    OP_LT,
    OP_LTE,
    OP_GTE,
    OP_RSHIFT,
    OP_LSHIFT,
    OP_BITOR,
    OP_BITAND,
    OP_PERC,
    OP_BITAC,
    
    OP_TRUE,
    OP_FALSE,
    OP_NONE,
    OP_PLUS_PLUS,
    
    OP_NEGATE,

    OP_TILDE,

    OP_POP,

    OP_DUP,

    OP_MATCH,

    OP_FF,

    OP_DEFINE_GLOBAL,
    OP_GET_GLOBAL,
    OP_SET_GLOBAL,
    OP_CALL,

    OP_ASSIGN,

    OP_IMPORT,

    OP_LEN,
    OP_JUMP,
    OP_JUMP_IF_FALSE,
    OP_LOOP,
    OP_GET_ITER,
    OP_ITER_NEXT,

    OP_SCOPE_ENTER,
    OP_SCOPE_EXIT,

    OP_ITER,
    OP_TUPLE,
    OP_ARRAY,
    OP_INDEX,
    OP_RANGE,
    OP_HM,
    OP_INSTANCE,

    OP_REASSIGN,
    OP_FUNCTION,
    OP_BLOCK,

    OP_PREFIX,
    OP_SUFFIX,

    OP_MEMBER,
    OP_MEMBER_SET,
    OP_RETURN,
    OP_YIELD,
    OP_BREAK,
    OP_CONTINUE,

    OP_EXIT,
    
    OP_END,

    OP_ERROR_MSG,
    OP_ERROR,
} OPCode;

#endif