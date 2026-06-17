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
    assert(vm != NULL && obj != NULL);              \
    if (JN_IS_ERROR(obj)) return vm_error(vm, obj); \
    push(vm, obj);                                  \
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
    chuck->capacity = 100;
    chuck->constants_count = 0;
    chuck->constants_capacity = 100;
    chuck->code = malloc(sizeof(uint8_t) * chuck->capacity);
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
}

void vm_free(JnVM* vm)
{
    assert(vm != NULL);
    free(vm->sp);
    free(vm->ip);
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

static int vm_error(JnVM* vm, JnObject* obj)
{
    assert(obj != NULL && JN_IS_ERROR(obj));
    J_Context* ctx = Jn_get_context();
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
    printf("%s\n", obj->expection.error_msg);
    print_source_lines(ctx->source.source, ctx->cur_line, ctx->column, 2);
    return INTERPRET_ERROR;
}

static InterpretResult die(JnVM* vm, const char* msg, ...)
{
    J_Context* ctx = Jn_get_context();
    ctx->cur_line = vm_line(vm);
    ctx->column = vm_column(vm);
    va_list arg; va_start(arg, msg);
    fprintf(
        stderr, 
        "RuntimeError: '%s':%d:%d \n\t",  ctx->source.filename ? ctx->source.filename: "main",
        ctx->cur_line, ctx->column
    );
    vfprintf(stderr, msg, arg);
    fputc('\n', stderr);
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

static JnObject *a, *b, *key, *value, *array, *pos;

InterpretResult vm_run(JnVM* vm)
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
        J_Context* ctx = Jn_get_context();
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
                a = pop(vm);
                b = pop(vm);
                a = jn_intern_obj(eval_binary(b, a, EVAL_ADD));
                if (NULL == a)
                    return die(vm, "Invalid binary");
                PUSH(vm, a);
                break;
            case OP_SUB:
                a = pop(vm);
                b = pop(vm);
                a = eval_binary(b, a, EVAL_SUB);
                if (NULL == a)
                    return die(vm, "Invalid binary");
                PUSH(vm, a);
                break;
            case OP_MUL:
                a = pop(vm);
                b = pop(vm);
                a->int32 = a->int32 * b->int32;
                PUSH(vm, a);
                break;
            case OP_BITAND:
                a = pop(vm);
                b = pop(vm);
                a = eval_binary(b, a, EVAL_BAND);
                if (NULL == a)
                    return die(vm, "Invalid binary");
                PUSH(vm, a);
                break;
            case OP_BITOR:
                a = pop(vm);
                b = pop(vm);
                a = eval_binary(b, a, EVAL_BOR);
                if (NULL == a)
                    return die(vm, "Invalid binary");
                PUSH(vm, a);
                break;
            case OP_PERC:
                a = pop(vm);
                b = pop(vm);
                a = eval_binary(b, a, EVAL_PERC);
                if (NULL == a)
                    return die(vm, "Invalid binary");
                PUSH(vm, a);
                break;
            case OP_DIV:
                a = pop(vm); b = pop(vm);
                a = eval_binary(b, a, EVAL_DIV);
                PUSH(vm, a);
                break;
            case OP_BITAC:
                a = pop(vm);
                b = pop(vm);
                a = eval_binary(b, a, EVAL_BAC);
                if (NULL == a)
                    return die(vm, "Invalid binary");
                PUSH(vm, a);
                break;
            case OP_EQUAL:
                a = pop(vm);
                b = pop(vm);
                a = eval_binary(b, a, EVAL_EQUAL);
                if (NULL == a)
                    return die(vm, "Invalid binary");
                PUSH(vm, a);
                break;
            case OP_LSHIFT:
                a = pop(vm);
                b = pop(vm);
                a = eval_binary(b, a, EVAL_LSHIFT);
                if (NULL == a)
                    return die(vm, "Invalid binary");
                PUSH(vm, a);
                break;
            case OP_RSHIFT:
                a = pop(vm);
                b = pop(vm);
                a = eval_binary(b, a, EVAL_RSHIFT);
                if (NULL == a)
                    return die(vm, "Invalid binary");
                PUSH(vm, a);
                break;
            case OP_NEQ:
                a = pop(vm);
                b = pop(vm);
                a = eval_binary(b, a, EVAL_NOTEQUAL);
                if (NULL == a)
                    return die(vm, "Invalid binary");
                PUSH(vm, a);
                break;
            case OP_GT:
                a = pop(vm);
                b = pop(vm);
                a = eval_binary(b, a, EVAL_GT);
                if (NULL == a)
                    return die(vm, "Invalid binary");
                PUSH(vm, a);
                break;
            case OP_GTE:
                a = pop(vm);
                b = pop(vm);
                a = eval_binary(b, a, EVAL_GTE);
                if (NULL == a)
                    return die(vm, "Invalid binary");
                PUSH(vm, a);
                break;
            case OP_LT:
                a = pop(vm);
                b = pop(vm);
                a = eval_binary(b, a, EVAL_LT);
                if (NULL == a)
                    return die(vm, "Invalid binary");
                PUSH(vm, a);
                break;
            case OP_LTE:
                a = pop(vm);
                b = pop(vm);
                a = eval_binary(b, a, EVAL_LTE);
                if (NULL == a)
                    return die(vm, "Invalid binary");
                PUSH(vm, a);
                break;
            case OP_ARRAY:
                count = READ_BYTE();
                JnArrayObject* arr = NULL;
                for (int i = count - 1; i >= 0; --i)
                {
                    JN_SET_ARRAY(arr, pop(vm), i);
                }
                assert(arr != NULL);
                o = JN_OBJECT(ARRAY_TYPE);
                o->arr = arr;
                PUSH(vm, o);
                break;
            case OP_ITER:
                // count = READ_BYTE();
                // iter = ObjectIter(count);
                // for (int i = count - 1; i >= 0; --i)
                // {
                //     //PUSHItem(iter, pop(vm));
                //     iter->items[i] = pop(vm);
                //     iter->count++;
                // }
                // o = jn_obj_new(ITER_TYPE);
                // o->iter = iter;
                // PUSH(vm, o);
                break;
            case OP_HM:
                count = READ_BYTE();
                Jn_Hashmap* map = NULL;
                for (int i = count - 1; i >= 0; --i)
                {
                    value = pop(vm); key = pop(vm);
                    JN_HASHMAP_INSERT(map, key, value, i);
                }
                assert(map != NULL);
                JnObject* obj = JN_OBJECT(HASHMAP_TYPE);
                obj->hashmap = map;
                PUSH(vm, obj);
                break;
            case OP_REASSIGN:
                ident = READ_IDENT();
                int t_op = READ_BYTE();
                a = pop(vm);
                Jn_environ_E* entry = environ_get(vm->env, ident);
                if (NULL == a || NULL == entry)
                    return die(vm, "undefine variable '%s', \tI think you meant ':=' but forgot.", ident);
                if (entry->value->constant)
                    return die(vm, "Cannot reassign a variable of const.");
                o = entry->value;
                switch (t_op)
                {
                    case TOKEN_APLUS:
                        b = eval_binary(o, a, EVAL_ADD);
                        if (NULL == b)
                            break;
                        jn_obj_reassign(o, b);
                        break;
                    case TOKEN_AMINUS:
                        b = eval_binary(o, a, EVAL_SUB);
                        if (NULL == b)
                            break;
                        jn_obj_reassign(o, b);
                        break;
                    case TOKEN_EQUAL:
                        jn_obj_reassign(o, a);
                        break;
                    case TOKEN_ASTAR:
                        b = eval_binary(o, a, EVAL_MUL);
                        if (NULL == b)
                            break;
                        jn_obj_reassign(o, b);
                        break;
                    case TOKEN_ARSHIFT:
                        b = eval_binary(o, a, EVAL_RSHIFT);
                        if (NULL == b)
                            break;
                        jn_obj_reassign(o, b);

                        break;
                    case TOKEN_ALSHIFT:
                        b = eval_binary(o, a, EVAL_LSHIFT);
                        if (NULL == b)
                            break;
                        jn_obj_reassign(o, b);
                        break;
                    case TOKEN_BITAC:
                        b = eval_binary(o, a, EVAL_BAC);
                        if (NULL == b)
                            break;
                        jn_obj_reassign(o, b);
                        break;
                    case TOKEN_APERCENTAGE:
                        b = eval_binary(o, a, EVAL_PERC);
                        if (NULL == b)
                            break;
                        jn_obj_reassign(o, b);
                        break;
                    case TOKEN_ABITAC:
                        b = eval_binary(o, a, EVAL_BAC);
                        if (NULL == b)
                            break;
                        jn_obj_reassign(o, b);
                        break;
                    case TOKEN_ASLASH:
                        b = eval_binary(o, a, EVAL_DIV);
                        if (NULL == b)
                            break;
                        jn_obj_reassign(o, b);
                        break;
                    case TOKEN_ABITAND:
                        b = eval_binary(o, a, EVAL_BAND);
                        if (NULL == b)
                            break;
                        jn_obj_reassign(o, b);
                        break;
                    case TOKEN_ABITOR:
                        b = eval_binary(o, a, EVAL_BOR);
                        if (NULL == b)
                            break;
                        jn_obj_reassign(o, b);
                        break;
                    default:
                        return die(vm, "invalid operator.");
                }
                break;
            case OP_IN:
                a = pop(vm);
                b = pop(vm);
                a = eval_binary(b, a, EVAL_IN);
                PUSH(vm, a);
                break;
            case OP_NOT_IN:
                a = pop(vm);
                b = pop(vm);
                a = eval_binary(b, a, EVAL_NOT_IN);
                PUSH(vm, a);
                break;
            case OP_IS:
                b = pop(vm); a = pop(vm);
                a = eval_binary(a, b, EVAL_IS);
                if (NULL == a)
                    return die(vm, "Invalid binary opration.");
                PUSH(vm, a);
                break;
            case OP_AND:
                b = pop(vm); a = pop(vm);
                a = jn_intern_obj(eval_binary(a, b, EVAL_AND));
                if (NULL == a)
                    return die(vm, "Invalid binary opration.");
                PUSH(vm, a);
                break;
            case OP_OR:
                b = pop(vm); a = pop(vm);
                a = jn_intern_obj(eval_binary(a, b, EVAL_OR));
                if (NULL == a)
                    return die(vm, "Invalid binary opration.");
                PUSH(vm, a);
                break;
            case OP_NOT:
                o = pop(vm);
                PUSH(vm, jn_obj_bool(!is_truthy(o)));
                break;
            case OP_ASSERT:
                o = pop(vm);
                char * msg = READ_IDENT();
                if (!is_truthy(o))
                    return vm_error(vm, JN_RAISE_EXCPETION(ASSERT_ERROR, msg));
                break;
            case OP_MEMBER:
                char* field = READ_IDENT(); o = pop(vm); op = READ_BYTE();
                printf("FIeld member %s, Object type %d Token %d\n", ident, o->type, op);
                // I assuming every object is an enum object: TODO
                switch (op)
                {
                    case TOKEN_EXR:
                        // TODO
                        break;
                    case TOKEN_DOT:
                        break; // instance call
                    default:
                        return die(vm, "Got an invalid member token %d\n", op);
                }
                PUSH(vm, jn_obj_none()); // for now
                break;
            case OP_RANGE:
                int has_step = READ_BYTE();
                op = READ_BYTE();
                int start = JN_AS_INT(pop(vm)); int stop = JN_AS_INT(pop(vm));
                int step = 0;
                if (has_step)
                    step = JN_AS_INT(pop(vm));
                PUSH(vm, JN_OBJECT_RANGE(start, stop, step));
                break;
            case OP_GET_GLOBAL:
                ident = READ_IDENT();
                Jn_environ_E* ent = environ_get(vm->env, ident);
                if (ent == NULL)
                    return die(vm, "Seem like you did not define a variable '%s'.", ident);
                assert(ent->value != NULL);
                PUSH(vm, jn_intern_obj(ent->value));
                break;
            case OP_PRINTLN:
                JnObject* out = pop(vm);
                print_JnObject(out);
                putchar('\n');
                break;
            case OP_NEGATE:
                o = pop(vm);
                if (NULL == o) break;
                switch (o->type)
                {
                    case INT_TYPE:
                        o->int32 = -(o->int32);
                        PUSH(vm, o);
                        break;
                    case FLOAT_TYPE:
                        o->float32 = -o->float32;
                        PUSH(vm, o);
                        break;
                    default:
                        return die(vm, "Invalid type.");
                }
                break;
            case OP_LEN:
                o = pop(vm);
                switch (o->type)
                {
                case ARRAY_TYPE:
                    PUSH(vm, JN_RETURN_INT(JN_AS_ARRAY(o)->size));
                    break;
                default:
                    return die(vm, "Expected an iterable.");
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
                    return die(vm, "Object not set.");
                o->constant = is_const;
                environ_insert(vm->env, ident, o);
                break;
            case OP_INDEX:
                array = pop(vm);
                JnObject* idx_key = pop(vm);
                if (!array || !idx_key)
                    return die(vm, "None value array or pos.");
                if (JN_IS_ARRAY(array) && idx_key->type != INT_TYPE && idx_key->type != RANGE_TYPE)
                    return die(vm, "pos is not an int");
                int index = (idx_key->type == INT_TYPE) ? idx_key->int32: range_len(&idx_key->range);
                switch (array->type)
                {
                    case ARRAY_TYPE:
                        o = JN_GET_ARRAY(array->arr, index);
                        if (o == NULL) return die(vm, "Invalid array index.");
                        PUSH(vm, o);
                        break;
                    case STR_TYPE:
                        if (index < 0)
                        {
                            index += array->str->len;
                        }
                        if (index >= array->str->len)
                            return die(vm, "invalid index got %d.", index);
                        char* str = malloc(2);
                        str[0] = array->str->chars[index];
                        str[1] = '\0';
                        o = jn_obj_string(str);
                        PUSH(vm, o);
                        break;
                    case ITER_TYPE:
                        // if (index < 0 || index >= array->iter->count)
                        //     return die(vm, "pos is > or < array length");
                        // o = array->iter->items[index];
                        // PUSH(vm, o);
                        break;
                    case HASHMAP_TYPE:
                    Jn_HashEntry* entry = JN_HASHMAP_GET((array->hashmap), idx_key);
                    if (entry == NULL)
                        return die(vm, "invalid key.");
                    PUSH(vm, entry->value);
                    break;
                    default:
                        return die(vm, "Expected an iterable but got '%s'.", "TODO");
                }
                break;
            case OP_SET_INDEX:
                value = pop(vm); array = pop(vm); pos = pop(vm);
                if (JN_IS_ARRAY(array) && !JN_IS_INT(pos) && !JN_IS_RANGE(pos))
                    return die(vm, "Expected type 'int' or 'range' but got 'TODO'.");
                index = pos->int32;
                switch (array->type)
                {
                    case ARRAY_TYPE:
                        if (index < 0)
                        {
                            index += array->arr->size;
                        }
                        if (index >= array->arr->size)
                            return die(vm, "Got an invalid index; expected max '%d' but got '%d'.", array->arr->size, index);
                        array->arr->items[index] = value;
                        break;
                    case STR_TYPE:
                        if (index >= array->str->len)
                                return die(vm, "Got an invalid index; expected max '%d' but got '%d'.", array->str->len, index);
                        if (value->type != STR_TYPE)
                            return die(vm, "string index expect a string value.");
                        if (value->str->len > 0)
                            return die(vm, "Can only set a char to a string.");
                        array->str->chars[index] = value->str->chars[0];
                        break;
                    case HASHMAP_TYPE:
                        JN_HASMAP_PUT(array->hashmap, pos, value);
                        break;
                    case ITER_TYPE:
                        return die(vm, "Iter object does not support index setting.");
                    default:
                        return die(vm, "type does not support index setting.");
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
                    return die(vm, "object is not iterable.");
                PUSH(vm, JN_ITER_INIT(iterable));
                break;
            case OP_ITER_NEXT:
                JnObject* iter_obj = pop(vm);
                if (!_JN_CHECK_TYPE(iter_obj, ITER_TYPE))
                    return die(vm, "Expected an iter type.");
                JnIterObject* _iter = JN_AS_ITER(iter_obj);
                JnObject* target = _iter->obj;
                assert(target != NULL);
                switch (target->type)
                {
                case ARRAY_TYPE:
                    if (_iter->index >= JN_AS_ARRAY(target)->size)
                    {
                        PUSH(vm, JN_RETURN_BOOL(false));
                        break;
                    }
                    PUSH(vm, JN_RETURN_INT(_iter->index));
                    tmp = JN_AS_ARRAY(target)->items[_iter->index++];
                    assert(tmp != NULL);
                    PUSH(vm, tmp);
                    PUSH(vm, JN_RETURN_BOOL(true));
                    break;
                case HASHMAP_TYPE:
                    if (_iter->index >= JN_AS_HM(target)->size)
                    {
                        PUSH(vm, JN_RETURN_BOOL(false));
                        break;
                    }
                    PUSH(vm, JN_RETURN_INT(_iter->index));
                    tmp = target->hashmap->buckets[_iter->index++].key;
                    PUSH(vm, tmp);
                    PUSH(vm, JN_RETURN_BOOL(true));
                    break;
                default:
                    break;
                }
                break;
            case OP_CALL:{
                count = READ_BYTE(); o = pop(vm);
                if (NULL == o)
                    return die(vm, "undefine function '%s'.", ident);
                if (o->type != FUNCTION_TYPE && o->type != NATIVE_TYPE)
                    return die(vm, "%s is not a callable.", ident);
                JnObject* args[20];
                size_t len = 0;
                for (int i = count - 1; i >= 0; --i)
                {
                    args[i] = pop(vm);
                    len++;
                }
                switch (o->type)
                {
                    case NATIVE_TYPE: {
                        JN_Args arg = Jn_make_arg(args, len);
                        a = o->native_fn->fn(arg);
                        if (a == NULL)
                            return die(vm, "SystemError: got NULL");
                        PUSH(vm, a);
                        break;
                    }
                    case FUNCTION_TYPE: {
                        //  TODO
                        JnFunctionObject* fn = o->fn;
                        if (fn->name == NULL)
                            fn->name = "<lambda>";
                        if (count != fn->arity)
                            return die(
                                vm, 
                                "function '%s' expected %d args but got %d",
                                fn->name, fn->arity, count
                            );
                        if (vm->frame_count >= _FRAME_MAX)
                            return die(vm, "Stack Overflow");
                        CallFrame* current = &vm->frames[vm->frame_count++];
                        current->fn = fn;
                        current->ip = vm->ip;
                        current->env = vm->env;
                        local = Jn_environ_init(fn->env);

                        for (int i = fn->arity - 1; i >= 0; --i)
                        {
                            environ_insert(local, fn->params[i], args[i]);
                        }
                        vm->env = local;
                        vm->ip = fn->chuck->code;

                        break;
                    }
                    default: 
                        return die(vm, "Invalid function call.");
                }
                break;
            }
            case OP_ERROR_MSG:
                ident = READ_IDENT();
                printf("%s\n", ident);
                return INTERPRET_RUNTIME_ERROR;
            case OP_END:
                return INTERPRET_OK;
            case OP_RETURN:
                o = pop(vm);
                if (vm->frame_count == 0)
                {
                    PUSH(vm, o);
                    return INTERPRET_OK;
                }
                Jn_environ* old = vm->env;
                CallFrame* frame = &vm->frames[vm->frame_count - 1];
                vm->ip = frame->ip;
                vm->env = frame->env;
                free(old);
                PUSH(vm, o);
                break;
            case OP_ERROR:
                return INTERPRET_RUNTIME_ERROR;
            default:
                return die(vm, "System error.");
        }
    }
    #undef READ_BYTE
    #undef READ_CONST
}

void compile(AST* node, Chuck* chuck)
{
    int id, idx, jump, offset, loop_start, exit_jmp;
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
    case AST_ARRAY:
        for (size_t i = 0; i < node->array.count; i++)
            compile(node->array.elements[i], chuck);
        WRITE_CHUCK(chuck, OP_ARRAY);
        write_chuck_loc(chuck, node->array.count, line, column);
        break;
    case AST_TUPLE:
        for (size_t i = 0; i < node->tuple.count; ++i)
            compile(node->tuple.elements[i], chuck);
        write_chuck_loc(chuck, OP_ITER, line, column);
        write_chuck_loc(chuck, node->tuple.count, line, column);
        break;
    case AST_MEMBER:
        compile(node->member.callie, chuck);
        idx = add_ident(chuck, node->member.field);
        write_chuck_loc(chuck, OP_MEMBER, line, column);
        write_chuck_loc(chuck, idx, line, column);
        write_chuck_loc(chuck, node->member.tok, line, column);
        break;
    case AST_CALL:
        
        for (int i = 0; i < node->call.pos_count; i++)
        {
            compile(node->call.pos_args[i], chuck);
        }
        compile(node->call.callee, chuck);
        write_chuck_loc(chuck, OP_CALL, line, column);
        write_chuck_loc(chuck, node->call.pos_count, line, column);
        break;
    case AST_BLOCK:
        WRITE_CHUCK(chuck, OP_SCOPE_ENTER);
        for (size_t i = 0; i < node->block.count; i++)
        {
            compile(node->block.statements[i], chuck);
        }
        write_chuck_loc(chuck, OP_SCOPE_EXIT, line, column);
        break;
    case AST_PRINTLN:
        compile(node->println.out, chuck);
        write_chuck_loc(chuck, OP_PRINTLN, line, column);
        break;
    case AST_ASSERT:
        compile(node->assert_stmt.cond, chuck);
        if (node->assert_stmt.msg != NULL)
            id = add_ident(chuck, node->assert_stmt.msg);
        else
            id  = add_ident(chuck, "Assertion failed.");

        write_chuck_loc(chuck, OP_ASSERT, line, column);
        write_chuck_loc(chuck, id, line, column);
        break;
    case AST_UNARY:
        compile(node->unary.right, chuck);
        switch (node->unary.op)
        {
            case TOKEN_MINUS:
                write_chuck_loc(chuck, OP_NEGATE, line, column);
                break;
            // case TOKEN_STAR:
            //     write_chuck_loc(chuck, OP_MUL);
            //     break;
            case TOKEN_NOT:
                write_chuck_loc(chuck, OP_NOT, line, column);
                break;
            default:
                write_chuck_loc(chuck, OP_ERROR, line, column);
                break;
        }
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
    case AST_LAMBDA:
        Chuck* lamda_chuck = malloc(sizeof(Chuck));
        assert(lamda_chuck != NULL);
        chuck_init(lamda_chuck);
        lamda_chuck->env = chuck->env;
        compile(node->lambda_node.expr, lamda_chuck);
        WRITE_CHUCK(lamda_chuck, OP_RETURN);
        WRITE_CHUCK(lamda_chuck, OP_END);
        JnObject* lambda_obj = jn_obj_function(
            lamda_chuck, 
            node->lambda_node.args, 
            node->lambda_node.count, 
            NULL // lambda functions
        );
        lambda_obj->fn->env = chuck->env;
        idx = add_constant(chuck, lambda_obj);
        WRITE_CHUCK(chuck, OP_CONSTANT);
        WRITE_CHUCK(chuck, idx);
        break;
    case AST_FUNCTION: 
        // Chuck fn_chuck;
        // fn_chuck.env = chuck->env;
        // chuck_init(&fn_chuck);
        // compile(node->fn_node.block, &fn_chuck);
        // // idx = add_constant(&fn_chuck, obj_none());
        // // WRITE_CHUCK(&fn_chuck, OP_CONSTANT);
        // // WRITE_CHUCK(&fn_chuck, idx);
        // WRITE_CHUCK(&fn_chuck, OP_END);
        // JnObject* objFn = jn_obj_function(
        //     &fn_chuck,
        //     node->fn_node.params,
        //     node->fn_node.count,
        //     node->fn_node.name
        // );
        // idx = add_constant(chuck, objFn);
        // WRITE_CHUCK(chuck, OP_CONSTANT);
        // WRITE_CHUCK(chuck, idx);

        // id = add_ident(chuck, node->fn_node.name);
        // WRITE_CHUCK(chuck, OP_SET_GLOBAL);
        // WRITE_CHUCK(chuck, id);
        // WRITE_CHUCK(chuck, 1);
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
            fprintf(stderr, "SystemError: add 'break' outside a loop.\n");
            exit(72);
        }
        jump = emit_jump(chuck, OP_JUMP);
        loop = &loop_stack[loop_depth - 1];
        loop->breaks[loop->break_count++] = jump;
        break;
    case AST_RETURN:
        compile(node->return_stmt.value, chuck);
        WRITE_CHUCK(chuck, OP_RETURN);
        break;
    case AST_CONTINUE:
        if (loop_depth <= 0)
        {
            fprintf(stderr, "SystemError: add 'continue' outside a loop.\n");
            exit(72);
        }
        loop = &loop_stack[loop_depth - 1];
        jump = emit_jump(chuck, OP_JUMP);
        loop->continues[loop->continue_count++] = jump;
        break;
    
    case AST_FOR:
        WRITE_CHUCK(chuck, OP_SCOPE_ENTER);
        char tmp[200];
        snprintf(tmp, sizeof(tmp), "__iter_%d", iter_count++);
        compile(node->for_node.iter, chuck);
        int iter_id = add_ident(chuck, tmp);
        WRITE_CHUCK(chuck, OP_SET_GLOBAL);
        WRITE_CHUCK(chuck, iter_id);
        WRITE_CHUCK(chuck, 0);

        idx = add_constant(chuck, JN_RETURN_INT(0));
        WRITE_CHUCK(chuck, OP_CONSTANT);
        WRITE_CHUCK(chuck, idx);

        int idx_id = add_ident(chuck, node->for_node.index);
        WRITE_CHUCK(chuck, OP_SET_GLOBAL);
        WRITE_CHUCK(chuck, idx_id);
        WRITE_CHUCK(chuck, 0);

        loop = &loop_stack[loop_depth++];
        loop->break_count = 0;
        loop->continue_count = 0;

        int loop_start = current_offset(chuck);
        loop->loop_offset = loop_start;

        WRITE_CHUCK(chuck, OP_GET_GLOBAL);
        WRITE_CHUCK(chuck, idx_id);

        WRITE_CHUCK(chuck, OP_GET_GLOBAL);
        WRITE_CHUCK(chuck, iter_id);

        WRITE_CHUCK(chuck, OP_LEN);
        WRITE_CHUCK(chuck, OP_LT);

        exit_jmp = emit_jump(chuck, OP_JUMP_IF_FALSE);

        WRITE_CHUCK(chuck, OP_GET_GLOBAL);
        WRITE_CHUCK(chuck, idx_id);
        WRITE_CHUCK(chuck, OP_GET_GLOBAL);
        WRITE_CHUCK(chuck, iter_id);

        WRITE_CHUCK(chuck, OP_INDEX);

        int var_id = add_ident(chuck, node->for_node.ident);
        WRITE_CHUCK(chuck, OP_SET_GLOBAL);
        WRITE_CHUCK(chuck, var_id);
        WRITE_CHUCK(chuck, 0);

        compile(node->for_node.block, chuck);

        for (int i = 0; i < loop->continue_count; i++)
        {
            patch_jump(chuck, loop->continues[i]);
        }
        WRITE_CHUCK(chuck, OP_CONSTANT);
        WRITE_CHUCK(chuck, add_constant(chuck, JN_RETURN_INT(1)));
        WRITE_CHUCK(chuck, OP_REASSIGN);
        WRITE_CHUCK(chuck, idx_id);
        WRITE_CHUCK(chuck, TOKEN_APLUS);

        emit_loop(chuck, loop_start);
        patch_jump(chuck, exit_jmp);

        for (int i = 0; i < loop->break_count; ++i)
        {
            patch_jump(chuck, loop->breaks[i]);
        }
        loop_depth--;
        WRITE_CHUCK(chuck, OP_SCOPE_EXIT);
        break;
    // TODO
    // case AST_FOR: {
        
    //     LoopContext* loop_for = &loop_stack[loop_depth++];
    //     offset = current_offset(chuck);
    //     loop_for->loop_offset = offset;
    //     loop_for->break_count = 0;
    //     loop_for->continue_count = 0;

    //     WRITE_CHUCK(chuck, OP_SCOPE_ENTER);

    //     compile(node->for_node.iter, chuck);
    //     WRITE_CHUCK(chuck, OP_GET_ITER);
    //     // push iter object to __iter variable
    //     char tmp[200];
    //     snprintf(tmp, sizeof(tmp), "__iter_%d", iter_count++);
    //     int iter_slot = add_ident(chuck, strdup(tmp));
    //     WRITE_CHUCK(chuck, OP_SET_GLOBAL);
    //     WRITE_CHUCK(chuck, iter_slot);
    //     WRITE_CHUCK(chuck, 1);

    //     WRITE_CHUCK(chuck, OP_GET_GLOBAL);
    //     WRITE_CHUCK(chuck, iter_slot);
        
    //     WRITE_CHUCK(chuck, OP_ITER_NEXT);

    //     exit_jmp = emit_jump(chuck, OP_JUMP_IF_FALSE);
            
    //     int var_id = add_ident(chuck, node->for_node.ident);
    //     WRITE_CHUCK(chuck, OP_SET_GLOBAL);
    //     WRITE_CHUCK(chuck, var_id);
    //     WRITE_CHUCK(chuck, 0);

    //     if (node->for_node.index != NULL)
    //     {
    //         int idx = add_ident(chuck, node->for_node.index);
    //         WRITE_CHUCK(chuck, OP_SET_GLOBAL);
    //         WRITE_CHUCK(chuck, idx);
    //         WRITE_CHUCK(chuck, 0);
    //     }

    //     compile(node->for_node.block, chuck);
        
    //     emit_loop(chuck, offset);
    //     patch_jump(chuck, exit_jmp);
        
    //     for (int i = 0; i < loop_for->continue_count; i++)
    //     {
    //         patch_jump(chuck, loop_for->continues[i]);
    //     }

    //     for (int i = 0; i < loop_for->break_count; i++)
    //     {
    //         patch_jump(chuck, loop_for->breaks[i]);
    //     }
    //     WRITE_CHUCK(chuck, OP_SCOPE_EXIT);
    //     loop_depth--;
    //     break;
    // }
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
        int exit_jump = emit_jump(chuck, OP_JUMP_IF_FALSE);
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
        id = add_ident(chuck, node->reassign.ident);
        if (node->reassign.op == TOKEN_WALRUS)
        {
            WRITE_CHUCK(chuck, OP_SET_GLOBAL);
            WRITE_CHUCK(chuck, id);
            WRITE_CHUCK(chuck, 0);
            break;
        }
        WRITE_CHUCK(chuck, OP_REASSIGN);
        WRITE_CHUCK(chuck, id);
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

#undef err