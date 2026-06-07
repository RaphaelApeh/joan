#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>

#include "object.h"
#include "helper.h"
#include "vm.h"

JnObject NoneObj = {0};

static InternEntry* intern_pool[INTER_SIZE];

static inline bool ObjIntCmp(JnObject* key1, JnObject* key2)
{
    double k1 = tonumber(key1), k2 = tonumber(key2);
    return k1 == k2;
}

static inline bool ObjStrCmp(JnObject* key1, JnObject* key2)
{
    return key1->str->hash == key2->str->hash;
}

static inline bool ObjectBoolCmp(JnObject* key1, JnObject* key2)
{
    // return key1->bool8 == key2->bool2;
}

static bool jn_obj_equal(JnObject* obj1, JnObject* obj2)
{
    // if (obj1->type != obj2->type)
    //     return false;
    // switch (obj1->type)
    // {
    //     case INT_TYPE:  return obj1->int32 == obj2->int32;
    //     case BOOL_TYPE: return obj1->bool8 == obj2->bool8;
    //     case FLOAT_TYPE: return obj1->float32 == obj2->float32;
    //     case STR_TYPE: return obj1->str->hash == obj2->str->hash;
    //     default: return false;
    // }
}

static uint64_t hash_object(JnObject* obj)
{
    // switch (obj->type)
    // {
    //     case INT_TYPE:
    //         return (uint64_t)obj->int32;
    //     case BOOL_TYPE:
    //         return obj->bool8;
    //     case FLOAT_TYPE:
    //         return obj->float32;
    //     case STR_TYPE:
    //         return obj->str->hash;
    //     default:
    //         printf("Unsupported Type\n");
    //         return 0;
    // }
}

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

JnObject* jn_obj_hashmap(J_DArray_Obj* jd_obj)
{
    JnObject* obj =  jn_obj_new(HASHMAP_TYPE);
    // obj->hashmap = jd_obj;
    return obj;
}

void hashmap_set(JnObject* hm, JnObject* key, JnObject* value)
{
    // if (NULL == hm || NULL == key || NULL == value)
    //     return;
    // ObjHM* obj = hashmap_get(hm, key);
    // if (obj != NULL)
    // {
    //     obj->value = value;
    //     return;
    // }
    // if (hm->hashmap->size >= hm->hashmap->capacity)
    // {
    //     hm->hashmap->capacity *= 2;
    //     RESIZE_DOBJ(hm->hashmap);
    // }
    // hm->hashmap->items[hm->hashmap->size++] = hashmap_init(key, value);
}

// ObjHM* hashmap_get(JnObject* hm, JnObject* obj)
// {
//     for (int i = 0; i < hm->hashmap->size; ++i)
//     {
//         ObjHM* ele  = hm->hashmap->items[i];
//         if (jn_obj_equal(ele->key, obj) && ele->hash == hash_object(obj))
//             return ele;
//     }
//     return NULL;
// }

// ObjHM* hashmap_init(JnObject* key, JnObject* value)
// {
//     ObjHM* hm = malloc(sizeof(ObjHM));
//     hm->key = key;
//     hm->value = value;
//     hm->hash = hash_object(key);
//     return hm;
// }


JnObject* jn_obj_none(void)
{
    NoneObj.type = NONE_TYPE;
    NoneObj.int32 = 0;
    return &NoneObj;
}

JnObject* jn_intern_obj(JnObject* obj)
{
    if (obj->type == ENUM_TYPE || obj->type == FUNCTION_TYPE || obj->type == HASHMAP_TYPE || obj->type == NATIVE_TYPE || obj->type == NONE_TYPE)
        return obj;
    uint64_t hash = hash_object(obj);
    size_t idx = hash % INTER_SIZE;
    InternEntry* entry = intern_pool[idx];
    
    while(entry)
    {
        if (jn_obj_equal(entry->obj, obj))
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

static void print_array(JnObject* obj)
{
    if (NULL == obj && obj->type != ARRAY_TYPE)
        return;
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
    // for (int i = 0; i < obj->hashmap->size; ++i)
    // {
    //     ObjHM* hm = obj->hashmap->items[i];
    //     print_object(hm->key);
    //     putchar(':');
    //     print_object(hm->value);
    //     if (i < obj->hashmap->size - 1)
    //         fprintf(stderr, ", ");
    // }
    putchar('}');
}

// Jn* ObjectIter(unsigned int capacity)
// {
//     IterJnObject* iter = malloc(sizeof(IterObject));
//     if (capacity <= 0)
//     {
//         capacity = 100;
//     }
//     iter->items = malloc(sizeof(Object *) * capacity);
//     iter->capacity = 100;
//     iter->count = 0;
//     return iter;
// }

void print_JnObject(JnObject* obj)
{
    if (NULL == obj) return;
    
    switch (obj->type)
    {
        case INT_TYPE:
            fprintf(stderr, "%llu", obj->int32); break;
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