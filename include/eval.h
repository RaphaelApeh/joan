#ifndef JOAN_EVAL_H
#define JOAN_EVAL_H
#include "object.h"

typedef enum{
    EVAL_ADD,
    EVAL_SUB,
    EVAL_MUL,
    EVAL_DIV,
    EVAL_LSHIFT,
    EVAL_RSHIFT,
    EVAL_POW, // **
    EVAL_GTE, // >=
    EVAL_GT, // >
    EVAL_LT, // <
    EVAL_LTE, // <=
    EVAL_PERC, // %
    EVAL_BAND, // &
    EVAL_BOR, // |
    EVAL_BAC, // ^
    EVAL_IN,
    EVAL_NOT_IN, // not in
    EVAL_OR,
    EVAL_AND,
    EVAL_IS,
    EVAL_EQUAL,
    EVAL_NOTEQUAL,
} BinaryOp;

JnObject* eval_binary(Jn_State*, JnObject* lhs, JnObject* rhs, BinaryOp op);

#endif //JOAN_EVAL_H