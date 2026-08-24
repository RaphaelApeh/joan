#include <assert.h>
#include <stdlib.h>
#include "vm.h"
#include "gc.h"


void* gc_alloc(Jn_State* state, size_t size, JnTypeObject type)
{
    Jn_GC* gc = state->gc;
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
        case JN_ARRAY_TYPE:
            for (int i = 0; i < obj->arr->size; ++i)
                mark_object(obj->arr->items[i]);
            obj->marked = true;
            break;
        case JN_TUPLE_TYPE:
            for (int i = 0; i < obj->tuple->size; ++i)
                mark_object(obj->tuple->items[i]);
            obj->marked = true;
            break;
        case JN_HASHMAP_TYPE:
            for (long i = 0; i < obj->hashmap->size; ++i)
            {
                mark_object(obj->hashmap->buckets[i].key);
                mark_object(obj->hashmap->buckets[i].value);
            }
            obj->marked = true;
            break;
        default:
            break;
    }
}
void mark_roots(JnVM* vm)
{
    for (JnObject** slot = vm->stack; slot < vm->sp; slot++)
        mark_object(*slot);

    for (int i = 0; i < vm->global->size; ++i)
        mark_object(vm->global->buckets[i].value);
}

void Jn_freeObject(JnObject* obj)
{
    if (NULL == obj) return;
    switch (obj->type)
    {
        case JN_STRING_TYPE:
            free(obj->str->chars);
            free(obj->str);
            break;
        case JN_ARRAY_TYPE:
            for (int i = 0; i < obj->arr->size; ++i)
                Jn_freeObject(obj->arr->items[i]);
            free(obj->arr);
            break;
        case JN_TUPLE_TYPE:
            {
                for (int i = 0; i < obj->tuple->size; ++i)
                    Jn_freeObject(obj->tuple->items[i]);
                free(obj->tuple);
            } break;
        case JN_HASHMAP_TYPE:
            for (int i = 0; i < obj->hashmap->size; ++i)
            {
                Jn_freeObject(obj->hashmap->buckets[i].key);
                Jn_freeObject(obj->hashmap->buckets[i].value);
            }
            free(obj->hashmap);
            break;
        case JN_FUNCTION_TYPE:
            chuck_free(obj->fn->chuck);
            free(obj->fn->chuck);
            free(obj->fn->env->buckets);
            free(obj->fn->env);
            free(obj->fn);
            break;
        case JN_NATIVE_TYPE:
            free(obj->native_fn->fnName);
            free(obj->native_fn);
            break;
        case JN_ITER_TYPE:
            Jn_freeObject(obj->iter->obj);
            free(obj->iter);
            break;
        case JN_GENERATOR_TYPE:{
            // vm_free(obj->gen->vm); TODO: impl vm_free()
            free(obj->gen);
        } break;
        case JN_MODULE_TYPE:
            free(obj->module->env->buckets);
            free(obj->module->env);
            free(obj->module);
            break;
        case JN_INSTANCE_TYPE:
            free(obj->instance->fields->buckets);
            free(obj->instance->fields);
            // free(obj->instance->obj);
            free(obj->instance);
            break;
        // TODo
        // case JN_ARG_TYPE:
        //     free(obj->arg.args);
        //     break;
        default:
            break;
        }
    free(obj);
    obj = NULL;
}

JN_API size_t jn_obj_size(JnObject* obj)
{
    if (NULL == obj) return 0;

    size_t size = 0;

    switch (jn_obj__type(obj))
    {
        case JN_STRING_TYPE:
            size = sizeof(*obj->str) + (obj->str->length + 1); // +1 for null terminator
            break;

        case JN_ARRAY_TYPE:
            size = sizeof(*obj->arr) + obj->arr->capacity * sizeof(JnObject*);
            break;

        case JN_TUPLE_TYPE:
            size = sizeof(*obj->tuple) + obj->tuple->size * sizeof(JnObject*);
            break;

        case JN_HASHMAP_TYPE:
            size = sizeof(*obj->hashmap) + obj->hashmap->capacity * sizeof(*obj->hashmap->buckets);
            break;

        case JN_FUNCTION_TYPE:
            size = sizeof(*obj->fn);
            if (obj->fn->env)
                size += sizeof(*obj->fn->env) + obj->fn->env->capacity * sizeof(*obj->fn->env->buckets);
            break;

        case JN_NATIVE_TYPE:
            size = sizeof(*obj->native_fn);
            break;

        case JN_ITER_TYPE:
            size = sizeof(*obj->iter);
            break;

        case JN_MODULE_TYPE:
            size = sizeof(*obj->module);
            if (obj->module->env)
                size += sizeof(*obj->module->env) + obj->module->env->capacity * sizeof(*obj->module->env->buckets);
            break;

        case JN_INSTANCE_TYPE:
            size = sizeof(*obj->instance);
            if (obj->instance->fields)
                size += sizeof(*obj->instance->fields) + obj->instance->fields->capacity * sizeof(*obj->instance->fields->buckets);
            break;

        default:
            size = sizeof(JnObject);
            break;
    }

    return size;
}

void sweep(Jn_State* state){
    JnObject** obj = &state->gc->objects;
    long count = 0;
    while (*obj)
    {
        if (!(*obj)->marked)
        {
            JnObject* unreached = *obj;
            *obj = unreached->next;
            #if defined(JOAN_DEBUG)
                printf("Freeing object count (%ld)....\n", count++);
            #endif
            state->gc->bytes_allocated -= jn_object_size(unreached);
            state->gc->object_count--;
            Jn_freeObject(unreached);
        } else {
            (*obj)->marked = false;
            obj = &(*obj)->next;
        }
    }
}

JN_API void Jn_gc_sweep(Jn_State*  state) { sweep(state); }
JN_API void Jn_garbage_collect(Jn_State* state) { gc_collect(state); }

void gc_collect(Jn_State* state)
{
    mark_roots(state->vm);
    sweep(state);
    state->gc->next_gc = state->gc->bytes_allocated * 2;
}
