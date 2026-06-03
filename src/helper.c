#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include "object.h"
#include "helper.h"
#include "ast.h"



unsigned long djb2_hash(unsigned char* str)
{
    int c;
    unsigned long hash = 5281;
    while (c = *str++)
        hash = ((hash << 5) + hash) + c;
    return hash;
}

unsigned long fnv_hash(const void* key, uint32_t h)
{
    h ^= 2166136261UL;
    const uint8_t* d = (const uint8_t*)key;
    for (int i = 0; d[i] != '\0'; ++i)
    {
        h ^= d[i];
        h *= 16777619;
    }
    return h;
}

bool isnumber(JnObject* obj)
{
    if (NULL == obj) return false;
    if (obj->type == INT_TYPE || obj->type == FLOAT_TYPE)
        return true;
    return false;
}

double tonumber(JnObject* obj)
{
    if (obj->type == INT_TYPE)
        return (double)obj->int32;
    return obj->float32;
}

void runtime_error(char* msg, ...)
{
    va_list arg;
    va_start(arg, msg);
    vfprintf(stderr, msg, arg);
    va_end(arg);
    exit(72);
}


case_t* init_case(Arena* arena)
{
    case_t* caseObj = arena_alloc(arena, sizeof(case_t));
    caseObj->count = 0;
    caseObj->capacity = 100;
    caseObj->cases = arena_alloc(
        arena, sizeof(case_o) * caseObj->capacity
    );
    return caseObj;
}

void push_case(case_t* caseObj, AST* sub, AST* block)
{
    if (NULL == caseObj) return;
    if (caseObj->count >= caseObj->capacity)
    {
        caseObj->capacity *= 2;
        caseObj->cases = realloc(caseObj->cases, sizeof(case_o) * caseObj->capacity);
    }
    caseObj->cases[caseObj->count++] = (case_o){.pattern = sub, .block = block};
}

void call_add_pos(AST* call, AST* arg)
{
    call->call.pos_args[call->call.pos_count++] = arg;
}

param_t* param_init(void)
{
    param_t* param = malloc(sizeof(param));
    param->count = 0;
    param->capacity = 10;
    param->params = malloc(sizeof(param_o) * 10);
    return param;
}

void param_add(param_t* param, const char* ident, AST* value)
{
    if (NULL == param)
        perror("param is NULL");
    if (param->count >= param->capacity)
    {
        param->capacity *= 2;
        param->params = realloc(param->params, sizeof(param_o) * param->capacity);
    }
    param->params[param->count++] = (param_o){
        .ident = (char *)ident, .ast = value
    };
}

elseif* elseif_init(void)
{
    elseif* elif = malloc(sizeof(elseif));
    if (elif == NULL)
        perror("Memory allocation failed.");
    elif->count = 0;
    elif->capacity = 10;
    elif->children = malloc(
        sizeof(elif_node) * elif->capacity
    );
    return elif;
}

void elseif_add(elseif* elif, AST* block, AST* cond)
{
    if (elif->count >= elif->capacity)
    {
        elif->capacity *= 2;
        elif->children = realloc(
            elif->children,
            sizeof(elif_node) * elif->capacity
        );
    }
    elif->children[elif->count++] = (elif_node){
        .cond = cond,
        .stmt = block,
    };
}