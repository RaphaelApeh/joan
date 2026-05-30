#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include "vm.h"
#include "chuck.h"
#include "ast.h"
#include "object.h"
#include "helper.h"
#include "eval.h"


static LoopContext loop_stack[256];
static int loop_depth = 0;

#define err(vm, msg) do{ \
    printf("Error[main:%d:%d] %s\n", (vm)->p->curr.line, (vm)->p->curr.column, msg);\
    return INTERPRET_RUNTIME_ERROR; \
} while(false)


static InterpretResult die(VM* vm, const char* msg, ...)
{
    va_list arg; va_start(arg, msg);
    fprintf( // TODO: current impl does not get the exact line and column
        stderr, 
        "Error[main:%d:%d] ",
        vm->p->curr.line, vm->p->curr.column
    );
    vfprintf(stderr, msg, arg);
    fputc('\n', stderr);
    va_end(arg);
    return INTERPRET_RUNTIME_ERROR;
}

static void push(VM* vm, Object* object)
{
    if (NULL == object) return;
    *vm->sp++ = object;
}

static Object* pop(VM* vm){ return *--vm->sp; }

InterpretResult vm_run(VM* vm)
{
    #define READ_BYTE() (*vm->ip++)
    #define READ_CONST() (vm->chuck->constants[READ_BYTE()])
    #define READ_IDENT() (vm->chuck->idents[READ_BYTE()])
    int count;
    Object* o = NULL;
    IterObject* iter = NULL;
    Object *a, *b;
    Object* key, *value;
    Object *array, *pos;
    char* ident;
    uint16_t offset;
    for (;;)
    {
        uint8_t op = READ_BYTE();
        switch (op)
        {
            case OP_CONSTANT:
                o = READ_CONST();
                push(vm, internObject(o));
                break;
            case OP_ADD:
                a = pop(vm);
                b = pop(vm);
                a = internObject(eval_binary(b, a, EVAL_ADD));
                if (NULL == a)
                    err(vm, "Invalid binary");
                push(vm, a);
                break;
            case OP_SUB:
                a = pop(vm);
                b = pop(vm);
                a = eval_binary(b, a, EVAL_SUB);
                if (NULL == a)
                    err(vm, "Invalid binary");
                push(vm, a);
                break;
            case OP_MUL:
                a = pop(vm);
                b = pop(vm);
                a->o_int = a->o_int * b->o_int;
                push(vm, a);
                break;
            case OP_BITAND:
                a = pop(vm);
                b = pop(vm);
                a = eval_binary(b, a, EVAL_BAND);
                if (NULL == a)
                    err(vm, "Invalid binary");
                push(vm, a);
                break;
            case OP_BITOR:
                a = pop(vm);
                b = pop(vm);
                a = eval_binary(b, a, EVAL_BOR);
                if (NULL == a)
                    err(vm, "Invalid binary");
                push(vm, a);
                break;
            case OP_PERC:
                a = pop(vm);
                b = pop(vm);
                a = eval_binary(b, a, EVAL_PERC);
                if (NULL == a)
                    err(vm, "Invalid binary");
                push(vm, a);
                break;
            case OP_DIV:
                a = pop(vm); b = pop(vm);
                a = eval_binary(b, a, EVAL_DIV);
                push(vm, a);
                break;
            case OP_BITAC:
                a = pop(vm);
                b = pop(vm);
                a = eval_binary(b, a, EVAL_BAC);
                if (NULL == a)
                    err(vm, "Invalid binary");
                push(vm, a);
                break;
            case OP_EQUAL:
                a = pop(vm);
                b = pop(vm);
                a = eval_binary(b, a, EVAL_EQUAL);
                if (NULL == a)
                    err(vm, "Invalid binary");
                push(vm, a);
                break;
            case OP_LSHIFT:
                a = pop(vm);
                b = pop(vm);
                a = eval_binary(b, a, EVAL_LSHIFT);
                if (NULL == a)
                    err(vm, "Invalid binary");
                push(vm, a);
                break;
            case OP_RSHIFT:
                a = pop(vm);
                b = pop(vm);
                a = eval_binary(b, a, EVAL_RSHIFT);
                if (NULL == a)
                    err(vm, "Invalid binary");
                push(vm, a);
                break;
            case OP_NEQ:
                a = pop(vm);
                b = pop(vm);
                a = eval_binary(b, a, EVAL_NOTEQUAL);
                if (NULL == a)
                    err(vm, "Invalid binary");
                push(vm, a);
                break;
            case OP_GT:
                a = pop(vm);
                b = pop(vm);
                a = eval_binary(b, a, EVAL_GT);
                if (NULL == a)
                    err(vm, "Invalid binary");
                push(vm, a);
                break;
            case OP_GTE:
                a = pop(vm);
                b = pop(vm);
                a = eval_binary(b, a, EVAL_GTE);
                if (NULL == a)
                    err(vm, "Invalid binary");
                push(vm, a);
                break;
            case OP_LT:
                a = pop(vm);
                b = pop(vm);
                a = eval_binary(b, a, EVAL_LT);
                if (NULL == a)
                    err(vm, "Invalid binary");
                push(vm, a);
                break;
            case OP_LTE:
                a = pop(vm);
                b = pop(vm);
                a = eval_binary(b, a, EVAL_LTE);
                if (NULL == a)
                    err(vm, "Invalid binary");
                push(vm, a);
                break;
            case OP_ARRAY:
                count = READ_BYTE();
                array_t* arr = init_array();
                for (int i = count - 1; i >= 0; --i)
                {
                    //TODO
                    arr->items[i] = pop(vm);
                    arr->count++;
                }
                o = obj_new(ARRAY_TYPE);
                o->o_array = arr;
                push(vm, o);
                break;
            case OP_ITER:
                count = READ_BYTE();
                iter = ObjectIter(count);
                for (int i = count - 1; i >= 0; --i)
                {
                    //pushItem(iter, pop(vm));
                    iter->items[i] = pop(vm);
                    iter->count++;
                }
                o = obj_new(ITER_TYPE);
                o->iter = iter;
                push(vm, o);
                break;
            case OP_HM:
                count = READ_BYTE();
                J_DArray_Obj* jd_obj = malloc(sizeof(J_DArray_Obj));
                jd_obj->size = 0;
                jd_obj->capacity = count;
                jd_obj->items = malloc(sizeof(ObjHM *) * count);
                for (int i = count - 1; i >= 0; --i)
                {
                    value = pop(vm); key = pop(vm);
                    jd_obj->items[i] = HM_OBJ(key, value);
                    jd_obj->size++;
                }
                Object* obj = obj_new(HASHMAP_TYPE);
                obj->hashmap = jd_obj;
                push(vm, obj);
                break;
            case OP_REASSIGN:
                ident = READ_IDENT();
                int t_op = READ_BYTE();
                a = pop(vm);
                entry_t* entry = get_envEntry(vm->env, ident);
                if (NULL == a || NULL == entry)
                    return die(vm, "undefine variable.");
                if (entry->is_const)
                    return die(vm, "Cannot reassign a variable of const.");
                o = entry->value;
                o->kind = a->kind;
                switch (t_op)
                {
                    case TOKEN_APLUS:
                        b = eval_binary(o, a, EVAL_ADD);
                        if (NULL == b)
                            break;
                        *o = *b; // TODO
                        break;
                    case TOKEN_AMINUS:
                        b = eval_binary(o, a, EVAL_SUB);
                        if (NULL == b)
                            break;
                        *o = *b; // TODO
                        break;
                    case TOKEN_EQUAL:
                        internObject(o);
                        *o = *a; //TODO
                        break;
                    case TOKEN_ASTAR:
                        b = eval_binary(o, a, EVAL_MUL);
                        if (NULL == b)
                            break;
                        *o = *b; // TODO
                        break;
                    case TOKEN_ARSHIFT:
                        b = eval_binary(o, a, EVAL_RSHIFT);
                        if (NULL == b)
                            break;
                        *o = *b;
                        break;
                    case TOKEN_ALSHIFT:
                        b = eval_binary(o, a, EVAL_LSHIFT);
                        if (NULL == b)
                            break;
                        *o = *b;
                        break;
                    case TOKEN_BITAC:
                        b = eval_binary(o, a, EVAL_BAC);
                        if (NULL == b)
                            break;
                        *o = *b;
                        break;
                    case TOKEN_APERCENTAGE:
                        b = eval_binary(o, a, EVAL_PERC);
                        if (NULL == b)
                            break;
                        *o = *b;
                        break;
                    case TOKEN_ABITAC:
                        b = eval_binary(o, a, EVAL_BAC);
                        if (NULL == b)
                            break;
                        *o = *b;
                        break;
                    case TOKEN_ASLASH:
                        b = eval_binary(o, a, EVAL_DIV);
                        if (NULL == b)
                            break;
                        *o = *b;
                        break;
                    case TOKEN_ABITAND:
                        b = eval_binary(o, a, EVAL_BAND);
                        if (NULL == b)
                            break;
                        *o = *b;
                        break;
                    case TOKEN_ABITOR:
                        b = eval_binary(o, a, EVAL_BOR);
                        if (NULL == b)
                            break;
                        *o = *b;
                        break;
                    default:
                        err(vm, "invalid operator.");
                }
                break;
            case OP_IN:
                a = pop(vm);
                b = pop(vm);
                a = eval_binary(b, a, EVAL_IN);
                push(vm, a);
                break;
            case OP_IS:
                b = pop(vm); a = pop(vm);
                a = eval_binary(a, b, EVAL_IS);
                if (NULL == a)
                    return die(vm, "Invalid binary opration.");
                push(vm, a);
                break;
            case OP_AND:
                b = pop(vm); a = pop(vm);
                a = internObject(eval_binary(a, b, EVAL_AND));
                if (NULL == a)
                    return die(vm, "Invalid binary opration.");
                push(vm, a);
                break;
            case OP_OR:
                b = pop(vm); a = pop(vm);
                a = internObject(eval_binary(a, b, EVAL_OR));
                if (NULL == a)
                    return die(vm, "Invalid binary opration.");
                push(vm, a);
                break;
            case OP_NOT:
                o = pop(vm);
                push(vm, obj_bool(!is_truthy(o)));
                break;
            case OP_ASSERT:
                o = pop(vm);
                char * msg = READ_IDENT();
                if (!is_truthy(o))
                    return die(vm, msg);
                // push(vm, obj_none());
                break;
            case OP_ENUM:
                ident = READ_IDENT();
                count = READ_BYTE();
                printf("ENUM ident %s, fields %d\n", ident, count);
                // for (int i = count; i <= 0; --i)
                // {
                //     printf("Field %s\n", READ_IDENT());
                // }
                break;
            case OP_RANGE:
                Object *b = pop(vm), *a = pop(vm);
                if (a->kind != INT_TYPE || b->kind != INT_TYPE)
                    return die(vm, "Expected type int but got TODO:");
                int start = a->o_int, end = b->o_int;
                int tmp;
                if (start > end)
                {
                    tmp = end;
                    end = start;
                    start = tmp;
                }
                iter = ObjectIter(b->o_int);
                for (int i = start; i < end; ++i)
                {
                    pushItem(iter, obj_int(i));
                }
                o = obj_new(ITER_TYPE);
                o->iter = iter;
                push(vm, o);
                break;
            case OP_GET_GLOBAL:
                ident = READ_IDENT();
                o = get_env(vm->env, ident);
                if (NULL == o)
                    return die(vm, "undefine variable '%s'.", ident);
                push(vm, internObject(o));
                break;
            case OP_PRINTLN:
                Object* out = pop(vm);
                print_object(out);
                putchar('\n');
                break;
            case OP_NEGATE:
                o = pop(vm);
                if (NULL == o) break;
                switch (o->kind)
                {
                    case INT_TYPE:
                        o->o_int = -(o->o_int);
                        push(vm, o);
                        break;
                    case FLOAT_TYPE:
                        o->o_float = -o->o_float;
                        push(vm, o);
                        break;
                    default:
                        err(vm, "Invalid type.");
                }
                break;
            case OP_POP:
                pop(vm); break;
            case OP_DUP:
                Object* top = *(vm->sp - 1);
                push(vm, top); break;
            case OP_SET_GLOBAL:
                o = pop(vm);
                ident = READ_IDENT();
                bool is_const = (bool)READ_BYTE();
                if (o == NULL || ident == NULL)
                    err(vm, "Object not set.");
                set_env(vm->env, ident, o, is_const, false);
                break;
            case OP_INDEX:
                array = pop(vm);
                pos = pop(vm);
                if (!array || !pos)
                    err(vm, "None value array or pos.");
                if (array->kind != ARRAY_TYPE && array->kind != STR_TYPE && array->kind != ITER_TYPE && array->kind != HASHMAP_TYPE)
                    err(vm, "Invalid kind for array or pos.");
                if (array->kind == ARRAY_TYPE && pos->kind != INT_TYPE)
                    err(vm, "pos is not an int");
                int index = pos->o_int;
                switch (array->kind)
                {
                    case ARRAY_TYPE:
                        if (index < 0 || pos->o_int >= array->o_array->count)
                            err(vm, "pos is > or < array length");
                        o = array->o_array->items[pos->o_int];
                        push(vm, o);
                        break;
                    case STR_TYPE:
                        if (index < 0 || index >= array->str->len)
                            return INTERPRET_RUNTIME_ERROR;
                        char* str = malloc(2);
                        str[0] = array->str->chars[index];
                        str[1] = '\0';
                        o = obj_string(str);
                        push(vm, o);
                        break;
                    case ITER_TYPE:
                        if (index < 0 || index >= array->iter->count)
                            return die(vm, "pos is > or < array length");
                        o = array->iter->items[index];
                        push(vm, o);
                        break;
                    case HASHMAP_TYPE:
                    ObjHM* hm = GetObject(array, pos);
                    if (NULL == hm)
                        return die(vm, "index error");
                    push(vm, hm->value); break;
                    default:
                        err(vm, "Got an invaild array type");
                }
                break;
            case OP_SET_INDEX:
                value = pop(vm); array = pop(vm); pos = pop(vm);
                index = pos->o_int;
                
                if (array->kind != HASHMAP_TYPE && pos->kind != INT_TYPE)
                    return die(vm, "Expected an 'int' type");
                if (array->kind != HASHMAP_TYPE && index < 0)
                    return die(vm, "Got an negative index value.");
                
                switch (array->kind)
                {
                    case ARRAY_TYPE:
                        if (index >= array->o_array->count)
                            return die(vm, "Got an invalid index; expected max '%d' but got '%d'.", array->o_array->count, index);
                        array->o_array->items[index]->kind = value->kind;
                        array->o_array->items[index] = value;
                        break;
                    case STR_TYPE:
                        if (index >= array->str->len)
                                return die(vm, "Got an invalid index; expected max '%d' but got '%d'.", array->str->len, index);
                        if (value->kind != STR_TYPE)
                            return die(vm, "string index expect a string value.");
                        if (value->str->len > 0)
                            return die(vm, "Can only set a char to a string.");
                        array->str->chars[index] = value->str->chars[0];
                        break;
                    case HASHMAP_TYPE:
                            ObjHM* hm = GetObject(array, pos);
                        if (NULL == hm)
                            return die(vm, "index error");
                        hm->value->kind = pos->kind;
                        hm->value = value;
                        break;
                    case ITER_TYPE:
                        return die(vm, "Iter object does not support index setting.");
                    default:
                        return die(vm, "type does not support index setting.");
                 }
                break;
            case OP_SCOPE_ENTER:
                env_t* local = init_env(vm->env);
                vm->env = local;
                break;
            case OP_SCOPE_EXIT:
                if (vm->env->parent)
                {
                    env_t* old = vm->env;
                    vm->env = vm->env->parent;
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
                o = pop(vm);
                if (!is_truthy(o))
                    vm->ip += offset;
                break;
            case OP_LOOP:
                offset = (READ_BYTE() << 8);
                offset |= READ_BYTE();
                vm->ip -= offset;
                break;
            case OP_CALL:{
                count = READ_BYTE();  ident = READ_IDENT();
                o = get_env(vm->env, ident);
                if (NULL == o)
                    return die(vm, "undefine function '%s'.", ident);
                if (o->kind != FUNCTION_TYPE && o->kind != NATIVE_TYPE)
                    return die(vm, "%s is not a callable.", ident);
                Object* args[20];
                size_t len = 0;
                for (int i = count - 1; i >= 0; --i)
                {
                    args[i] = pop(vm);
                }
                switch (o->kind)
                {
                    case NATIVE_TYPE: {
                        a = o->o_nativefn->fn(args, (size_t)count);
                        if (a == NULL)
                            return die(vm, "SystemError: got NULL");
                        push(vm, a);
                        break;
                    }
                    case FUNCTION_TYPE: {
                        //  TODO
                        ObjFunction* fn = o->fn;
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
                        env_t* local = init_env(vm->env);
                        for (int i = 0; i < fn->arity; i++)
                        {
                            set_env(local, fn->params[i], args[i], false, false);
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
                    push(vm, o);
                    return INTERPRET_OK;
                }
                env_t* old = vm->env;
                CallFrame* frame = &vm->frames[--vm->frame_count];
                vm->ip = frame->ip;
                vm->env = frame->env;
                free(old);
                push(vm, o);
                break;
            case OP_ERROR:
                return INTERPRET_RUNTIME_ERROR;
            default:
                err(vm, "System error.");
        }
    }
    #undef READ_BYTE
    #undef READ_CONST
}

void compile(AST* node, Chuck* chuck)
{
    int id, idx, jump, offset;
    LoopContext* loop;
    switch (node->type)
    {
    case AST_LITERAL:
        idx = add_constant(chuck, node->literal);
        write_chuck(chuck, OP_CONSTANT);
        write_chuck(chuck, idx);
        break;
    case AST_ERROR:
        id = add_ident(chuck, (char *)node->error_msg);
        write_chuck(chuck, OP_ERROR_MSG);
        write_chuck(chuck, id);
        break;
    case AST_IDENTIFIER:
        char* ident = (char *)node->identifier;
        id = add_ident(chuck, ident);
        write_chuck(chuck, OP_GET_GLOBAL);
        write_chuck(chuck, id);
        break;
    case AST_ARRAY:
        for (size_t i = 0; i < node->array.count; i++)
            compile(node->array.elements[i], chuck);
        write_chuck(chuck, OP_ARRAY);
        write_chuck(chuck, node->array.count);
        break;
    case AST_TUPLE:
        for (size_t i = 0; i < node->tuple.count; ++i)
            compile(node->tuple.elements[i], chuck);
        write_chuck(chuck, OP_ITER);
        write_chuck(chuck, node->tuple.count);
        break;
    case AST_CALL:
        for (int i = 0; i < node->call.pos_count; i++)
        {
            compile(node->call.pos_args[i], chuck);
        }
        id = add_ident(chuck, (char *)node->call.callee);
        write_chuck(chuck, OP_CALL);
        write_chuck(chuck, node->call.pos_count);
        write_chuck(chuck, id);
        break;
    case AST_BLOCK:
        write_chuck(chuck, OP_SCOPE_ENTER);
        for (size_t i = 0; i < node->block.count; i++)
        {
            compile(node->block.statements[i], chuck);
            // if (i != node->block.count - 1)
            //     write_chuck(chuck, OP_POP);
        }
        write_chuck(chuck, OP_SCOPE_EXIT);
        break;
    case AST_PRINTLN:
        compile(node->println.out, chuck);
        write_chuck(chuck, OP_PRINTLN);
        break;
    case AST_ASSERT:
        compile(node->assert_stmt.cond, chuck);
        if (node->assert_stmt.msg != NULL)
            id = add_ident(chuck, node->assert_stmt.msg);
        else
            id  = add_ident(chuck, "Assertion failed.");

        write_chuck(chuck, OP_ASSERT);
        write_chuck(chuck, id);
        break;
    case AST_UNARY:
        compile(node->unary.right, chuck);
        switch (node->unary.op)
        {
            case TOKEN_MINUS:
                write_chuck(chuck, OP_NEGATE);
                break;
            // case TOKEN_STAR:
            //     write_chuck(chuck, OP_MUL);
            //     break;
            case TOKEN_NOT:
                write_chuck(chuck, OP_NOT);
                break;
            default:
                write_chuck(chuck, OP_ERROR);
                break;
        }
        break;
    case AST_BINARY:
        compile(node->binary.left, chuck);
        compile(node->binary.right, chuck);
        switch (node->binary.op)
        {
            case TOKEN_PLUS:
                write_chuck(chuck, OP_ADD);
                break;
            case TOKEN_STAR:
                write_chuck(chuck, OP_MUL);
                break;
            case TOKEN_MINUS:
                write_chuck(chuck, OP_SUB);
                break;
            case TOKEN_RSHIFT:
                write_chuck(chuck, OP_RSHIFT);
                break;
            case TOKEN_LSHIFT:
                write_chuck(chuck, OP_LSHIFT);
                break;
            case TOKEN_EQEQ:
                write_chuck(chuck, OP_EQUAL);
                break;
            case TOKEN_NEQ:
                write_chuck(chuck, OP_NEQ);
                break;
            case TOKEN_GT:
                write_chuck(chuck, OP_GT);
                break;
            case TOKEN_GTE:
                write_chuck(chuck, OP_GTE);
                break;
            case TOKEN_LT:
                write_chuck(chuck, OP_LT);
                break;
            case TOKEN_LTE:
                write_chuck(chuck, OP_LTE);
                break;
            case TOKEN_BITAND:
                write_chuck(chuck, OP_BITAND);
                break;
            case TOKEN_BITOR:
                write_chuck(chuck, OP_BITOR);
                break;
            case TOKEN_BITAC:
                write_chuck(chuck, OP_BITAC);
                break;
            case TOKEN_PERCENTAGE:
                write_chuck(chuck, OP_PERC);
                break;
            case TOKEN_IN:
                write_chuck(chuck, OP_IN);
                break;
            case TOKEN_SLASH:
                write_chuck(chuck, OP_DIV);
                break;
            case TOKEN_RANGE:
                write_chuck(chuck, OP_RANGE); break;
            case TOKEN_IS:
                write_chuck(chuck, OP_IS); break;
            case TOKEN_AND:
                write_chuck(chuck, OP_AND); break;
            case TOKEN_OR:
                write_chuck(chuck, OP_OR); break;
            default:
                break;
        }
        break;
    case AST_ENUM: 
        ident = node->enum_stmt.ident;
        Object* enumObj = obj_enum(ident, node->enum_stmt.fields, node->enum_stmt.count);
        idx = add_constant(chuck, enumObj);
        write_chuck(chuck, OP_CONSTANT);
        write_chuck(chuck, idx);

        id = add_ident(chuck, ident);
        write_chuck(chuck, OP_SET_GLOBAL);
        write_chuck(chuck, id);
        write_chuck(chuck, 1);
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
            write_chuck(chuck, OP_DUP);
            compile(caseObj.pattern, chuck);
            write_chuck(chuck, OP_EQUAL);
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
    case AST_FUNCTION: 
        Chuck fn_chuck;
        fn_chuck.env = chuck->env;
        chuck_init(&fn_chuck);
        compile(node->fn_node.block, &fn_chuck);
        // idx = add_constant(&fn_chuck, obj_none());
        // write_chuck(&fn_chuck, OP_CONSTANT);
        // write_chuck(&fn_chuck, idx);
        write_chuck(&fn_chuck, OP_END);
        Object* objFn = obj_function(
            &fn_chuck,
            node->fn_node.params,
            node->fn_node.count,
            node->fn_node.name
        );
        idx = add_constant(chuck, objFn);
        write_chuck(chuck, OP_CONSTANT);
        write_chuck(chuck, idx);

        id = add_ident(chuck, node->fn_node.name);
        write_chuck(chuck, OP_SET_GLOBAL);
        write_chuck(chuck, id);
        write_chuck(chuck, 1);
        break;
    case AST_IF:
        compile(node->if_node.condition, chuck);
        int false_jump = emit_jump(chuck, OP_JUMP_IF_FALSE);
        // write_chuck(chuck, OP_POP);
        compile(node->if_node.then, chuck);
        int end_jump = emit_jump(chuck, OP_JUMP);
        patch_jump(chuck, false_jump);
        // write_chuck(chuck, OP_POP);
        for (size_t i = 0; i < node->if_node.elseif->count; i++)
        {
            elif_node elif = node->if_node.elseif->children[i];
            compile(elif.cond, chuck);
            int elif_false = emit_jump(chuck, OP_JUMP_IF_FALSE);
            // write_chuck(chuck, OP_POP);
            compile(elif.stmt, chuck);
            int elif_end = emit_jump(chuck, OP_JUMP);
            patch_jump(chuck, elif_false);
            // write_chuck(chuck, OP_POP);
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
        write_chuck(chuck, OP_HM);
        write_chuck(chuck, node->hmp_node.count);
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
        write_chuck(chuck, OP_RETURN);
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

        emit_loop(chuck, loop->loop_offset);
        
        for (int i = 0; i < loop->break_count; i++)
        {
            patch_jump(chuck, loop->breaks[i]);
        }
        loop_depth--;
        break;
    case AST_REASSIGN:
        compile(node->reassign.value, chuck);
        id = add_ident(chuck, node->reassign.ident);
        // if (!id) break;
        write_chuck(chuck, OP_REASSIGN);
        write_chuck(chuck, id);
        write_chuck(chuck, node->reassign.op);
        break;
    case AST_ASSIGN:
        compile(node->assign.value, chuck);
        id = add_ident(chuck, node->assign.name);
        write_chuck(chuck, OP_SET_GLOBAL);
        write_chuck(chuck, id);
        write_chuck(chuck, (uint8_t)node->assign.is_const);
        break;
    case AST_ARRAY_INDEX:
        compile(node->index.pos, chuck);
        compile(node->index.array, chuck);
        if (node->index.is_set)
        {
            compile(node->index.value, chuck);
            write_chuck(chuck, OP_SET_INDEX);
        } else write_chuck(chuck, OP_INDEX);
        break;
    default:
        break;
    }
}

#undef err