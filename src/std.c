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


#ifndef C_STRING_H
#include "optionals/c_string.h"
#endif


#define MAX_OBJECT_ARGS 50


struct JnObjectMethod {
    char* fn_name;
    JN_CMethod method;
};

static JnObject* push_method(JnObject* self, JnObject* args);

// Hashmap methods
static JnObject* hashmap_from_idx(JnObject* self, JnObject* args);

// String methods
static JnObject* string_ends(JnObject* self, JnObject* arg);
static JnObject* string_starts(JnObject* self, JnObject* arg);

static struct JnObjectMethod STRING_METHODS[] = {
    {"ends", string_ends},
    {"starts", string_starts},
    {NULL, NULL}
};


static struct JnObjectMethod HASHMAP_METHODS[] = {
    {"push", push_method},
    {"from_index", hashmap_from_idx},
    {NULL, NULL}
};

static struct JnObjectMethod ARRAY_METHODS[] = {
    {"push", push_method},
    {NULL, NULL}
};


JN_API JN_CMethod call_method(JnObject* obj, const char* method_name)
{
    assert(obj != NULL);
    struct JnObjectMethod* METHODS = NULL;
    switch (JN_OBJ_TYPE(obj))
    {
        case STR_TYPE:
            METHODS = STRING_METHODS;
            break;
        case ARRAY_TYPE:
            METHODS = ARRAY_METHODS;
            break;
        case HASHMAP_TYPE:
            METHODS = HASHMAP_METHODS;
            break;
        default:
            return NULL;
    }
    assert(METHODS != NULL);

    for (int i = 0;; ++i)
    {
        if (
            METHODS[i].fn_name == NULL ||
            METHODS[i].method == NULL
        ) break;
        if (memcmp(method_name, (&METHODS[i])->fn_name, strlen(method_name)) == 0)
            return METHODS[i].method;
    }
    return NULL;
}


static JnObject* push_method(JnObject* self, JnObject* args)
{
    if (!JN_IS_ITERABLE(self))
        return JN_RAISE_EXCPETION(
            UNDEFINE_ERROR, 
            "push() method does not support '%s'.", 
            JN_OBJ_TO_STRING(self)
        );
    if (JN_ARGS_COUNT(args) != 1)
        return JN_RAISE_EXCPETION(TYPE_ERROR, "obj.push() accept only one argumemt but got (%s).", JN_ARGS_COUNT(args));
    
    JnObject* obj = JN_GET_ARG(args);
    switch (JN_OBJ_TYPE(self))
    {
        case ARRAY_TYPE:
            JN_SET_ARRAY(JN_AS_ARRAY(self), obj, JN_AS_ARRAY(self)->size);
            break;
        case STR_TYPE:
            assert(false && "Not Implemented.");
            // char* str = strstr(JN_AS_CSTRING(self), JN_AS_CSTRING(obj));
            break;
        case HASHMAP_TYPE:
            if (JN_OBJ_TYPE(obj) != HASHMAP_TYPE)
                return JN_RAISE_EXCPETION(TYPE_ERROR, "<Hashmap>.push() expected a hashmap object.");
            Jn_Hashmap* map =  JN_AS_HASHMAP(obj);
            // TODO: think of a better way to implement it.
            for (int i = 0; i < map->size; ++i)
            {
                JN_HASHMAP_INSERT(
                    JN_AS_HASHMAP(self), 
                    map->buckets[i].key, 
                    map->buckets[i].value,
                    JN_AS_HASHMAP(self)->size + i
                );
            }
            break;
        default:
            return JN_RAISE_EXCPETION(TYPE_ERROR, "Invalid stuff.");
    }
    return JN_RETURN_NONE;
}


// Hashmap Methods

static JnObject* hashmap_from_idx(JnObject* self, JnObject* args)
{
    int count = JN_ARGS_COUNT(args);
    if (count != 1)
        return JN_RAISE_EXCPETION(TYPE_ERROR, "from_index() expected one argument but (got %d).", count);
    if (JN_OBJ_TYPE(JN_GET_ARG(args)) != INT_TYPE)
        return JN_RAISE_EXCPETION(TYPE_ERROR, "from_index() expect an integer.");
    int index = JN_AS_INT(JN_GET_ARG(args));
    JnObject* obj = Jnhashmap_get_from_index(JN_AS_HASHMAP(self), index);
    if (NULL == obj)
        return JN_RAISE_EXCPETION(SYS_ERROR, "from_index(): internal error.");
    return obj;
}

// String Methods

static JnObject* string_ends(JnObject* self, JnObject* args)
{
    int count = JN_ARGS_COUNT(args);
    if (!JN_IS_STRING(self))
        return JN_RAISE_EXCPETION(TYPE_ERROR, "object is not of type 'string'.");
    if (count != 1)
        return JN_RAISE_EXCPETION(TYPE_ERROR, "string.ends() expected 1 argument (got %d).", count);
    JnObject* suf_object = JN_GET_ARG(args);

    bool ends = strends(
        JN_AS_CSTRING(self),
        JN_AS_CSTRING(suf_object)
    );
    return JN_RETURN_BOOL(ends);
}


static JnObject* string_starts(JnObject* self, JnObject* arg)
{
    int count = JN_ARGS_COUNT(arg);
    if (!JN_IS_STRING(self))
        return JN_RAISE_EXCPETION(TYPE_ERROR, "object is not of type 'string'.");
    if (count != 1)
        return JN_RAISE_EXCPETION(TYPE_ERROR, "string.starts() expected 1 argument (got %d).", count);
    JnObject* suf_object = JN_GET_ARG(arg);

    bool ends = strstarts(
        JN_AS_CSTRING(self),
        JN_AS_CSTRING(suf_object)
    );
    return JN_RETURN_BOOL(ends);
}

static JnObject* native_len(JnObject* args)
{
    int count = JN_ARGS_COUNT(args);
    if (count > 1)
        return JN_RAISE_EXCPETION(TYPE_ERROR, "len() expected an 1 argument but got %d.", count);
    if (!JN_IS_ITERABLE(JN_GET_ARG(args)))
        return JN_RAISE_EXCPETION(TYPE_ERROR, "len() expect an iterable type.");
    int len = 0;
    JnObject* len_obj = JN_GET_ARG(args);
    switch(JN_OBJ_TYPE(len_obj))
    {
        case ARRAY_TYPE:
            len = (int)JN_AS_ARRAY(len_obj)->size;
            return JN_RETURN_INT(len);
        case STR_TYPE:
            len = JN_AS_STRING(len_obj)->len;
            return JN_RETURN_INT(len);
        case RANGE_TYPE:
            len = range_len(JN_AS_RANGE(len_obj));
            return JN_RETURN_INT(len);
        case HASHMAP_TYPE:
            len = (int)(JN_AS_HASHMAP(len_obj)->size);
            return JN_RETURN_INT(len);
    }
    return JN_RAISE_EXCPETION(NOT_IMPLEMENT_ERROR, "len() does not support this type at the moment.");
}

static JnObject* native_gets(JnObject* args)
{
    int count = JN_ARGS_COUNT(args);
    if (count > 1 || count < 1)
        return JN_RAISE_EXCPETION(TYPE_ERROR, "gets() require one argument but got %d.", count);
    JnObject* obj = JN_GET_ARG(args);
    if (!JN_IS_STRING(obj))
        return JN_RAISE_EXCPETION(TYPE_ERROR, "gets() expects a string but got TODO");
    fprintf(stderr, "%s", JN_AS_CSTRING(obj));
    char c;
    char* buff = malloc(sizeof(char) * 100);
    int len = 0, cap = 100;
    while ((c = getc(stdin)) != '\n')
    {   
        if (len > cap)
        {
            cap *= 2;
            buff = realloc(buff, sizeof(char) * cap);
        }
        buff[len++] = c;
    }
    buff[len] = '\0';
    return JN_RETURN_STRING(buff);
}

static JnObject* native_put(JnObject* args)
{
    int count = JN_ARGS_COUNT(args);
    if (count > 1 || count < 1)
        return JN_RAISE_EXCPETION(TYPE_ERROR, "put() require only one argument but got %d.", count);
    print_JnObject(JN_GET_ARG(args));
    return JN_RETURN_NONE;
}

static JnObject* native_toint(JnObject* args)
{
    int count = JN_ARGS_COUNT(args);
    if (count > 1 || count < 1)
        return JN_RAISE_EXCPETION(TYPE_ERROR, "toint() require only one argument but got %d.", count);
    JnObject* obj = JN_GET_ARG(args);
    switch(JN_OBJ_TYPE(obj))
    {
        case STR_TYPE:
            return JN_RETURN_INT(strtol(JN_AS_CSTRING(obj), NULL, 10));
        case INT_TYPE:
            return obj;
        case FLOAT_TYPE:
            return JN_RETURN_INT((long)JN_AS_FLOAT(obj));
        case CHAR_TYPE:
            return JN_RETURN_INT((unsigned int)JN_AS_CHAR(obj));
        case BOOL_TYPE:
            return JN_RETURN_INT(JN_AS_BOOL(obj));
        default:
            return JN_RAISE_EXCPETION(TYPE_ERROR, "toint() does not support this type 'TODO'. ");
    }
    return NULL; // ERROR
}

static JnObject* native_tofloat(JnObject* args)
{
    int count = JN_ARGS_COUNT(args);
    if (count > 1 || count < 1)
        return JN_RAISE_EXCPETION(TYPE_ERROR, "tofloat() require only one argument but got %d.", count);
    JnObject* obj = JN_GET_ARG(args);
    switch(JN_OBJ_TYPE(obj))
    {
        case STR_TYPE:
            return JN_RETURN_FLOAT(strtod(JN_AS_CSTRING(obj), NULL));
        case INT_TYPE:
            return JN_RETURN_FLOAT((double)JN_AS_INT(obj));
        case FLOAT_TYPE:
            return obj;
        case BOOL_TYPE:
            return JN_RETURN_FLOAT(JN_AS_BOOL(obj));
        default:
            return JN_RAISE_EXCPETION(TYPE_ERROR, "tofloat() does not support this type 'TODO'. ");
    }
    return NULL;
}

static JnObject* native_tochar(JnObject* args)
{
    int count = JN_ARGS_COUNT(args);
    if (count > 1 || count < 1)
        return JN_RAISE_EXCPETION(TYPE_ERROR, "tochar() require only one argument but got %d.", count);
    JnObject* obj = JN_GET_ARG(args);
    switch(JN_OBJ_TYPE(obj))
    {
        case INT_TYPE:
            return JN_RETURN_CHAR((char) JN_AS_INT(obj));
        case CHAR_TYPE:
            return obj;
        case FLOAT_TYPE:
            return JN_RETURN_CHAR((char) JN_AS_FLOAT(obj));
        default:
            return JN_RAISE_EXCPETION(TYPE_ERROR, "tochar() does not support this type 'TODO'. ");
    }
    return NULL;
}

static JnObject* native_isinstance(JnObject* arg)
{
    // Example:
    // isinstance("Hello", string) // true
    // isinstance("World", bool) // false
    
}

static JnObject* native_sleep(JnObject* args)
{
    int count = JN_ARGS_COUNT(args);
    if (count > 1 || count < 1)
        return JN_RAISE_EXCPETION(TYPE_ERROR, "sleep() require only one argument but got %d.", count);
    if (!JN_AS_INT(JN_GET_ARG(args)))
        return JN_RAISE_EXCPETION(TYPE_ERROR, "sleep() takes an int type but got TODO.");
    
    #ifdef _WIN32
        sleep(JN_AS_INT(JN_GET_ARG(args)) * 1000);
    #else
        sleep(JN_AS_INT(JN_GET_ARG(args)));
    #endif
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

    // type
    Jn_register(state, "int", NULL, JN_RETURN_TYPE_OBJECT("int", INT_TYPE));
    Jn_register(state, "string", NULL, JN_RETURN_TYPE_OBJECT("string", STR_TYPE));
    Jn_register(state, "float", NULL, JN_RETURN_TYPE_OBJECT("float", FLOAT_TYPE));
    Jn_register(state, "bool", NULL, JN_RETURN_TYPE_OBJECT("bool", BOOL_TYPE));
    Jn_register(state, "char", NULL, JN_RETURN_TYPE_OBJECT("char", CHAR_TYPE));

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