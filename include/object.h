#ifndef JOAN_OBJECT_H
#define JOAN_OBJECT_H
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "Joan.h"
#include "helper.h"
#include "env.h"


typedef struct JnObject JnObject;

#define JNSTR_OBJ(s) (JnStringObject){.chars = strdup((s)), .len = strlen((s)), .hash = djb2_hash((s))}

typedef struct InternEntry {
    JnObject* obj;
    struct InternEntry* next;
} InternEntry;

int64_t range_len(JnRange* r);
int64_t range_at(JnRange* r, int64_t idx);
JnObject* jn_intern_obj(JnObject* obj);

bool is_truthy(JnObject* obj);
void print_JnObject(JnObject* obj);
#endif