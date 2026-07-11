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
#define LONG_HEX_NUM3 0x9e3779b97f4e7c15ULL
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

JnObject* jn_obj_new(J_State* state, JnTypeObject type)
{
    JnObject* obj = gc_alloc(state, sizeof(JnObject), type);
    assert(obj != NULL);
    return obj;
}

JnObject* jn_obj_int(J_State* state, long int_val)
{
    JnObject* obj =  jn_obj_new(state, JN_INT_TYPE);
    obj->int_val = int_val;
    return obj;
}

JnObject* jn_obj_string(J_State* state, char* str)
{
    JnObject* obj =  jn_obj_new(state, JN_STRING_TYPE);
    JnStringObject* strObj = malloc(sizeof(JnStringObject));
    *strObj = JNSTR_OBJ(str);
    obj->str = strObj;
    return obj;
}

JnObject* jn_obj_char(J_State* state, char c)
{
    JnObject* obj =  jn_obj_new(state, JN_CHAR_TYPE);
    obj->j_char = c;
    return obj;
}

JnObject* jn_obj_bool(J_State* state, bool bool_val)
{
    JnObject* obj =  jn_obj_new(state, JN_BOOL_TYPE);
    obj->bool_val = bool_val;
    return obj;
}

JnObject* jn_obj_float(J_State* state, double float_val)
{
    JnObject* obj =  jn_obj_new(state, JN_FLOAT_TYPE);
    obj->float_val = float_val;
    return obj;
}


JnObject* jn_obj_none(void)
{
    NoneObj.type = JN_NONE_TYPE;
    NoneObj.int_val = 0;
    return &NoneObj;
}

JnObject* jn_obj_iter(J_State* state, JnObject* obj)
{
    JnObject* new_obj = jn_obj_new(state, JN_ITER_TYPE);
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

JnObject* jn_obj_range(J_State* state, int64_t start, int64_t stop, int64_t step)
{
    JnObject* obj = jn_obj_new(state, JN_RANGE_TYPE);
    obj->range.start = start;
    obj->range.stop = stop;
    obj->range.step = (step == 0) ? 1 : step;
    return obj;
}

JnObject* jn_obj_arg(J_State* state, JnObject** args, char** arg_names, size_t count)
{
    JnObject* obj = jn_obj_new(state, JN_ARG_TYPE);
    obj->arg.args = args;
    obj->arg.count = count;
    obj->arg.arg_names = arg_names;
    return obj;
}

JnObject* jn_obj_error(J_State* state, int type, char* msg, ...)
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
    return obj;
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
        snprintf(buffer, 256, "%ld", JN_AS_INT(obj));
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

JnObject* jn_obj_type(J_State* state, char* type_name, JnTypeObject type, Jn_CFunction fn)
{
    JnObject* obj = jn_obj_new(state, JN_OBJECT_TYPE);
    obj->type_val.typename = type_name;
    obj->type_val.type = type;
    obj->type_val.ctor = fn;
    return obj;
}

JnObject* jn_obj_method(J_State* state, JnObject* obj, JN_CMethod method)
{
    JnObject* new_obj = jn_obj_new(state, JN_METHOD_TYPE);
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
            type = obj->type_val.typename; break;
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
    }
    #undef _SET_TYPE
}

uint64_t Jn_object_hash(JnObject* obj)
{
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
            h = hash_mix(djb2_hash(obj->module->name));
            return hash_combine(h, hash_mix(djb2_hash(obj->module->path)));
        }
        case JN_STRUCT_TYPE:
        {
            h = hash_mix(djb2_hash(obj->struct_obj->name));
            for (int i = 0; i < obj->struct_obj->field_count; ++i)
            {
                h = hash_combine(h, hash_mix(djb2_hash(obj->struct_obj->fields[i])));
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
            return hash_mix(djb2_hash(obj->fn->name));
        }
        case JN_NATIVE_TYPE:
        {
            return hash_mix(djb2_hash(obj->native_fn->fnName));
        }
        case JN_OBJECT_TYPE:
            return hash_mix(djb2_hash(obj->type_val.typename));
        default:
            return hash_mix((uintptr_t)obj);
    }
}

JnObject* jn_intern_obj(JnObject* obj)
{
    if (JN_IS_ARRAY(obj) || JN_IS_HASHMAP(obj) || JN_IS_STRUCT(obj))
        return obj;
    uint64_t hash = Jn_object_hash(obj);
    size_t idx = hash % JN_INTER_SIZE;
    InternEntry* entry = intern_pool[idx];
    
    while(entry)
    {
        if (entry->obj->type == obj->type && Jn_object_hash(entry->obj) == Jn_object_hash(obj))
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


JnObject* jn_obj_lambda(J_State* state, AST* expr, char** params, int arity, Jn_environ* env)
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
    JnObject* obj = jn_obj_new(state, JN_FUNCTION_TYPE);
    obj->fn = fn;
    return obj;
}

JnObject* jn_obj_function(
    J_State* state,
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
    return obj;
}

JnObject* jn_obj_struct(J_State* state, char* name, char** fields)
{
    JnObject* obj = jn_obj_new(state, JN_STRUCT_TYPE);
    JnStruct* struct_obj = JN_ALLOC(sizeof(JnStruct));
    struct_obj->fields = fields;
    struct_obj->name = name;
    obj->struct_obj = struct_obj;
    return obj;
}

JnObject* bind_argument(J_State* state, JnObject* obj, char** fields, JnObject** values, long count)
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

JnObject* jn_obj_instance(J_State* state, JnObject* from_obj, Jn_environ* fields)
{
    JnObject* obj = jn_obj_new(state, JN_INSTANCE_TYPE);
    JnInstance* instance = JN_ALLOC(sizeof(JnInstance));
    instance->obj = from_obj;
    instance->fields = Jn_environ_init(fields);
    obj->instance = instance;
    return obj;
}

JnObject* jn_obj_module(J_State* state, char* name, char* path, Jn_environ* env)
{
    JnObject* obj = jn_obj_new(state, JN_MODULE_TYPE);
    JnModule* mod = malloc(sizeof(JnModule));
    mod->name = name;
    mod->env = env;
    mod->path = path;
    mod->alias = NULL; // TODO
    obj->module = mod;
    return obj;
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

void print_JnObject(JnObject* obj)
{
    if (NULL == obj) return;
    
    switch (JN_OBJ_TYPE(obj))
    {
        case JN_INT_TYPE:
            fprintf(stderr, "%lld", JN_AS_INT(obj)); break;
        case JN_CHAR_TYPE:
            fprintf(stderr, "%c", JN_AS_CHAR(obj)); break;
        case JN_STRING_TYPE:
            fprintf(stderr, "\"%s\"", (JN_AS_STRING(obj)->len != 0) ? JN_AS_CSTRING(obj) : "None");
            break;
        case JN_BOOL_TYPE:
            fprintf(stderr, (JN_AS_BOOL(obj)) ? "true": "false"); break;
        case JN_FLOAT_TYPE:
            fprintf(stderr, "%.15g", JN_AS_FLOAT(obj)); break;
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
            fprintf(stderr, "<Module '%s' at '%s'>", obj->module->name, obj->module->path);
            break;
        case JN_NONE_TYPE:
            fprintf(stderr, "None");
            break;
        case JN_RANGE_TYPE:
            fprintf(stderr, "<Range (%lld, %lld, %ld)>", obj->range.start, obj->range.stop, obj->range.step);
            break;
        case JN_ENUM_TYPE:
            fprintf(stderr, "<Enum>");
            break;
        case JN_FUNCTION_TYPE:
            fprintf(stdout, "<function '%s' args=%d>",obj->fn->name, obj->fn->arity);
            break;
        case JN_ITER_TYPE:
            fprintf(stderr, "<iter '");
            print_JnObject(obj->iter->obj);
            fprintf(stderr, "' >");
            break;
        case JN_OBJECT_TYPE:
            fprintf(stderr, "<%s>", obj->type_val.typename); break;
        case JN_METHOD_TYPE:
            fprintf(stdout, "<method function for '"); print_JnObject(obj->method.obj); fprintf(stdout, "' at %p>", obj->method.fn);
            break;
        case JN_STRUCT_TYPE:
            fprintf(stdout, "struct{%s}", (obj->struct_obj->name) ? obj->struct_obj->name : "<unsigned>"); break;
        case JN_INSTANCE_TYPE:
            fprintf(stdout, "<struct{%s} at '%p'>", obj->instance->obj->struct_obj->name, obj->instance->obj); break;
        case JN_NATIVE_TYPE:
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