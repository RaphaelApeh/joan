#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include "eval.h"
#include "helper.h"

#define eval_bin(l, r, op) jn_obj_float(tonumber((l)) op tonumber((r)))

#define eval_bin_int(l, r, op) jn_obj_int((int)tonumber((l)) op (int)tonumber((r)))

#define eval_bin_bool(l, r, op) jn_obj_bool(tonumber((l)) op tonumber((r)))

// TODO

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
        case NONE_TYPE:
            return 0;
        default:
            return 0;
    }
}

static char* format_string(char* fmt, JnObject* obj)
{
    int len = 0;
    int capacity = 256;
    char* str = malloc(sizeof(capacity));
    while(*fmt)
    {
        char tmp = *(fmt + 1);
        if (*fmt == '{' && tmp == '}')
        {
            fmt += 2;
            char* s = JN_OBJECT_CSTRING(obj);
            assert(s != NULL);
            int n = strlen(s);
            while (len + n + 1 >= capacity)
            {
                capacity *= 2;
                str = realloc(str, n);
            }
            memcpy(str + len, s, n);
            len += n;
            free(s);
        }
        else str[len++] = *fmt++;
    }
    str[len] = '\0';
    return str;
}

JnObject* eval_binary(JnObject* lhs, JnObject* rhs, BinaryOp op)
{
    bool is_true = false;
    char* str = NULL;
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
            case ARRAY_TYPE: {
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
                return JN_RETURN_BOOL(op == EVAL_IN ? is_true : !is_true);
            }
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
            case EVAL_POW:
                return JN_RETURN_INT(pow(tonumber(lhs), tonumber(rhs)));
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
    if (JN_IS_STRING(lhs) && op == EVAL_LSHIFT)
    {
        // TODO: add support for multiple objects.
        char* s = format_string(lhs->str->chars, rhs);
        return JN_RETURN_STRING(s);
    }
    if (JN_IS_CHAR(lhs) && JN_IS_STRING(rhs))
    {
        switch (op)
        {
        case EVAL_ADD:
            return JN_RAISE_EXCPETION(TYPE_ERROR, "'+' is not supported for a char and string type.");
        case EVAL_IN:
            str = memchr(JN_AS_STRING(rhs)->chars, JN_AS_CHAR(lhs), JN_AS_STRING(rhs)->len);
            is_true = str != NULL;
            return JN_RETURN_BOOL(is_true);
        case EVAL_NOT_IN:
            str = memchr(JN_AS_STRING(rhs)->chars, JN_AS_CHAR(lhs), JN_AS_STRING(rhs)->len);
            is_true = str == NULL;
            return JN_RETURN_BOOL(!is_true);        
        default:
            return JN_RAISE_EXCPETION(TYPE_ERROR, "char does not support this operator for string.");
        }
    }
    if (lhs->type == STR_TYPE && rhs->type == STR_TYPE)
    {
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
    if (JN_IS_CHAR(lhs) && JN_IS_BOOL(rhs))
    {
        return lhs;
    }
    return JN_RAISE_EXCPETION(TYPE_ERROR, "Invalid type %d %d.", lhs->type, rhs->type);
    end:
        return NULL;
}

#undef eval_bin
#undef eval_bin_int
#undef eval_bin_bool