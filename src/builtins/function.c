#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "Joan.h"
#include "builtins/function.h"

static JnObject* builtin_len(JnObject** argv, size_t argc);
static JnObject* builtin_gets(JnObject** argv, size_t argc);
static JnObject* builtin_put(JnObject** argv, size_t argc);
static JnObject* builtin_id(JnObject** argv, size_t argc);

NativeFunction builtin_functions[] = {
    {.name = "len", .func = builtin_len},
    {.name = "gets", .func = builtin_gets},
    {.name = "put", .func = builtin_put},
    {.name = "id", .func = builtin_id}
};

static JnObject* builtin_len(JnObject** argv, size_t argc)
{
    if ((int)argc > 1 || (int)argc < 1)
        return NULL;
    if (argv[0]->type != STR_TYPE && argv[0]->type != ARRAY_TYPE && argv[0]->type != ITER_TYPE)
        return NULL;
    if (argv[0]->type == STR_TYPE)
        return jn_obj_int(argv[0]->str->len);
    return jn_obj_int(argv[0]->arr->size);
}

static JnObject* builtin_gets(JnObject** argv, size_t argc)
{
    if (argc > 1 || argc < 1)
        return NULL;
    if (argv[0]->type != STR_TYPE)
        return NULL;
    fprintf(stderr, "%s", argv[0]->str->chars);
    char buf[1024] = {0};
    char* str = fgets(buf, 1024, stdin);
    str[strlen(str) - 1] = '\0'; // remove "\n" char
    return jn_obj_string(str);
}

static JnObject* builtin_put(JnObject** argv, size_t argc)
{
    if (argc > 1 || argc < 1)
        return NULL;
    print_JnObject(argv[0]);
    return JN_RETURN_NONE; // must return something
}

static JnObject* builtin_id(JnObject** argv, size_t argc)
{
    if (argc > 1 || argc < 1)
        return NULL;
    uintptr_t ptr = (uintptr_t)argv[0];
    return jn_intern_obj(jn_obj_int((long)ptr));
}


JN_API void Jn_load_Cfunctions(void)
{
    int count = (int) sizeof(builtin_functions) / sizeof(builtin_functions[0]);
    JnObject* obj;
    for(int i = 0; i < count; i++)
    {
        NativeFunction fn = builtin_functions[i];
        obj = jn_obj_new(NATIVE_TYPE);
        obj->native_fn = malloc(sizeof(JnNativeObject));
        obj->native_fn->fnName = fn.name;
        obj->native_fn->fn = fn.func;
        // set_env(env, fn.name, obj, true, false);
    }
}