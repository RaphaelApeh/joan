/* =======================================================
 Joan.h
 Full Public C API for Joan Programming Language.
==========================================================

MIT License

Copyright (c) 2026 Raphael Apeh

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef JOAN_H
#define JOAN_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _INC_STDIO
#include  <stdio.h>
#endif

#ifndef _GCC_WRAP_STDINT_H
#include <stdint.h>
#endif

#ifndef _STDBOOL_H
#include <stdbool.h>
#endif

#define JOAN_VERSION_MAJOR 0
#define JOAN_VERSION_MINOR 7
#define JOAN_VERSION_PATCH 3

#define JOAN_VERSION "0.7.3"

#ifdef _WIN32
    #ifdef JN_BUILD_DLL
    #define JN_API __declspec(dllexport)
    #else
    #define JN_API
    #endif
#else
    #define JN_API
#endif

typedef enum{
    NONE_TYPE = 0,
    STR_TYPE,
    CHAR_TYPE,
    INT_TYPE,
    BOOL_TYPE,
    FLOAT_TYPE,
    ARRAY_TYPE,
    TUPLE_TYPE,
    RANGE_TYPE,
    HASHMAP_TYPE,
    FUNCTION_TYPE,
    NATIVE_TYPE,
    METHOD_TYPE,
    ITER_TYPE,
    INSTANCE_TYPE,
    MODULE_TYPE,
    OBJECT_TYPE,
    ENUM_TYPE,
    STRUCT_TYPE,
    ARG_TYPE,
    ERROR_TYPE,
} JnTypeObject;

typedef struct AST AST;
typedef struct GC GC;
typedef struct InternEntry InternEntry;
typedef struct joan_parser_t joan_parser_t;
typedef struct Arena Arena;
typedef struct JnObject JnObject;
typedef struct JnVM JnVM;
typedef struct J_State J_State;
typedef struct J_Context J_Context;
typedef struct JN_Args JN_Args;
typedef JnObject* (*Jn_CFunction)(J_State* state, JnObject* args);
typedef JnObject* (*JN_CMethod) (J_State* state, JnObject* self, JnObject* args);
typedef void* (*JnObject_Alloc)(size_t size, JnTypeObject type);
typedef struct Jn_CModule Jn_CModule;
typedef struct Jn_environ_E Jn_environ_E;
typedef struct Jn_environ Jn_environ;

// Object internal pool
#define JN_INTER_SIZE 1 << 10
// max JnObject object store
#define JN_MAX_OBJECT 0xff << 10

#define JNSTR_OBJ(s) (JnStringObject){.chars = strdup((s)), .len = strlen((s)), .hash = djb2_hash((s))}

#define JN_ARGS_COUNT(obj) ((obj)->arg.count)
#define JN_GET_ARG(obj) ((obj)->arg.args[0])
#define JN_ARG_EXPECT_TYPE(obj, t) do { \
    if (!_JN_CHECK_TYPE(obj, t))        \
        return JN_RAISE_EXCPETION(TYPE_ERROR, "Got a type mismatch.");\
} while (0)
#define JN_OBJECT_ARG(state, objects, params, count) jn_obj_arg(state, (objects), (params), (count))
#define JN_GET_ARGS(obj, idx) ((obj)->arg.args[idx])
#define JN_MAKE_ARGS(state, cap) Jn_make_args(state, cap)
#define JN_ADD_ARG(args, obj) Jn_add_arg(args, obj)
#define JN_GET_INSTANCE(obj) obj->instance
#define JN_OBJECT(state, type) jn_obj_new(state, type)
#define JN_OBJ_TO_STRING(obj) jn_obj_to_string(obj)
#define JN_CALL_NATIVE(state, fn_obj, args) fn_obj->native_fn->fn(state, args)
#define JN_RAISE_EXCPETION(state, t, msg, ...) jn_obj_error(state, t, msg, ##__VA_ARGS__)
#define JN_RETURN_NONE jn_obj_none()
#define JN_RETURN_INT(state, i) jn_obj_int(state, (i))
#define JN_RETURN_BOOL(state, b) jn_obj_bool(state, (b))
#define JN_RETURN_TRUE(state) JN_RETURN_BOOL(state, 1)
#define JN_RETURN_FALSE(state) JN_RETURN_BOOL(state, 0)
#define JN_RETURN_STRING(state, s) jn_obj_string(state, (s))
#define JN_RETURN_CHAR(state, c) jn_obj_char(state, (c))
#define JN_RETURN_FLOAT(state, d) jn_obj_float(state, d)
#define JN_RETURN_TYPE_OBJECT(state, t_n, t, fn) jn_obj_type(state, t_n, t, fn)
#define JN_OBJECT_CSTRING(obj) Jn_object_cstring(obj)
#define JN_RETURN_STRUCT(state, name, fields) jn_obj_struct(state, (name), fields)
#define JN_RETURN_INSTANCE(obj, fields) jn_obj_instance((obj), (fields))
#define JN_OBJECT_RANGE(state, start, stop, step) jn_obj_range(state, start, stop, step)
#define JN_OBJECT_VALUE(obj) // TODO 
#define JN_AS_CHAR(obj) (obj)->j_char
#define JN_AS_STRING(obj) (obj)->str
#define JN_AS_CSTRING(obj) (JN_AS_STRING(obj)->chars)
#define JN_AS_INT(obj) (obj)->int_val
#define JN_AS_FLOAT(obj) (obj)->float_val
#define JN_AS_ARRAY(obj) (obj)->arr
#define JN_AS_ITER(obj) (obj)->iter
#define JN_AS_BOOL(obj) (obj)->bool_val
#define JN_AS_RANGE(obj) (&((obj)->range))
#define JN_AS_HASHMAP(obj) (obj)->hashmap
#define JN_AS_STRUCT(obj) (obj)->struct_obj
#define JN_OBJ_TYPE(obj) (obj)->type
#define _JN_CHECK_TYPE(obj, t) ((obj)->type == (t))
#define JN_IS_NONE(obj) _JN_CHECK_TYPE(obj, NONE_TYPE)
#define JN_IS_BOOL(obj) _JN_CHECK_TYPE(obj, BOOL_TYPE)
#define JN_TO_BOOL(obj) is_truthy(obj)
#define JN_IS_INT(obj) _JN_CHECK_TYPE(obj, INT_TYPE)
#define JN_IS_STRING(obj) _JN_CHECK_TYPE(obj, STR_TYPE)
#define JN_IS_FLOAT(obj) _JN_CHECK_TYPE(obj, FLOAT_TYPE)
#define JN_IS_ARRAY(obj) _JN_CHECK_TYPE(obj, ARRAY_TYPE)
#define JN_IS_ARGS(args) _JN_CHECK_TYPE(args, ARG_TYPE)
#define JN_IS_HASHMAP(obj) _JN_CHECK_TYPE(obj, HASHMAP_TYPE)
#define JN_IS_ITER(obj) _JN_CHECK_TYPE(obj, ITER_TYPE)
#define JN_IS_NATIVE(obj) _JN_CHECK_TYPE(obj, NATIVE_TYPE)
#define JN_IS_CHAR(obj) _JN_CHECK_TYPE(obj, CHAR_TYPE)
#define JN_IS_RANGE(obj) _JN_CHECK_TYPE(obj, RANGE_TYPE)
#define JN_IS_ERROR(obj) _JN_CHECK_TYPE(obj, ERROR_TYPE)
#define JN_IS_STRUCT(obj) _JN_CHECK_TYPE(obj, STRUCT_TYPE)
#define JN_IS_TUPLE(obj) _JN_CHECK_TYPE(obj, TUPLE_TYPE)
#define JN_IS_INSTANCE(obj) _JN_CHECK_TYPE(obj, INSTANCE_TYPE)
#define JN_IS_FUNCTION(obj) _JN_CHECK_TYPE(obj, FUNCTION_TYPE)
#define JN_IS_ITERABLE(obj) (JN_IS_HASHMAP(obj) || JN_IS_ARRAY(obj) || JN_IS_STRING(obj) || JN_IS_ITER(obj) || JN_IS_RANGE(obj))
#define JN_HASHMAP_GET(map, key) Jn_hashmap_get(map, key)
#define JN_HASMAP_PUT(map, key, value) Jn_hashmap_put(map, key, value)
#define JN_HASHMAP_GET_FROM_STRING(map, str) Jn_hashmap_get_string((map), (str))
#define JN_HASHMAP_INSERT(map, k, v, i) do {    \
    if ((map) == NULL) {                        \
        (map) = malloc(sizeof(Jn_Hashmap));      \
        (map)->capacity = 100;                     \
        map->buckets = malloc(sizeof(Jn_HashEntry) * (map)->capacity);\
        (map)->size = 0;                            \
    }                                                \
    Jn_hashmap_insert((map), (k), (v), (i));          \
}while(false)

#define JN_HASHMAP_INSERT_STRING(map, str, v) do {    \
    if ((map) == NULL) {                        \
        (map) = malloc(sizeof(Jn_Hashmap));      \
        (map)->capacity = 100;                     \
        map->buckets = malloc(sizeof(Jn_HashEntry) * (map)->capacity);\
        (map)->size = 0;                            \
    }                                                \
    Jn_hashmap_from_string(map, (str), (value));            \
}while(false)

#define JN_ALLOC(s) malloc(s)

#define JN_DEFAULT_HM(obj) do{\
    (obj) = malloc(sizeof(JnArrayObject));   \
    (obj)->capacity = 100;                    \
    (obj)->size = 0;                            \
    (obj)->buckets = malloc(sizeof(Jn_HashEntry) * (obj)->capacity);  \
}while(false)


#define JN_ARRAY_DEFAULT(arr) do {\
        (arr) = malloc(sizeof(JnArrayObject));          \
        (arr)->capacity = 100;                          \
        (arr)->size = 0;                                \
        (arr)->items = malloc(sizeof(JnObject *) * (arr)->capacity);    \
} while(false)

#define JN_SET_ARRAY(arr, obj, i) do{                  \
    if ((arr) == NULL){                                 \
        (arr) = malloc(sizeof(JnArrayObject));          \
        (arr)->capacity = 100;                          \
        (arr)->size = 0;                                \
        (arr)->items = malloc(sizeof(JnObject *) * (arr)->capacity);    \
    }                                                   \
    if ((i) >= (arr)->capacity){                                  \
        (arr)->capacity *= 2;                             \
        (arr)->items = realloc((arr)->items, sizeof(JnObject* ) * (arr)->capacity);   \
    }                                                     \
    (arr)->items[(i)] = (obj);                                 \
    (arr)->size++;                                         \
} while(false)

#define JN_GET_ARRAY(arr, idx) jn_obj_array_get(arr, idx)
#define JN_AS_HM(obj) obj->hashmap
#define JN_ITER_INIT(state, obj) jn_obj_iter(state, obj)
#define JN_ERROR_PRINT(type) ((type) == IMPORT_ERROR ? "IMPORT ERROR": (type) == SYS_ERROR ? "SYSTEM_ERROR" : (type) == SYNTAX_ERROR ? "SYNTAX ERROR" : (type) ==   ASSERT_ERROR ? "ASSERTION ERROR" : (type) == TYPE_ERROR ? "TYPE ERROR" : (type) == NOT_IMPLEMENT_ERROR ? "NOT IMPLEMENT ERROR" : "UNDEFINE ERROR")
// State

struct JN_Args
{
    JnObject** args;
    char** arg_names; // default to NULL
    size_t count;
};

typedef struct {
    const char* filename;
    char* source;
} J_Source;

typedef struct J_Context {
    J_Source source;
    int cur_line, column;
} J_Context;

typedef enum {
    RUNTIME_ERROR, ASSERT_ERROR, SYS_ERROR, IMPORT_ERROR, SYNTAX_ERROR,
    TYPE_ERROR, NOT_IMPLEMENT_ERROR, UNDEFINE_ERROR
} JN_CERROR_TYPE;


typedef struct {
    const char* filename, *error_msg, *var_name; // TODO: remove var_name
    int line, col, code;
    JN_CERROR_TYPE type;
} Jn_Error;

typedef struct J_State
{
    JnVM* vm;
    GC* gc;
    Arena* arena;
    joan_parser_t* parser;
    Jn_Error error;
    InternEntry* intern_pool[JN_INTER_SIZE];
    JnObject_Alloc alloc_fn;
    Jn_environ* globals;
    char** symbols;
    J_Context cxt;
    size_t symbols_count, symbols_capacity;
    int running;
} J_State;


void set_symbols(J_State* state, char* str);
/*

Jn_CModule math_mod[] = {
    {"PI", NULL, JN_RETURN_FLOAT(3.14)}
    {NULL, NULL, NULL} // required
}

Jn_CRegistry* math_lib = register_module("math", state, math_mod) 
*/
struct Jn_CModule {
    char *name, *doc;
    JnObject* obj;
};

typedef struct {
    const char* fnName;
    Jn_CFunction fn;
} JnStaticMethod;

typedef struct {
    Jn_CModule* modules;
    char* mod_name;
} Jn_CRegistry;

JN_API JnObject* make_native(char* name, Jn_CFunction fn);
JN_API Jn_CRegistry* register_module(char* name, J_State* state, Jn_CModule* module);

// Object Type
typedef struct Chuck Chuck;
typedef JnObject* (* NativeFn) (JnObject** argv, size_t argc);
typedef JnObject* (* MethodFn) (JnObject* self, JnObject** argv, size_t argc);

typedef struct {
    char* chars;
    unsigned long hash;
    long len;
} JnStringObject;

typedef struct {
    Jn_CFunction fn;
    char* fnName;
} JnNativeObject;

typedef struct {
    const char* typename;
    JnTypeObject type;
    Jn_CFunction ctor;
    JnStaticMethod* methods;
} JnType;

typedef struct {
    Chuck* chuck;
    Jn_environ* env;
    char** params;
    char* name;
    int arity, is_lambda;
} JnFunctionObject;

typedef struct {
    JnObject** items;
    size_t size;
    size_t capacity;
} JnArrayObject;

typedef struct {
    uint64_t start, stop;
    uint16_t step;
} JnRange;

typedef struct {
    JnObject* obj;
    int index;
} JnIterObject;

typedef struct Jn_HashEntry {
    JnObject* key;
    JnObject* value;
    uint64_t hash;
} Jn_HashEntry;

typedef struct Jn_Hashmap{
    Jn_HashEntry* buckets;
    size_t size, capacity;
} Jn_Hashmap;

typedef struct {
    Jn_environ* env;
    const char* ident;
} Jn_Enum;

typedef struct {
    char* name;
    char** fields;
    long field_count;
} JnStruct;

typedef struct {
    JnObject* obj;
    Jn_environ* fields;
} JnInstance;

typedef struct {
    Jn_environ* env;
    char *name, *path;
    char* alias;
} JnModule;

typedef long long JnIntObject;
typedef double JnFloatObject;
typedef bool JnBoolObject;
typedef char JnCharObject;

// Object

typedef struct JnObject{
    union
    {
        JnStringObject* str;
        JnArrayObject* arr;
        JnArrayObject* tuple;
        JnFunctionObject* fn;
        JnIterObject* iter;
        Jn_Hashmap* hashmap;
        JnNativeObject* native_fn;
        Jn_Enum* enum_n;
        JnModule* module;
        JnStruct* struct_obj;
        JnInstance* instance;
        struct {
            JN_CMethod fn;
            JnObject* obj;
        } method;
        JnType type_val;
        JN_Args arg;
        Jn_Error expection;
        JnRange range;
        JnIntObject int_val;
        JnFloatObject float_val;
        JnBoolObject bool_val;
        JnCharObject j_char;
    };
    JnObject* next;
    const char* doc;
    JnTypeObject type;
    int marked;
    int constant;
} JnObject;


JN_API void Jn_repl(void);

void* Jn_alloc(size_t size);
// Helpers
unsigned long djb2_hash(unsigned const char* str);

JN_API JnObject* Jn_make_args(J_State* state, size_t capacity);
JN_API void Jn_add_arg(JnObject* args, JnObject* obj);


JN_API JnObject* Jn_import_module(J_State* state, char* path, int is_std);

// Register native function
JN_API void Jn_define_fn(J_State* state, const char*, Jn_CFunction);
JN_API void Jn_register_fn(J_State* state, char* name, char* doc, Jn_CFunction fn);
JN_API void Jn_register(J_State* state, const char* name, const char* doc, JnObject* obj);

// Compile & Run
JN_API int Jn_parse(J_State*, const char*);
JN_API int Jn_compile(J_State*);
JN_API int Jn_exec(J_State*);

// Load builtin function
JN_API void Jn_load_Cfunctions(J_State* state);
// Load repl funtions
JN_API void Jn_load_repl_functions(J_State* state);
// Call user-define functions
JN_API JnObject* Jn_call_fn(J_State*, char* fn_name, JnObject* args);
JN_API J_Context* Jn_get_context(J_State*);
// Jn_exec_from_file(FILE* fptr);
JN_API void Jn_program_init(J_State*);
JN_API int Jn_exec_program(J_State* state, const char* source);
JN_API int Jn_exec_string(J_State*, const char*);
JN_API int Jn_exec_REPL(J_State*, const char* source);
// Main Execution function
JN_API int Jn_execute_main(J_State*, const char*);
// Execute for FILE ptr.
JN_API int Jn_exec_from_file(J_State*, FILE*);

JN_API void Jn_program_close(J_State*);

JN_API JN_CMethod call_method(JnObject* obj, const char* method_name);

// Object functions
JnObject* jn_obj_new(J_State*, JnTypeObject type);
JnObject* jn_obj_int(J_State*, long o_int);
JnObject* jn_obj_string(J_State*, char* str);
JnObject* jn_obj_char(J_State*, char c);
JnObject* jn_obj_none(void);
JnObject* jn_obj_bool(J_State*, bool o_bool);
JnObject* jn_obj_range(J_State*, int64_t start, int64_t stop, int64_t step);
JnObject* jn_obj_float(J_State*, double o_float);
JnObject* jn_obj_iter(J_State*, JnObject* iter);
JnObject* jn_obj_type(J_State*, char* type_name, JnTypeObject type, Jn_CFunction fn);
JnObject* jn_obj_function(J_State*, AST* block, Jn_environ* env, char** params, int arity, char* name);
JnObject* jn_obj_lambda(J_State*, AST* expr, char** params, int arity, Jn_environ* env);
JnObject* jn_obj_module(J_State*, char* name, char* path, Jn_environ* env);
JnObject* jn_obj_struct(J_State*, char* name, char** fields);
JnObject* jn_obj_arg(J_State*, JnObject** args, char** arg_names, size_t count);
JnObject* jn_obj_method(J_State*, JnObject* obj, JN_CMethod method);
JnObject* jn_obj_instance(J_State*, JnObject* obj, Jn_environ* fields);
uint64_t Jn_object_hash(JnObject* obj);
char* jn_obj_to_string(JnObject* obj);
char* jn_obj_cstring(JnObject* obj);
JnObject* jn_obj_array_get(JnArrayObject* arr, int idx);
JnObject* jn_obj_error(J_State*, int type, char* msg, ...);
char* Jn_object_cstring(JnObject* obj);
bool is_truthy(JnObject* obj);

//Hashmap Functions
Jn_HashEntry* Jn_hashmap_get(Jn_Hashmap* map, JnObject* key);
void Jn_hashmap_insert(Jn_Hashmap* map, JnObject* key, JnObject* value, int idx);
void Jn_hashmap_put(Jn_Hashmap* map, JnObject* key, JnObject* value);
JnObject* Jnhashmap_get_from_index(Jn_Hashmap* map, int index);
void Jn_hashmap_from_string(Jn_Hashmap* map, char* str, JnObject* value);
JnObject* Jn_hashmap_get_string(Jn_Hashmap* map, char* str);

#ifdef __cplusplus
}
#endif

#endif // JOAN_H