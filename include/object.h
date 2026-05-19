#ifndef OBJECT_H
#define OBJECT_H
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "env.h"

typedef struct array_t array_t;
typedef struct Object Object;

#define STR_OBJ(s)(String8{.str = (s), .len = strlen((s))})

typedef struct {
    char* str;
    size_t len;
} String8;

typedef enum{
    NONE_TYPE = -1,
    STR_TYPE = 0,
    INT_TYPE,
    BOOL_TYPE,
    FLOAT_TYPE,
    ARRAY_TYPE,
    DICT_TYPE,
    FUNCTION_TYPE,
} ObjectType;

typedef struct array_t
{
    Object** items;
    size_t count;
    size_t capacity;
} array_t;


typedef struct Object{
    ObjectType kind;
    union{
        char* o_string;
        int o_int;
        bool o_bool;
        double o_float;
        array_t* o_array;
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