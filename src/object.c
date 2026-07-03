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


#define LONG_HEX_NUM 0xbf58476d1ce4e5b9ULL
#define LONG_HEX_NUM2 0x94d049bb133111ebULL
#define LONG_HEX_NUM3 0x9e37779b97f4e7c15ULL
JnObject NoneObj = {0};

static InternEntry* intern_pool[JN_INTER_SIZE];

static inline uint64_t hash_mix(uint64_t x)
{
    x ^=  x >> 30;
    x *= LONG_HEX_NUM;
    x ^= x >> 27;
    x *= LONG_HEX_NUM2;
    x ^= x >> 31;
    return x;
}

static inline uint64_t hash_combine(uint64_t a, uint64_t b)
{
    return hash_mix(a ^ (b + LONG_HEX_NUM3 + 
            (a << 6) + (a >> 2)));
}

JnObject* jn_obj_new(JnTypeObject type)
{
    JnObject* obj = gc_alloc(sizeof(JnObject), type);
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

JnObject* jn_obj_iter(JnObject* obj)
{
    JnObject* new_obj = jn_obj_new(ITER_TYPE);
    JnIterObject* iter = malloc(sizeof(JnIterObject)); // TODO
    iter->obj = obj;
    iter->index = 0;
    new_obj->iter = iter;
    return new_obj;
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

JnObject* jn_obj_range(int64_t start, int64_t stop, int64_t step)
{
    JnObject* obj = jn_obj_new(RANGE_TYPE);
    obj->range.start = start;
    obj->range.stop = stop;
    obj->range.step = (step == 0) ? 1 : step;
    return obj;
}

JnObject* jn_obj_arg(JnObject** args, char** arg_names, size_t count)
{
    JnObject* obj = jn_obj_new(ARG_TYPE);
    obj->arg.args = args;
    obj->arg.count = count;
    obj->arg.arg_names = arg_names;
    return obj;
}

JnObject* jn_obj_error(int type, char* msg, ...)
{
    char buffer[1 << 10];
    JnObject* obj = jn_obj_new(ERROR_TYPE);
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
    return obj;
}

JnObject* jn_obj_type(char* type_name, JnTypeObject type)
{
    JnObject* obj = jn_obj_new(OBJECT_TYPE);
    obj->type_obj.type_name = type_name;
    obj->type_obj.type = type;
    obj->type_obj.is_union = false; // DEFAULT
    obj->type_obj.union_types = NULL; // DEFAULT NULL
    return obj;
}

JnObject* jn_obj_method(JnObject* obj, JN_CMethod method)
{
    JnObject* new_obj = jn_obj_new(METHOD_TYPE);
    new_obj->method.fn = method;
    new_obj->method.obj = obj;
    return new_obj;
}

char* jn_obj_to_string(JnObject* obj)
{
    if (!obj) return NULL;
    char* type = NULL;
    switch (obj->type)
    {
        case ARRAY_TYPE:
            type = "Array"; break;
        case INT_TYPE:
            type = "int"; break;
        case STR_TYPE:
            type = "string"; break;
        case BOOL_TYPE:
            type = "bool"; break;
        case CHAR_TYPE:
            type = "char"; break;
        case HASHMAP_TYPE:
            type = "hashmap"; break;
        case MODULE_TYPE:
            type = "module"; break;
        case OBJECT_TYPE:
            type = obj->type_obj.type_name; break;
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
        case INT_TYPE:
            _SET_TYPE(INT_TYPE);
            dest->int32 = src->int32;
            break;
        case STR_TYPE:
            _SET_TYPE(STR_TYPE);
            dest->str = src->str;
            break;
        case BOOL_TYPE:
            _SET_TYPE(BOOL_TYPE);
            dest->bool8 = src->bool8;
            break;
        case FLOAT_TYPE:
            _SET_TYPE(FLOAT_TYPE);
            dest->float32 = src->float32;
            break;
        case CHAR_TYPE:
            _SET_TYPE(CHAR_TYPE);
            dest->j_char = src->j_char;
            break;
        case RANGE_TYPE:
            _SET_TYPE(RANGE_TYPE);
            dest->range = src->range;
            break;
        case ARRAY_TYPE:
            _SET_TYPE(ARRAY_TYPE);
            dest->arr = src->arr;
            break;
        case HASHMAP_TYPE:
            _SET_TYPE(HASHMAP_TYPE);
            dest->hashmap = src->hashmap;
            break;
        case FUNCTION_TYPE:
            _SET_TYPE(FUNCTION_TYPE);
            dest->fn = src->fn;
            break;
        case STRUCT_TYPE:
            _SET_TYPE(STRUCT_TYPE);
            dest->struct_obj = src->struct_obj;
            break;
        case INSTANCE_TYPE:
            _SET_TYPE(INSTANCE_TYPE);
            dest->instance = src->instance;
            break;
    }
    #undef _SET_TYPE
}

static uint64_t hash_object(JnObject* obj)
{
    if (!obj) return 0;
    uint64_t h;
    switch (obj->type)
    {
        case NONE_TYPE:
            return LONG_HEX_NUM3;
        case INT_TYPE:
            return hash_mix((uint64_t)obj->int32);
        case BOOL_TYPE:
            return hash_mix(JN_AS_BOOL(obj));
        case FLOAT_TYPE:
            union {
                double d;
                uint64_t u;
            } bits;
            bits.d = JN_AS_FLOAT(obj);
            return hash_mix(bits.u);
        case STR_TYPE:
            return hash_mix(JN_AS_STRING(obj)->hash);
        case CHAR_TYPE:
            return hash_mix((unsigned char) JN_AS_CHAR(obj));
        case RANGE_TYPE:
        {
            h = hash_mix(JN_AS_RANGE(obj)->start);
            h = hash_combine(h, hash_mix(JN_AS_RANGE(obj)->stop));
            h = hash_combine(h, hash_mix(JN_AS_RANGE(obj)->step));
            return h;          
        }
        case ITER_TYPE:
        {
            h = hash_object(JN_AS_ITER(obj)->obj);
            return hash_combine(h, hash_mix(JN_AS_ITER(obj)->index));
        }
        case MODULE_TYPE:
        {
            h = hash_mix(djb2_hash(obj->module->name));
            return hash_combine(h, hash_mix(djb2_hash(obj->module->path)));
        }
        case STRUCT_TYPE:
        {
            h = hash_mix(djb2_hash(obj->struct_obj->name));
            for (int i = 0; i < obj->struct_obj->field_count; ++i)
            {
                h = hash_combine(h, hash_mix(djb2_hash(obj->struct_obj->fields[i])));
            }
            return h;
        }
        case METHOD_TYPE:
        {
            h = hash_object(obj->method.obj);
            return hash_combine(h, hash_mix((uintptr_t)obj->method.fn));
        }
        case INSTANCE_TYPE:
        {
            h = hash_object(obj->instance->obj);
            return h;
        }
        case FUNCTION_TYPE:
        {
            return hash_mix(djb2_hash(obj->fn->name));
        }
        case NATIVE_TYPE:
        {
            return hash_mix(djb2_hash(obj->native_fn->fnName));
        }
        case OBJECT_TYPE:
            return hash_mix(djb2_hash(obj->type_obj.type_name));
        default:
            return hash_mix((uintptr_t)obj);
    }
}

JnObject* jn_intern_obj(JnObject* obj)
{
    if (JN_IS_ARRAY(obj) || JN_IS_HASHMAP(obj) || JN_IS_STRUCT(obj))
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


JnObject* jn_obj_lambda(AST* expr, char** params, int arity, Jn_environ* env)
{
    Chuck* chuck = JN_ALLOC(sizeof(Chuck));
    chuck_init(chuck);
    chuck->env = env;
    compile(expr, chuck);
    write_chuck_loc(chuck, OP_RETURN, expr->line, expr->col);
    JnFunctionObject* fn = JN_ALLOC(sizeof(JnFunctionObject));
    fn->chuck = chuck;
    fn->env = Jn_environ_init(env);
    fn->params = params;
    fn->arity = arity;
    fn->name = DEFAULT_LAMBDA_NAME;
    fn->is_lambda = 1;
    JnObject* obj = jn_obj_new(FUNCTION_TYPE);
    obj->fn = fn;
    return obj;
}

JnObject* jn_obj_function(
    AST* block,
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
    write_chuck_loc(chuck, OP_RETURN, block->line, block->col);
    JnFunctionObject* fn = JN_ALLOC(sizeof(JnFunctionObject));
    fn->chuck = chuck;
    fn->env = Jn_environ_init(env);
    fn->params = params;
    fn->arity = arity;
    fn->is_lambda = false;
    fn->name = name;
    JnObject* obj = jn_obj_new(FUNCTION_TYPE);
    obj->fn = fn;
    return obj;
}

JnObject* jn_obj_struct(char* name, char** fields)
{
    JnObject* obj = jn_obj_new(STRUCT_TYPE);
    JnStruct* struct_obj = JN_ALLOC(sizeof(JnStruct));
    struct_obj->fields = fields;
    struct_obj->name = name;
    obj->struct_obj = struct_obj;
    return obj;
}

JnObject* bind_argument(JnObject* obj, char** fields, JnObject** values, long count)
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
            return JN_RAISE_EXCPETION(UNDEFINE_ERROR, "Unkown field '%s'.", fields[i]);
        
        entt->value = values[i];
    }
    return jn_obj_instance(obj, env);
}

JnObject* jn_obj_instance(JnObject* from_obj, Jn_environ* fields)
{
    JnObject* obj = jn_obj_new(INSTANCE_TYPE);
    JnInstance* instance = JN_ALLOC(sizeof(JnInstance));
    instance->obj = from_obj;
    instance->fields = Jn_environ_init(fields);
    obj->instance = instance;
    return obj;
}

JnObject* jn_obj_module(char* name, char* path, Jn_environ* env)
{
    JnObject* obj = jn_obj_new(MODULE_TYPE);
    JnModule* mod = malloc(sizeof(JnModule));
    mod->name = name;
    mod->env = env;
    mod->path = path;
    mod->alias = NULL; // TODO
    obj->module = mod;
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

static void print_tuple(JnObject* obj)
{
    if (!obj) return;
    fprintf(stdout, "(");
    for (size_t i = 0; i < obj->tuple->size; ++i)
    {
        if (i > 0) printf(", ");
        print_JnObject(obj->tuple->items[i]);
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
        print_JnObject(hm->key);
        printf(": ");
        print_JnObject(hm->value);
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
            return strdup("<object>");
    }
    buf:
        return strdup(buffer);
}

void print_JnObject(JnObject* obj)
{
    if (NULL == obj) return;
    
    switch (JN_OBJ_TYPE(obj))
    {
        case INT_TYPE:
            fprintf(stderr, "%lld", JN_AS_INT(obj)); break;
        case CHAR_TYPE:
            fprintf(stderr, "%c", JN_AS_CHAR(obj)); break;
        case STR_TYPE:
            fprintf(stderr, "\"%s\"", (JN_AS_STRING(obj)->len != 0) ? JN_AS_CSTRING(obj) : "None");
            break;
        case BOOL_TYPE:
            fprintf(stderr, (JN_AS_BOOL(obj)) ? "true": "false"); break;
        case FLOAT_TYPE:
            fprintf(stderr, "%.15g", JN_AS_FLOAT(obj)); break;
        case ARRAY_TYPE:
            print_array(obj);
            break;
        case TUPLE_TYPE:
            print_tuple(obj);
            break;
        case HASHMAP_TYPE:
            print_hashmap(obj);
            break;
        case MODULE_TYPE:
            fprintf(stderr, "<Module '%s' at '%s'>", obj->module->name, obj->module->path);
            break;
        case NONE_TYPE:
            fprintf(stderr, "None");
            break;
        case RANGE_TYPE:
            fprintf(stderr, "<Range (%lld, %lld, %ld)>", obj->range.start, obj->range.stop, obj->range.step);
            break;
        case ENUM_TYPE:
            fprintf(stderr, "<Enum>");
            break;
        case FUNCTION_TYPE:
            fprintf(stdout, "<function '%s' args=%d>",obj->fn->name, obj->fn->arity);
            break;
        case ITER_TYPE:
            fprintf(stderr, "<iter '");
            print_JnObject(obj->iter->obj);
            fprintf(stderr, "' >");
            break;
        case OBJECT_TYPE:
            fprintf(stderr, "<%s>", obj->type_obj.type_name); break;
        case METHOD_TYPE:
            fprintf(stdout, "<method function for '"); print_JnObject(obj->method.obj); fprintf(stdout, "' at %p>", obj->method.fn);
            break;
        case STRUCT_TYPE:
            fprintf(stdout, "struct{%s}", (obj->struct_obj->name) ? obj->struct_obj->name : "<unsigned>"); break;
        case INSTANCE_TYPE:
            fprintf(stdout, "<struct{%s} at '%p'>", obj->instance->obj->struct_obj->name, obj->instance->obj); break;
        case NATIVE_TYPE:
            fprintf(stderr, "<function <%s> at %p>", obj->native_fn->fnName, obj->native_fn);
            break;
        default:
            fprintf(stderr, "<unsigned>");
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
    case CHAR_TYPE:
        return JN_AS_CHAR(obj) != '\0';
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