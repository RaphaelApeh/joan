#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include "ast.h"
#include "Joan.h"
#include "object.h"
#include "opcode.h"
#include "emit.h"
#include "helper.h"
#include "vm.h"
#include "gc.h"


#ifndef C_STRING_H
#include "optionals/c_string.h"
#endif

#define JN_INTERN_OBJECT(obj) jn_obj_intern(state, (obj))
#define LONG_HEX_NUM 0xbf58476d1ce4e5b9ULL
#define LONG_HEX_NUM2 0x94d049bb133111ebULL
#define LONG_HEX_NUM3 0x9e3779b97f4e7c15ULL
JnObject NoneObj = {0};

JN_INLINE uint64_t hash_mix(uint64_t x)
{
    x ^=  x >> 30;
    x *= LONG_HEX_NUM;
    x ^= x >> 27;
    x *= LONG_HEX_NUM2;
    x ^= x >> 31;
    return x;
}

JN_INLINE uint64_t hash_combine(uint64_t a, uint64_t b)
{
    return hash_mix(a ^ (b + LONG_HEX_NUM3 + 
            (a << 6) + (a >> 2)));
}

JnObject* jn_obj_new(Jn_State* state, JnTypeObject type)
{
    JnObject* obj = gc_alloc(state, sizeof(JnObject), type);
    assert(obj != NULL);
    return obj;
}

JnObject* jn_obj_int(Jn_State* state, long int_val)
{
    JnObject* obj =  jn_obj_new(state, JN_INT_TYPE);
    obj->int_val = int_val;
    return JN_INTERN_OBJECT(obj);
}

JnObject* jn_obj_string(Jn_State* state, char* str)
{
    JnObject* obj =  jn_obj_new(state, JN_STRING_TYPE);
    Jn_String* strObj = Jn_alloc(sizeof(Jn_String));
    Jn_String S_Obj = JNSTR_OBJ(str);
    strObj->chars = S_Obj.chars;
    strObj->hash = S_Obj.hash;
    strObj->len = S_Obj.len;
    obj->str = strObj;
    return JN_INTERN_OBJECT(obj);
}

JnObject* jn_obj_char(Jn_State* state, char c)
{
    JnObject* obj =  jn_obj_new(state, JN_CHAR_TYPE);
    obj->j_char = c;
    return JN_INTERN_OBJECT(obj);
}

JnObject* jn_obj_bool(Jn_State* state, bool bool_val)
{
    JnObject* obj =  jn_obj_new(state, JN_BOOL_TYPE);
    obj->bool_val = bool_val;
    return JN_INTERN_OBJECT(obj);
}

JnObject* jn_obj_float(Jn_State* state, double float_val)
{
    JnObject* obj =  jn_obj_new(state, JN_FLOAT_TYPE);
    obj->float_val = float_val;
    return JN_INTERN_OBJECT(obj);
}


JnObject* jn_obj_none(void)
{
    NoneObj.type = JN_NONE_TYPE;
    NoneObj.int_val = 0;
    return &NoneObj;
}

JnObject* jn_obj_iter(Jn_State* state, JnObject* obj)
{
    JnObject* new_obj = jn_obj_new(state, JN_ITER_TYPE);
    Jn_Iter* iter = malloc(sizeof(Jn_Iter)); // TODO
    iter->obj = obj;
    iter->index = 0;
    new_obj->iter = iter;
    return JN_INTERN_OBJECT(new_obj);
}

int64_t range_len(JnRange* r)
{
    if (r->step > 0)
    {
        if (r->start >= r->stop)
            return 0;
        return (r->stop - r->start + r->step - 1) / r->step;
    }
    if (r->start <= r->stop)
        return 0;
    return (r->start - r->stop - r->step - 1) / (-r->step);
}

int64_t range_at(JnRange* r, int64_t idx)
{
    return r->start + (idx * r->step);
}

JnObject* jn_obj_range(Jn_State* state, int64_t start, int64_t stop, int64_t step)
{
    JnObject* obj = jn_obj_new(state, JN_RANGE_TYPE);
    obj->range.start = start;
    obj->range.stop = stop;
    obj->range.step = (step == 0) ? 1 : step;
    return JN_INTERN_OBJECT(obj);
}

JnObject* jn_obj_arg(Jn_State* state, JnObject** args, char** arg_names, size_t count)
{
    JnObject* obj = jn_obj_new(state, JN_ARG_TYPE);
    obj->arg.args = args;
    obj->arg.count = count;
    obj->arg.arg_names = arg_names;
    return JN_INTERN_OBJECT(obj);
}

JnObject* jn_obj_error(Jn_State* state, int type, char* msg, ...)
{
    char buffer[1 << 10];
    JnObject* obj = jn_obj_new(state, JN_ERROR_TYPE);
    obj->expection.type = type;
    va_list arg; va_start(arg, msg);
    vsnprintf(buffer, sizeof(buffer), msg, arg);
    va_end(arg);
    // defualt values
    obj->expection.error_msg = strdup(buffer);
    obj->expection.col = 0;
    obj->expection.line = 0;
    obj->expection.filename = NULL;
    obj->expection.var_name = NULL;
    return JN_INTERN_OBJECT(obj);
}


static void strip(const char* str)
{
    while (*str)
    {
        if (*str == '\\' && *str)
            str += 2;
        str++;
    }
}


char* jn_obj_cstring(JnObject* obj)
{
    if (!obj) return NULL;
    static char buffer[256];
    switch (JN_OBJ_TYPE(obj))
    {
    case JN_INT_TYPE:
        snprintf(buffer, 256, "%lld", JN_AS_INT(obj));
        break;
    case JN_FLOAT_TYPE:
        snprintf(buffer, 256, "%15g", JN_AS_FLOAT(obj));
        break;
    case JN_BOOL_TYPE:
        snprintf(buffer, 256, "%s", JN_AS_BOOL(obj) ? "true" : "false");
        break;
    default:
        assert(false && "Not yet Implemented.");
        break;
    }
    strip(buffer);
    return buffer;
}

JnObject* jn_obj_type(Jn_State* state, char* type_name, JnTypeObject type, Jn_CFunction fn)
{
    JnObject* obj = jn_obj_new(state, JN_OBJECT_TYPE);
    obj->type_val.typename = type_name;
    obj->type_val.type = type;
    obj->type_val.ctor = fn;
    return JN_INTERN_OBJECT(obj);
}

JnObject* jn_obj_method(Jn_State* state, JnObject* obj, JN_CMethod method)
{
    JnObject* new_obj = jn_obj_new(state, JN_METHOD_TYPE);
    new_obj->method.fn = method;
    new_obj->method.obj = obj;
    return JN_INTERN_OBJECT(new_obj);
}

char* jn_obj_to_string(JnObject* obj)
{
    if (!obj) return NULL;
    char* type = NULL;
    switch (obj->type)
    {
        case JN_ARRAY_TYPE:
            type = "Array"; break;
        case JN_INT_TYPE:
            type = "int"; break;
        case JN_STRING_TYPE:
            type = "string"; break;
        case JN_BOOL_TYPE:
            type = "bool"; break;
        case JN_CHAR_TYPE:
            type = "char"; break;
        case JN_HASHMAP_TYPE:
            type = "hashmap"; break;
        case JN_MODULE_TYPE:
            type = "module"; break;
        case JN_OBJECT_TYPE:
            type = (char *)obj->type_val.typename; break;
        default:
            type = "<object>"; break;
    }
    return strdup(type);
}


void jn_obj_reassign(JnObject* dest, JnObject* src)
{
    #define _SET_TYPE(t) dest->type = t
    assert(dest && src);
    switch (src->type)
    {
        case JN_INT_TYPE:
            _SET_TYPE(JN_INT_TYPE);
            dest->int_val = src->int_val;
            break;
        case JN_STRING_TYPE:
            _SET_TYPE(JN_STRING_TYPE);
            dest->str = src->str;
            break;
        case JN_BOOL_TYPE:
            _SET_TYPE(JN_BOOL_TYPE);
            dest->bool_val = src->bool_val;
            break;
        case JN_FLOAT_TYPE:
            _SET_TYPE(JN_FLOAT_TYPE);
            dest->float_val = src->float_val;
            break;
        case JN_CHAR_TYPE:
            _SET_TYPE(JN_CHAR_TYPE);
            dest->j_char = src->j_char;
            break;
        case JN_RANGE_TYPE:
            _SET_TYPE(JN_RANGE_TYPE);
            dest->range = src->range;
            break;
        case JN_ARRAY_TYPE:
            _SET_TYPE(JN_ARRAY_TYPE);
            dest->arr = src->arr;
            break;
        case JN_HASHMAP_TYPE:
            _SET_TYPE(JN_HASHMAP_TYPE);
            dest->hashmap = src->hashmap;
            break;
        case JN_FUNCTION_TYPE:
            _SET_TYPE(JN_FUNCTION_TYPE);
            dest->fn = src->fn;
            break;
        case JN_STRUCT_TYPE:
            _SET_TYPE(JN_STRUCT_TYPE);
            dest->struct_obj = src->struct_obj;
            break;
        case JN_INSTANCE_TYPE:
            _SET_TYPE(JN_INSTANCE_TYPE);
            dest->instance = src->instance;
            break;
        default:
            assert(false && "TODO");
    }
    #undef _SET_TYPE
}

JN_API JnObject* jn_obj_cfn(Jn_State* state, char* name, Jn_CFunction fn)
{
    JnObject* obj = jn_obj_new(state, JN_NATIVE_TYPE);
    obj->native_fn = Jn_alloc(sizeof(Jn_Native));
    obj->native_fn->fnName = strdup(name);
    obj->native_fn->fn = fn;
    return JN_INTERN_OBJECT(obj);
}

uint64_t Jn_object_hash(JnObject* obj)
{
    #define HASH_MIX_STRING(str)  hash_mix(djb2_hash((const unsigned char *)(str)))
    if (!obj) return 0;
    uint64_t h;
    switch (obj->type)
    {
        case JN_NONE_TYPE:
            return LONG_HEX_NUM3;
        case JN_INT_TYPE:
            return hash_mix((uint64_t)obj->int_val);
        case JN_BOOL_TYPE:
            return hash_mix(JN_AS_BOOL(obj));
        case JN_FLOAT_TYPE:
            union {
                double d;
                uint64_t u;
            } bits;
            bits.d = JN_AS_FLOAT(obj);
            return hash_mix(bits.u);
        case JN_STRING_TYPE:
            return hash_mix(JN_AS_STRING(obj)->hash);
        case JN_CHAR_TYPE:
            return hash_mix((unsigned char) JN_AS_CHAR(obj));
        case JN_RANGE_TYPE:
        {
            h = hash_mix(JN_AS_RANGE(obj)->start);
            h = hash_combine(h, hash_mix(JN_AS_RANGE(obj)->stop));
            h = hash_combine(h, hash_mix(JN_AS_RANGE(obj)->step));
            return h;          
        }
        case JN_ITER_TYPE:
        {
            h = Jn_object_hash(JN_AS_ITER(obj)->obj);
            return hash_combine(h, hash_mix(JN_AS_ITER(obj)->index));
        }
        case JN_MODULE_TYPE:
        {
            h = HASH_MIX_STRING(obj->module->name);
            return hash_combine(h, HASH_MIX_STRING(obj->module->path));
        }
        case JN_STRUCT_TYPE:
        {
            h = HASH_MIX_STRING(obj->struct_obj->name);
            for (int i = 0; i < obj->struct_obj->field_count; ++i)
            {
                h = hash_combine(h, HASH_MIX_STRING(obj->struct_obj->fields[i]));
            }
            return h;
        }
        case JN_METHOD_TYPE:
        {
            h = Jn_object_hash(obj->method.obj);
            return hash_combine(h, hash_mix((uintptr_t)obj->method.fn));
        }
        case JN_INSTANCE_TYPE:
        {
            h = Jn_object_hash(obj->instance->obj);
            return h;
        }
        case JN_FUNCTION_TYPE:
        {
            return HASH_MIX_STRING(obj->fn->name);
        }
        case JN_NATIVE_TYPE:
        {
            return hash_mix((uint64_t)(uintptr_t)obj->native_fn);
        }
        case JN_OBJECT_TYPE:
            return HASH_MIX_STRING(obj->type_val.typename);
        case JN_ARRAY_TYPE:
        {
            h = hash_mix(obj->arr->size);
            for (size_t i = 0; i < obj->arr->size; ++i)
            {
                h = hash_combine(h, Jn_object_hash(obj->arr->items[i]));
            }
            return h;
        } break;
        case JN_HASHMAP_TYPE:
        {
            h = hash_mix(obj->hashmap->size);
            for (size_t i = 0; i < obj->hashmap->size; ++i)
            {
                h = hash_combine(h, Jn_object_hash(obj->hashmap->buckets[i].key));
                h = hash_combine(h, Jn_object_hash(obj->hashmap->buckets[i].value));
            }
            return h;
        }
        default:
            return hash_mix((uintptr_t)obj);
    }
    #undef HASH_MIX_STRING
}

JnObject* jn_obj_intern(Jn_State* state, JnObject* obj)
{
    if (JN_IS_ARRAY(obj) || JN_IS_NATIVE(obj) || JN_IS_HASHMAP(obj) || JN_IS_STRUCT(obj))
        return obj;
    uint64_t hash = Jn_object_hash(obj);
    size_t idx = hash % JN_INTER_SIZE;
    JnInternEntry* entry = state->intern_pool[idx];
    
    while(entry)
    {
        if (entry->obj->type == obj->type && Jn_object_hash(entry->obj) == Jn_object_hash(obj))
        {
            return entry->obj;
        }
        entry = entry->next;
    }

    JnInternEntry* new_entry = malloc(sizeof(JnInternEntry));
    new_entry->obj = obj;
    new_entry->next = state->intern_pool[idx];
    state->intern_pool[idx] = new_entry;
    return obj;
}

bool jn_obj_equals(JnObject* obj, JnObject* other)
{
    if (!obj || !other) return false;
    return (
        obj->type == other->type && 
        Jn_object_hash(obj) == Jn_object_hash(other)
    );
}

JnObject* jn_obj_lambda(Jn_State* state, Jn_Node* expr, char** params, int arity, Jn_environ* env)
{
    Chuck* chuck = JN_ALLOC(sizeof(Chuck));
    chuck_init(chuck);
    chuck->env = env;
    compile(expr, chuck);
    if (expr->type == AST_BLOCK)
    {
        write_chuck_loc(chuck, OP_NONE, expr->line, expr->col);
    }
    write_chuck_loc(chuck, OP_RETURN, expr->line, expr->col);
    JnFunctionObject* fn = JN_ALLOC(sizeof(JnFunctionObject));
    fn->chuck = chuck;
    fn->env = Jn_environ_init(env);
    fn->params = params;
    fn->arity = arity;
    fn->name = DEFAULT_LAMBDA_NAME;
    fn->is_lambda = 1;
    JnObject* obj = jn_obj_new(state, JN_FUNCTION_TYPE);
    obj->fn = fn;
    return JN_INTERN_OBJECT(obj);
}

JnObject* jn_obj_function(
    Jn_State* state,
    Jn_Node* block,
    Jn_environ* env,
    char** params, 
    int arity, 
    char* name
)
{
    Chuck* chuck = JN_ALLOC(sizeof(Chuck));
    chuck_init(chuck);
    chuck->env = env;
    compile(block, chuck);
    write_chuck_loc(chuck, OP_NONE, block->line, block->col);    
    write_chuck_loc(chuck, OP_RETURN, block->line, block->col);
    JnFunctionObject* fn = JN_ALLOC(sizeof(JnFunctionObject));
    fn->chuck = chuck;
    fn->env = Jn_environ_init(env);
    fn->params = params;
    fn->arity = arity;
    fn->is_lambda = false;
    fn->name = name;
    JnObject* obj = jn_obj_new(state, JN_FUNCTION_TYPE);
    obj->fn = fn;
    return JN_INTERN_OBJECT(obj);
}

JnObject* jn_obj_struct(Jn_State* state, char* name, char** fields)
{
    JnObject* obj = jn_obj_new(state, JN_STRUCT_TYPE);
    JnStruct* struct_obj = JN_ALLOC(sizeof(JnStruct));
    struct_obj->fields = fields;
    struct_obj->name = name;
    obj->struct_obj = struct_obj;
    return JN_INTERN_OBJECT(obj);
}

JnObject* bind_argument(Jn_State* state, JnObject* obj, char** fields, JnObject** values, long count)
{
    Jn_environ* env = Jn_environ_init(NULL);
    for (int i = 0; i < obj->struct_obj->field_count; ++i)
    {
        environ_insert(env, obj->struct_obj->fields[i], JN_RETURN_NONE);
    }
    
    for (int i = 0; i < count; ++i)
    {
        if (fields[i] == NULL)
        {
            fields[i] = strdup(obj->struct_obj->fields[i]);
        }
        Jn_environ_E* entt = environ_get(env, fields[i]);
        if (!entt || !entt->value)
            return JN_RAISE_EXCPETION(state, UNDEFINE_ERROR, "Unkown field '%s'.", fields[i]);
        
        entt->value = values[i];
    }
    return jn_obj_instance(state, obj, env);
}

JnObject* jn_obj_instance(Jn_State* state, JnObject* from_obj, Jn_environ* fields)
{
    JnObject* obj = jn_obj_new(state, JN_INSTANCE_TYPE);
    JnInstance* instance = JN_ALLOC(sizeof(JnInstance));
    instance->obj = from_obj;
    instance->fields = Jn_environ_init(fields);
    obj->instance = instance;
    return JN_INTERN_OBJECT(obj);
}

JN_API JnObject* jn_obj_array(Jn_State* state)
{
    JnObject* obj = jn_obj_new(state, JN_ARRAY_TYPE);
    obj->arr = Jn_alloc(sizeof(Jn_Array));
    obj->arr->size = 0;
    obj->arr->capacity = JN_INITIAL_CAPACITY;
    obj->arr->items = Jn_alloc(sizeof(*obj) * JN_INITIAL_CAPACITY);
    return obj;
}

JnObject* jn_obj_module(Jn_State* state, char* name, char* path, Jn_environ* env)
{
    JnObject* obj = jn_obj_new(state, JN_MODULE_TYPE);
    JnModule* mod = malloc(sizeof(JnModule));
    mod->name = name;
    mod->env = env;
    mod->path = path;
    mod->alias = NULL; // TODO
    obj->module = mod;
    return JN_INTERN_OBJECT(obj);
}


JnObject* jn_obj_array_get(Jn_Array* arr, int idx)
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

JnObject* jn_obj_copy(JnObject* src){ return Jn_alloc_dup(src, sizeof(JnObject));}

static void print_array(JnObject* obj)
{
    assert(obj != NULL);
    fprintf(stderr, "[");
    for (size_t i = 0; i < obj->arr->size; i++)
    {
        jn_obj_print(obj->arr->items[i]);
        if (i < obj->arr->size - 1)
            fprintf(stderr, ", ");
    }
    fprintf(stderr, "]");
}

static void print_tuple(JnObject* obj)
{
    if (!obj) return;
    fprintf(stdout, "(");
    for (size_t i = 0; i < obj->tuple->size; ++i)
    {
        if (i > 0) printf(", ");
        jn_obj_print(obj->tuple->items[i]);
    }
    if (obj->tuple->size == 1)
        putchar(',');
    fprintf(stdout, ")");
}

static void print_hashmap(JnObject* obj)
{
    fprintf(stdout, "#{");
    for (int i = 0; i < obj->hashmap->size; ++i)
    {
        Jn_HashEntry* hm = &obj->hashmap->buckets[i];
        jn_obj_print(hm->key);
        printf(": ");
        jn_obj_print(hm->value);
        if (i < obj->hashmap->size - 1)
            fprintf(stdout, ", ");
    }
    putchar('}');
}


char* Jn_object_cstring(JnObject* obj)
{
    assert(obj != NULL);
    static char buffer[0xff];
    switch (obj->type)
    {
        case JN_INT_TYPE:
            snprintf(buffer, sizeof(buffer), "%lld", obj->int_val);
            goto buf;
        case JN_STRING_TYPE:
            return strdup(obj->str->chars);
        case JN_FLOAT_TYPE:
            snprintf(buffer, sizeof(buffer), "%.15g", obj->float_val);
            goto buf;
        case JN_BOOL_TYPE:
            snprintf(buffer, sizeof(buffer), "%s", obj->bool_val ? "true": "false");
            goto buf;
        case JN_CHAR_TYPE:
            snprintf(buffer, sizeof(buffer), "%c", JN_AS_CHAR(obj));
            goto buf;
        default:
            return strdup("<object>");
    }
    buf:
        return strdup(buffer);
}

void jn_obj_print(JnObject* obj)
{
    if (NULL == obj) return;
    switch (JN_OBJ_TYPE(obj))
    {
        case JN_INT_TYPE:
            fprintf(stdout, "%lld", JN_AS_INT(obj)); break;
        case JN_CHAR_TYPE:
            fprintf(stdout, "%c", JN_AS_CHAR(obj)); break;
        case JN_STRING_TYPE:
            fprintf(stdout, "\"%.*s\"", (int)JN_AS_STRING(obj)->len, (JN_AS_STRING(obj)->len != 0) ? JN_AS_CSTRING(obj) : "None");
            break;
        case JN_BOOL_TYPE:
            fprintf(stdout, (JN_AS_BOOL(obj)) ? "true": "false"); break;
        case JN_FLOAT_TYPE:
            fprintf(stdout, "%.15g", JN_AS_FLOAT(obj)); break;
        case JN_ARRAY_TYPE:
            print_array(obj);
            break;
        case JN_TUPLE_TYPE:
            print_tuple(obj);
            break;
        case JN_HASHMAP_TYPE:
            print_hashmap(obj);
            break;
        case JN_MODULE_TYPE:
            fprintf(stdout, "<Module '%s' at '%s'>", obj->module->name, obj->module->path);
            break;
        case JN_NONE_TYPE:
            fprintf(stdout, "None");
            break;
        case JN_RANGE_TYPE:
            fprintf(stdout, "<Range (%lld, %lld, %d)>", obj->range.start, obj->range.stop, obj->range.step);
            break;
        case JN_ENUM_TYPE:
            fprintf(stdout, "<Enum>");
            break;
        case JN_FUNCTION_TYPE:
            fprintf(stdout, "<function '%s' args=%d>",obj->fn->name, obj->fn->arity);
            break;
        case JN_ITER_TYPE:
            fprintf(stdout, "<iter '");
            jn_obj_print(obj->iter->obj);
            fprintf(stdout, "' >");
            break;
        case JN_OBJECT_TYPE:
            fprintf(stdout, "<%s>", obj->type_val.typename); break;
        case JN_METHOD_TYPE:
            fprintf(stdout, "<method function for "); jn_obj_print(obj->method.obj); fprintf(stdout, " at %p>", obj->method.fn);
            break;
        case JN_STRUCT_TYPE:
            fprintf(stdout, "struct{%s}", (obj->struct_obj->name) ? obj->struct_obj->name : "<unsigned>"); break;
        case JN_INSTANCE_TYPE:
            fprintf(stdout, "<struct{%s} at '%p'>", obj->instance->obj->struct_obj->name, obj->instance->obj); break;
        case JN_NATIVE_TYPE:
            fprintf(stdout, "<function <%s> at %p>", obj->native_fn->fnName, obj->native_fn);
            break;
        default:
            fprintf(stderr, "<unsigned>");
            break;
    }
}

bool jn_obj_truthy(JnObject* obj)
{
    if (!obj) return false;
    switch (obj->type)
    {
    case JN_NONE_TYPE: return false;
    case JN_BOOL_TYPE:
        return obj->bool_val;
    case JN_CHAR_TYPE:
        return JN_AS_CHAR(obj) != '\0';
    case JN_INT_TYPE:  
        return obj->int_val != 0;
    case JN_FLOAT_TYPE:
        return obj->float_val != 0;
    case JN_STRING_TYPE:
        return obj->str->len != 0;
    case JN_INSTANCE_TYPE:
    case JN_STRUCT_TYPE:
    case JN_NATIVE_TYPE:
    case JN_ENUM_TYPE:
    case JN_FUNCTION_TYPE:
        return true;
    default:
        return false;
    }
}

int jn_obj_count(JnObject* obj)
{
    if (!JN_IS_ITERABLE(obj)) return -1;
    switch (JN_OBJ_TYPE(obj))
    {
    case JN_ARRAY_TYPE:
        return (int)JN_AS_ARRAY(obj)->size;
    case JN_HASHMAP_TYPE:
        return (int)JN_AS_HASHMAP(obj)->size;
    case JN_STRING_TYPE:
        return (int)JN_AS_STRING(obj)->len;
    case JN_RANGE_TYPE:
        return (int)range_len(JN_AS_RANGE(obj));
    case JN_TUPLE_TYPE:
        return (int)JN_AS_TUPLE(obj)->size;
    default:
        return -1;
    }
}
void jn_arr_pop(JnObject* arr_obj, JnObject** value)
{
    assert(arr_obj != NULL);
    Jn_Array* iter = JN_IS_ARRAY(arr_obj) ? JN_AS_ARRAY(arr_obj) : JN_AS_TUPLE(arr_obj);
    memcpy(*value, iter->items[iter->size - 1], sizeof(JnObject));
    Jn_mem_zero(iter->items[iter->size - 1], sizeof(JnObject));
    iter->size--;
}

void jn_arr_copy(JnObject* dest, JnObject* src)
{
    Jn_Array* iter = JN_IS_ARRAY(dest) ? JN_AS_ARRAY(dest) : JN_AS_TUPLE(dest);
    Jn_Array* src_iter = JN_IS_ARRAY(src) ? JN_AS_ARRAY(src) : JN_AS_TUPLE(src);
    jn_arr_grow(src, iter->size);
    memcpy(iter->items, src_iter->items, src_iter->size * sizeof(JnObject *));
}

int jn_arr_grow(JnObject* arr_obj, size_t new_size)
{
    if (NULL == arr_obj) return -1;
    Jn_Array* iter = JN_IS_ARRAY(arr_obj) ? JN_AS_ARRAY(arr_obj) : JN_AS_TUPLE(arr_obj);
    if (iter->size + new_size >= iter->capacity)
    {
        if (iter->capacity == 0) iter->capacity = JN_INITIAL_CAPACITY;
        while (iter->size + new_size > iter->capacity)
            iter->capacity *= 2;
        iter->items = Jn_realloc(iter->items, sizeof(JnObject *) * iter->capacity);
        return 1;
    }
    return 0;
}

void jn_arr_append(JnObject* arr_obj, JnObject* value)
{
    if (jn_arr_grow(arr_obj, 0) == -1) return;
    Jn_Array* iter = JN_IS_ARRAY(arr_obj) ? JN_AS_ARRAY(arr_obj) : JN_AS_TUPLE(arr_obj);
    iter->items[iter->size++] = value;
}

void jn_arr_append_many(JnObject* arr_obj, JnObject** argv, size_t argc)
{
    Jn_Array* iter = JN_IS_ARRAY(arr_obj) ? JN_AS_ARRAY(arr_obj) : JN_AS_TUPLE(arr_obj);
    if (jn_arr_grow(arr_obj, argc) == -1) return;
    memmove(iter->items + iter->size, argv, sizeof(JnObject *) * argc);
    iter->size += argc;
}

void jn_arr_clear(JnObject* arr_obj)
{
    Jn_mem_zero(JN_AS_ARRAY(arr_obj)->items, sizeof(JnObject *) * jn_obj_count(arr_obj));
    JN_AS_ARRAY(arr_obj)->size = 0;
}

JnObject* jn_arr_remove(JnObject* arr_obj, int index)
{
    Jn_Array* iter = JN_IS_ARRAY(arr_obj) ? JN_AS_ARRAY(arr_obj) : JN_AS_TUPLE(arr_obj);
    if (index < 0)
    {
        index += iter->size;
    }
    if (index < 0 || index >= iter->size)   return NULL;
    JnObject* rm_obj = iter->items[index];
    memmove(&iter->items[index], &iter->items[index + 1], (iter->size - index - 1) * sizeof(JnObject *));
    iter->size--;
    return rm_obj;
}

JnObject* jn_arr_get(JnObject* arr_obj, int index)
{
    Jn_Array* iter = JN_IS_ARRAY(arr_obj) ? JN_AS_ARRAY(arr_obj) : JN_AS_TUPLE(arr_obj);
    if (index < 0)
    {
        index += iter->size;
    }
    if (index < 0 || index >= iter->size) return NULL;
    return iter->items[index];
}