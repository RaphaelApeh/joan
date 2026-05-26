#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "eval.h"

#define eval_bin(l, r, op) obj_float(tonumber((l)) op tonumber((r)))

#define eval_bin_int(l, r, op) obj_int((int)tonumber((l)) op (int)tonumber((r)))

#define eval_bin_bool(l, r, op) obj_bool(tonumber((l)) op tonumber((r)))

static bool isnumber(Object* obj)
{
    if (NULL == obj) return false;
    if (obj->kind == INT_TYPE || obj->kind == FLOAT_TYPE)
        return true;
    return false;
}

static double tonumber(Object* obj)
{
    if (obj->kind == INT_TYPE)
        return (double)obj->o_int;
    return obj->o_float;
}

Object* eval_binary(Object* lhs, Object* rhs, BinaryOp op)
{
    bool is_true = false;
    if (NULL == lhs || NULL == rhs)
        goto end;
    
    if (op == EVAL_IS)
    {
        if (!is_truthy(lhs) && rhs->kind == NONE_TYPE)
            is_true = true;
        else if (lhs == rhs)
            is_true = true;
        return obj_bool(is_true);
    }else if (op == EVAL_AND)
    {
        if (is_truthy(lhs) && is_truthy(rhs))
            is_true = true;
        return obj_bool(is_true);
    } else if (op == EVAL_OR)
    {
        if (is_truthy(lhs) || is_truthy(rhs))
            is_true = true;
        return obj_bool(is_true);   
    }
    if (isnumber(lhs) && isnumber(rhs))
    {
        ObjectType ot;
        switch (op)
        {
            case EVAL_ADD:
                ot = lhs->kind;
                if (ot == INT_TYPE)
                    return obj_int((int)(tonumber(lhs) + tonumber(rhs)));
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
    
    if (lhs->kind == STR_TYPE && rhs->kind == STR_TYPE)
    {
        char* str = NULL;
        switch (op)
        {
            case EVAL_ADD:
                str = strcat(lhs->str->str, rhs->str->str);
                return obj_string(str);
            case EVAL_IS:
            case EVAL_EQUAL:
                return obj_bool(
                    lhs->str->hash == rhs->str->hash
                );
            case EVAL_IN:
                return obj_bool(strstr(lhs->str->str, rhs->str->str) == NULL);
            case EVAL_NOTEQUAL:
                return obj_bool(
                    lhs->str->hash != rhs->str->hash
                );
            default:
                goto end;
        }
    }
    if (lhs->kind == BOOL_TYPE && rhs->kind == BOOL_TYPE)
    {
        switch (op)
        {
        case EVAL_EQUAL:
            return obj_bool(lhs->o_bool == rhs->o_bool);
        case EVAL_NOTEQUAL:
            return obj_bool(lhs->o_bool != rhs->o_bool);
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