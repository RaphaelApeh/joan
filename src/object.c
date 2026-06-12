#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include "Joan.h"
#include "object.h"
#include "helper.h"
#include "vm.h"

JnObject NoneObj = {0};

static InternEntry* intern_pool[JN_INTER_SIZE];


JnObject* jn_obj_new(JnTypeObject type)
{
    J_State* state = Jn_get_state();
    JnObject* obj = state->alloc_fn(sizeof(JnObject), type);
    assert(obj != NULL);
    return obj;
}

JnObject* jn_obj_int(long int32)
{
    if (int32 <= 0)
    {
        NoneObj.type = INT_TYPE;
        NoneObj.int32 = int32;
        return &NoneObj;
    }
    JnObject* obj =  jn_obj_new(INT_TYPE);
    obj->int32 = int32;
    return obj;
}

JnObject* jn_obj_string(char* str)
{
    JnObject* obj =  jn_obj_new(STR_TYPE);
    JnStringObject* strObj = malloc(sizeof(JnStringObject));
    *strObj = JNSTR_OBJ(str);
    obj->str = strObj;
    return obj;
}

JnObject* jn_obj_char(char c)
{
    JnObject* obj =  jn_obj_new(CHAR_TYPE);
    obj->j_char = c;
    return obj;
}

JnObject* jn_obj_bool(bool bool8)
{
    JnObject* obj =  jn_obj_new(BOOL_TYPE);
    obj->bool8 = bool8;
    return obj;
}

JnObject* jn_obj_float(double float32)
{
    JnObject* obj =  jn_obj_new(FLOAT_TYPE);
    obj->float32 = float32;
    return obj;
}


JnObject* jn_obj_none(void)
{
    NoneObj.type = NONE_TYPE;
    NoneObj.int32 = 0;
    return &NoneObj;
}

JnObject* jn_obj_iter(JnObject* iter)
{
    JnObject* obj = jn_obj_new(ITER_TYPE);
    JnIterObject* i = malloc(sizeof(JnIterObject)); // TODO
    i->obj = iter;
    i->index = 0;
    return obj;
}

static uint64_t hash_object(JnObject* obj)
{
    switch (obj->type)
    {
        case INT_TYPE:
            return (uint64_t)obj->int32;
        case BOOL_TYPE:
            return obj->bool8;
        case FLOAT_TYPE:
            return obj->float32;
        case STR_TYPE:
            return obj->str->hash;
        case CHAR_TYPE:
            return ( int )obj->j_char;
        default:
            return 0;
    }
}

JnObject* jn_intern_obj(JnObject* obj)
{
    if (obj->type == ENUM_TYPE || obj->type == FUNCTION_TYPE || obj->type == HASHMAP_TYPE || obj->type == NATIVE_TYPE || obj->type == NONE_TYPE)
        return obj;
    uint64_t hash = hash_object(obj);
    size_t idx = hash % JN_INTER_SIZE;
    InternEntry* entry = intern_pool[idx];
    
    while(entry)
    {
        if (entry->obj->type == obj->type && hash_object(entry->obj) == hash_object(obj))
        {
            return entry->obj;
        }
        entry = entry->next;
    }

    InternEntry* new_entry = malloc(sizeof(InternEntry));
    new_entry->obj = obj;
    new_entry->next = intern_pool[idx];
    intern_pool[idx] = new_entry;
    return obj;
}

JnObject* obj_function(Chuck* chuck, char** params, int arity, char* name)
{
    JnObject* obj =  jn_obj_new(FUNCTION_TYPE);
    JnFunctionObject* fn = malloc(sizeof(JnFunctionObject));
    fn->arity = arity;
    fn->chuck = chuck;
    fn->name = name;
    fn->params = params;
    obj->fn = fn;
    return obj;
}

JnObject* jn_obj_enum(Jn_Hashmap* map, char** fields, int count)
{
    assert(map != NULL);
    for (int i = 0; i < count; ++i)
    {
        // TODO
    }
}

JnObject* jn_obj_array_get(JnArrayObject* arr, int idx)
{
    assert(arr != NULL);
    if (idx < 0)
    {
        idx += arr->size;
    }
    if (idx >= arr->size)
        return NULL;
    return arr->items[idx];
}

static void print_array(JnObject* obj)
{
    assert(obj != NULL);
    fprintf(stderr, "[");
    for (size_t i = 0; i < obj->arr->size; i++)
    {
        print_JnObject(obj->arr->items[i]);
        if (i < obj->arr->size - 1)
            fprintf(stderr, ", ");
    }
    fprintf(stderr, "]");
}

static void print_hashmap(JnObject* obj)
{
    fprintf(stderr, "#{");
    for (int i = 0; i < obj->hashmap->size; ++i)
    {
        Jn_HashEntry* hm = &obj->hashmap->buckets[i];
        print_JnObject(hm->key);
        putchar(':');
        print_JnObject(hm->value);
        if (i < obj->hashmap->size - 1)
            fprintf(stderr, ", ");
    }
    putchar('}');
}


char* Jn_object_cstring(JnObject* obj)
{
    assert(obj != NULL);
    static char buffer[0xff];
    switch (obj->type)
    {
        case INT_TYPE:
            snprintf(buffer, sizeof(buffer), "%lld", obj->int32);
            goto buf;
        case STR_TYPE:
            return strdup(obj->str->chars);
        case FLOAT_TYPE:
            snprintf(buffer, sizeof(buffer), "%.15g", obj->float32);
            goto buf;
        case BOOL_TYPE:
            snprintf(buffer, sizeof(buffer), "%s", obj->bool8 ? "true": "false");
            goto buf;
        case CHAR_TYPE:
            snprintf(buffer, sizeof(buffer), "%c", JN_AS_CHAR(obj));
            goto buf;
        default:
            return strdup("<error>"); // TODO
    }
    buf:
        return strdup(buffer);
}

void print_JnObject(JnObject* obj)
{
    if (NULL == obj) return;
    
    switch (obj->type)
    {
        case INT_TYPE:
            fprintf(stderr, "%lld", obj->int32); break;
        case CHAR_TYPE:
            fprintf(stderr, "%c", obj->j_char); break;
        case STR_TYPE:
            fprintf(stderr, "%s", (obj->str->len != 0) ? obj->str->chars : "None");
            break;
        case BOOL_TYPE:
            fprintf(stderr, (obj->bool8) ? "true": "false"); break;
        case FLOAT_TYPE:
            fprintf(stderr, "%.15g", obj->float32); break;
        case ARRAY_TYPE:
            print_array(obj);
            break;
        case HASHMAP_TYPE:
            print_hashmap(obj);
            break;
        case NONE_TYPE:
            fprintf(stderr, "None");
            break;
        case ENUM_TYPE:
            fprintf(stderr, "<Enum>");
            break;
        case FUNCTION_TYPE:
            //TODO
            break;
        case ITER_TYPE:
            break;
        case NATIVE_TYPE:
            fprintf(stderr, "<function <%s> at %p>", obj->native_fn->fnName, obj->native_fn);
            break;
        default:
            fprintf(stderr, "undefine");
            break;
    }
}

bool is_truthy(JnObject* obj)
{
    if (!obj) return false;
    switch (obj->type)
    {
    case BOOL_TYPE:
        return obj->bool8;
    case INT_TYPE:  
        return obj->int32 != 0;
    case FLOAT_TYPE:
        return obj->float32 != 0;
    case STR_TYPE:
        return obj->str->len != 0;
    case NATIVE_TYPE:
    case ENUM_TYPE:
    case FUNCTION_TYPE:
        return true;
    default:
        return false;
    }
}