#ifndef HELPER_H

#define HELPER_H
#include <stdint.h>
#include "arena.h"

typedef struct AST AST;
typedef uint64_t u64;
typedef struct JnObject JnObject;
typedef struct J_DArray_Obj J_DArray_Obj;

#define RESIZE_DOBJ(arr) (arr)->items = realloc((arr)->items, sizeof(*(arr)->items) * (arr)->capacity)

#define PUSH_ITEM(arr, obj) do {\
    if ((arr)->size >= (arr)->capacity)\
    {                                   \
        (arr)->capacity *= 2;             \
        RESIZE_DOBJ(arr);                   \
    }                                        \
    (arr)->items[(arr)->size++] = (obj);    \
    } while(false)

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

struct J_DArray_Obj {
    void** items;
    size_t size;
    size_t capacity;
};

// Hash functions
unsigned long fnv_hash(const void* key, uint32_t h);
unsigned long djb2_hash(unsigned char* str);

bool isnumber(JnObject* obj);
double tonumber(JnObject* obj);

void print_source_line(char* source, int line, int column);

void runtime_error(char* msg, ...);
void call_add_pos(AST* call, AST* arg);
param_t* param_init(void);
case_t* init_case(Arena* arena);
void push_case(case_t* caseObj, AST* sub, AST* block);

void param_add(param_t* param, const char* ident, AST* value);

elseif* elseif_init(void);
void elseif_add(elseif* elif, AST* block, AST* cond);
#endif