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

#ifndef _Jn_GCC_WRAP_STDINT_H
#include <stdint.h>
#endif

#ifndef _STDBOOL_H
#include <stdbool.h>
#endif

#ifdef _WIN32
#define JN_WINDOWS
#include <direct.h>
#include <windows.h>
#endif // _WIN32

#ifdef _MSC_VER
#define JN_MSVC
#endif // MSC_VER

#if defined(__APPLE__) && defined(__MACH__)
#define JN_APPLE
#endif

#ifdef __linux__
#define JN_LINUX
#endif

#if (defined(JN_LINUX) || defined(JN_APPLE) || defined(__unix__)) && !defined(JN_WINDOWS)
#include <termios.h>
#include <unistd.h>
#endif

#ifdef JN_MSVC
#define JN_INLINE static __forceinline
#else
#define JN_INLINE static inline
#endif

#ifdef JN_WINDOWS
typedef HANDLE JnHandle;
#define JN_INVALID_HANDLE_VALUE INVALID_HANDLE_VALUE
#else
typedef int JnHandle;
#define JN_INVALID_HANDLE_VALUE (-1)
#endif

#define JOAN_VERSION_MAJOR 0
#define JOAN_VERSION_MINOR 7
#define JOAN_VERSION_PATCH 8

#define JOAN_VERSION "0.7.8"
#define JOAN_EXT "jt"
#define JOAN_BRANCH "main"

#ifdef _WIN32
    #ifdef JN_BUILD_DLL
    #define JN_API __declspec(dllexport)
    #else
    #define JN_API __declspec(dllimport)
    #endif
#else
    #define JN_API
#endif

// JN_USE_ASCII

#if !defined(JN_WINDOWS) || defined(JN_USE_ASCII)
enum {
    JN_COLOR_RESET = 0,
    JN_COLOR_BLACK = 30,
    JN_COLOR_RED = 31,
    JN_COLOR_GREEN = 32,
    JN_COLOR_YELLOW = 33,
    JN_COLOR_BLUE = 34,
};

#else 
enum {
    JN_COLOR_RESET = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
    JN_COLOR_BLACK = 0,
    JN_COLOR_RED = FOREGROUND_RED,
    JN_COLOR_GREEN = FOREGROUND_GREEN,
    JN_COLOR_BLUE = FOREGROUND_BLUE,
    JN_COLOR_YELLOW = FOREGROUND_RED | FOREGROUND_GREEN,
};
#endif


#define JN_INITIAL_CAPACITY 0xff

typedef enum{
    JN_NONE_TYPE = 0,
    JN_STRING_TYPE,
    JN_CHAR_TYPE,
    JN_INT_TYPE,
    JN_BOOL_TYPE,
    JN_FLOAT_TYPE,
    JN_ARRAY_TYPE,
    JN_TUPLE_TYPE,
    JN_RANGE_TYPE,
    JN_HASHMAP_TYPE,
    JN_GENERATOR_TYPE,
    JN_FUNCTION_TYPE,
    JN_NATIVE_TYPE,
    JN_METHOD_TYPE,
    JN_ITER_TYPE,
    JN_INSTANCE_TYPE,
    JN_MODULE_TYPE,
    JN_OBJECT_TYPE,
    JN_ENUM_TYPE,
    JN_STRUCT_TYPE,
    JN_ARG_TYPE,
    JN_UNKOWN_TYPE,
    JN_ERROR_TYPE,
} JnTypeObject;

typedef struct Jn_State Joan;
typedef struct Jn_GC Jn_GC;
typedef struct JnInternEntry JnInternEntry;
typedef struct Jn_Parser Jn_Parser;
typedef struct Jn_Arena Jn_Arena;
typedef struct JnObject JnObject;
typedef struct JnVM JnVM;
typedef struct Jn_State Jn_State;
typedef struct J_Context J_Context;
typedef struct JN_Args JN_Args;
typedef struct Jn_Buffer Jn_Buffer;
typedef struct Jn_Node Jn_Node;
typedef struct Jn_Lexer Jn_Lexer;
typedef struct Jn_Token Jn_Token;
typedef struct Jn_Gen Jn_Gen;
typedef JnObject* (*Jn_CFunction)(Jn_State* state, JnObject* args);
typedef JnObject* (*JN_CMethod) (Jn_State* state, JnObject* self, JnObject* args);
typedef void* (*JnObject_Alloc)(size_t size, JnTypeObject type);
typedef JnObject* (*JnForeignHandler)(Jn_State* state, const char* fn_name, int params, JnObject* args);
typedef struct Jn_CModule Jn_CModule;
typedef struct Jn_environ_E Jn_environ_E;
typedef struct Jn_environ Jn_environ;

// Object internal pool
#define JN_INTER_SIZE 1 << 10
// max JnObject object store
#define JN_MAX_OBJECT 0xff << 10

#ifndef JN_STACK_MAX
#define JN_STACK_MAX 1024
#endif

#ifndef JN_FRAME_MAX
#define JN_FRAME_MAX 256
#endif

#define JN_LOOP_MAX 0xff

#define jn_obj_println(obj) do{                     \
    jn_obj_print(obj);                               \
    putchar('\n');                                    \
} while (false)

#define JN_LOG(msg, ...) do {                               \
    fprintf(stderr, "[MESSAGE]: ");                         \
    fprintf(stderr, msg, ##__VA_ARGS__);                    \
} while (false)

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
#define JN_NEW_STRING(state, str) JN_RETURN_STRING(state, str)
#define JN_NEW_INT(state, i) JN_RETURN_INT(state, i)
#define JN_NEW_CHAR(state, c) JN_RETURN_CHAR(state, c)
#define JN_NEW_FLOAT(state, f) JN_RETURN_FLOAT(state, f)
#define JN_NEW_BOOL(state, b) JN_RETURN_BOOL(state, b)
#define JN_OBJECT_RANGE(state, start, stop, step) jn_obj_range(state, start, stop, step)
#define JN_OBJECT_VALUE(obj) // TODO 
#define JN_AS_CHAR(obj) (obj)->j_char
#define JN_AS_STRING(obj) (obj)->str
#define JN_AS_CSTRING(obj) (JN_AS_STRING(obj)->chars)
#define JN_STRING_LEN(obj) JN_AS_STRING(obj)->len
#define JN_AS_INT(obj) (obj)->int_val
#define JN_AS_FLOAT(obj) (obj)->float_val
#define JN_AS_ARRAY(obj) (obj)->arr
#define JN_AS_TUPLE(obj) (obj)->tuple
#define JN_AS_ITER(obj) (obj)->iter
#define JN_AS_BOOL(obj) (obj)->bool_val
#define JN_AS_RANGE(obj) (&((obj)->range))
#define JN_AS_HASHMAP(obj) (obj)->hashmap
#define JN_AS_STRUCT(obj) (obj)->struct_obj
#define JN_OBJ_TYPE(obj) (obj)->type
#define _JN_CHECK_TYPE(obj, t) ((obj)->type == (t))
#define JN_IS_NONE(obj) _JN_CHECK_TYPE(obj, JN_NONE_TYPE)
#define JN_IS_BOOL(obj) _JN_CHECK_TYPE(obj, JN_BOOL_TYPE)
#define JN_TO_BOOL(obj) jn_obj_truthy(obj)
#define JN_IS_INT(obj) _JN_CHECK_TYPE(obj, JN_INT_TYPE)
#define JN_IS_STRING(obj) _JN_CHECK_TYPE(obj, JN_STRING_TYPE)
#define JN_IS_FLOAT(obj) _JN_CHECK_TYPE(obj, JN_FLOAT_TYPE)
#define JN_IS_ARRAY(obj) _JN_CHECK_TYPE(obj, JN_ARRAY_TYPE)
#define JN_IS_ARGS(args) _JN_CHECK_TYPE(args, JN_ARG_TYPE)
#define JN_IS_HASHMAP(obj) _JN_CHECK_TYPE(obj, JN_HASHMAP_TYPE)
#define JN_IS_ITER(obj) _JN_CHECK_TYPE(obj, JN_ITER_TYPE)
#define JN_IS_NATIVE(obj) _JN_CHECK_TYPE(obj, JN_NATIVE_TYPE)
#define JN_IS_CHAR(obj) _JN_CHECK_TYPE(obj, JN_CHAR_TYPE)
#define JN_IS_RANGE(obj) _JN_CHECK_TYPE(obj, JN_RANGE_TYPE)
#define JN_IS_ERROR(obj) _JN_CHECK_TYPE(obj, JN_ERROR_TYPE)
#define JN_IS_STRUCT(obj) _JN_CHECK_TYPE(obj, JN_STRUCT_TYPE)
#define JN_IS_TUPLE(obj) _JN_CHECK_TYPE(obj, JN_TUPLE_TYPE)
#define JN_IS_INSTANCE(obj) _JN_CHECK_TYPE(obj, JN_INSTANCE_TYPE)
#define JN_IS_FUNCTION(obj) _JN_CHECK_TYPE(obj, JN_FUNCTION_TYPE)
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
    (obj) = malloc(sizeof(Jn_Array));   \
    (obj)->capacity = 100;                    \
    (obj)->size = 0;                            \
    (obj)->buckets = malloc(sizeof(Jn_HashEntry) * (obj)->capacity);  \
}while(false)


#define JN_ARRAY_DEFAULT(arr) do {\
        (arr) = malloc(sizeof(Jn_Array));          \
        (arr)->capacity = 100;                          \
        (arr)->size = 0;                                \
        (arr)->items = malloc(sizeof(JnObject *) * (arr)->capacity);    \
} while(false)

#define JN_SET_ARRAY(arr, obj, i) do{                  \
    if ((arr) == NULL){                                 \
        (arr) = malloc(sizeof(Jn_Array));          \
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


#define jn_obj__type(obj)   (NULL == obj) ? -1 : obj->type
#define Jn__iter_foreach(var, iter)     \
    for (JnObject** __iter = iter->items, ** __end = __iter + iter->size,, * ##var = *__iter; \
        __iter < __end && ((##var = *__iter), 1); ++__iter)

// WARNING: Only works with Arrays and Tuples
#define Jn_foreach(var, obj)    \
    Jn__iter_foreach(##var, JN_IS_ARRAY(obj) ? JN_AS_ARRAY(obj) : JN_AS_TUPLE(obj))

#define Jn_append(obj, ...)                                 \
    jn_arr_append_many(obj, ((JnObject* []){__VA_ARGS__}),  \
        (sizeof(JnObject* []){__VA_ARGS__}) / (sizeof(JnObject *)))

#define Jn_append_none(obj) jn_arr_append(obj, JN_RETURN_NONE)

#define JN_GET_ARRAY(arr, idx) jn_obj_array_get(arr, idx)
#define JN_AS_HM(obj) obj->hashmap
#define JN_ITER_INIT(state, obj) jn_obj_iter(state, obj)
#define JN_ERROR_PRINT(type) ((type) == IMPORT_ERROR ? "IMPORT ERROR": (type) == SYS_ERROR ? "SYSTEM_ERROR" : (type) == SYNTAX_ERROR ? "SYNTAX ERROR" : (type) ==   ASSERT_ERROR ? "ASSERTION ERROR" : (type) == TYPE_ERROR ? "TYPE ERROR" : (type) == NOT_IMPLEMENT_ERROR ? "NOT IMPLEMENT ERROR" : (type) == MATH_ERROR ? "MATH ERROR" : "UNDEFINE ERROR")

typedef enum 
{
    JN_INTERPRET_OK,
    JN_INTERPRET_YEILD,
    JN_INTERPRET_EXIT,
    JN_INTERPRET_RUNTIME_ERROR,
    JN_INTERPRET_ERROR,
} JnVMInterpretResult;


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
    char** argv; int argc;
    int cur_line, column;
} J_Context;

typedef enum {
    RUNTIME_ERROR, ASSERT_ERROR, SYS_ERROR, IMPORT_ERROR, SYNTAX_ERROR,
    TYPE_ERROR, MATH_ERROR,NOT_IMPLEMENT_ERROR, UNDEFINE_ERROR
} JN_CERROR_TYPE;


typedef struct {
    const char* filename, *error_msg, *var_name; // TODO: remove var_name
    int line, col, code;
    JN_CERROR_TYPE type;
} Jn_Error;

typedef struct Jn_State
{
    JnVM* vm;
    Jn_GC* gc;
    Jn_Arena* arena;
    Jn_Parser* parser;
    Jn_Error error;
    JnInternEntry* intern_pool[JN_INTER_SIZE];
    JnObject_Alloc alloc_fn;
    Jn_environ* globals;
    JnForeignHandler foreign_handler;
    const char** symbols;
    J_Context cxt;
    size_t symbols_count, symbols_capacity;
    int running;
} Jn_State;


void set_symbols(Jn_State* state, const char* str);
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

JN_API JnObject* Jn_make_native(char* name, Jn_State* state, Jn_CFunction fn);
JN_API void Jn_register_module(char* name, Jn_State* state, Jn_CModule* module);

// Object Type
typedef struct Chuck Chuck;
typedef JnObject* (* NativeFn) (JnObject** argv, size_t argc);
typedef JnObject* (* MethodFn) (JnObject* self, JnObject** argv, size_t argc);

typedef struct {
    char* chars;
    unsigned long hash;
    long len;
} Jn_String;

typedef struct {
    Jn_CFunction fn;
    char* fnName;
} Jn_Native;

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
    int arity;
    bool is_yield, is_lambda;
} Jn_Function;

typedef struct {
    JnObject** items;
    size_t size;
    size_t capacity;
} Jn_Array;

typedef struct {
    uint64_t start, stop;
    uint16_t step;
} JnRange;

typedef struct {
    JnObject* obj;
    int index;
} Jn_Iter;

typedef struct Jn_HashEntry {
    JnObject* key;
    JnObject* value;
    uint64_t hash;
} Jn_HashEntry;

typedef struct Jn_Hashmap{
    Jn_HashEntry* buckets;
    size_t size, capacity;
} Jn_Hashmap;

struct Jn_Gen {
    JnVM* vm;
    bool done;
};
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

typedef long long Jn_Integer;
typedef double Jn_Float;
typedef bool Jn_Bool;
typedef char Jn_Char;
typedef  Jn_Array Jn_Tuple;
// Object

typedef struct JnObject{
    union
    {
        Jn_String* str;
        Jn_Array* arr;
        Jn_Tuple* tuple;
        Jn_Function* fn;
        Jn_Iter* iter;
        Jn_Hashmap* hashmap;
        Jn_Native* native_fn;
        Jn_Enum* enum_n;
        JnModule* module;
        JnStruct* struct_obj;
        JnInstance* instance;
        Jn_Gen* gen;
        void* ptr_val;
        struct {
            JN_CMethod fn;
            JnObject* obj;
        } method;
        JnType type_val;
        JN_Args arg;
        Jn_Error expection;
        JnRange range;
        Jn_Integer int_val;
        Jn_Float float_val;
        Jn_Bool bool_val;
        Jn_Char j_char;
    };
    JnObject* next;
    const char* doc;
    JnTypeObject type;
    int marked;
    int constant;
} JnObject;


// Run REPL
JN_API void Jn_repl(Jn_State* state);

// Execute and run repl
JN_API void Jn_run_iterative(Jn_State* state, const char* filename);

// Main Allocator
JN_API void* Jn_alloc(size_t size);
JN_API void* Jn_realloc(void* ptr, size_t size);
JN_API void* Jn_alloc_dup(void* ptr, size_t size);
JN_API void Jn_free(void* ptr);
JN_API void Jn_mem_zero(void* ptr, size_t size);

JN_API bool Jn_file_exists(const char* filename);

// Lexer
// Example:
// Jn_Token tok;
// while (Jn_get_next_token(&lex, &tok))
//{
//      printf("Token = %s", tok.lexeme);
//}
// WARNING: INTERNAL FUNCTION
JN_API bool Jn_get_next_token(Jn_Lexer* lex, Jn_Token* tok);

// Parser
JN_API int Jn_read_file(Jn_Buffer* Out, const char* filename);
JN_API Jn_Node* Jn_parse_file(Jn_State* state, const char* filename);

// Object Argument helper
JN_API JnObject* Jn_make_args(Jn_State* state, size_t capacity);
JN_API void Jn_add_arg(JnObject* args, JnObject* obj);

// Set a custom foreign handler
JN_API void Jn_add_handler(Jn_State* state, JnForeignHandler fn);

// Defualt foreign handler
JN_API void* Jn_defualt_handler(Jn_State* state, const char* fn_name, int params, JnObject* args);

JN_API JnObject* Jn_import_module(Jn_State* state, char* path, int is_std);

// IO

// Custom implementation of get_pass()
JN_API char* Jn_get_pass(const char* msg);

// Color Stuff
JN_API void Jn_color_fprintf(FILE* _Std, int color, const char* fmt, ...);
#define Jn_color_printf(color, fmt, ...) Jn_color_fprintf(stdout, (color), (fmt), ##__VA_ARGS__)

#define Jn_warning_printf(fmt, ...) \
    Jn_color_fprintf(stderr, JN_COLOR_YELLOW, (fmt), ##__VA_ARGS__)

#define Jn_error_printf(fmt, ...) Jn_color_fprintf(stderr, JN_COLOR_RED, (fmt), ##__VA_ARGS__)

// Register native function
JN_API void Jn_define_fn(Jn_State* state, const char*, Jn_CFunction);
JN_API void Jn_register_fn(Jn_State* state, char* name, char* doc, Jn_CFunction fn);
JN_API void Jn_register(Jn_State* state, const char* name, const char* doc, JnObject* obj);


// Buffer

struct Jn_Buffer {
    char* data;
    size_t len, cap;
};

JN_API int Jn_buff_init(Jn_Buffer* B);
JN_API void Jn_buff_add_char(Jn_Buffer* B, char c);
JN_API void Jn_buff_add_string(Jn_Buffer* B, const char* str);
JN_API void Jn_buff_add_nstring(Jn_Buffer* B, const char* str, size_t len);
JN_API void Jn_buff_clear(Jn_Buffer* B);
JN_API char* Jn_buff_to_string(Jn_Buffer* B);

JN_API int Jn_snprintf(char* buff, size_t size, const char* fmt, ...);
JN_API int Jn_vsnprintf(char* buff, size_t size, const char* fmt, va_list ap);

// Stack / Push

JN_API void Jn_pushnone(Jn_State*);
JN_API void Jn_pushcfunc(Jn_State*, Jn_CFunction);
JN_API void Jn_pushobject(Jn_State*, JnObject*);
JN_API void Jn_pushinteger(Jn_State*, Jn_Integer);
JN_API void Jn_pushstring(Jn_State*, char*);
JN_API void Jn_pushfloat(Jn_State*, Jn_Float);
JN_API void Jn_pushchar(Jn_State*, Jn_Char);

JN_API JnObject* Jn_gettop(Jn_State*);
JN_API int Jn_settop(Jn_State*, JnObject*);
JN_API void Jn_setinst(Jn_State*, int);
JN_API JnObject* Jn_pop(Jn_State*);


// Globals

JN_API void Jn_set_global(Jn_State*, char*, JnObject*);
JN_API JnObject* Jn_get_global(Jn_State*, char*);

// Variable stuff
JN_API JnObject* Jn_get_variable(Jn_State* state, const char* name);
JN_API bool Jn_has_variable(Jn_State* state, const char* name);

// Compile & Run
JN_API int Jn_compile(Jn_State*);
JN_API int Jn_exec(Jn_State*);

// Load builtin function
JN_API void Jn_load_Cfunctions(Jn_State* state);
// Load repl funtions
JN_API void Jn_load_repl_functions(Jn_State* state);
// Call user-define functions
JN_API JnObject* Jn_call_fn(Jn_State*, char* fn_name, JnObject* args);
// State Context
JN_API J_Context* Jn_get_context(Jn_State*);
// Jn_exec_from_file(FILE* fptr);
JN_API void Jn_program_init(Jn_State*, char**, int);
JN_API int Jn_exec_program(Jn_State* state, const char* filename, const char* source);
JN_API int Jn_exec_string(Jn_State*, const char*);
JN_API int Jn_exec_REPL(Jn_State*, const char* source);
// Main Execution function
JN_API int Jn_execute_main(Jn_State*, const char*);
// Execute for FILE ptr.
JN_API int Jn_exec_from_file(Jn_State*, char*, FILE*);

JN_API void Jn_program_close(Jn_State*);

JN_API JN_CMethod call_method(JnObject* obj, const char* method_name);

JN_API void Jn_tokenizer(Jn_State*, FILE*);

// Object functions
JN_API JnObject* jn_obj_new(Jn_State*, JnTypeObject type);
JN_API JnObject* jn_obj_int(Jn_State*, Jn_Integer);
JN_API JnObject* jn_obj_string(Jn_State*, char* str);
JN_API void jn_obj_copy(Jn_State* state, JnObject* dest, JnObject* src);
JN_API JnObject* jn_obj_char(Jn_State*, Jn_Char);
JN_API JnObject* jn_obj_none(void);
JN_API JnObject* jn_obj_bool(Jn_State*, Jn_Bool);
JN_API JnObject* jn_obj_range(Jn_State*, int64_t start, int64_t stop, int64_t step);
JN_API JnObject* jn_obj_float(Jn_State*, Jn_Float);
JnObject* jn_obj_iter(Jn_State*, JnObject* iter);
JN_API JnObject* jn_obj_array(Jn_State* state);
JN_API JnObject* jn_obj_cfn(Jn_State* state, char* name, Jn_CFunction fn);
JnObject* jn_obj_type(Jn_State*, char* type_name, JnTypeObject type, Jn_CFunction fn);
JnObject* jn_obj_intern(Jn_State* state, JnObject* obj);
JnObject* jn_obj_gen(Jn_State* state, JnVM* gvm);
JnObject* jn_obj_module(Jn_State*, char* name, char* path, Jn_environ* env);
JnObject* jn_obj_struct(Jn_State*, char* name, char** fields);
JnObject* jn_obj_arg(Jn_State*, JnObject** args, char** arg_names, size_t count);
JnObject* jn_obj_method(Jn_State*, JnObject* obj, JN_CMethod method);
JnObject* jn_obj_instance(Jn_State*, JnObject* obj, Jn_environ* fields);
JN_API bool jn_obj_equals(JnObject* obj, JnObject* other);
JN_API bool jn_obj_match(JnObject* obj, JnObject* other);
JN_API uint64_t Jn_object_hash(JnObject* obj);
JN_API const char* jn_obj_to_string(JnObject* obj);
char* jn_obj_cstring(JnObject* obj);
JN_API int jn_obj_count(JnObject* obj);
JnObject* jn_obj_array_get(Jn_Array* arr, int idx);
JnObject* jn_obj_error(Jn_State*, int type, char* msg, ...);
char* Jn_object_cstring(JnObject* obj);
JN_API bool jn_obj_truthy(JnObject* obj);
JN_API void jn_obj_print(JnObject* obj);
// Array functions
void jn_arr_pop(JnObject* arr_obj, JnObject** value);
void jn_arr_copy(JnObject* dest, JnObject* src);
void jn_arr_append(JnObject* arr_obj, JnObject* value);
void jn_arr_clear(JnObject* arr_obj);
JnObject* jn_arr_remove(JnObject* arr_obj, int index);
JnObject* jn_arr_get(JnObject* arr_obj, int index);
// Returns -1 if function failed, returns 1 if increment capacity defualt 0
int jn_arr_grow(JnObject* arr_obj, size_t new_size);
void jn_arr_append_many(JnObject* arr_obj, JnObject** argv, size_t argc);


//Hashmap Functions
Jn_HashEntry* Jn_hashmap_get(Jn_Hashmap* map, JnObject* key);
void Jn_hashmap_insert(Jn_Hashmap* map, JnObject* key, JnObject* value, int idx);
void Jn_hashmap_put(Jn_Hashmap* map, JnObject* key, JnObject* value);
JnObject* Jnhashmap_get_from_index(Jn_Hashmap* map, int index);
bool Jnhashmap_remove(Jn_Hashmap* map, JnObject* key);

#ifdef __cplusplus
}
#endif

#endif // JOAN_H