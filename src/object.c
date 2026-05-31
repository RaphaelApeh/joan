#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "chuck.h"
#include "object.h"
#include "helper.h"


Object NoneObj = {0};
Object TrueObj = {0};

static InternEntry* intern_pool[INTER_SIZE];

static inline bool ObjIntCmp(Object* key1, Object* key2)
{
    double k1 = tonumber(key1), k2 = tonumber(key2);
    return k1 == k2;
}

static inline bool ObjStrCmp(Object* key1, Object* key2)
{
    return key1->str->hash == key2->str->hash;
}

static inline bool ObjectBoolCmp(Object* key1, Object* key2)
{
    return key1->o_bool == key2->o_bool;
}

static bool objEqual(Object* obj1, Object* obj2)
{
    if (obj1->kind != obj2->kind)
        return false;
    switch (obj1->kind)
    {
        case INT_TYPE:  return obj1->o_int == obj2->o_int;
        case BOOL_TYPE: return obj1->o_bool == obj2->o_bool;
        case STR_TYPE: return obj1->str->hash == obj2->str->hash;
        default: return false;
    }
}

static uint64_t hashObject(Object* obj)
{
    switch (obj->kind)
    {
        case INT_TYPE:
            return (uint64_t)obj->o_int;
        case BOOL_TYPE:
            return obj->o_bool;
        case FLOAT_TYPE:
            return obj->o_float;
        case STR_TYPE:
            return obj->str->hash;
        default:
            printf("Unsupported Type\n");
            return 0;
    }
}

Object* obj_new(ObjectType kind)
{
    Object* obj = malloc(sizeof(Object));
    obj->kind = kind;
    return obj;
}

Object* obj_int(long o_int)
{
    if (o_int <= 0)
    {
        NoneObj.kind = INT_TYPE;
        NoneObj.o_int = o_int;
        return &NoneObj;
    }
    Object* obj = obj_new(INT_TYPE);
    obj->o_int = o_int;
    return obj;
}

Object* obj_string(char* str)
{
    Object* obj = obj_new(STR_TYPE);
    ObjString* strObj = malloc(sizeof(ObjString));
    *strObj = STR_OBJ(str); // TODO
    // strObj->str = strdup(str);
    // strObj->hash = djb2_hash(str);
    // strObj->len = strlen(str);
    obj->str = strObj;
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

void SetObject(Object* hm, Object* key, Object* value)
{
    if (NULL == hm || NULL == key || NULL == value)
        return;
    if (hm->hashmap->size > hm->hashmap->capacity)
        RESIZE_DOBJ(hm->hashmap);
    uint64_t hash = hashObject(key);
    hm->hashmap->items[hm->hashmap->size++] = HM_OBJ(key, value);
}
ObjHM* GetObject(Object* hm, Object* obj)
{
    for (int i = 0; i < hm->hashmap->size; ++i)
    {
        ObjHM* ele  = hm->hashmap->items[i];
        if (objEqual(ele->key, obj) && ele->hash == hashObject(obj))
            return ele;
    }
    return NULL;
}

void SetHmObject(Object* hm, Object* key, Object* value)
{
    ObjHM* obj_hm;
    if (hm == NULL || key == NULL || value == NULL)
        return;
    if (hm->kind != HASHMAP_TYPE)
        return;
    if ((obj_hm = GetObject(hm, key)) != NULL)
    {
        obj_hm->value = value;
        return;
    }
    if (hm->hashmap->size >= hm->hashmap->capacity)
        RESIZE_DOBJ(hm->hashmap);
    hm->hashmap->items[hm->hashmap->size++] = HM_OBJ(key, value);
}

ObjHM* HM_OBJ(Object* key, Object* value)
{
    ObjHM* hm = malloc(sizeof(ObjHM));
    hm->key = key;
    hm->value = value;
    hm->hash = hashObject(key);
    return hm;
}

ObjField* FieldObj(char* field_name, Object* field_value)
{
    ObjField* obj = malloc(sizeof(ObjField));
    obj->field_name = strdup(field_name);
    obj->field_value = field_value;
    return obj;
}


Object* obj_enum(char* ident, char** fields, int count)
{
    J_DArray_Obj* jd_obj = malloc(sizeof(J_DArray_Obj));
    jd_obj->size = 0;
    jd_obj->capacity = count;
    jd_obj->items = malloc(sizeof(ObjHM *) * count);
    for (int i = 0; i < count; ++i)
    {
        jd_obj->items[jd_obj->size++] = FieldObj(fields[i], obj_int(i));
    }
    Object* enumObj = obj_new(ENUM_TYPE);
    enumObj->JEnum = malloc(sizeof(JEnumObj));
    enumObj->JEnum->fields = jd_obj;
    enumObj->JEnum->ident = ident;
    return enumObj;
}

Object* obj_none(void)
{
    //Probably not the best way to do it.
    NoneObj.kind = NONE_TYPE;
    NoneObj.o_int = 0;
    return &NoneObj;
}

Object* internObject(Object* obj)
{
    if (obj->kind == ENUM_TYPE || obj->kind == FUNCTION_TYPE || obj->kind == HASHMAP_TYPE || obj->kind == NATIVE_TYPE || obj->kind == NONE_TYPE)
        return obj;
    uint64_t hash = hashObject(obj);
    size_t idx = hash % INTER_SIZE;
    InternEntry* entry = intern_pool[idx];
    
    while(entry)
    {
        if (objEqual(entry->obj, obj))
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

Object* obj_function(Chuck* chuck, char** params, int arity, char* name)
{
    Object* obj = obj_new(FUNCTION_TYPE);
    ObjFunction* fn = malloc(sizeof(ObjFunction));
    fn->arity = arity;
    fn->chuck = chuck;
    fn->name = name;
    fn->params = params;
    obj->fn = fn;
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
static void print_array(Object* obj)
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

static void print_hashmap(Object* obj)
{
    fprintf(stderr, "#{");
    for (int i = 0; i < obj->hashmap->size; ++i)
    {
        ObjHM* hm = obj->hashmap->items[i];
        print_object(hm->key);
        putchar(':');
        print_object(hm->value);
        if (i < obj->hashmap->size - 1)
            fprintf(stderr, ", ");
    }
    putchar('}');
}

IterObject* ObjectIter(unsigned int capacity)
{
    IterObject* iter = malloc(sizeof(IterObject));
    if (capacity <= 0)
    {
        capacity = 100;
    }
    iter->items = malloc(sizeof(Object *) * capacity);
    iter->capacity = 100;
    iter->count = 0;
    return iter;
}

void print_object(Object* obj)
{
    if (NULL == obj) return;
    
    switch (obj->kind)
    {
        case INT_TYPE:
            fprintf(stderr, "%llu", obj->o_int); break;
        case STR_TYPE:
            fprintf(stderr, "%s", (obj->str->len != 0) ? obj->str->chars : "None");
            break;
        case BOOL_TYPE:
            fprintf(stderr, (obj->o_bool) ? "true": "false"); break;
        case FLOAT_TYPE:
            fprintf(stderr, "%.15g", obj->o_float); break;
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
            fprintf(stderr, "<Enum '%s' fields(%d) >", obj->JEnum->ident, obj->JEnum->fields->size);
            break;
        case FUNCTION_TYPE:
            //TODO
            break;
        case ITER_TYPE:
            fprintf(stderr, "<iter <%llu> at %p>", obj->iter->count, obj->iter);
            break;
        case NATIVE_TYPE:
            fprintf(stderr, "<function <%s> at %p>", obj->o_nativefn->fnName, obj->o_nativefn);
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
        return obj->str->len != 0;
    case NATIVE_TYPE:
    case FUNCTION_TYPE:
        return true;
    default:
        return false;
    }
}