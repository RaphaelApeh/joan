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
    Object *a, *b;
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
                push(vm, o);
                break;
            case OP_ADD:
                a = pop(vm);
                b = pop(vm);
                a = eval_binary(b, a, EVAL_ADD);
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
            case OP_REASSIGN:
                ident = READ_IDENT();
                int t_op = READ_BYTE();
                a = pop(vm);
                o = get_env(vm->env, ident);
                if (NULL == a)
                    err(vm, "undefine variable.");
                if (o->kind != a->kind)
                    return die(vm, "Expected a type '%s' but got '%s'."); // TODO: get type str repr
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
            case OP_NOT:
                o = pop(vm);
                push(vm, obj_bool(!is_truthy(o)));
                break;
            case OP_GET_GLOBAL:
                ident = READ_IDENT();
                o = get_env(vm->env, ident);
                if (NULL == o)
                    return die(vm, "undefine variable '%s'.", ident);
                push(vm, o);
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
                pop(vm);
                break;
            case OP_SET_GLOBAL:
                o = pop(vm);
                ident = READ_IDENT();
                bool is_const = (bool)READ_BYTE();
                if (o == NULL || ident == NULL)
                    err(vm, "Object not set.");
                set_env(vm->env, ident, o, is_const, false);
                // push(vm, o);
                break;
            case OP_INDEX:
                array = pop(vm);
                pos = pop(vm);
                if (!array || !pos)
                    err(vm, "None value array or pos.");
                if (array->kind != ARRAY_TYPE && array->kind != STR_TYPE)
                    err(vm, "Invalid kind for array or pos.");
                if (pos->kind != INT_TYPE)
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
                        if (index < 0 || index >= strlen(array->o_string))
                            return INTERPRET_RUNTIME_ERROR;
                        char* str = malloc(2);
                        str[0] = array->o_string[index];
                        str[1] = '\0';
                        o = obj_string(str);
                        push(vm, o);
                        break;
                    default:
                        err(vm, "Got an invaild array type");
                }
                break;
            case OP_SET_INDEX:
                Object* value = pop(vm); array = pop(vm); pos = pop(vm);
                if (array->kind != ARRAY_TYPE && array->kind != STR_TYPE)
                    return die(vm, "Expected a type 'array' or 'string'.");
                index = pos->o_int;
                if (index < 0)
                    return die(vm, "Got an negative index value.");
                switch (array->kind)
                {
                    case ARRAY_TYPE:
                        if (index >= array->o_array->count)
                            return die(vm, "Got an invalid index; expected max '%d' but got '%d'.", array->o_array->count, index);
                        array->o_array->items[index] = value;
                        break;
                    case STR_TYPE:
                    if (index >= strlen(array->o_string))
                            return die(vm, "Got an invalid index; expected max '%d' but got '%d'.", strlen(array->o_string), index);
                    if (value->kind != STR_TYPE)
                        return die(vm, "string index expect a string value.");
                    if (strlen(value->o_string) > 0)
                        return die(vm, "Can only set a char to a string.");
                    array->o_string[index] = value->o_string[0];
                    break;
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
            case OP_CALL:
                count = READ_BYTE();  ident = READ_IDENT();
                o = get_env(vm->env, ident);
                if (NULL == o)
                    return die(vm, "undefine function '%s'.", ident);
                if (o->kind != FUNCTION_TYPE && o->kind != NATIVE_TYPE)
                    return die(vm, "%s is not a callable.", ident);
                Object* args[20];
                size_t len = 0;
                for (int i = 0; i < count; ++i)
                {
                    args[len++] = pop(vm);
                }
                a = o->o_nativefn->fn(args, (size_t)count);
                if (a == NULL)
                    return die(vm, "SystemError: got NULL");
                push(vm, a);
                break;
                switch (o->kind)
                {
                    case NATIVE_TYPE: {
                        a = o->o_nativefn->fn(args, (size_t)count);
                        if (a == NULL)
                            return die(vm, "SystemError: got NULL");
                        push(vm, a);
                        break;
                    }
                    default: 
                        return die(vm, "Invalid function call.");
                }
                break;
            case OP_ERROR_MSG:
                ident = READ_IDENT();
                printf("%s\n", ident);
                return INTERPRET_RUNTIME_ERROR;
            case OP_RETURN:
                return INTERPRET_OK;
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
    int id, jump, offset;
    LoopContext* loop;
    switch (node->type)
    {
    case AST_LITERAL:
        int idx = add_constant(chuck, node->literal);
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
            default:
                break;
        }
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
        if (loop_depth <= 0)
        {
            fprintf(stderr, "SystemError: add 'break' outside a loop.\n");
            exit(72);
        }
        // TODO
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