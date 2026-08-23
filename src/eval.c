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

#define eval_bin_char(l, r, op) (JN_IS_CHAR(l)) ? jn_obj_char(state, (char)tonumber_c((l)) op (char)tonumber_c((r))) : jn_obj_int(state, tonumber_c((l)) op (int)tonumber_c((r)))

#define eval_bin_bool(l, r, op) jn_obj_bool(state, tonumber((l)) op tonumber((r)))

#define eval_bin_bool_c(l, r, op) jn_obj_bool(state, tonumber_c((l)) op tonumber_c((r)))


JN_INLINE bool isnumber(JnObject* obj)
{
    if (NULL == obj) return false;
    if (obj->type == JN_INT_TYPE || obj->type == JN_FLOAT_TYPE)
        return true;
    return false;
}

JN_INLINE double tonumber(JnObject* obj)
{
    if (obj->type == JN_INT_TYPE)
        return (double)obj->int_val;
    return obj->float_val;
}

JN_INLINE bool isnumber_c(JnObject* obj)
{
    if (NULL == obj) return false;
    
    if (obj->type == JN_INT_TYPE || obj->type == JN_CHAR_TYPE)
        return true;
    
    return false;
}

JN_INLINE int tonumber_c(JnObject* obj)
{
    assert(obj);
    if (obj->type == JN_CHAR_TYPE)
        return (int) JN_AS_CHAR(obj);
    return (int)obj->int_val;
}


static bool hashmap_contains(JnObject* key, Jn_Hashmap* map)
{
    return Jn_hashmap_get(map, key) != NULL;
}

static bool array_contains(JnObject* key, Jn_Array* arr)
{
    for (size_t i = 0; i < arr->size; ++i)
    {
        if (jn_obj_equals(key, arr->items[i]))
            return true;
    }
    return false;
}

static JnObject* eval_int(Jn_State* state, JnObject* lhs, JnObject* rhs, int op);
static JnObject* eval_char(Jn_State* state, JnObject* lhs, JnObject* rhs, int op);
static JnObject* eval_bool(Jn_State* state, JnObject* lhs, JnObject* rhs, int op);
static JnObject* eval_string(Jn_State* state, JnObject* lhs, JnObject* rhs, int op);
static JnObject* eval_float(Jn_State* state, JnObject* lhs, JnObject* rhs, int op);
static JnObject* eval_array(Jn_State* state, JnObject* lhs, JnObject* rhs, int op);
static JnObject* eval_hashmap(Jn_State* state, JnObject* lhs, JnObject* rhs, int op);


bool jn_obj_match(JnObject* obj, JnObject* other)
{
    if (jn_obj_equals(obj, other))
        return true;
    
    switch (JN_OBJ_TYPE(other))
    {
        case JN_ARRAY_TYPE:
        case JN_TUPLE_TYPE:
            return array_contains(obj, JN_IS_ARRAY(other) ? JN_AS_ARRAY(other) : JN_AS_TUPLE(other));
        case JN_HASHMAP_TYPE:
            return hashmap_contains(obj, JN_AS_HASHMAP(other));
        case JN_STRING_TYPE:
        {
            if (!JN_IS_CHAR(obj)) return false;
            char* ret_str = memchr(JN_AS_CSTRING(other), JN_AS_CHAR(obj), JN_STRING_LEN(other));
            return ret_str != NULL;
        }
        default:
            break;
    }
    return false;
}

JnObject* eval_binary(Jn_State* state, JnObject* lhs, JnObject* rhs, BinaryOp op)
{
    bool is_true = false;
    switch (op)
    {
    case EVAL_IS:
        if (!jn_obj_truthy(lhs) && rhs->type == JN_NONE_TYPE)
            is_true = true;
        else if (lhs == rhs)
            is_true = true;
        return JN_RETURN_BOOL(state, is_true);
    case EVAL_AND:
        if (jn_obj_truthy(lhs) && jn_obj_truthy(rhs))
            is_true = true;
        return JN_RETURN_BOOL(state, is_true);
    case EVAL_OR:
        if (jn_obj_truthy(lhs) || jn_obj_truthy(rhs))
            is_true = true;
        return JN_RETURN_BOOL(state, is_true);
    case EVAL_EQUAL:{
        return JN_RETURN_BOOL(state, jn_obj_equals(lhs, rhs));
    } break;
    default:
        break;
    }
    
    if (JN_IS_ARRAY(rhs))
        return eval_array(state, lhs, rhs, op);
    
    if (JN_IS_HASHMAP(rhs))
        return eval_hashmap(state, lhs, rhs, op);
    
    if (JN_IS_RANGE(rhs))
    {
        // TODO
    }
    if (isnumber(lhs) && isnumber(rhs))
    {
        if (JN_IS_INT(lhs))
            return eval_int(state, lhs, rhs, op);
        return eval_float(state, lhs, rhs, op);
    }
    if (isnumber_c(lhs) && isnumber_c(rhs))
        return eval_char(state, lhs, rhs, op);

    if (JN_IS_CHAR(lhs) && JN_IS_STRING(rhs))
        return eval_char(state, lhs, rhs, op);

    if (JN_IS_STRING(lhs) && JN_IS_STRING(rhs))
        return eval_string(state, lhs, rhs, op);

    if (JN_IS_BOOL(lhs) && JN_IS_BOOL(rhs))
        return eval_bool(state, lhs, rhs, op);
    
    return JN_RAISE_EXCPETION(
        state, TYPE_ERROR, 
        "\"%s\" does not support operation with \"%s\".",
        jn_obj_to_string(lhs), jn_obj_to_string(rhs)
    );
}


static JnObject* eval_int(Jn_State* state, JnObject* lhs, JnObject* rhs, int op)
{
    
    switch (op)
    {
        case EVAL_ADD:
            if (JN_IS_INT(lhs))
                return eval_bin_int(lhs, rhs, +);                    
            return eval_bin(lhs, rhs, +);
        case EVAL_MUL:
            if (JN_IS_INT(lhs))
                return eval_bin_int(lhs, rhs, *);                    
            return eval_bin(lhs, rhs, *);
        case EVAL_POW:
            return JN_RETURN_INT(state, pow(tonumber(lhs), tonumber(rhs)));
        case EVAL_EQUAL:
            if (JN_IS_INT(lhs))
                return eval_bin_int(lhs, rhs, ==);    
            return eval_bin_bool(lhs, rhs, ==);
        case EVAL_SUB:
            if (JN_IS_INT(lhs))
                return eval_bin_int(lhs, rhs, -);
            return eval_bin(lhs, rhs, -);
        case EVAL_DIV:
            if (JN_IS_INT(rhs) && JN_AS_INT(rhs) == 0)
                return JN_RAISE_EXCPETION(state, MATH_ERROR, "Division by zero.");
            return eval_bin_int(lhs, rhs, /);
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
            return JN_RAISE_EXCPETION(state, TYPE_ERROR, "int does not support this operation");
    }
}

static JnObject* eval_char(Jn_State* state, JnObject* lhs, JnObject* rhs, int op)
{
    if (isnumber_c(lhs) && isnumber_c(rhs))
    {
        switch (op)
        {
            case EVAL_ADD:
                return eval_bin_char(lhs, rhs, +);
            case EVAL_MUL:
                return eval_bin_char(lhs, rhs, *);
            case EVAL_EQUAL:
                return eval_bin_bool_c(lhs, rhs, ==);
            case EVAL_SUB:
                return eval_bin_char(lhs, rhs, -);
            case EVAL_DIV:
                return eval_bin_char(lhs, rhs, /);
            case EVAL_LT:
                return eval_bin_bool_c(lhs, rhs, <);
            case EVAL_LTE:
                return eval_bin_bool_c(lhs, rhs, <=);
            case EVAL_NOTEQUAL:
                return eval_bin_bool_c(lhs, rhs, !=);
            case EVAL_GT:
                return eval_bin_bool_c(lhs, rhs, >);
            case EVAL_GTE:
                return eval_bin_bool_c(lhs, rhs, >=);
            case EVAL_LSHIFT:
                return eval_bin_char(lhs, rhs, <<);
            case EVAL_RSHIFT:
                return eval_bin_char(lhs, rhs, >>);
            case EVAL_PERC:
                return eval_bin_char(lhs, rhs, %);
            case EVAL_BAND:
                return eval_bin_char(lhs, rhs, &);
            case EVAL_BOR:
                return eval_bin_char(lhs, rhs, |);
            case EVAL_BAC:
                return eval_bin_char(lhs, rhs, ^);
            default:
                return JN_RAISE_EXCPETION(state, TYPE_ERROR, "char does not support this operation");
        }
    }
    if (!JN_IS_STRING(rhs))
    {
        return JN_RAISE_EXCPETION(state, TYPE_ERROR, "expected char to string operation");
    }
    // char and string
    char* str;
    bool is_true = false;
    switch (op)
    {
    case EVAL_ADD:
        return JN_RAISE_EXCPETION(state, TYPE_ERROR, "'+' is not supported for a char and string type.");
    case EVAL_IN:
        str = memchr(JN_AS_CSTRING(rhs), JN_AS_CHAR(lhs), JN_AS_STRING(rhs)->len);
        is_true = str != NULL;
        return JN_RETURN_BOOL(state, is_true);
    case EVAL_NOT_IN:
        str = memchr(JN_AS_CSTRING(rhs), JN_AS_CHAR(lhs), JN_AS_STRING(rhs)->len);
        is_true = str == NULL;
        return JN_RETURN_BOOL(state, is_true);        
    default:
        return JN_RAISE_EXCPETION(state, TYPE_ERROR, "char does not support this operator for string.");
    }
}

static JnObject* eval_bool(Jn_State* state, JnObject* lhs, JnObject* rhs, int op)
{
    switch (op)
    {
        case EVAL_EQUAL:
            return JN_RETURN_BOOL(state, lhs->bool_val  == rhs->bool_val );
        case EVAL_NOTEQUAL:
            return JN_RETURN_BOOL(state, lhs->bool_val  != rhs->bool_val );
        case EVAL_ADD:
            return JN_RETURN_INT(state, JN_AS_BOOL(lhs) + JN_AS_BOOL(rhs));
        default:
            return JN_RAISE_EXCPETION(state, TYPE_ERROR, "Invalid operation for bool");
    }
}

static JnObject* eval_string(Jn_State* state, JnObject* lhs, JnObject* rhs, int op)
{
    char* str;
    switch (op)
    {
        case EVAL_ADD:
            str = strcat(JN_AS_CSTRING(lhs), JN_AS_CSTRING(rhs));
            return JN_RETURN_STRING(state, str);
        case EVAL_EQUAL:
            return JN_RETURN_BOOL(
                state,
                lhs->str->hash == rhs->str->hash
            );
        case EVAL_IN:
            return JN_RETURN_BOOL(state, strstr(JN_AS_CSTRING(lhs), JN_AS_CSTRING(rhs)) == NULL);
        case EVAL_NOTEQUAL:
            return JN_RETURN_BOOL(
                state,
                lhs->str->hash != rhs->str->hash
            );
        default:
            return JN_RAISE_EXCPETION(state, TYPE_ERROR, "Invalid operation for string.");
    }
}

static JnObject* eval_float(Jn_State* state, JnObject* lhs, JnObject* rhs, int op)
{
    switch (op)
    {
        case EVAL_ADD:
            return eval_bin(lhs, rhs, +);
        case EVAL_MUL:
            return eval_bin(lhs, rhs, *);
        case EVAL_POW:
            return JN_RETURN_FLOAT(state, pow(tonumber(lhs), tonumber(rhs)));
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
        default:
            return JN_RAISE_EXCPETION(state, TYPE_ERROR, "int does not support this operation");
    }

}

static JnObject* eval_array(Jn_State* state, JnObject* lhs, JnObject* rhs, int op)
{
    switch (op)
    {
        case EVAL_IN:
        {
            return JN_RETURN_BOOL(state, array_contains(lhs, rhs->arr));
        }
        case EVAL_NOT_IN:
        {
            return JN_RETURN_BOOL(state, !array_contains(lhs, rhs->arr));
        }
    default:
        return JN_RAISE_EXCPETION(state, TYPE_ERROR, "Invalid operation.");
    }
}

static JnObject* eval_hashmap(Jn_State* state, JnObject* lhs, JnObject* rhs, int op)
{
    switch (op)
    {
        case EVAL_IN:
        {
            return JN_RETURN_BOOL(state, hashmap_contains(lhs, rhs->hashmap));
        }
        case EVAL_NOT_IN:
        {
            return JN_RETURN_BOOL(state, !hashmap_contains(lhs, rhs->hashmap));
        }
    default:
        return JN_RAISE_EXCPETION(state, TYPE_ERROR, "Invalid operation.");
    }
}
#undef eval_bin
#undef eval_bin_int
#undef eval_bin_bool