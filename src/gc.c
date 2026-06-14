#include <assert.h>
#include <stdlib.h>
#include "vm.h"
#include "gc.h"


void* gc_alloc(size_t size, JnTypeObject type)
{
    J_State* state = Jn_get_state();
    GC* gc = state->gc;
    JnObject* obj = malloc(size);
    assert(obj != NULL);
    memset(obj, 0, size);
    obj->type = type;
    obj->marked = false;
    obj->next = gc->objects;
    gc->objects = obj;
    gc->object_count++;
    gc->bytes_allocated += size;
    if (state->gc->bytes_allocated > state->gc->next_gc)
    {
        gc_collect(state);
    }
    return obj;
}

void mark_object(JnObject* obj)
{
    if (NULL == obj) return;
    if (obj->marked) return;

    obj->marked = true;
    switch (obj->type)
    {
        case ARRAY_TYPE:
            for (int i = 0; i < obj->arr->size; ++i)
                mark_object(obj->arr->items[i]);
            obj->marked = true;
            break;
        case HASHMAP_TYPE:
            for (long i = 0; i < obj->hashmap->size; ++i)
            {
                mark_object(obj->hashmap->buckets[i].key);
                mark_object(obj->hashmap->buckets[i].value);
            }
            obj->marked = true;
            break;
    }
}
void mark_roots(JnVM* vm)
{
    for (JnObject** slot = vm->stack; slot < vm->sp; slot++)
        mark_object(*slot);

    for (int i = 0; i < vm->global->count; ++i)
        mark_object(vm->global->entries[i].value);
}

void Jn_freeObject(JnObject* obj)
{
    if (NULL == obj) return;
    switch (obj->type)
    {
        case STR_TYPE:
            free(obj->str->chars);
            free(obj->str);
            break;
        case ARRAY_TYPE:
            for (int i = 0; i < obj->arr->size; ++i)
                Jn_freeObject(obj->arr->items[i]);
            free(obj->arr);
            break;
        case HASHMAP_TYPE:
            for (int i = 0; i < obj->hashmap->size; ++i)
            {
                Jn_freeObject(obj->hashmap->buckets[i].key);
                Jn_freeObject(obj->hashmap->buckets[i].value);
            }
            free(obj->hashmap);
            break;
        case FUNCTION_TYPE:
            break; // TODO
        case ITER_TYPE:
            Jn_freeObject(obj->iter->obj);
            free(obj->iter);
            break;
    }
    free(obj);
}

void sweep(J_State* state){
    JnObject** obj = &state->gc->objects;

    while (*obj)
    {
        if (!(*obj)->marked)
        {
            JnObject* unreached = *obj;
            *obj = unreached->next;
            Jn_freeObject(unreached);
        } else {
            (*obj)->marked = false;
            obj = &(*obj)->next;
        }
    }
}
void gc_collect(J_State* state)
{
    mark_roots(state->vm);
    sweep(state);
    state->gc->next_gc = state->gc->bytes_allocated * 2;
}