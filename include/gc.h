#ifndef GC_H
#include <stdbool.h>
#include <stdint.h>
#include "object.h"

typedef struct GCObject GCObject;
typedef struct VM VM;

struct GCObject{
    GCObject* next;
    bool marked;
    ObjectType kind;
};

typedef struct{
    GCObject* objects;
    size_t bytes_allocated;
    size_t next_gc;
} GC;


void* gc_alloc(VM* vm, size_t size, ObjectType kind);
void mark_gcobj(GCObject* gcobj);
void mark_object(Object* obj);
void mark_roots(VM* vm);
void sweep(VM* vm);
void gc_collect(VM* vm);
#endif