#ifndef OBJECT_H
#define OBJECT_H
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "env.h"

typedef struct array_t array_t;
typedef struct Object Object;

#define STR_OBJ(s)(ObjString{.str = (s), .len = strlen((s))})

typedef Object* (* NativeFn) (Object** argv, size_t argc);

typedef struct {
    char* str;
    size_t len;
} ObjString;


typedef struct {
    NativeFn fn;
    char* fnName;
} NativeObject;

typedef struct {
    size_t start;
    size_t end;
} RangeObject;

typedef enum{
    NONE_TYPE = -1,
    STR_TYPE = 0,
    INT_TYPE,
    BOOL_TYPE,
    FLOAT_TYPE,
    ARRAY_TYPE,
    DICT_TYPE,
    FUNCTION_TYPE,
    NATIVE_TYPE,
    ITER_TYPE,
    INSTANCE_TYPE,
    ENUM_TYPE,
} ObjectType;

typedef struct array_t
{
    size_t count;
    size_t capacity;
    Object** items;
} array_t;


typedef struct Object{
    ObjectType kind;
    union{
        char* o_string; // TODO: change to ObjString o_string;
        int o_int; // TODO: change to ....(something)
        bool o_bool;
        double o_float;
        array_t* o_array;
        NativeObject* o_nativefn;
    };
} Object;

//Object
Object* obj_new(ObjectType kind);
Object* obj_int(long o_int);
Object* obj_string(char* str);
Object* obj_none(void);
Object* obj_bool(bool o_bool);
Object* obj_float(double o_float);

array_t* init_array(void);
void array_add(array_t* arr, Object* obj);
bool is_truthy(Object* obj);
void print_object(Object* obj);
#endif