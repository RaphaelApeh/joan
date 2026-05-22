#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "builtins/function.h"

static Object* builtin_len(Object** argv, size_t argc);
static Object* builtin_gets(Object** argv, size_t argc);
static Object* builtin_put(Object** argv, size_t argc);

NativeFunction builtin_functions[] = {
    {.name = "len", .func = builtin_len},
    {.name = "gets", .func = builtin_gets},
    {.name = "put", .func = builtin_put}
};

static Object* builtin_len(Object** argv, size_t argc)
{
    if ((int)argc > 1 || (int)argc < 1)
        return NULL;
    if (argv[0]->kind != STR_TYPE && argv[0]->kind != ARRAY_TYPE && argv[0]->kind != ITER_TYPE)
        return NULL;
    if (argv[0]->kind == STR_TYPE)
        return obj_int(strlen(argv[0]->o_string));
    else if (argv[0]->kind == ITER_TYPE)
        return obj_int(argv[0]->iter->count);
    return obj_int(argv[0]->o_array->count);
}

static Object* builtin_gets(Object** argv, size_t argc)
{
    if (argc > 1 || argc < 1)
        return NULL;
    if (argv[0]->kind != STR_TYPE)
        return NULL;
    fprintf(stderr, "%s", argv[0]->o_string);
    char buf[1024] = {0};
    char* str = fgets(buf, 1024, stdin);
    str[strlen(str) - 1] = '\0'; // remove "\n" char
    return obj_string(str);
}

static Object* builtin_put(Object** argv, size_t argc)
{
    if (argc > 1 || argc < 1)
        return NULL;
    print_object(argv[0]);
    return obj_none(); // must return something
}

void set_functions(env_t* env)
{
    int count = (int) sizeof(builtin_functions) / sizeof(builtin_functions[0]);
    Object* obj;
    for(int i = 0; i < count; i++)
    {
        NativeFunction fn = builtin_functions[i];
        obj = obj_new(NATIVE_TYPE);
        obj->o_nativefn = malloc(sizeof(NativeObject));
        obj->o_nativefn->fnName = fn.name;
        obj->o_nativefn->fn = fn.func;
        set_env(env, fn.name, obj, true, false);
    }
}