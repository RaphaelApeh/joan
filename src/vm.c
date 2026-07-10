#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include "Joan.h"
#include "vm.h"
#include "opcode.h"
#include "ast.h"
#include "object.h"
#include "helper.h"
#include "eval.h"
#include "emit.h"

#define WRITE_CHUCK(chuck, OP) write_chuck_loc(chuck, OP, line, column);

#define PUSH(vm, obj)  do { \
    assert((vm) != NULL || (obj) != NULL);              \
    if (JN_IS_ERROR((obj))) return vm_error(state, vm, (obj)); \
    push(vm, (obj));                                  \
}while(false)

static LoopContext loop_stack[256];
static int loop_depth = 0;

static int iter_count = 0;
static Jn_environ* local;

void Jnvm_init(JnVM* vm, Chuck* chuck)
{
    vm->frame_count = 0;
    vm->ip = chuck->code;
    vm->sp = vm->stack;
    vm->global = chuck->env;
    vm->env = vm->global;
}

void chuck_init(Chuck* chuck)
{
    chuck->count = 0;
    chuck->ident_count = 0;
    chuck->ident_capacity = 100;
    chuck->capacity = 200;
    chuck->constants_count = 0;
    chuck->constants_capacity = 100;
    chuck->code = malloc(sizeof(uint8_t) * chuck->capacity);
    chuck->lines = malloc(sizeof(int) * 200);
    chuck->columns = malloc(sizeof(int) * 200);
    assert(chuck->lines != NULL);
    assert(chuck->columns != NULL);
    assert(chuck->code != NULL);
    chuck->constants = malloc(sizeof(JnObject *) * chuck->constants_capacity);
    assert(chuck->constants != NULL);
    chuck->idents = malloc(sizeof(char *) * chuck->ident_capacity);
    assert(chuck->idents != NULL);
}

void chuck_free(Chuck* chuck)
{
    assert(chuck != NULL);
    free(chuck->code);
    free(chuck->constants);
    free(chuck->idents);
    free(chuck->lines);
    free(chuck->columns);
}

void reset_vm(JnVM* vm)
{
    // RESET
    vm->chuck->count = 0;
    vm->ip = vm->chuck->code;
    Jnvm_init(vm, vm->chuck);
}

static inline int vm_line(JnVM* vm)
{
    size_t ip = (size_t)(vm->ip - vm->chuck->code);
    if (ip == 0)  return 0;
    return vm->chuck->lines[ip - 1];
}
static inline int vm_column(JnVM* vm)
{
    size_t ip = (size_t)(vm->ip - vm->chuck->code);
    if (ip == 0)  return 0;
    return vm->chuck->columns[ip - 1];
}

static int vm_error(J_State* state, JnVM* vm, JnObject* obj)
{
    assert(obj != NULL && JN_IS_ERROR(obj));
    J_Context* ctx = Jn_get_context(state);
    ctx->cur_line = vm_line(vm);
    ctx->column = vm_column(vm);
    obj->expection.filename = (char *)ctx->source.filename;
    obj->expection.line = ctx->cur_line;
    obj->expection.col = ctx->column;
    printf(JN_ERROR_PRINT(obj->expection.type));
    putchar(':');
    fprintf(
        stderr, 
        " '%s':%d:%d \n\t",  ctx->source.filename ? ctx->source.filename: "main",
        ctx->cur_line, ctx->column
    );
    printf("%s\n\n", obj->expection.error_msg);
    if (obj->expection.type == UNDEFINE_ERROR)
    {
        struct FuzzMatch matches[300]; // TODO
        int n = fuzzy_match(
            obj->expection.var_name,
            state->symbols,
            state->symbols_count,
            matches
        );
        if (n > 0)
        {
            printf("\nDid you mean: ");
            for (int i = 0; i < n; ++i)
            {
                if (i > 0)
                    putchar(',');
                printf(" %s", matches[i].word);
            }
            printf("\n");
        }
    }
    print_source_lines(ctx->source.source, ctx->cur_line, ctx->column, 2);
    return INTERPRET_ERROR;
}

static int die(J_State* state, JnVM* vm, const char* msg, ...)
{
    J_Context* ctx = Jn_get_context(state);
    ctx->cur_line = vm_line(vm);
    ctx->column = vm_column(vm);
    va_list arg; va_start(arg, msg);
    fprintf(
        stderr, 
        "RuntimeError: '%s':%d:%d \n\t",  ctx->source.filename ? ctx->source.filename: "main",
        ctx->cur_line, ctx->column
    );
    vfprintf(stderr, msg, arg);
    printf("\n\n");
    va_end(arg);
    print_source_lines(ctx->source.source, ctx->cur_line, ctx->column, 2);
    return INTERPRET_RUNTIME_ERROR;
}

static void push(JnVM* vm, JnObject* object)
{
    assert(object != NULL);
    *vm->sp++ = object;
}

static JnObject* vm_peek(JnVM* vm, int d) {return vm->sp[-1 - d];}

static JnObject* pop(JnVM* vm){ return *--vm->sp; }


static JnObject* call_function(J_State* state, JnVM* vm, JnObject* obj, JnObject** args)
{
    JnFunctionObject* fn = obj->fn;
    JnVM child;
    Jnvm_init(&child, fn->chuck);
    child.env = fn->env;
    child.chuck = fn->chuck;
    for (int i = 0; i < fn->arity; ++i)
    {
        environ_insert(child.env, fn->params[i], args[i]);
    }
    InterpretResult r = vm_run(state, &child);
    if (r == INTERPRET_OK)
        return pop(&child);
    return JN_RAISE_EXCPETION(state, SYS_ERROR, "Extra error to annoy you. Good luck debugging :).");
}

static JnObject *a, *b, *key, *value, *array, *pos;

int vm_run(J_State* state, JnVM* vm)
{
    #define READ_BYTE() (*vm->ip++)
    #define READ_CONST() (vm->chuck->constants[READ_BYTE()])
    #define READ_IDENT() (vm->chuck->idents[READ_BYTE()])
    int count;
    JnObject* tmp;
    JnObject* o = NULL;
    char* ident;
    uint16_t offset;
    for (;;)
    {
        J_Context* ctx = Jn_get_context(state);
        ctx->cur_line = vm_line(vm);
        ctx->column = vm_column(vm);
        uint8_t op = READ_BYTE();
        switch (op)
        {
            case OP_CONSTANT:
                o = READ_CONST();
                PUSH(vm, jn_intern_obj(o));
                break;
            case OP_ADD:
                a = pop(vm); b = pop(vm);
                o = jn_intern_obj(eval_binary(state, b, a, EVAL_ADD));
                PUSH(vm, o);
                break;
            case OP_SUB:
                a = pop(vm); b = pop(vm);
                o = eval_binary(state, b, a, EVAL_SUB);
                PUSH(vm, o);
                break;
            case OP_MUL:
                a = pop(vm); b = pop(vm);
                o = eval_binary(state, b, a, EVAL_MUL);
                PUSH(vm, o);
                break;
            case OP_BITAND:
                a = pop(vm); b = pop(vm);
                o = eval_binary(state, b, a, EVAL_BAND);
                PUSH(vm, o);
                break;
            case OP_BITOR:
                a = pop(vm); b = pop(vm);
                o = eval_binary(state, b, a, EVAL_BOR);
                PUSH(vm, o);
                break;
            case OP_PERC:
                a = pop(vm); b = pop(vm);
                o = eval_binary(state, b, a, EVAL_PERC);
                PUSH(vm, o);
                break;
            case OP_DIV:
                a = pop(vm); b = pop(vm);
                o = eval_binary(state, b, a, EVAL_DIV);
                PUSH(vm, o);
                break;
            case OP_BITAC:
                a = pop(vm); b = pop(vm);
                o = eval_binary(state, b, a, EVAL_BAC);
                PUSH(vm, o);
                break;
            case OP_EQUAL:
                a = pop(vm); b = pop(vm);
                o = eval_binary(state, b, a, EVAL_EQUAL);
                PUSH(vm, o);
                break;
            case OP_LSHIFT:
                a = pop(vm); b = pop(vm);
                o = eval_binary(state, b, a, EVAL_LSHIFT);
                PUSH(vm, o);
                break;
            case OP_RSHIFT:
                a = pop(vm); b = pop(vm);
                o = eval_binary(state, b, a, EVAL_RSHIFT);
                PUSH(vm, o);
                break;
            case OP_NEQ:
                a = pop(vm); b = pop(vm);
                o = eval_binary(state, b, a, EVAL_NOTEQUAL);
                PUSH(vm, o);
                break;
            case OP_GT:
                a = pop(vm); b = pop(vm);
                o = eval_binary(state, b, a, EVAL_GT);
                PUSH(vm, o);
                break;
            case OP_GTE:
                a = pop(vm); b = pop(vm);
                o = eval_binary(state, b, a, EVAL_GTE);
                PUSH(vm, o);
                break;
            case OP_LT:
                a = pop(vm); b = pop(vm);
                o = eval_binary(state, b, a, EVAL_LT);
                PUSH(vm, o);
                break;
            case OP_LTE:
                a = pop(vm); b = pop(vm);
                o = eval_binary(state, b, a, EVAL_LTE);
                PUSH(vm, o);
                break;
            case OP_IMPORT: 
                char* lib = READ_IDENT();
                bool is_std = READ_BYTE();
                bool exists = file_exists(lib);
                o = Jn_import_module(NULL, lib, is_std);
                if (o == NULL) return die(state,vm, "Import error.");
                PUSH(vm, o);
                break;

            case OP_INSTANCE:
                count = READ_BYTE();
                char** fields = JN_ALLOC(sizeof(char *) * 32);
                long fields_count = 0;
                JnObject** values = JN_ALLOC(sizeof(JnObject *) * 32);
                long values_count = 0;
                for (int i = 0; i < count; ++i)
                {
                    fields[i] = READ_IDENT();
                    fields_count++;
                }
                for (int i = count - 1; i >= 0; --i)
                {
                    values[i] = pop(vm);
                    values_count++;
                }
                if (fields_count > values_count)
                    return die(state,vm, "For some reason you have more fields name than values.");
                JnObject* object_type = pop(vm);
                if (object_type->type == OBJECT_TYPE)
                {
                    o = JN_OBJECT_ARG(state, values, NULL, values_count);
                    PUSH(vm, (object_type->type_val.ctor(state, o)));
                    break;
                }
                if (!JN_IS_STRUCT(object_type))
                    return die(state,vm, "Expected a struct type but (got '%d').", object_type->type);
                
                JnObject* instance_obj = bind_argument(state, object_type, fields, values, count);

                PUSH(vm, instance_obj);
                // PUSH(vm, JN_RETURN_NONE);
                break;
            case OP_TUPLE:
                count = READ_BYTE();
                JnArrayObject* arr = NULL;
                for (int i = count - 1; i >= 0; --i)
                {
                    JN_SET_ARRAY(arr, pop(vm), i);
                }
                if (arr == NULL)
                {
                    JN_ARRAY_DEFAULT(arr);
                }
                assert(arr != NULL);
                o = JN_OBJECT(state, TUPLE_TYPE);
                o->tuple = arr;
                PUSH(vm, o);
                break;
            case OP_ARRAY:
                count = READ_BYTE();
                arr = NULL;
                for (int i = count - 1; i >= 0; --i)
                {
                    JN_SET_ARRAY(arr, pop(vm), i);
                }
                if (arr == NULL)
                {
                    JN_ARRAY_DEFAULT(arr);
                }
                assert(arr != NULL);
                o = JN_OBJECT(state, ARRAY_TYPE);
                o->arr = arr;
                PUSH(vm, o);
                break;
            case OP_HM:
                count = READ_BYTE();
                Jn_Hashmap* map = NULL;
                for (int i = count - 1; i >= 0; --i)
                {
                    value = pop(vm); key = pop(vm);
                    JN_HASHMAP_INSERT(map, key, value, i);
                }
                if (map == NULL)
                {
                    JN_DEFAULT_HM(map);
                }
                assert(map != NULL);
                JnObject* obj = JN_OBJECT(state, HASHMAP_TYPE);
                obj->hashmap = map;
                PUSH(vm, obj);
                break;
            case OP_REASSIGN:
                int t_op = READ_BYTE();
                o = pop(vm); a = pop(vm);
                if (o->constant)
                    return die(state,vm, "Seem like you are trying to reassign a variable of type const, \t did you mean ':=' but used '::'.");
                switch (t_op)
                {
                    case TOKEN_APLUS:
                        b = eval_binary(state, o, a, EVAL_ADD);
                        if (NULL == b)
                            break;
                        jn_obj_reassign(o, b);
                        break;
                    case TOKEN_AMINUS:
                        b = eval_binary(state, o, a, EVAL_SUB);
                        if (NULL == b)
                            break;
                        jn_obj_reassign(o, b);
                        break;
                    case TOKEN_EQUAL:
                        jn_obj_reassign(o, a);
                        break;
                    case TOKEN_ASTAR:
                        b = eval_binary(state, o, a, EVAL_MUL);
                        if (NULL == b)
                            break;
                        jn_obj_reassign(o, b);
                        break;
                    case TOKEN_ARSHIFT:
                        b = eval_binary(state, o, a, EVAL_RSHIFT);
                        if (NULL == b)
                            break;
                        jn_obj_reassign(o, b);

                        break;
                    case TOKEN_ALSHIFT:
                        b = eval_binary(state, o, a, EVAL_LSHIFT);
                        if (NULL == b)
                            break;
                        jn_obj_reassign(o, b);
                        break;
                    case TOKEN_BITAC:
                        b = eval_binary(state, o, a, EVAL_BAC);
                        if (NULL == b)
                            break;
                        jn_obj_reassign(o, b);
                        break;
                    case TOKEN_APERCENTAGE:
                        b = eval_binary(state, o, a, EVAL_PERC);
                        if (NULL == b)
                            break;
                        jn_obj_reassign(o, b);
                        break;
                    case TOKEN_ABITAC:
                        b = eval_binary(state, o, a, EVAL_BAC);
                        if (NULL == b)
                            break;
                        jn_obj_reassign(o, b);
                        break;
                    case TOKEN_ASLASH:
                        b = eval_binary(state, o, a, EVAL_DIV);
                        if (NULL == b)
                            break;
                        jn_obj_reassign(o, b);
                        break;
                    case TOKEN_ABITAND:
                        b = eval_binary(state, o, a, EVAL_BAND);
                        if (NULL == b)
                            break;
                        jn_obj_reassign(o, b);
                        break;
                    case TOKEN_ABITOR:
                        b = eval_binary(state, o, a, EVAL_BOR);
                        if (NULL == b)
                            break;
                        jn_obj_reassign(o, b);
                        break;
                    default:
                        return die(state,vm, "invalid operator.");
                }
                break;
            case OP_POW:
                a = pop(vm); b = pop(vm);
                a = eval_binary(state, b, a, EVAL_POW);
                PUSH(vm, a);
                break;
            case OP_IN:
                a = pop(vm);
                b = pop(vm);
                a = eval_binary(state, b, a, EVAL_IN);
                PUSH(vm, a);
                break;
            case OP_NOT_IN:
                a = pop(vm);
                b = pop(vm);
                a = eval_binary(state, b, a, EVAL_NOT_IN);
                PUSH(vm, a);
                break;
            case OP_IS:
                b = pop(vm); a = pop(vm);
                a = eval_binary(state, a, b, EVAL_IS);
                if (NULL == a)
                    return die(state,vm, "Invalid binary opration.");
                PUSH(vm, a);
                break;
            case OP_AND:
                b = pop(vm); a = pop(vm);
                a = jn_intern_obj(eval_binary(state, a, b, EVAL_AND));
                if (NULL == a)
                    return die(state,vm, "Invalid binary opration.");
                PUSH(vm, a);
                break;
            case OP_OR:
                b = pop(vm); a = pop(vm);
                a = jn_intern_obj(eval_binary(state, a, b, EVAL_OR));
                if (NULL == a)
                    return die(state,vm, "Invalid binary opration.");
                PUSH(vm, a);
                break;
            case OP_NOT:
                o = pop(vm);
                PUSH(vm, jn_obj_bool(state, !is_truthy(o)));
                break;
            case OP_MEMBER:
                char* field = READ_IDENT(); o = pop(vm);
                Jn_environ_E* entt = NULL;
                switch (o->type)
                {
                case MODULE_TYPE:
                    entt = environ_get(o->module->env, field);
                    if (entt == NULL)
                        return die(state,vm, "module does not have attribute '%s'.", field);
                    PUSH(vm, entt->value);
                    break;
                case INSTANCE_TYPE:
                    entt = environ_get(o->instance->fields, field);
                    if (entt == NULL)
                        return die(state,vm, "struct object does not have field '%s'.", field);
                    PUSH(vm, entt->value);
                    break;
                case HASHMAP_TYPE:
                case ARRAY_TYPE:
                case STR_TYPE:
                    JN_CMethod method = call_method(o, field);
                    if (method == NULL)
                        return die(state,vm, "object does not have field '%s'", field);
                    PUSH(vm, jn_obj_method(state, o, method));
                    break;
                default:
                    return die(state,vm, "Object does not support member attribute.");
                }
                break;
            case OP_RANGE:
                int has_step = READ_BYTE();
                op = READ_BYTE();
                JnObject *start_obj = pop(vm), *stop_obj = pop(vm);
                if (
                    !JN_IS_INT(start_obj) && !JN_IS_CHAR(start_obj)
                    && !JN_IS_INT(stop_obj) && !JN_IS_CHAR(stop_obj)
                )
                    return vm_error(state, vm, JN_RAISE_EXCPETION(state, TYPE_ERROR, "range object require a int or char"));
                
                int start = JN_AS_INT(start_obj); int stop = JN_AS_INT(stop_obj);
                int step = 0;
                if (has_step)
                    step = JN_AS_INT(pop(vm));
                PUSH(vm, JN_OBJECT_RANGE(state, start, stop, step));
                break;
            case OP_GET_GLOBAL:
                ident = READ_IDENT();
                Jn_environ_E* ent = environ_get(vm->env, ident);
                if (ent == NULL)
                {
                    JnObject* err_obj = JN_RAISE_EXCPETION(state, UNDEFINE_ERROR, "Seem like you did not define a variable '%s'.", ident);
                    err_obj->expection.var_name = ident;
                    return vm_error(state, vm, err_obj);
                }
                assert(ent->value != NULL);
                PUSH(vm, jn_intern_obj(ent->value));
                break;
            case OP_PRINTLN:
                JnObject* out = pop(vm);
                print_JnObject(out);
                putchar('\n');
                break;
            case OP_PLUS_PLUS:
                o = pop(vm);
                if (!JN_IS_INT(o))
                    return die(state,vm, "++ expected an int.");
                ++JN_AS_INT(o);
                PUSH(vm, o);
                break;
            case OP_NEGATE:
                o = pop(vm);
                if (NULL == o) break;
                switch (o->type)
                {
                    case INT_TYPE:
                        o->int_val = -(o->int_val);
                        PUSH(vm, o);
                        break;
                    case FLOAT_TYPE:
                        o->float_val = -o->float_val;
                        PUSH(vm, o);
                        break;
                    default:
                        return die(state,vm, "Invalid type.");
                }
                break;
            case OP_LEN:
                o = pop(vm);
                switch (o->type)
                {
                case ARRAY_TYPE:
                    PUSH(vm, JN_RETURN_INT(state, JN_AS_ARRAY(o)->size));
                    break;
                default:
                    return die(state,vm, "Expected an iterable.");
                }
                break;
            case OP_POP:
                pop(vm); break;
            case OP_DUP:
                JnObject* top = *(vm->sp - 1);
                PUSH(vm, top); break;
            case OP_SET_GLOBAL:
                o = pop(vm);
                ident = READ_IDENT();
                bool is_const = (bool)READ_BYTE();
                if (o == NULL || ident == NULL)
                    return die(state,vm, "Object not set.");
                o->constant = is_const;
                environ_insert(vm->env, ident, o);
                break;
            case OP_INDEX:
                array = pop(vm);
                JnObject* idx_key = pop(vm);
                if (!array || !idx_key)
                    return die(state,vm, "None value array or pos.");
                int index;
                switch (array->type)
                {
                    case ARRAY_TYPE:
                        if (idx_key->type != INT_TYPE && idx_key->type != RANGE_TYPE)
                            return die(state,vm, "getter attrib is not of type int or range.");
                        index = (idx_key->type == INT_TYPE) ? idx_key->int_val : range_len(&idx_key->range);
                        o = JN_GET_ARRAY(array->arr, index);
                        if (o == NULL) return die(state,vm, "Invalid array index.");
                        PUSH(vm, o);
                        break;
                    case STR_TYPE:
                        index = JN_AS_INT(idx_key);
                        if (index < 0)
                        {
                            index += array->str->len;
                        }
                        if (index >= array->str->len)
                            return die(state,vm, "invalid index got %d.", index);
                        o = JN_RETURN_CHAR(state, (array->str->chars[index]));
                        PUSH(vm, o);
                        break;
                    case RANGE_TYPE:
                        if (!JN_IS_INT(idx_key))
                            return die(state,vm, "expected an int.");
                        PUSH(vm, JN_RETURN_INT(state, range_at(&array->range, JN_AS_INT(idx_key))));
                        break;
                    case HASHMAP_TYPE:
                    Jn_HashEntry* entry = JN_HASHMAP_GET((array->hashmap), idx_key);
                    if (entry == NULL)
                        return die(state,vm, "invalid key.");
                    PUSH(vm, entry->value);
                    break;
                    default:
                        return die(state,vm, "Expected an iterable but got '%s'.", "TODO");
                }
                break;
            case OP_SET_INDEX:
                value = pop(vm); array = pop(vm); pos = pop(vm);
                if (JN_IS_ARRAY(array) && !JN_IS_INT(pos) && !JN_IS_RANGE(pos))
                    return die(state,vm, "Expected type 'int' or 'range' but got 'TODO'.");
                index = pos->int_val;
                switch (array->type)
                {
                    case ARRAY_TYPE:
                        if (index < 0)
                        {
                            index += array->arr->size;
                        }
                        if (index >= array->arr->size)
                            return die(state,vm, "Got an invalid index; expected max '%d' but got '%d'.", array->arr->size, index);
                        array->arr->items[index] = value;
                        break;
                    case STR_TYPE:
                        if (index >= array->str->len)
                                return die(state,vm, "Got an invalid index; expected max '%d' but got '%d'.", array->str->len, index);
                        if (!JN_IS_CHAR(value))
                            return die(state,vm, "string index expect a char type.");
                        array->str->chars[index] = JN_AS_CHAR(value);
                        break;
                    case HASHMAP_TYPE:
                        JN_HASMAP_PUT(array->hashmap, pos, value);
                        break;
                    default:
                        return die(state,vm, "type does not support index setting.");
                 }
                break;
            case OP_SCOPE_ENTER:
                local = Jn_environ_init(vm->env);
                vm->env = local;
                break;
            case OP_SCOPE_EXIT:
                if (vm->env->parent)
                {
                    Jn_environ* old = vm->env;
                    vm->env = vm->env->parent;
                    free(old->buckets);
                    free(old);
                }
                break;
            case OP_JUMP:
                offset = (READ_BYTE() << 8);
                offset |= READ_BYTE();
                vm->ip += offset;
                break;
            case OP_JUMP_IF_FALSE:
                offset = (READ_BYTE() << 8);
                offset |= READ_BYTE();
                o = pop(vm);//vm_peek(vm, 0);
                assert(o != NULL);
                if (!is_truthy(o))
                    vm->ip += offset;
                break;
            case OP_LOOP:
                offset = (READ_BYTE() << 8);
                offset |= READ_BYTE();
                vm->ip -= offset;
                break;
            case OP_GET_ITER:
                JnObject* iterable = pop(vm);
                if (!JN_IS_ITERABLE(iterable))
                    return die(state,vm, "object is not iterable.");
                PUSH(vm, JN_ITER_INIT(state, iterable));
                break;
            case OP_ITER_NEXT:
                JnObject* iter_obj = pop(vm);
                if (!_JN_CHECK_TYPE(iter_obj, ITER_TYPE))
                    return die(state,vm, "Expected an iter type.");
                JnIterObject* _iter = JN_AS_ITER(iter_obj);
                JnObject* target = _iter->obj;
                assert(target != NULL);
                switch (target->type)
                {
                case ARRAY_TYPE:
                    if (_iter->index >= JN_AS_ARRAY(target)->size)
                    {
                        PUSH(vm, JN_RETURN_BOOL(state, false));
                        break;
                    }
                    PUSH(vm, JN_RETURN_INT(state, _iter->index));
                    tmp = JN_AS_ARRAY(target)->items[_iter->index];
                    assert(tmp != NULL);
                    PUSH(vm, tmp);
                    _iter->index++;
                    PUSH(vm, JN_RETURN_BOOL(state, true));
                    break;
                case HASHMAP_TYPE:
                    if (_iter->index >= JN_AS_HM(target)->size)
                    {
                        PUSH(vm, JN_RETURN_BOOL(state, false));
                        break;
                    }
                    PUSH(vm, JN_RETURN_INT(state, _iter->index));
                    tmp = target->hashmap->buckets[_iter->index].key;
                    PUSH(vm, tmp);
                    _iter->index++;
                    PUSH(vm, JN_RETURN_BOOL(state, true));
                    break;
                default:
                    break;
                }
                break;
            case OP_CALL:{
                count = READ_BYTE();

                JnObject* args[20];
                size_t len = 0;
                for (int i = count - 1; i >= 0; --i)
                {
                    args[i] = pop(vm);
                    len++;
                }
                o = pop(vm);

                if (NULL == o)
                    return die(state,vm, "undefine function.");
                JnObject* arg = NULL;
                switch (o->type)
                {
                    case NATIVE_TYPE: {
                        arg = JN_OBJECT_ARG(state, args, NULL, len);
                        a = JN_CALL_NATIVE(state, o, arg);
                        if (a == NULL)
                            return die(state,vm, "SystemError: got NULL");
                        PUSH(vm, a);
                        break;
                    }
                    case METHOD_TYPE:
                        arg = JN_OBJECT_ARG(state, args, NULL, len);
                        a = o->method.fn(state, o->method.obj, arg);
                        if (NULL == a) return die(state,vm, "Invalid method call.");
                        PUSH(vm, a);
                        break;
                    case FUNCTION_TYPE: {
                        JnFunctionObject* fn = o->fn;
                        if (count != fn->arity)
                            return die(state,
                                vm, 
                                "function '%s' expected %d args but got %d",
                                fn->name, fn->arity, count
                            );
                        JnObject* res = call_function(state, vm, o, args);
                        PUSH(vm, res);
                        break;
                    }
                    default: 
                        return die(state,vm, "Invalid function call.");
                }
                break;
            }
            case OP_NONE:
                PUSH(vm, JN_RETURN_NONE); break;
            case OP_ERROR_MSG:
                ident = READ_IDENT();
                return vm_error(state, vm, JN_RAISE_EXCPETION(state, SYNTAX_ERROR, ident));
            case OP_END:
                return INTERPRET_OK;
            case OP_RETURN:
                o = pop(vm);
                PUSH(vm, o);
                return INTERPRET_OK;
            case OP_ERROR:
                return INTERPRET_RUNTIME_ERROR;
            default:
                return die(state,vm, "System error.");
        }
    }
    #undef READ_BYTE
    #undef READ_CONST
}

void compile(AST* node, Chuck* chuck)
{
    int id, idx, jump, offset, loop_start, exit_jmp, exit_jump;
    LoopContext* loop;
    int line = node->line;
    int column = node->col;
    switch (node->type)
    {
    case AST_LITERAL:
        idx = add_constant(chuck, node->literal);
        write_chuck_loc(chuck, OP_CONSTANT, line, column);
        write_chuck_loc(chuck, idx, line, column);
        break;
    case AST_ERROR:
        id = add_ident(chuck, (char *)node->error_msg);
        write_chuck_loc(chuck, OP_ERROR_MSG, line, column);
        write_chuck_loc(chuck, id, line, column);
        break;
    case AST_IDENTIFIER:
        char* ident = (char *)node->identifier;
        id = add_ident(chuck, ident);
        write_chuck_loc(chuck, OP_GET_GLOBAL, line, column);
        write_chuck_loc(chuck, id, line, column);
        break;
    case AST_MULTI_VAR:
        for (int i = 0; i < node->assign_multiple.count; ++i)
        {
            compile(node->assign_multiple.value, chuck);
            id = add_ident(chuck, node->assign_multiple.idents[i]);
            WRITE_CHUCK(chuck, OP_SET_GLOBAL);
            WRITE_CHUCK(chuck, id);
            WRITE_CHUCK(chuck, node->assign_multiple.op == TOKEN_SETTER);
        }
        break;
    case AST_TUPLE:
        printf("Elements count %d\n", node->tuple.count);
        for (size_t i = 0; i < node->tuple.count; ++i)
            compile(node->tuple.elements[i], chuck);
        WRITE_CHUCK(chuck, OP_TUPLE);
        WRITE_CHUCK(chuck, node->tuple.count);
        break;
    case AST_ARRAY:
        for (size_t i = 0; i < node->array.count; i++)
            compile(node->array.elements[i], chuck);
        WRITE_CHUCK(chuck, OP_ARRAY);
        write_chuck_loc(chuck, node->array.count, line, column);
        break;
    case AST_MEMBER:
        compile(node->member.callie, chuck);
        if (node->member.field->type == AST_IDENTIFIER)
        {
            id = add_ident(chuck, (char *)node->member.field->identifier);
            WRITE_CHUCK(chuck, OP_MEMBER);
            WRITE_CHUCK(chuck, id);
            // WRITE_CHUCK(chuck, node->member.tok); // TODO: '.' instance call and ':' static or class method call
            break;
        } else if (node->member.field->type == AST_CALL){
            AST* call = node->member.field;
            if (call->call.callee->type != AST_IDENTIFIER)
            {
                id = add_ident(chuck, "Invalid member attribute.");
                write_chuck_loc(chuck, OP_ERROR_MSG, line, column);
                write_chuck_loc(chuck, id, line, column);
                break;
            }
            id = add_ident(chuck, (char *) call->call.callee->identifier);
            WRITE_CHUCK(chuck, OP_MEMBER);
            WRITE_CHUCK(chuck, id);
            for (int i = 0; i < call->call.pos_count; ++i)
                compile(call->call.pos_args[i], chuck);
            WRITE_CHUCK(chuck, OP_CALL);
            WRITE_CHUCK(chuck, call->call.pos_count);
            break;
        }
        // id = add_ident(chuck, (char *)node->member.field->identifier);
        // WRITE_CHUCK(chuck, OP_MEMBER);
        // WRITE_CHUCK(chuck, id);
         // WRITE_CHUCK(chuck, node->member.tok); // TODO: '.' instance call and ':' static or class method call
        break;
    case AST_CALL:
        compile(node->call.callee, chuck);        
        for (int i = 0; i < node->call.pos_count; i++)
        {
            compile(node->call.pos_args[i], chuck);
        }
        write_chuck_loc(chuck, OP_CALL, line, column);
        write_chuck_loc(chuck, node->call.pos_count, line, column);
        break;
    case AST_BLOCK:
        WRITE_CHUCK(chuck, OP_SCOPE_ENTER);
        for (size_t i = 0; i < node->block.count; i++)
            compile(node->block.statements[i], chuck);
        write_chuck_loc(chuck, OP_SCOPE_EXIT, line, column);
        break;
    case AST_PRINTLN:
        compile(node->println.out, chuck);
        write_chuck_loc(chuck, OP_PRINTLN, line, column);
        break;
    case AST_UNARY:
        compile(node->unary.right, chuck);
        switch (node->unary.op)
        {
            case TOKEN_MINUS:
                write_chuck_loc(chuck, OP_NEGATE, line, column);
                break;
            case TOKEN_PLUS_PLUS:
                WRITE_CHUCK(chuck, OP_PLUS_PLUS);
                break;
            case TOKEN_NOT:
                write_chuck_loc(chuck, OP_NOT, line, column);
                break;
            default:
                write_chuck_loc(chuck, OP_ERROR, line, column);
                break;
        }
        break;
    case AST_DEFINE:
        printf("Hello World\n");
        break;
    case AST_IMPORT:
        printf("%s\n", node->import_node.lib);
        for (int i = 0; i < node->import_node.count; ++i)
            printf("Field: %s\n", node->import_node.fields[i]);
        break;
    case AST_BINARY:
        compile(node->binary.left, chuck);
        compile(node->binary.right, chuck);
        switch (node->binary.op)
        {
            case TOKEN_PLUS:
                write_chuck_loc(chuck, OP_ADD, line, column);
                break;
            case TOKEN_STAR:
                write_chuck_loc(chuck, OP_MUL, line, column);
                break;
            case TOKEN_MINUS:
                write_chuck_loc(chuck, OP_SUB, line, column);
                break;
            case TOKEN_RSHIFT:
                write_chuck_loc(chuck, OP_RSHIFT, line, column);
                break;
            case TOKEN_LSHIFT:
                write_chuck_loc(chuck, OP_LSHIFT, line, column);
                break;
            case TOKEN_EQEQ:
                WRITE_CHUCK(chuck, OP_EQUAL);
                break;
            case TOKEN_NEQ:
                WRITE_CHUCK(chuck, OP_NEQ);
                break;
            case TOKEN_GT:
                WRITE_CHUCK(chuck, OP_GT);
                break;
            case TOKEN_GTE:
                WRITE_CHUCK(chuck, OP_GTE);
                break;
            case TOKEN_LT:
                WRITE_CHUCK(chuck, OP_LT);
                break;
            case TOKEN_LTE:
                WRITE_CHUCK(chuck, OP_LTE);
                break;
            case TOKEN_BITAND:
                WRITE_CHUCK(chuck, OP_BITAND);
                break;
            case TOKEN_BITOR:
                WRITE_CHUCK(chuck, OP_BITOR);
                break;
            case TOKEN_BITAC:
                WRITE_CHUCK(chuck, OP_BITAC);
                break;
            case TOKEN_PERCENTAGE:
                WRITE_CHUCK(chuck, OP_PERC);
                break;
            case TOKEN_IN:
                WRITE_CHUCK(chuck, OP_IN);
                break;
            case TOKEN_NOT_IN:
                WRITE_CHUCK(chuck, OP_NOT_IN);
                break;
            case TOKEN_SLASH:
                WRITE_CHUCK(chuck, OP_DIV);
                break;
            case TOKEN_IS:
                WRITE_CHUCK(chuck, OP_IS); break;
            case TOKEN_AND:
                WRITE_CHUCK(chuck, OP_AND); break;
            case TOKEN_OR:
                WRITE_CHUCK(chuck, OP_OR); break;
            case TOKEN_POW:
                WRITE_CHUCK(chuck, OP_POW);
                break;
            default:
                break;
        }
        break;

    case AST_RANGE:
        if (node->range_node.has_step)
            compile(node->range_node.step, chuck);
        compile(node->range_node.stop, chuck);
        compile(node->range_node.start, chuck);
        WRITE_CHUCK(chuck, OP_RANGE);
        WRITE_CHUCK(chuck, node->range_node.has_step);
        WRITE_CHUCK(chuck, node->range_node.op);
        break;
    case AST_ENUM: 
        // TODO
        // ident = node->enum_stmt.ident;
        // JnObject* enumObj = obj_enum(ident, node->enum_stmt.fields, node->enum_stmt.count);
        // idx = add_constant(chuck, enumObj);
        // WRITE_CHUCK(chuck, OP_CONSTANT);
        // WRITE_CHUCK(chuck, idx);

        // id = add_ident(chuck, ident);
        // WRITE_CHUCK(chuck, OP_SET_GLOBAL);
        // WRITE_CHUCK(chuck, id);
        // WRITE_CHUCK(chuck, 1);
        break;
    case AST_INLINE_IF:
        compile(node->inline_if_stmt.cond, chuck);
        int inline_false_jmp = emit_jump(chuck, OP_JUMP_IF_FALSE);
        compile(node->inline_if_stmt.then, chuck);
        int inline_end_jmp = emit_jump(chuck, OP_JUMP);
        patch_jump(chuck, inline_false_jmp);
        compile(node->inline_if_stmt.otherwise, chuck);
        patch_jump(chuck, inline_end_jmp);
        break;
    case AST_MATCH:
        compile(node->match_node.subject, chuck);
        int end_jumps[256];
        int end_count = 0;
        for (size_t i = 0; i < node->match_node.cases->count; ++i)
        {
            case_o caseObj = node->match_node.cases->cases[i];
            WRITE_CHUCK(chuck, OP_DUP);
            compile(caseObj.pattern, chuck);
            WRITE_CHUCK(chuck, OP_EQUAL);
            int next_case = emit_jump(chuck, OP_JUMP_IF_FALSE);
            compile(caseObj.block, chuck);
            end_jumps[end_count++] = emit_jump(chuck, OP_JUMP);
            patch_jump(chuck, next_case);
        }
        if (node->match_node.def)
        {
            compile(node->match_node.def, chuck);
        }
        for (int i = 0; i < end_count; ++i)
            patch_jump(chuck, end_jumps[i]);

        break;
    case AST_STRUCT:
        JnObject* struct_obj =  JN_RETURN_STRUCT(node->state, NULL, node->struct_node.fields);
        struct_obj->struct_obj->field_count = node->struct_node.count;
        idx = add_constant(chuck, struct_obj);
        WRITE_CHUCK(chuck, OP_CONSTANT);
        WRITE_CHUCK(chuck, idx);
        break;

    case AST_INSTANCE:
        compile(node->instance_node.object, chuck);

        for (int i = 0; i < node->instance_node.count; ++i)
        {
            compile(node->instance_node.values[i], chuck);
        }
        WRITE_CHUCK(chuck, OP_INSTANCE);
        WRITE_CHUCK(chuck, node->instance_node.count);

        for (int i = 0; i < node->instance_node.count; ++i)
        {
            id = add_ident(chuck, node->instance_node.fields[i]);
            WRITE_CHUCK(chuck, id);
        }
        break;
    case AST_LAMBDA:
        JnObject* lambda_obj = jn_obj_lambda(
            node->state,
            node->lambda_node.expr,
            node->lambda_node.args,
            node->lambda_node.count,
            chuck->env
        );
        idx = add_constant(chuck, lambda_obj);
        WRITE_CHUCK(chuck, OP_CONSTANT);
        WRITE_CHUCK(chuck, idx);
        break;
    case AST_FUNCTION:
        JnObject* fn_obj = jn_obj_function(
            node->state,
            node->fn_node.block,
            chuck->env, node->fn_node.params, node->fn_node.count, node->fn_node.name
        );
        idx = add_constant(chuck, fn_obj);
        WRITE_CHUCK(chuck, OP_CONSTANT);
        WRITE_CHUCK(chuck, idx);

        id = add_ident(chuck, node->fn_node.name);
        WRITE_CHUCK(chuck, OP_SET_GLOBAL);
        WRITE_CHUCK(chuck, id);
        WRITE_CHUCK(chuck, 0);
        break;
    case AST_IF:
        compile(node->if_node.condition, chuck);
        int false_jump = emit_jump(chuck, OP_JUMP_IF_FALSE);
        // WRITE_CHUCK(chuck, OP_POP);
        compile(node->if_node.then, chuck);
        int end_jump = emit_jump(chuck, OP_JUMP);
        patch_jump(chuck, false_jump);
        // WRITE_CHUCK(chuck, OP_POP);
        for (size_t i = 0; i < node->if_node.elseif->count; i++)
        {
            elif_node elif = node->if_node.elseif->children[i];
            compile(elif.cond, chuck);
            int elif_false = emit_jump(chuck, OP_JUMP_IF_FALSE);
            // WRITE_CHUCK(chuck, OP_POP);
            compile(elif.stmt, chuck);
            int elif_end = emit_jump(chuck, OP_JUMP);
            patch_jump(chuck, elif_false);
            // WRITE_CHUCK(chuck, OP_POP);
            patch_jump(chuck, elif_end);
        }
        if (node->if_node.else_node)
            compile(node->if_node.else_node, chuck);
        patch_jump(chuck, end_jump);
        break;
    case AST_HASHMAP:
        for (size_t i = 0; i < node->hmp_node.count; ++i)
        {
            compile(node->hmp_node.keys[i], chuck);
            compile(node->hmp_node.values[i], chuck);
        }
        WRITE_CHUCK(chuck, OP_HM);
        WRITE_CHUCK(chuck, node->hmp_node.count);
        break;
    case AST_BREAK:
        if (loop_depth <= 0)
        {
            id = add_ident(chuck, "cannot add 'break' outside a loop.");
            write_chuck_loc(chuck, OP_ERROR_MSG, line, column);
            write_chuck_loc(chuck, id, line, column);
            break;
        }
        jump = emit_jump(chuck, OP_JUMP);
        loop = &loop_stack[loop_depth - 1];
        loop->breaks[loop->break_count++] = jump;
        break;
    case AST_RETURN:
        if (node->return_stmt.value != NULL)
            compile(node->return_stmt.value, chuck);
        else
            WRITE_CHUCK(chuck, OP_NONE);
        
        WRITE_CHUCK(chuck, OP_RETURN);
        break;
    case AST_CONTINUE:
        if (loop_depth <= 0)
        {
            id = add_ident(chuck, "cannot add 'continue' outside a loop.");
            write_chuck_loc(chuck, OP_ERROR_MSG, line, column);
            write_chuck_loc(chuck, id, line, column);
            break;
        }
        loop = &loop_stack[loop_depth - 1];
        jump = emit_jump(chuck, OP_JUMP);
        loop->continues[loop->continue_count++] = jump;
        break;

    case AST_FOR:
        loop = &loop_stack[loop_depth++];
        
        loop->break_count = 0;
        loop->continue_count = 0;
        
        WRITE_CHUCK(chuck, OP_SCOPE_ENTER);

        if (node->for_node.init != NULL)
            compile(node->for_node.init, chuck);

        offset = current_offset(chuck);
        loop->loop_offset = offset;

        if (node->for_node.cond)
            compile(node->for_node.cond, chuck);
        else {
            WRITE_CHUCK(chuck, OP_TRUE);
        }
        exit_jump = emit_jump(chuck, OP_JUMP_IF_FALSE);
        
        compile(node->for_node.block, chuck);

        int continue_target = current_offset(chuck);

        for (int i = 0; i < loop->continue_count; ++i)
        {
            patch_jump_to(chuck, loop->continues[i], continue_target);
        }
        
        if (node->for_node.incr)
            compile(node->for_node.incr, chuck);

        emit_loop(chuck, offset);
        patch_jump(chuck, exit_jump);

        for (int i = 0; i < loop->break_count; i++)
        {
            patch_jump(chuck, loop->breaks[i]);
        }
        WRITE_CHUCK(chuck, OP_SCOPE_EXIT);
        loop_depth--;
        break;
    case AST_LOOP:
        LoopContext* loop_p = &loop_stack[loop_depth++];
        
        offset = current_offset(chuck);
        loop_p->loop_offset = offset;
        loop_p->break_count = 0;
        loop_p->continue_count = 0;

        compile(node->loop_stmt.block, chuck);
        
        for (int i = 0; i < loop_p->continue_count; i++)
        {
            patch_jump(chuck, loop_p->continues[i]);
        }
        emit_loop(chuck, loop_p->loop_offset);
        for (int i = 0; i < loop_p->break_count; i++)
        {
            patch_jump(chuck, loop_p->breaks[i]);
        }
        loop_depth--;
        break;
    case AST_WHILE:
        loop = &loop_stack[loop_depth++];
        
        offset = current_offset(chuck);
        loop->loop_offset = offset;
        loop->break_count = 0;
        loop->continue_count = 0;
        
        compile(node->while_node.cond, chuck);
        exit_jump = emit_jump(chuck, OP_JUMP_IF_FALSE);
        compile(node->while_node.block, chuck);
        emit_loop(chuck, offset);
        patch_jump(chuck, exit_jump);
        
        for (int i = 0; i < loop->continue_count; i++)
        {
            patch_jump(chuck, loop->continues[i]);
        }
        
        for (int i = 0; i < loop->break_count; i++)
        {
            patch_jump(chuck, loop->breaks[i]);
        }
        loop_depth--;
        break;
    case AST_REASSIGN:
        compile(node->reassign.value, chuck);

        if (node->reassign.op == TOKEN_WALRUS || node->reassign.op == TOKEN_SETTER)
        {
            if (node->reassign.expr->type != AST_IDENTIFIER)
            {
                id = add_ident(chuck, "What a weird way to initialize a variable.");
                write_chuck_loc(chuck, OP_ERROR_MSG, line, node->reassign.expr->col);
                write_chuck_loc(chuck, id, line, node->reassign.expr->col);
                break;
            }
            id = add_ident(chuck, (char *)node->reassign.expr->identifier);

            WRITE_CHUCK(chuck, OP_SET_GLOBAL);
            WRITE_CHUCK(chuck, id);
            WRITE_CHUCK(chuck, node->reassign.op == TOKEN_SETTER);
            break;
        }

        compile(node->reassign.expr, chuck);

        WRITE_CHUCK(chuck, OP_REASSIGN);
        WRITE_CHUCK(chuck, node->reassign.op);
        break;
    case AST_ASSIGN:
        compile(node->assign.value, chuck);
        id = add_ident(chuck, node->assign.name);
        WRITE_CHUCK(chuck, OP_SET_GLOBAL);
        WRITE_CHUCK(chuck, id);
        WRITE_CHUCK(chuck, (uint8_t)node->assign.is_const);
        break;
    case AST_ARRAY_INDEX:
        compile(node->index.pos, chuck);
        compile(node->index.array, chuck);
        if (node->index.is_set)
        {
            compile(node->index.value, chuck);
            WRITE_CHUCK(chuck, OP_SET_INDEX);
            break;
        } 
        WRITE_CHUCK(chuck, OP_INDEX);
        break;
    default:
        break;
    }
}
