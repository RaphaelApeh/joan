#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "eval.h"
#include "helper.h"

#define eval_bin(l, r, op) jn_obj_float(tonumber((l)) op tonumber((r)))

#define eval_bin_int(l, r, op) jn_obj_int((int)tonumber((l)) op (int)tonumber((r)))

#define eval_bin_bool(l, r, op) jn_obj_bool(tonumber((l)) op tonumber((r)))


JnObject* eval_binary(JnObject* lhs, JnObject* rhs, BinaryOp op)
{
    bool is_true = false;
    if (NULL == lhs || NULL == rhs)
        goto end;
    
    if (op == EVAL_IS)
    {
        if (!is_truthy(lhs) && rhs->type == NONE_TYPE)
            is_true = true;
        else if (lhs == rhs)
            is_true = true;
        return jn_obj_bool(is_true);
    }else if (op == EVAL_AND)
    {
        if (is_truthy(lhs) && is_truthy(rhs))
            is_true = true;
        return jn_obj_bool(is_true);
    } else if (op == EVAL_OR)
    {
        if (is_truthy(lhs) || is_truthy(rhs))
            is_true = true;
        return jn_obj_bool(is_true);   
    }
    if (JN_IS_ITERABLE(rhs) && (op == EVAL_IN || op == EVAL_NOT_IN))
    {
        switch (rhs->type)
        {
            case ARRAY_TYPE: break; // TODO
            case ITER_TYPE: break; // TODO
            case HASHMAP_TYPE: {
                bool tmp;
                if (op == EVAL_IN)
                    tmp = JN_HASHMAP_GET(rhs->hashmap, lhs) != NULL;
                else
                    tmp = JN_HASHMAP_GET(rhs->hashmap, lhs) == NULL;
                return JN_RETURN_BOOL(tmp);
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
                if (ot == INT_TYPE)
                    return jn_obj_int((int)(tonumber(lhs) + tonumber(rhs)));
                return eval_bin(lhs, rhs, +);
            case EVAL_MUL:
                return eval_bin(lhs, rhs, *);
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
    
    if (lhs->type == STR_TYPE && rhs->type == STR_TYPE)
    {
        char* str = NULL;
        switch (op)
        {
            case EVAL_ADD:
                str = strcat(lhs->str->chars, rhs->str->chars);
                return jn_obj_string(str);
            case EVAL_IS:
            case EVAL_EQUAL:
                return jn_obj_bool(
                    lhs->str->hash == rhs->str->hash
                );
            case EVAL_IN:
                return jn_obj_bool(strstr(lhs->str->chars, rhs->str->chars) == NULL);
            case EVAL_NOTEQUAL:
                return jn_obj_bool(
                    lhs->str->hash != rhs->str->hash
                );
            default:
                goto end;
        }
    }
    if (lhs->type == BOOL_TYPE && rhs->type == BOOL_TYPE)
    {
        switch (op)
        {
        case EVAL_EQUAL:
            return jn_obj_bool(lhs->bool8 == rhs->bool8);
        case EVAL_NOTEQUAL:
            return jn_obj_bool(lhs->bool8 != rhs->bool8);
        default:
            goto end;
        }
    }
    
    return NULL;
    end:
        return NULL;
}

#undef eval_bin
#undef eval_bin_int
#undef eval_bin_bool