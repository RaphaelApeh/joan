#ifndef OBJECT_H
#define OBJECT_H
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "helper.h"
#include "env.h"

typedef struct Chuck Chuck;

typedef struct array_t array_t;
typedef struct Object Object;

#define STR_OBJ(s) (ObjString){.str = (s), .len = strlen((s)), .hash = djb2_hash((s))}

#define pushItem(arr, obj) do{\
    if ((arr)->count >= (arr)->capacity)\
    {\
        (arr)->capacity *= 2;\
        (arr)->items = realloc((arr)->items, sizeof(Object *) * (arr)->capacity);\
    }\
    (arr)->items[(arr)->count++] = (obj); \
}while(false)

typedef Object* (* NativeFn) (Object** argv, size_t argc);
typedef Object* (* MethodFn) (Object* self, Object** argv, size_t argc);

typedef struct {
    char* str;
    unsigned long hash;
    long len;
} ObjString;


typedef struct {
    NativeFn fn;
    char* fnName;
} NativeObject;

typedef struct {
    size_t start;
    size_t end;
} RangeObject;

typedef struct {
    Object** items;
    size_t count;
    size_t capacity;
} IterObject;

typedef enum{
    NONE_TYPE = 0,
    STR_TYPE,
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

typedef struct {
    Chuck* chuck;
    char** params;
    int arity;
    char* name;
} ObjFunction;


typedef struct {
    Object** items;
    size_t size;
    size_t capacity;
} ObjArray;

typedef struct array_t
{
    size_t count;
    size_t capacity;
    Object** items;
} array_t;


typedef struct Object{
    ObjectType kind;
    union
    {
        ObjString* str;
        ObjArray* arr;
        array_t* o_array;
        NativeObject* o_nativefn;
        IterObject* iter;
        ObjFunction* fn;
        double o_float;
        bool o_bool;
        int o_int; // TODO: change to ....(something)
    };
} Object;

//Object
Object* obj_new(ObjectType kind);
Object* obj_int(long o_int);
Object* obj_string(char* str);
Object* obj_none(void);
Object* obj_bool(bool o_bool);
Object* obj_float(double o_float);
IterObject* ObjectIter(unsigned int capacity);
Object* obj_function(Chuck* chuck, char** params, int arity, char* name);

array_t* init_array(void);
void array_add(array_t* arr, Object* obj);
bool is_truthy(Object* obj);
void print_object(Object* obj);
#endif