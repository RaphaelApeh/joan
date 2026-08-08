#ifndef JOAN_OBJECT_H
#define JOAN_OBJECT_H
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "Joan.h"
#include "helper.h"
#include "env.h"


#define JNSTR_OBJ(s) (JnStringObject){.chars = str_esc(s), .len = strlen((s)), .hash = djb2_hash((const unsigned char *)(s))}

#define DEFAULT_LAMBDA_NAME "<lambda>"
typedef struct JnObject JnObject;

typedef struct JnInternEntry {
    JnObject* obj;
    struct JnInternEntry* next;
} JnInternEntry;

JnObject* jn_obj_function(J_State*, AST* block, Jn_environ* env, char** params, int arity, char* name);
JnObject* jn_obj_lambda(J_State*, AST* expr, char** params, int arity, Jn_environ* env);
int64_t range_len(JnRange* r);
int64_t range_at(JnRange* r, int64_t idx);
void jn_obj_reassign(JnObject* dest, JnObject* src);
JnObject* bind_argument(J_State*, JnObject* obj, char** fields, JnObject** values, long count);
#endif