#ifndef JOAN_Jn_GC_H
#define JOAN_Jn_GC_H
#include <stdbool.h>
#include <stdint.h>
#include "Joan.h"

typedef struct Jn_GC{
    JnObject* objects;
    size_t bytes_allocated, object_count;
    size_t next_gc;
} Jn_GC;


void* gc_alloc(Jn_State* state, size_t size, JnTypeObject type);
void mark_object(JnObject* obj);
void mark_roots(JnVM* vm);
void sweep(Jn_State* state);
void gc_collect(Jn_State* state);
#endif // JOAN_Jn_GC_H