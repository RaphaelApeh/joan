#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>

#include "eval.h"
#include "helper.h"

#define eval_bin(l, r, op) jn_obj_float(state, tonumber((l)) op tonumber((r)))

#define eval_bin_int(l, r, op) jn_obj_int(state, (int)tonumber((l)) op (int)tonumber((r)))

#define eval_bin_bool(l, r, op) jn_obj_bool(state, tonumber((l)) op tonumber((r)))

// TODO

static uint64_t hash_object(JnObject* obj)
{
    switch (obj->type)
    {
        case JN_INT_TYPE:
            return (uint64_t)obj->int_val;
        case JN_BOOL_TYPE:
            return obj->bool_val;
        case JN_FLOAT_TYPE:
            return obj->float_val;
        case JN_STRING_TYPE:
            return obj->str->hash;
        case JN_NONE_TYPE:
            return 0;
        default:
            return 0;
    }
}

JnObject* eval_binary(J_State* state, JnObject* lhs, JnObject* rhs, BinaryOp op)
{
    bool is_true = false;
    char* str = NULL;
    if (NULL == lhs || NULL == rhs)
        goto end;
    
    if (op == EVAL_IS)
    {
        if (!is_truthy(lhs) && rhs->type == JN_NONE_TYPE)
            is_true = true;
        else if (lhs == rhs)
            is_true = true;
        return JN_RETURN_BOOL(state, is_true);
    }else if (op == EVAL_AND)
    {
        if (is_truthy(lhs) && is_truthy(rhs))
            is_true = true;
        return JN_RETURN_BOOL(state, is_true);
    } else if (op == EVAL_OR)
    {
        if (is_truthy(lhs) || is_truthy(rhs))
            is_true = true;
        return JN_RETURN_BOOL(state, is_true);   
    }
    if (JN_IS_ITERABLE(rhs) && (op == EVAL_IN || op == EVAL_NOT_IN))
    {
        switch (rhs->type)
        {
            case JN_ARRAY_TYPE: {
                // Not the best way to do it.
                is_true = false;
                for (int i = 0; i < rhs->arr->size; ++i)
                {
                    if (
                        rhs->arr->items[i]->type == lhs->type && 
                        hash_object(rhs->arr->items[i]) == hash_object(lhs)
                    )
                    {
                        is_true = true;
                        break;
                    }
                }
                return JN_RETURN_BOOL(state, op == EVAL_IN ? is_true : !is_true);
            }
            case JN_ITER_TYPE: break; // TODO
            case JN_HASHMAP_TYPE: {
                bool tmp;
                if (op == EVAL_IN)
                    tmp = JN_HASHMAP_GET(rhs->hashmap, lhs) != NULL;
                else
                    tmp = JN_HASHMAP_GET(rhs->hashmap, lhs) == NULL;
                return JN_RETURN_BOOL(state, tmp);
            }
        }
    }
    if (isnumber(lhs) && isnumber(rhs))
    {
        JnTypeObject ot;
        switch (op)
        {
            case EVAL_ADD:
                ot = lhs->type;
                if (ot == JN_INT_TYPE)
                    return jn_obj_int(state, (int)(tonumber(lhs) + tonumber(rhs)));
                return eval_bin(lhs, rhs, +);
            case EVAL_MUL:
                return eval_bin(lhs, rhs, *);
            case EVAL_POW:
                return JN_RETURN_INT(state, pow(tonumber(lhs), tonumber(rhs)));
            case EVAL_EQUAL:
                return eval_bin_bool(lhs, rhs, ==);
            case EVAL_SUB:
                return eval_bin(lhs, rhs, -);
            case EVAL_DIV:
                return eval_bin(lhs, rhs, /);
            case EVAL_LT:
                return eval_bin_bool(lhs, rhs, <);
            case EVAL_LTE:
                return eval_bin_bool(lhs, rhs, <=);
            case EVAL_NOTEQUAL:
                return eval_bin_bool(lhs, rhs, !=);
            case EVAL_GT:
                return eval_bin_bool(lhs, rhs, >);
            case EVAL_GTE:
                return eval_bin_bool(lhs, rhs, >=);
            case EVAL_LSHIFT:
                return eval_bin_int(lhs, rhs, <<);
            case EVAL_RSHIFT:
                return eval_bin_int(lhs, rhs, >>);
            case EVAL_PERC:
                return eval_bin_int(lhs, rhs, %);
            case EVAL_BAND:
                return eval_bin_int(lhs, rhs, &);
            case EVAL_BOR:
                return eval_bin_int(lhs, rhs, |);
            case EVAL_BAC:
                return eval_bin_int(lhs, rhs, ^);
            default:
                goto end;
        }
    }
    if (JN_IS_CHAR(lhs) && JN_IS_STRING(rhs))
    {
        switch (op)
        {
        case EVAL_ADD:
            return JN_RAISE_EXCPETION(state, TYPE_ERROR, "'+' is not supported for a char and string type.");
        case EVAL_IN:
            str = memchr(JN_AS_STRING(rhs)->chars, JN_AS_CHAR(lhs), JN_AS_STRING(rhs)->len);
            is_true = str != NULL;
            return JN_RETURN_BOOL(state, is_true);
        case EVAL_NOT_IN:
            str = memchr(JN_AS_STRING(rhs)->chars, JN_AS_CHAR(lhs), JN_AS_STRING(rhs)->len);
            is_true = str == NULL;
            return JN_RETURN_BOOL(state, is_true);        
        default:
            return JN_RAISE_EXCPETION(state, TYPE_ERROR, "char does not support this operator for string.");
        }
    }
    if (lhs->type == JN_STRING_TYPE && rhs->type == JN_STRING_TYPE)
    {
        switch (op)
        {
            case EVAL_ADD:
                str = strcat(lhs->str->chars, rhs->str->chars);
                return JN_RETURN_STRING(state, str);
            case EVAL_IS:
            case EVAL_EQUAL:
                return JN_RETURN_BOOL(
                    state,
                    lhs->str->hash == rhs->str->hash
                );
            case EVAL_IN:
                return JN_RETURN_BOOL(state, strstr(lhs->str->chars, rhs->str->chars) == NULL);
            case EVAL_NOTEQUAL:
                return JN_RETURN_BOOL(
                    state,
                    lhs->str->hash != rhs->str->hash
                );
            default:
                goto end;
        }
    }
    
    if (JN_IS_ITERABLE(rhs))
    {
        /// 
    }
    if (lhs->type == JN_BOOL_TYPE && rhs->type == JN_BOOL_TYPE)
    {
        switch (op)
        {
        case EVAL_EQUAL:
            return JN_RETURN_BOOL(state, lhs->bool_val  == rhs->bool_val );
        case EVAL_NOTEQUAL:
            return JN_RETURN_BOOL(state, lhs->bool_val  != rhs->bool_val );
        default:
            goto end;
        }
    }
    if (JN_IS_CHAR(lhs) && JN_IS_BOOL(rhs))
    {
        return lhs;
    }
    printf("Left: %d; Right: %d\n", lhs->type, rhs->type);
    return JN_RAISE_EXCPETION(state, TYPE_ERROR, "Type '%s' does not support operation with '%s'.", JN_OBJ_TO_STRING(lhs), JN_OBJ_TO_STRING(rhs));
    end:
        return NULL;
}

#undef eval_bin
#undef eval_bin_int
#undef eval_bin_bool