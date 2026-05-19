#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include "helper.h"
#include "ast.h"


void runtime_error(char* msg, ...)
{
    va_list arg;
    va_start(arg, msg);
    vfprintf(stderr, msg, arg);
    va_end(arg);
    exit(72);
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