#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "object.h"
#include "helper.h"



Object* obj_new(ObjectType kind)
{
    Object* obj = malloc(sizeof(Object));
    obj->kind = kind;
    return obj;
}

Object* obj_int(long o_int)
{
    Object* obj = obj_new(INT_TYPE);
    obj->o_int = o_int;
    return obj;
}

Object* obj_string(char* str)
{
    Object* obj = obj_new(STR_TYPE);
    obj->o_string = strdup(str);
    return obj;
}

Object* obj_bool(bool o_bool)
{
    Object* obj = obj_new(BOOL_TYPE);
    obj->o_bool = o_bool;
    return obj;
}

Object* obj_float(double o_float)
{
    Object* obj = obj_new(FLOAT_TYPE);
    obj->o_float = o_float;
    return obj;
}

Object* obj_none(void)
{
    Object* obj = obj_new(NONE_TYPE);
    obj->o_int = -1;
    return obj;
}

array_t* init_array(void)
{
    array_t* arr = malloc(sizeof(arr));
    arr->count = 0;
    arr->capacity = 100;
    arr->items = malloc(sizeof(Object *) * 100);
    return arr;
}

void array_add(array_t* arr, Object* obj)
{
    if (arr->count >= arr->capacity)
    {
        arr->capacity *= 2;
        arr->items = realloc(arr->items, sizeof(Object *) * arr->capacity);
    }
    arr->items[arr->count++] = obj;
}
void print_array(Object* obj)
{
    if (NULL == obj && obj->kind != ARRAY_TYPE)
        return;
    fprintf(stderr, "[");
    for (size_t i = 0; i < obj->o_array->count; i++)
    {
        print_object(obj->o_array->items[i]);
        if (i < obj->o_array->count - 1)
            fprintf(stderr, ", ");
    }
    fprintf(stderr, "]");
}
void print_object(Object* obj)
{
    if (NULL == obj) return;
    
    switch (obj->kind)
    {
        case INT_TYPE:
            fprintf(stderr, "%d", obj->o_int); break;
        case STR_TYPE:
            fprintf(stderr, "\"%s\"", (obj->o_string[0] != '\0') ? obj->o_string : "None");
            break;
        case BOOL_TYPE:
            fprintf(stderr, (obj->o_bool) ? "true": "false"); break;
        case FLOAT_TYPE:
            fprintf(stderr, "%.15g", obj->o_float); break;
        case ARRAY_TYPE:
            print_array(obj);
            break;
        case NONE_TYPE:
            fprintf(stderr, "None");
            break;
        case FUNCTION_TYPE:
            //TODO
            break;
        default:
            fprintf(stderr, "undefine");
            break;
    }
}

bool is_truthy(Object* obj)
{
    if (!obj) return false;
    switch (obj->kind)
    {
    case BOOL_TYPE:
        return obj->o_bool;
    case INT_TYPE:  
        return obj->o_int != 0;
    case FLOAT_TYPE:
        return obj->o_float != 0;
    case STR_TYPE:
        return obj->o_string[0] != '\0';
    
    default:
        return false;
    }
}