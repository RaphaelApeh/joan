#include <assert.h>
#include <stdlib.h>
#include <Joan.h>
#include "vm.h"
#include "object.h"

#ifdef JN_WINDOWS

#define sleep Sleep
#endif


#ifndef C_STRING_H
#include "optionals/c_string.h"
#endif


#define MAX_OBJECT_ARGS 50


JN_API JnObject* Jn_make_args(Jn_State* state, size_t capacity)
{
    assert(capacity < MAX_OBJECT_ARGS);
    JnObject** objs = malloc(sizeof(JnObject *) * capacity);
    assert(objs);
    JnObject* obj = jn_obj_new(state, JN_ARG_TYPE);
    obj->arg.args = objs;
    obj->arg.count = 0;
    obj->arg.arg_names = NULL;
    return obj;
}

JN_API void Jn_add_arg(JnObject* args, JnObject* obj)
{
    assert(args && obj);
    args->arg.args[args->arg.count++] = obj;
}

struct JnObjectMethod {
    char* fn_name;
    JN_CMethod method;
};

static JnObject* push_method(Jn_State* state, JnObject* self, JnObject* args);

// Hashmap methods
static JnObject* hashmap_from_idx(Jn_State* state, JnObject* self, JnObject* args);

// String methods
static JnObject* string_ends(Jn_State* state, JnObject* self, JnObject* arg);
static JnObject* string_starts(Jn_State* state, JnObject* self, JnObject* arg);
static JnObject* string_split(Jn_State* state, JnObject* self, JnObject* arg);
static JnObject* string_repl(Jn_State* state, JnObject* self, JnObject* arg);
static JnObject* string_strip(Jn_State* state, JnObject* self, JnObject* arg);
static JnObject* string_part(Jn_State* state, JnObject* self, JnObject* arg);

// Static methods
static JnObject* string_utf8(Jn_State* state, JnObject* cls);

static struct JnObjectMethod STRING_METHODS[] = {
    {"ends", string_ends},
    {"starts", string_starts},
    {"push", push_method},
    {"split", string_split},
    {"repl", string_repl},
    {"strip", string_strip},
    {"part", string_part},
    {NULL, NULL}
};


static JnStaticMethod STRING_STATIC_METHODS[] = {
    {"from_utf8", string_utf8},
    {NULL, NULL}
};


static JnObject* hashmap_remove(Joan* state, JnObject* self, JnObject* args);

static struct JnObjectMethod HASHMAP_METHODS[] = {
    {"push", push_method},
    {"from_index", hashmap_from_idx},
    {"remove", hashmap_remove},
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
        case JN_STRING_TYPE:
            METHODS = STRING_METHODS;
            break;
        case JN_ARRAY_TYPE:
            METHODS = ARRAY_METHODS;
            break;
        case JN_HASHMAP_TYPE:
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


static JnObject* push_method(Jn_State* state, JnObject* self, JnObject* args)
{
    if (!JN_IS_ITERABLE(self))
        return JN_RAISE_EXCPETION(
            state,
            UNDEFINE_ERROR, 
            "push() method does not support '%s'.", 
            JN_OBJ_TO_STRING(self)
        );
    if (JN_ARGS_COUNT(args) != 1)
        return JN_RAISE_EXCPETION(state, TYPE_ERROR, "obj.push() accept only one argumemt but got (%s).", JN_ARGS_COUNT(args));
    
    JnObject* obj = JN_GET_ARG(args);
    switch (JN_OBJ_TYPE(self))
    {
        case JN_ARRAY_TYPE:
            JN_SET_ARRAY(JN_AS_ARRAY(self), obj, JN_AS_ARRAY(self)->size);
            break;
        case JN_STRING_TYPE:
            if (!JN_IS_STRING(obj))
                return JN_RAISE_EXCPETION(state, TYPE_ERROR, "string:push() expected a string.");
            assert(false && "TODO");
            // char* buff = strcat(JN_AS_CSTRING(self), JN_AS_CSTRING(obj));
            // *(self->str) = JNSTR_OBJ(buff); // But it works in my machine.
           break;
        case JN_HASHMAP_TYPE:
            if (JN_OBJ_TYPE(obj) != JN_HASHMAP_TYPE)
                return JN_RAISE_EXCPETION(state, TYPE_ERROR, "<Hashmap>.push() expected a hashmap object.");
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
            return JN_RAISE_EXCPETION(state, TYPE_ERROR, "Invalid stuff.");
    }
    return JN_RETURN_NONE;
}


// Hashmap Methods

static JnObject* hashmap_from_idx(Jn_State* state, JnObject* self, JnObject* args)
{
    int count = JN_ARGS_COUNT(args);
    if (count != 1)
        return JN_RAISE_EXCPETION(state, TYPE_ERROR, "from_index() expected one argument but (got %d).", count);
    if (JN_OBJ_TYPE(JN_GET_ARG(args)) != JN_INT_TYPE)
        return JN_RAISE_EXCPETION(state, TYPE_ERROR, "from_index() expect an integer.");
    int index = JN_AS_INT(JN_GET_ARG(args));
    JnObject* obj = Jnhashmap_get_from_index(JN_AS_HASHMAP(self), index);
    if (NULL == obj)
        return JN_RAISE_EXCPETION(state, SYS_ERROR, "from_index(): internal error.");
    return obj;
}

static JnObject* hashmap_remove(Joan* state, JnObject* self, JnObject* args)
{
    assert(false);
    bool Jnhashmap_remove(Jn_Hashmap* map, JnObject* key);
}
// String Methods

static JnObject* string_ends(Jn_State* state, JnObject* self, JnObject* args)
{
    int count = JN_ARGS_COUNT(args);
    if (!JN_IS_STRING(self))
        return JN_RAISE_EXCPETION(state, TYPE_ERROR, "object is not of type 'string'.");
    if (count != 1)
        return JN_RAISE_EXCPETION(state, TYPE_ERROR, "string.ends() expected 1 argument (got %d).", count);
    JnObject* suf_object = JN_GET_ARG(args);

    bool ends = strends(
        JN_AS_CSTRING(self),
        JN_AS_CSTRING(suf_object)
    );
    return JN_RETURN_BOOL(state, ends);
}


static JnObject* string_starts(Jn_State* state, JnObject* self, JnObject* arg)
{
    int count = JN_ARGS_COUNT(arg);
    if (!JN_IS_STRING(self))
        return JN_RAISE_EXCPETION(state, TYPE_ERROR, "object is not of type 'string'.");
    if (count != 1)
        return JN_RAISE_EXCPETION(state, TYPE_ERROR, "string.starts() expected 1 argument (got %d).", count);
    JnObject* suf_object = JN_GET_ARG(arg);

    bool ends = strstarts(
        JN_AS_CSTRING(self),
        JN_AS_CSTRING(suf_object)
    );
    return JN_RETURN_BOOL(state, ends);
}

static JnObject* string_split(Jn_State* state, JnObject* self, JnObject* arg)
{
    /*
    Example:
        "Josephine,Jane,Joan".split(',');
    */
    if (JN_ARGS_COUNT(arg) != 1)
    {
        return JN_RAISE_EXCPETION(state, TYPE_ERROR, ".split() expected one arguments but got (%d).", JN_ARGS_COUNT(arg));
    }
    JnObject* char_obj = JN_GET_ARG(arg);
    if (!JN_IS_CHAR(char_obj))
        return JN_RAISE_EXCPETION(state, TYPE_ERROR, ".split() expect a char type but got .TODO.");
    char* obj_str = JN_AS_CSTRING(self);
    int size;
    char c = JN_AS_CHAR(char_obj);
    char** items = strsplt(obj_str, c, &size);
    Jn_Array* arr = NULL;
    for (int i = 0; i < size; ++i)
    {
        JN_SET_ARRAY(arr, JN_RETURN_STRING(state, items[i]), i);
    }
    if (arr == NULL)
    {
        JN_ARRAY_DEFAULT(arr);
    }
    assert(arr != NULL);
    // TODO
    JnObject* obj = jn_obj_new(state, JN_ARRAY_TYPE);
    obj->arr = arr;
    return obj;
}

static JnObject* string_repl(Jn_State* state, JnObject* self, JnObject* arg)
{
    /*
    Example:
        "Dan".repl("a", "o");
        Don
    */
    if (JN_ARGS_COUNT(arg) != 2)
    {
        return JN_RAISE_EXCPETION(state, TYPE_ERROR, ".repl() expected two args");
    }

    if (!JN_IS_STRING(JN_GET_ARGS(arg, 0)) || !JN_IS_STRING(JN_GET_ARGS(arg, 1)))
        return JN_RAISE_EXCPETION(state, TYPE_ERROR, ".repl() expected both argument to be a string.");
    
    char* old = JN_AS_CSTRING(JN_GET_ARGS(arg, 0));
    char* new = JN_AS_CSTRING(JN_GET_ARGS(arg, 1));
    char* obj_str = JN_AS_CSTRING(self);

    char* new_str = strrpl(obj_str, old, new);
    return JN_RETURN_STRING(state, new_str);
}


static JnObject* string_strip(Jn_State* state, JnObject* self, JnObject* arg)
{
    /*
    Example:
        "   hello World   ".strip()
        "Hello World"
    */
    if (JN_ARGS_COUNT(arg) != 0)
        return JN_RAISE_EXCPETION(state, TYPE_ERROR, ".strip() accept no arugment.");
    char* obj_str = JN_AS_CSTRING(self);
    char* res = strstrp(obj_str);
    return JN_RETURN_STRING(state, res);
}

static JnObject* string_part(Jn_State* state, JnObject* self, JnObject* arg)
{
    /*
    Example:
        >>"key=value".part('=');
        ("key", "value")
        >>"key=value".part('?');
        ("key-value", None)
    */
    char* str_obj = JN_AS_CSTRING(self);
    if (JN_ARGS_COUNT(arg) != 1)
        return JN_RAISE_EXCPETION(state, TYPE_ERROR, ".part() expected one argument but got (%d).", JN_ARGS_COUNT(arg));
    
    JnObject* char_obj = JN_GET_ARG(arg);
    if (!JN_IS_CHAR(char_obj))
        return JN_RAISE_EXCPETION(state, TYPE_ERROR, ".part() expected a type 'char'.");
    
    char delim = JN_AS_CHAR(char_obj);

    char* left, *right;
    int ret = strpart(str_obj,delim, &left, &right);
    if (ret != 0)
        return JN_RETURN_NONE;

    Jn_Array* arr = NULL;
    if (NULL != left)
    {
        JN_SET_ARRAY(arr, JN_RETURN_STRING(state, left), 0);
    } else {
        JN_SET_ARRAY(arr, JN_RETURN_NONE, 0);        
    }
    if (NULL != right)
    {
        JN_SET_ARRAY(arr, JN_RETURN_STRING(state, right), 1);
    } else {
        JN_SET_ARRAY(arr, JN_RETURN_NONE, 1);
    }
    JnObject* ret_obj = JN_OBJECT(state, JN_TUPLE_TYPE);
    ret_obj->tuple = arr;
    return ret_obj;
}

static JnObject* string_utf8(Jn_State* state, JnObject* cls)
{
    if (!JN_IS_STRING(cls))
        return JN_RAISE_EXCPETION(state, TYPE_ERROR, ":from_utf8() epected a string");
    
    char* str_obj = JN_AS_CSTRING(cls);
    size_t len = strlen_utf8(str_obj);
    // TODO
}


// Native functions
static JnObject* native_getattr(Jn_State* state, JnObject* args)
{
    assert(false);
}


static JnObject* native_format(Jn_State* state, JnObject* args)
{
    /*
    Example:
        name := "John";
        greeting := format("Hello, %s", name);
        printf("GREETING : %s", greeting);
    */
    char* fmt, *typ_str;
    int count = JN_ARGS_COUNT(args);
    if (count < 1)
        return JN_RAISE_EXCPETION(state, TYPE_ERROR, "Expected a least one argument but (got %d).", count);
    
    JnObject* fmt_obj = JN_GET_ARG(args);
    if (!JN_IS_STRING(fmt_obj))
        return JN_RAISE_EXCPETION(
            state, 
            TYPE_ERROR, 
            "First argument must be a string."
        );
    char* str = JN_AS_CSTRING(fmt_obj);
    JnObject* obj;
    char* buff = malloc(sizeof(char) * 100);
    if (!buff) return JN_RETURN_NONE;
    int len = 0, cap = 100;
    int arg_count = 1;

    #define _APPEND_CHAR(ch) do{            \
        if (len + 1 >= cap) {               \
            cap *= 2;                       \
            buff = realloc(buff, cap);      \
        }                                   \
        buff[len++] = (ch);                 \
    }while (false)

    #define _APPEND_STR(str)   do{          \
        const char* __s = (str);            \
        while (*__s)                        \
            _APPEND_CHAR(*__s++);           \
    } while (false)

    while (*str)
    {
        if (*str != '%')
        {
            _APPEND_CHAR(*str++);
            continue;
        }
        str++;
        if (*str == '\0')   break;

        if (*str == '%')
        {
            _APPEND_CHAR('%');
            str++;
            continue;
        }

        if (arg_count >= count)
        {
            free(buff);
            return JN_RAISE_EXCPETION(state, TYPE_ERROR, "format() got too many arguments (%d).", count);
        }
        obj = JN_GET_ARGS(args, arg_count++);
        char tmp[128];
        switch (*str)
        {
            case 's':
            {
                if (!JN_IS_STRING(obj))
                {
                    free(buff);
                    return JN_RAISE_EXCPETION(
                        state,
                        TYPE_ERROR,
                        "%%s expects a string."
                    );
                }
                _APPEND_STR(JN_AS_CSTRING(obj));
            } break;
            case 'i':
            case 'd':
            {
                if (!JN_IS_INT(obj))
                {
                    fmt = "%d";
                    typ_str = "an integer";
                    goto err;
                }
                snprintf(tmp, sizeof(tmp), "%lld", (long long)JN_AS_INT(obj));
                _APPEND_STR(tmp);
            } break;
            case 'f':
            {
                if (!JN_IS_FLOAT(obj))
                {
                    fmt = "%f"; typ_str = "a float";
                    goto err;
                }
                snprintf(tmp, sizeof(tmp), "%g", JN_AS_FLOAT(obj));
                _APPEND_STR(tmp);
            } break;
            case 'c':
            {
                if (!JN_IS_CHAR(obj))
                {
                    fmt = "%c"; typ_str = "a character";
                    goto err;
                }
                _APPEND_CHAR(JN_AS_CHAR(obj));
            } break;
            case 'b':
            {
                char* expr = (JN_TO_BOOL(obj)) ? "true" : "false";
                _APPEND_STR(expr);
            } break;
            case 'v':
            {
                return JN_RAISE_EXCPETION(state, TYPE_ERROR, "format() does not support %%v, only printf() does.");
            }
            case '%': // Just in case.
                _APPEND_CHAR('%');
                break;
            default:
            {
                free(buff);
                return JN_RAISE_EXCPETION(state, TYPE_ERROR, "Unkown format specifier '%%%c'.", *str);
            }
        }    
        str++;    
    }
    buff[len] = '\0';
    JnObject* res = jn_obj_string(state, buff);
    free(buff);
    return res;
    // clean-up
    err:
        free(buff);
        return JN_RAISE_EXCPETION(state, TYPE_ERROR, "%s expected %s.", fmt, typ_str);
}

static JnObject* native_printf(Jn_State* state, JnObject* args)
{
    int count = JN_ARGS_COUNT(args);
    if (count < 1)
        return JN_RAISE_EXCPETION(state, TYPE_ERROR, "Expected a least one argument but (got %d).", count);
    
    // JN_ARG_EXPECT_TYPE(JN_GET_ARG(args), JN_STRING_TYPE);
    if (!JN_IS_STRING(JN_GET_ARG(args)))
        return NULL;
    char* str = JN_AS_CSTRING(JN_GET_ARG(args));
    JnObject* obj;
    int arg_count = 1;
    while (*str)
    {
        if (*str == '%')
        {
            if (arg_count >= count)
            {
                return JN_RAISE_EXCPETION(state, TYPE_ERROR, "printf() got too many arguments (%d).", count);
            }
            str++;
            switch (*str)
            {
            case 's':
                obj = JN_GET_ARGS(args, arg_count);
                if (!JN_IS_STRING(obj))
                    return JN_RAISE_EXCPETION(state, TYPE_ERROR, "printf() %%s expect type string.");
                printf("%s", JN_AS_CSTRING(obj));
                break;
            case 'i':
            case 'd':
                obj = JN_GET_ARGS(args, arg_count);
                if (!JN_IS_INT(obj))
                    return JN_RAISE_EXCPETION(state, TYPE_ERROR, "printf() %%d expect type int.");
                printf("%lld", JN_AS_INT(obj));
                break;
            case 'f':
                obj = JN_GET_ARGS(args, arg_count);
                if (!JN_IS_FLOAT(obj))
                    return JN_RAISE_EXCPETION(state, TYPE_ERROR, "printf() %%f expect type float.");
                printf("%15.g", JN_AS_FLOAT(obj));
                break;            
            case 'c':
                obj = JN_GET_ARGS(args, arg_count);
                if (!JN_IS_CHAR(obj) && !JN_IS_INT(obj))
                    return JN_RAISE_EXCPETION(state, TYPE_ERROR, "printf() %%c expect type char.");
                if (JN_IS_CHAR(obj))
                    printf("%c", JN_AS_CHAR(obj));
                else
                    printf("%c", (char)JN_AS_INT(obj));
                break;
            case 'b':
                obj = JN_GET_ARGS(args, arg_count);
                printf("%s", JN_TO_BOOL(obj) ? "true" : "false");
                break;
            case 'v':
                obj = JN_GET_ARGS(args, arg_count);
                jn_obj_print(obj);
                break;
            case 'p':
                obj = JN_GET_ARGS(args, arg_count);
                printf("%p", obj); break;
            case '%':
                putc('%', stdout);
                break;
            default:
                printf("Invalid format specifier '%c'.", *str);
            }
            ++str;
            ++arg_count;
        } else {
            putc(*str++, stdout);
        }
    }
    putc('\n', stdout); // Not sure.
    return JN_RETURN_NONE;
}

static JnObject* native_assert(Jn_State* state, JnObject* args)
{
    int count = JN_ARGS_COUNT(args);
    if (count < 1 || count > 2)
    {
        return JN_RAISE_EXCPETION(state, TYPE_ERROR, "assert() expected one/two argument(s) but got (%d).", count);
    }
    char* err_msg;
    if (count == 2)
    {
        err_msg = JN_AS_CSTRING(JN_GET_ARGS(args, 1));
    } else {
        err_msg = "Assertion failed.";
    }
    if (!JN_TO_BOOL(JN_GET_ARG(args)))
    {
        return JN_RAISE_EXCPETION(state, ASSERT_ERROR, err_msg);
    }
    return JN_RETURN_NONE;
}

static JnObject* native_hasattr(Jn_State* state, JnObject* args)
{
    int count = JN_ARGS_COUNT(args);
    if (count != 2)
        return JN_RAISE_EXCPETION(state, TYPE_ERROR, "hasattr() expected an 2 argument but got %d.", count);
    JnObject* obj = JN_GET_ARG(args), *str_obj = JN_GET_ARGS(args, 1);
    bool return_bool = false;
    if (!JN_IS_STRING(str_obj))
        return JN_RAISE_EXCPETION(state, TYPE_ERROR, "hasattr() expected a string object.");
    switch (JN_OBJ_TYPE(obj))
    {
        case JN_INT_TYPE:
        case JN_FLOAT_TYPE:
        case JN_STRING_TYPE:
        case JN_CHAR_TYPE:
        case JN_HASHMAP_TYPE:
        {
            return_bool = call_method(obj, JN_AS_CSTRING(str_obj)) != NULL;
            return JN_RETURN_BOOL(state, return_bool);
        }
        case JN_INSTANCE_TYPE:
        {
            JnInstance* instance = JN_GET_INSTANCE(obj);
            return_bool = environ_get(instance->fields, JN_AS_CSTRING(str_obj)) != NULL;
            return JN_RETURN_BOOL(state, return_bool);
        }
        case JN_STRUCT_TYPE:
        {
            return_bool = strstrcmp(JN_AS_STRUCT(obj)->fields, JN_AS_CSTRING(str_obj));
            return JN_RETURN_BOOL(state, return_bool);
        }
        default:
            return JN_RETURN_FALSE(state);
    }
    return JN_RAISE_EXCPETION(state, SYS_ERROR, "hasattr() something went wrong.");
}

static JnObject* native_len(Jn_State* state, JnObject* args)
{
    int count = JN_ARGS_COUNT(args);
    if (count > 1)
        return JN_RAISE_EXCPETION(state, TYPE_ERROR, "len() expected an 1 argument but got %d.", count);
    if (!JN_IS_ITERABLE(JN_GET_ARG(args)))
        return JN_RAISE_EXCPETION(state, TYPE_ERROR, "len() expect an iterable type.");
    int len = 0;
    JnObject* len_obj = JN_GET_ARG(args);
    switch(JN_OBJ_TYPE(len_obj))
    {
        case JN_ARRAY_TYPE:
            len = (int)JN_AS_ARRAY(len_obj)->size;
            return JN_RETURN_INT(state, len);
        case JN_STRING_TYPE:
            len = JN_AS_STRING(len_obj)->len;
            return JN_RETURN_INT(state, len);
        case JN_RANGE_TYPE:
            len = range_len(JN_AS_RANGE(len_obj));
            return JN_RETURN_INT(state, len);
        case JN_HASHMAP_TYPE:
            len = (int)(JN_AS_HASHMAP(len_obj)->size);
            return JN_RETURN_INT(state, len);
        default:
            return JN_RAISE_EXCPETION(state, NOT_IMPLEMENT_ERROR, "len() does not support this type at the moment.");

    }
    return JN_RAISE_EXCPETION(state, NOT_IMPLEMENT_ERROR, "len() does not support this type at the moment.");
}

static JnObject* native_gets(Jn_State* state, JnObject* args)
{
    int count = JN_ARGS_COUNT(args);
    if (count > 1 || count < 1)
        return JN_RAISE_EXCPETION(state, TYPE_ERROR, "gets() require one argument but got %d.", count);
    JnObject* obj = JN_GET_ARG(args);
    if (!JN_IS_STRING(obj))
        return JN_RAISE_EXCPETION(state, TYPE_ERROR, "gets() expects a string but got TODO");
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
    return JN_RETURN_STRING(state, buff);
}

static JnObject* native_put(Jn_State* state, JnObject* args)
{
    int count = JN_ARGS_COUNT(args);
    if (count > 1 || count < 1)
        return JN_RAISE_EXCPETION(state, TYPE_ERROR, "put() require only one argument but got %d.", count);
    jn_obj_print(JN_GET_ARG(args));
    return JN_RETURN_NONE;
}


static JnObject* native_typeof(Jn_State* state, JnObject* args)
{
    if (JN_ARGS_COUNT(args) != 1) return JN_RETURN_NONE;
    return JN_RETURN_INT(state, JN_OBJ_TYPE(JN_GET_ARG(args)));
}

static JnObject* native_toint(Jn_State* state, JnObject* args)
{
    int count = JN_ARGS_COUNT(args);
    if (count > 1 || count < 1)
        return JN_RAISE_EXCPETION(state, TYPE_ERROR, "toint() require only one argument but got %d.", count);
    JnObject* obj = JN_GET_ARG(args);
    switch(JN_OBJ_TYPE(obj))
    {
        case JN_STRING_TYPE:
            return JN_RETURN_INT(state, strtol(JN_AS_CSTRING(obj), NULL, 10));
        case JN_INT_TYPE:
            return obj;
        case JN_FLOAT_TYPE:
            return JN_RETURN_INT(state, (long)JN_AS_FLOAT(obj));
        case JN_CHAR_TYPE:
            return JN_RETURN_INT(state, (unsigned int)JN_AS_CHAR(obj));
        case JN_BOOL_TYPE:
            return JN_RETURN_INT(state, JN_AS_BOOL(obj));
        default:
            return JN_RAISE_EXCPETION(state, TYPE_ERROR, "toint() does not support this type 'TODO'. ");
    }
    return NULL;
}

static JnObject* native_tofloat(Jn_State* state, JnObject* args)
{
    int count = JN_ARGS_COUNT(args);
    if (count > 1 || count < 1)
        return JN_RAISE_EXCPETION(state, TYPE_ERROR, "tofloat() require only one argument but got %d.", count);
    JnObject* obj = JN_GET_ARG(args);
    switch(JN_OBJ_TYPE(obj))
    {
        case JN_STRING_TYPE:
            return JN_RETURN_FLOAT(state, strtod(JN_AS_CSTRING(obj), NULL));
        case JN_INT_TYPE:
            return JN_RETURN_FLOAT(state, (double)JN_AS_INT(obj));
        case JN_FLOAT_TYPE:
            return obj;
        case JN_BOOL_TYPE:
            return JN_RETURN_FLOAT(state, JN_AS_BOOL(obj));
        default:
            return JN_RAISE_EXCPETION(state, TYPE_ERROR, "tofloat() does not support this type 'TODO'. ");
    }
    return NULL;
}

static JnObject* native_tochar(Jn_State* state, JnObject* args)
{
    int count = JN_ARGS_COUNT(args);
    if (count > 1 || count < 1)
        return JN_RAISE_EXCPETION(state, TYPE_ERROR, "tochar() require only one argument but got %d.", count);
    JnObject* obj = JN_GET_ARG(args);
    switch(JN_OBJ_TYPE(obj))
    {
        case JN_INT_TYPE:
            return JN_RETURN_CHAR(state, (char) JN_AS_INT(obj));
        case JN_CHAR_TYPE:
            return obj;
        case JN_FLOAT_TYPE:
            return JN_RETURN_CHAR(state, (char) JN_AS_FLOAT(obj));
        default:
            return JN_RAISE_EXCPETION(state, TYPE_ERROR, "tochar() does not support this type 'TODO'. ");
    }
    return NULL;
}

static JnObject* native_isinstance(Jn_State* state, JnObject* arg)
{
    // Example:
    // isinstance("Hello", string) // true
    // isinstance("World", bool) // false
    return NULL;
}

static JnObject* native_exit(Jn_State* state, JnObject* args)
{
    int count = JN_ARGS_COUNT(args);
    int exit_code = 0;
    if (count > 1)
        return JN_RAISE_EXCPETION(state, TYPE_ERROR, "exit() expected one argument");

    if (count == 1 && !JN_IS_INT(JN_GET_ARG(args)))
        return JN_RAISE_EXCPETION(state, TYPE_ERROR, "exit() expected an int.");
    if (count)
    {
        exit_code = JN_AS_INT(JN_GET_ARG(args));
    }
    state->vm->exit_code = exit_code;
    state->vm->want_exit = true;
    return JN_RETURN_NONE;
}

static JnObject* native_defined(Jn_State* state, JnObject* args)
{
    if (JN_ARGS_COUNT(args) != 1)
        return JN_RAISE_EXCPETION(state, TYPE_ERROR, "defined() takes one argument but got (%d).",JN_ARGS_COUNT(args));
    
    JnObject* obj = JN_GET_ARG(args);
    if (!JN_IS_STRING(obj))
        return JN_RAISE_EXCPETION(state, TYPE_ERROR, "Expected a string literal.");
    char* var = JN_AS_CSTRING(obj);
    return JN_RETURN_BOOL(state, (environ_get(state->vm->env, var) != NULL));
}

static JnObject* native_sleep(Jn_State* state, JnObject* args)
{
    int count = JN_ARGS_COUNT(args);
    if (count > 1 || count < 1)
        return JN_RAISE_EXCPETION(state, TYPE_ERROR, "sleep() require only one argument but got %d.", count);
    if (!JN_AS_INT(JN_GET_ARG(args)))
        return JN_RAISE_EXCPETION(state, TYPE_ERROR, "sleep() takes an int type but got TODO.");
    
#ifdef JN_WINDOWS
    sleep(JN_AS_INT(JN_GET_ARG(args)) * 1000);
#else
    sleep(JN_AS_INT(JN_GET_ARG(args)));
#endif
    return JN_RETURN_NONE;
}


// Constructor
static JnObject* bool_ctor(Jn_State* state, JnObject* args)
{
    if (JN_ARGS_COUNT(args) != 1)
        return JN_RAISE_EXCPETION(state, TYPE_ERROR, "string{} expect one arguement.");
    
    JnObject* obj = JN_GET_ARG(args);
    if (JN_TO_BOOL(obj))
        return JN_RETURN_TRUE(state);
    return JN_RETURN_FALSE(state);
}
static JnObject* string_ctor(Jn_State* state, JnObject* args)
{
    if (JN_ARGS_COUNT(args) != 1)
        return JN_RAISE_EXCPETION(state, TYPE_ERROR, "string{} expect one arguement.");
    
    JnObject* obj = JN_GET_ARG(args);
    switch (JN_OBJ_TYPE(obj))
    {
        case JN_INT_TYPE:
        case JN_FLOAT_TYPE:
        case JN_BOOL_TYPE:
            return JN_RETURN_STRING(state, jn_obj_cstring(obj));
        case JN_STRING_TYPE:
            return obj;
        default:
            return JN_RAISE_EXCPETION(state, SYS_ERROR, "String does not support this type.");
    }

    return JN_RAISE_EXCPETION(state, SYS_ERROR, "String does not support this type.");
}

JN_API void Jn_register_fn(Jn_State* state, char* name, char* doc, Jn_CFunction fn)
{
    assert(state != NULL && name != NULL);
    JnObject* obj = jn_obj_cfn(state, name, fn);
    Jn_register(state, name, doc, obj);
}

JN_API void Jn_register(Jn_State* state, const char* name, const char* doc, JnObject* obj)
{
     assert(state != NULL && obj != NULL);
     obj->doc = doc;
     set_symbols(state, name);
     environ_insert(state->globals, (char* )name, obj);
}


JN_API void Jn_define_fn(Jn_State* state, const char* name, Jn_CFunction fn)
{
    Jn_register_fn(state, (char *)name, NULL, fn);
}


JN_API void Jn_register_module(char* name, Jn_State* state, Jn_CModule* module)
{
    // TODO
    assert(false && "Not yet Impl.");
}

JN_API JnObject* Jn_make_native(char* name, Jn_State* state, Jn_CFunction fn)
{
    assert(name && state && fn);
    Jn_Native* n_fn = Jn_alloc(sizeof(Jn_Native));
    n_fn->fn = fn;
    n_fn->fnName = strdup(name);
    JnObject* obj = JN_OBJECT(state, JN_NATIVE_TYPE);
    obj->native_fn = n_fn;
    return obj;
}


JN_API bool Jn_has_variable(Jn_State* state, const char* name)
{
    return environ_get(state->globals, (char *)name) != NULL;
}

JN_API JnObject* Jn_get_variable(Jn_State* state, const char* name)
{
    assert(state && name);
    Jn_environ_E* ent = environ_get(state->globals, (char*)name);
    if (NULL == ent) return NULL;
    if (NULL == ent->value) return NULL;
    return ent->value;
}

JN_API JnObject* Jn_call_fn(Jn_State* state, char* fn_name, JnObject* args)
{
    Jn_environ_E* entt = environ_get(state->globals, fn_name);
    if (!entt || !entt->value)
        return NULL;
    JnObject* fn_obj = entt->value;
    assert(JN_IS_NATIVE(fn_obj) || JN_IS_FUNCTION(fn_obj));
    if (JN_IS_NATIVE(fn_obj))
        return JN_CALL_NATIVE(state, fn_obj, args);
    Jn_Function* fn = fn_obj->fn;
    JnVM child;
    assert(fn->arity == JN_ARGS_COUNT(args));
    Jnvm_init(&child, fn->chuck);
    child.env = fn->env;
    child.chuck = fn->chuck;
    assert(JN_ARGS_COUNT(args) == fn->arity);
    for (int i = 0; i < fn->arity; ++i)
    {
        environ_insert(child.env, fn->params[i], JN_GET_ARGS(args, i));
    }
    int r = vm_run(state, &child);
    if (r == 0)
        return *--(child.sp);
    return JN_RETURN_NONE;
}


static JnObject* license_fn(Jn_State* state, JnObject* args)
{
    if (JN_ARGS_COUNT(args) != 0)
        return JN_RAISE_EXCPETION(state, TYPE_ERROR, "license() expected no argument (but got %d).", JN_ARGS_COUNT(args));
    
    printf(
    "\n"
    "    MIT License"

    "    Copyright (c) 2026 Raphael Apeh\n"

    "    Permission is hereby granted, free of charge, to any person obtaining a copy\n"
    "    of this software and associated documentation files (the \"Software\"), to deal\n"
    "    in the Software without restriction, including without limitation the rights\n"
    "    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell\n"
    "    copies of the Software, and to permit persons to whom the Software is\n"
    "    furnished to do so, subject to the following conditions:\n\n"

    "    The above copyright notice and this permission notice shall be included in all\n"
    "    copies or substantial portions of the Software.\n\n"

    "    THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\n"
    "    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\n"
    "    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\n"
    "    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER\n"
    "    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,\n"
    "    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE\n"
    "    SOFTWARE.\n\n"
    );
    return NULL;
}

JN_API void Jn_load_repl_functions(Jn_State* state)
{
    Jn_register_fn(state, "license", "print program license.", license_fn);
    Jn_register_fn(state, "licence", "print prgram licence.", license_fn);
}

JN_API void Jn_load_Cfunctions(Jn_State* state)
{
    // To be modify later.
    JnObject* arr_obj = jn_obj_array(state);
    Jn_register(state, "argv", "Command line arguments.", arr_obj);
#ifdef JN_WINDOWS
    Jn_register(state, "__WINDOWS__", "Check if it is a Windows system.", JN_RETURN_TRUE(state));
#elif JN_APPLE
    Jn_register(state, "__APPLE__", "Check if it is a Mac system.", JN_RETURN_TRUE(state));
#elif JN_LINUX
    Jn_register(state, "__LINUX__", "Check if it is a Linux system.", JN_RETURN_TRUE(state));
#endif
    char* filename = state->cxt.source.filename ? (char *)state->cxt.source.filename : "main";

    // types
    Jn_register(state, "int", NULL, JN_RETURN_TYPE_OBJECT(state, "int", JN_INT_TYPE, native_toint));
    Jn_register(state, "string", NULL, JN_RETURN_TYPE_OBJECT(state, "string", JN_STRING_TYPE, string_ctor));
    Jn_register(state, "float", NULL, JN_RETURN_TYPE_OBJECT(state, "float", JN_INT_TYPE, native_tofloat));
    Jn_register(state, "bool", NULL, JN_RETURN_TYPE_OBJECT(state, "bool", JN_BOOL_TYPE, bool_ctor));
    Jn_register(state, "char", NULL, JN_RETURN_TYPE_OBJECT(state, "char", JN_CHAR_TYPE, native_tochar));

    // DEFAULT
    Jn_register(state, "__FILE__", "Returns the filename or main in repl.", JN_RETURN_STRING(state, filename));
    Jn_register(state, "ARRAY", "Array type.", JN_RETURN_INT(state, JN_ARRAY_TYPE));
    Jn_register(state, "FLOAT", "Float type.", JN_RETURN_INT(state, JN_FLOAT_TYPE));
    Jn_register(state, "STRING", "String type.", JN_RETURN_INT(state, JN_STRING_TYPE));
    Jn_register(state, "INTEGER", "Int type.", JN_RETURN_INT(state, JN_INT_TYPE));
    Jn_register(state, "CHAR", "Char type.", JN_RETURN_INT(state, JN_CHAR_TYPE));

    // Functions
    Jn_register_fn(state, "len", "Returns the length of an iterable", native_len);
    Jn_register_fn(state, "gets", "Get user input.", native_gets);
    Jn_register_fn(state, "put", "print object without a new-line.", native_put);
    Jn_register_fn(state, "toint", "Convert an object to int.", native_toint);
    Jn_register_fn(state, "tofloat", "Convert an object to float.", native_tofloat);
    Jn_register_fn(state, "tochar", "Convert an object to char.", native_tochar);
    Jn_register_fn(state, "hasattr", "Return true if the object has the attribute.", native_hasattr);
    Jn_register_fn(state, "typeof", "Return the typeof an object.", native_typeof);
    Jn_register_fn(state, "printf", "C type of printf.", native_printf);
    Jn_register_fn(state, "assert", "Assert expression.", native_assert);
    Jn_register_fn(state, "sleep", "Sleep program", native_sleep);
    Jn_register_fn(state, "defined", "Check if a variable exists in the current scope.", native_defined);
    Jn_register_fn(state, "format", "String format specifier.", native_format);
    Jn_register_fn(state, "exit", "Exit from program", native_exit);
    // add other built-in functions
}