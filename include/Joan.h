/* =======================================================
 Joan.h
 Full Public C API for Joan Programming Language.
==========================================================
*/

#ifndef JOAN_H
#define JOAN_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define JOAN_VERSION_MAJOR 0
#define JOAN_VERSION_MINOR 6
#define JOAN_VERSION_PATCH 2

#define JOAN_VERSION "0.6.2"

#ifdef _WIN32
    #ifdef RB_BUILD_DLL
    #define JN_API __declspec(dllexport)
    #else
    #define JN_API
    #endif
#else
    #define JN_API
#endif

// Object Type
typedef enum{
    NONE_TYPE = 0,
    STR_TYPE,
    INT_TYPE,
    BOOL_TYPE,
    FLOAT_TYPE,
    ARRAY_TYPE,
    HASHMAP_TYPE,
    FUNCTION_TYPE,
    NATIVE_TYPE,
    ITER_TYPE,
    INSTANCE_TYPE,
    MODULE_TYPE,
    ENUM_TYPE,
} JnTypeObject;

typedef struct JnObject JnObject;
typedef struct JnVM JnVM;
typedef struct J_State J_State;
typedef struct J_Context J_Context;
typedef JnObject* (*Jn_CFunction)(JnObject** argv, size_t argc);
typedef void* (*JnObject_Alloc)(size_t size, JnTypeObject type);
typedef struct JN_Args JN_Args;
typedef struct Jn_CModule Jn_CModule;
extern J_State Jn_globalState;

JN_API void Jn_repl(void);

// Register native function
JN_API void Jn_register(const char* name, const char* doc, Jn_CFunction fn);

// Load builtin function
JN_API void Jn_load_Cfunctions(void);
// Call user-define functions
JN_API JnObject* Jn_call_fn(char* fn_name, JN_Args* args);
// Jn_exec_from_file(FILE* fptr);
JN_API void Jn_program_init(void);
JN_API int Jn_exec_program(char* source);
// Main Execution function
JN_API void Jn_execute_main(char* filepath);
// Execute for FILE ptr.
JN_API int Jn_exec_from_file(FILE* fptr);

JN_API void Jn_program_close(void);

#ifdef __cplusplus
}
#endif
#endif