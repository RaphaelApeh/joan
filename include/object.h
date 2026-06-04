#ifndef JOAN_OBJECT_H
#define JOAN_OBJECT_H
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "helper.h"
#include "env.h"

typedef struct Chuck Chuck;

typedef struct JnObject JnObject;

#define JNSTR_OBJ(s) (JnStringObject){.chars = strdup((s)), .len = strlen((s)), .hash = djb2_hash((s))}

#define JN_RETURN_NONE jn_obj_none()

#define JN_OBJ_PUSH(arr, obj)

#define INTER_SIZE 1024

typedef long long JnIntObject;
typedef double JnFloatObject;
typedef bool JnBoolObject;

typedef JnObject* (* NativeFn) (JnObject** argv, size_t argc);
typedef JnObject* (* MethodFn) (JnObject* self, JnObject** argv, size_t argc);

typedef struct {
    char* chars;
    unsigned long hash;
    long len;
} JnStringObject;

typedef struct {
    NativeFn fn;
    char* fnName;
} JnNativeObject;

typedef struct {
    JnObject** items;
    size_t count;
    size_t capacity;
} JnIterObject;

typedef struct {
    JnObject* key;
    JnObject* value;
    uint64_t hash;
} JnHashmapObject;

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

typedef struct {
    Chuck* chuck;
    char** params;
    int arity;
    char* name;
} JnFunctionObject;

typedef struct {
    JnObject** items;
    size_t size;
    size_t capacity;
} JnArrayObject;

typedef struct InternEntry {
    JnObject* obj;
    struct InternEntry* next;
} InternEntry;

typedef struct JnObject{
    union
    {
        JnStringObject* str;
        JnArrayObject* arr;
        JnFunctionObject* fn;
        JnIterObject* iter;
        J_DArray_Obj* hashmap;
        JnNativeObject* native_fn;
        JnIntObject int32;
        JnFloatObject float32;
        JnBoolObject bool8;
    };
    JnObject* next;
    JnTypeObject type;
} JnObject;

//JnObject
JnObject* jn_obj_new(JnTypeObject kind);
JnObject* jn_obj_int(long o_int);
JnObject* jn_obj_string(char* str);
JnObject* jn_obj_none(void);
JnObject* jn_obj_bool(bool o_bool);
JnObject* jn_obj_float(double o_float);
JnObject* jn_obj_function(Chuck* chuck, char** params, int arity, char* name);

// JnObject* obj_enum(char* ident, char** fields, int count);

JnObject* jn_intern_obj(JnObject* obj);

// HASHMAP functions
JnObject* obj_hashmap(J_DArray_Obj* jd_obj);
void hashmap_set(JnObject* hm, JnObject* key, JnObject* value);
// ObjHM* hashmap_get(JnObject* hm, JnObject* obj);
// ObjHM* hashmap_init(JnObject* key, JnObject* value);

// array_t* init_array(void);
// void array_add(array_t* arr, JnObject* obj);
bool is_truthy(JnObject* obj);
void print_JnObject(JnObject* obj);
#endif