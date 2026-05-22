#ifndef CHUCK_H
#define CHUCK_H
#include <stdint.h>
#include "object.h"

typedef struct Chuck Chuck;

typedef enum{
    OP_CONSTANT,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_NOT,
    OP_IN,
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

    OP_NEGATE,

    OP_POP,

    OP_DEFINE_GLOBAL,
    OP_GET_GLOBAL,
    OP_SET_GLOBAL,
    OP_SET_INDEX,
    OP_CALL,

    OP_JUMP,
    OP_JUMP_IF_FALSE,
    OP_LOOP,

    OP_SCOPE_ENTER,
    OP_SCOPE_EXIT,

    OP_ITER,
    OP_ARRAY,
    OP_INDEX,
    OP_RANGE,

    OP_REASSIGN,
    OP_FUNCTION,
    OP_BLOCK,

    OP_PRINTLN,
    OP_RETURN,
    OP_BREAK,
    OP_CONTINUE,

    OP_ERROR_MSG,
    OP_ERROR,
}OPCode;

typedef struct Chuck{
    uint8_t* code;
    size_t count;
    size_t capacity;

    Object** constants;
    int constants_count;
    int constants_capacity;

    char** idents;
    size_t ident_count;
    size_t ident_capacity;

    env_t* env;
} Chuck;


void chuck_init(Chuck* chuck);

// Helper functions

int add_ident(Chuck* chuck, char* ident);

void write_chuck(Chuck* chuck, uint8_t byte);

int add_constant(Chuck* chuck, Object* object);

int current_offset(Chuck* chuck);

int emit_jump(Chuck* chuck, uint8_t instrction);

void patch_jump(Chuck* chuck, int offset);

void emit_loop(Chuck* chuck, int loop_start);

#endif