#include <Joan.h>

#define MAX_OBJECT_ARGS 50

struct Jn_CModule {
    char* var_name;
    char* doc;
    Jn_CFunction fn;
};

struct JN_Args
{
    JnObject* args[MAX_OBJECT_ARGS];
    char** arg_names; // default to NULL
    size_t count;
};

static JnObject* native_len(JN_Args args)
{
    if (args.count > 1)
        return NULL;// return JN_RAISE_EXCPETION(TYPE_ERROR, "Expected an 1 argument but got %d.", args->count);
}

static JnObject* native_gets(JN_Args args)
{

}

static JnObject* native_put(JN_Args args)
{

}

static JnObject* native_sleep(JN_Args args)
{

}

JN_API void Jn_register(const char* name, const char* doc, Jn_CFunction fn)
{
     // TODO    
}

JN_API void Jn_register_module(char* name, Jn_CModule* module)
{
    // TODO
}

JN_API JnObject* Jn_call_fn(char* fn_name, JN_Args* args)
{

}

JN_API void Jn_load_Cfunctions(void)
{

}