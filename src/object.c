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

JnObject NoneObj = {0};

static InternEntry* intern_pool[JN_INTER_SIZE];


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
void jn_obj_reassign(JnObject* obj1, JnObject* obj2)
{
    assert(obj1 != NULL && obj2 != NULL);
    obj1->type = obj2->type;
    switch (obj2->type)
    {
        case INT_TYPE:
            obj1->int32 = obj2->int32;
            break;
        case STR_TYPE:
            obj1->str = malloc(sizeof(JnStringObject));
            memcpy(obj1->str, obj2->str, sizeof(*obj2->str));
            break;
        case BOOL_TYPE:
            obj1->bool8 = obj2->bool8;
            break;
        case FLOAT_TYPE:
            obj1->float32 = obj2->float32;
            break;
        case CHAR_TYPE:
            obj1->j_char = obj2->j_char;
            break;
        case RANGE_TYPE:
            obj1->range = obj2->range;
            break;
        default:
            assert(false && "Not yet impl reassign for this type TODO.");
    }
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
        case ITER_TYPE:
            return hash_object(obj->iter->obj) * obj->iter->index;
        case MODULE_TYPE:
            return (uint64_t) djb2_hash(obj->module->name);
        case STRUCT_TYPE:
            return (uintptr_t) obj->struct_obj;
        case INSTANCE_TYPE:
            return (uintptr_t) obj->instance->obj;
        case OBJECT_TYPE:
            return djb2_hash(obj->type_obj.type_name); // TODO
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


JnObject* jn_obj_lambda(AST* expr, char** params, int arity, Jn_environ* env)
{
    Chuck* chuck = JN_ALLOC(sizeof(Chuck));
    chuck_init(chuck);
    chuck->env = env;
    compile(expr, chuck);
    write_chuck_loc(chuck, OP_RETURN, expr->line, expr->col);
    write_chuck_loc(chuck, OP_END, expr->line, expr->col);
    JnFunctionObject* fn = JN_ALLOC(sizeof(JnFunctionObject));
    fn->chuck = chuck;
    fn->env = Jn_environ_init(env);
    fn->params = params;
    fn->arity = arity;
    fn->name = strdup("<lambda>");
    fn->is_lambda = 1;
    JnObject* obj = jn_obj_new(FUNCTION_TYPE);
    obj->fn = fn;
    printf("WORKING HERE ......\n");
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
    write_chuck_loc(chuck, OP_END, 0, 0);
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
            fprintf(stdout, "struct{%s}", obj->struct_obj->name ? obj->struct_obj->name : "<unsigned>"); break;
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