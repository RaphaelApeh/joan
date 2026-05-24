#ifndef HELPER_H

#define HELPER_H
#include <stdint.h>
#include "arena.h"

typedef struct AST AST;
typedef uint64_t u64;

typedef struct param_o
{
    char* ident;
    AST* ast;
} param_o;

typedef struct param_t{
    param_o* params;
    u64 count;
    u64 capacity;
} param_t;

typedef struct case_o{
    AST* pattern;
    AST* block;
} case_o;

typedef struct case_t{
    case_o* cases;
    u64 capacity;
    u64 count;
} case_t;

typedef struct{
    AST* cond;
    AST* stmt;
} elif_node;

typedef struct elseif{
    elif_node* children;
    size_t capacity;
    size_t count;
}elseif;

typedef struct attr_o{
    char* key;
    AST* value;
} attr_o;

typedef struct attr_t{
    attr_o* items;
    u64 count;
    u64 capacity;
}attr_t;

typedef struct klass_o{
    const char* name;
    attr_t* attrs;
} klass_o;

typedef struct klass_t{
    klass_o* cls;
    u64 count;
    u64 capacity;
} klass_t;

void runtime_error(char* msg, ...);
void call_add_pos(AST* call, AST* arg);
param_t* param_init(void);
case_t* init_case(Arena* arena);
void push_case(case_t* caseObj, AST* sub, AST* block);

void param_add(param_t* param, const char* ident, AST* value);

elseif* elseif_init(void);
void elseif_add(elseif* elif, AST* block, AST* cond);
#endif