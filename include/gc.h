#ifndef JOAN_GC_H
#define JOAN_GC_H
#include <stdbool.h>
#include <stdint.h>
#include "Joan.h"

typedef struct GC{
    JnObject* objects;
    size_t bytes_allocated, object_count;
    size_t next_gc;
} GC;


void* gc_alloc(J_State* state, size_t size, JnTypeObject type);
void mark_object(JnObject* obj);
void mark_roots(JnVM* vm);
void sweep(J_State* state);
void gc_collect(J_State* state);
#endif // JOAN_GC_H