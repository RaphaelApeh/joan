#include <assert.h>
#include <stdlib.h>
#include <Joan.h>
#include "object.h"

#ifdef _WIN32
#include <windows.h>
#define sleep Sleep

#elif
#include <unistd.h>
#endif

#define MAX_OBJECT_ARGS 50

struct Jn_CModule {
    char* var_name;
    char* doc;
    Jn_CFunction fn;
};

JN_API JN_Args Jn_make_arg(JnObject** objects, size_t count)
{
    struct JN_Args arg;
    arg.arg_names = NULL; // TODO
    arg.args = objects;
    arg.count = count;
    return arg;
}

static JnObject* native_len(JN_Args args)
{
    if (args.count > 1)
        return JN_RAISE_EXCPETION(TYPE_ERROR, "len() expected an 1 argument but got %d.", args.count);
    if (!JN_IS_ITERABLE(args.args[0]))
        return JN_RAISE_EXCPETION(TYPE_ERROR, "len() expect an iterable type.");
    int len = 0;
    switch(args.args[0]->type)
    {
        case ARRAY_TYPE:
            len = (int)args.args[0]->arr->size;
            return JN_RETURN_INT(len);
        case STR_TYPE:
            len = args.args[0]->str->len;
            return JN_RETURN_INT(len);
        case RANGE_TYPE:
            len = range_len(&args.args[0]->range);
            return JN_RETURN_INT(len);
        case HASHMAP_TYPE:
            len = (int)args.args[0]->hashmap->size;
            return JN_RETURN_INT(len);
    }
    return JN_RAISE_EXCPETION(NOT_IMPLEMENT_ERROR, "len() does not support this type at the moment.");
}

static JnObject* native_gets(JN_Args args)
{
    if (args.count > 1 || args.count < 1)
        return JN_RAISE_EXCPETION(TYPE_ERROR, "gets() require one argument but got %d.", args.count);
    if (!JN_IS_STRING(args.args[0]))
        return JN_RAISE_EXCPETION(TYPE_ERROR, "gets() expects a string but got TODO");
    fprintf(stderr, "%s", JN_AS_STRING(args.args[0])->chars);
    char buf[1024] = {0};
    char* str = fgets(buf, 1024, stdin);
    str[strlen(str) - 1] = '\0'; // remove "\n" char
    return JN_RETURN_STRING(str);
}

static JnObject* native_put(JN_Args args)
{
    if (args.count > 1 || args.count < 1)
        return JN_RAISE_EXCPETION(TYPE_ERROR, "put() require only one argument but got %d.", args.count);
    print_JnObject(args.args[0]);
    return JN_RETURN_NONE;
}

static JnObject* native_toint(JN_Args args)
{
    if (args.count > 1 || args.count < 1)
        return JN_RAISE_EXCPETION(TYPE_ERROR, "toint() require only one argument but got %d.", args.count);
    switch(args.args[0]->type)
    {
        case STR_TYPE:
            return JN_RETURN_INT(strtol(JN_AS_STRING(args.args[0])->chars, NULL, 10));
        case INT_TYPE:
            return args.args[0];
        case FLOAT_TYPE:
            return JN_RETURN_INT((long)JN_AS_FLOAT(args.args[0]));
        case BOOL_TYPE:
            return JN_RETURN_INT(args.args[0]->bool8);
        default:
            return JN_RAISE_EXCPETION(TYPE_ERROR, "toint() does not support this type 'TODO'. ");
    }
    return NULL; // ERROR
}

static JnObject* native_tofloat(JN_Args arg)
{
    if (arg.count > 1 || arg.count < 1)
        return JN_RAISE_EXCPETION(TYPE_ERROR, "tofloat() require only one argument but got %d.", arg.count);
    switch(arg.args[0]->type)
    {
        case STR_TYPE:
            return JN_RETURN_FLOAT(strtod(JN_AS_STRING(arg.args[0])->chars, NULL));
        case INT_TYPE:
            return JN_RETURN_FLOAT((double)JN_AS_INT(arg.args[0]));
        case FLOAT_TYPE:
            return arg.args[0];
        case BOOL_TYPE:
            return JN_RETURN_FLOAT(JN_AS_BOOL(arg.args[0]));
        default:
            return JN_RAISE_EXCPETION(TYPE_ERROR, "tofloat() does not support this type 'TODO'. ");
    }
    return NULL;
}

static JnObject* native_tochar(JN_Args arg)
{
    if (arg.count > 1 || arg.count < 1)
        return JN_RAISE_EXCPETION(TYPE_ERROR, "tochar() require only one argument but got %d.", arg.count);
    switch(arg.args[0]->type)
    {
        case INT_TYPE:
            return JN_RETURN_CHAR((char) JN_AS_INT(arg.args[0]));
        case CHAR_TYPE:
            return arg.args[0];
        case FLOAT_TYPE:
            return JN_RETURN_CHAR((char) JN_AS_FLOAT(arg.args[0]));
        default:
            return JN_RAISE_EXCPETION(TYPE_ERROR, "tochar() does not support this type '%d'. ", arg.args[0]->type);
    }
    return NULL;
}

static JnObject* native_sleep(JN_Args args)
{
    if (args.count > 1 || args.count < 1)
        return JN_RAISE_EXCPETION(TYPE_ERROR, "sleep() require only one argument but got %d.", args.count);
    if (!JN_AS_INT(args.args[0]))
        return JN_RAISE_EXCPETION(TYPE_ERROR, "sleep() takes an int type but got TODO.");
    
    sleep(JN_AS_INT(args.args[0]));
    return JN_RETURN_NONE;
}


JN_API void Jn_register_fn(J_State* state, char* name, char* doc, Jn_CFunction fn)
{
    assert(state != NULL && name != NULL);
    JnNativeObject* n_fn = JN_ALLOC(sizeof(JnNativeObject));
    n_fn->fn = fn;
    n_fn->fnName = strdup(name);
    JnObject* obj = JN_OBJECT(NATIVE_TYPE);
    obj->native_fn = n_fn;
    Jn_register(state, name, doc, obj);
}

JN_API void Jn_register(J_State* state, const char* name, const char* doc, JnObject* obj)
{
     assert(state != NULL && obj != NULL);
     obj->doc = doc;
     environ_insert(state->globals, (char* )name, obj);
}

JN_API void Jn_register_module(char* name, Jn_CModule* module)
{
    // TODO
}

JN_API JnObject* Jn_call_fn(char* fn_name, JN_Args* args)
{

}

JN_API void Jn_load_Cfunctions(J_State* state)
{
    bool win, apple, linux = false;
    #ifdef _WIN32
    win = true;
    #elif defined(__APPLE__) && defined(__MACH__)
    apple = true;
    #elif defined(__linux__)
    linux = true;
    #endif
    char* filename = state->cxt.source.filename ? (char *)state->cxt.source.filename : "main";

    Jn_register(state, "__WINDOWS__", "Check if it is a Windows system.", JN_RETURN_BOOL(win));
    Jn_register(state, "__APPLE__", "Check if it is a Mac system.", JN_RETURN_BOOL(apple));
    Jn_register(state, "__LINUX__", "Check if it is a Linux system.", JN_RETURN_BOOL(linux));
    Jn_register(state, "__FILE__", "Returns the filename or main in repl.", JN_RETURN_STRING(filename));
    Jn_register_fn(state, "len", "Returns the length of an iterable", native_len);
    Jn_register_fn(state, "gets", "Get user input.", native_gets);
    Jn_register_fn(state, "put", "print object without a new-line.", native_put);
    Jn_register_fn(state, "toint", "Convert an object to int.", native_toint);
    Jn_register_fn(state, "tofloat", "Convert an object to float.", native_tofloat);
    Jn_register_fn(state, "tochar", "Convert an object to char.", native_tochar);
    Jn_register_fn(state, "sleep", "Sleep program", native_sleep);
    // add other built-in functions
}