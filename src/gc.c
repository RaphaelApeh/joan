#include <stdlib.h>
#include "vm.h"
#include "gc.h"


void* gc_alloc(VM* vm, size_t size, JnTypeObject kind);
//{
    // GCObject* gcb = malloc(sizeof(gcb));
    // gcb->marked = false;
    // gcb->kind = kind;
    // gcb->next = vm->gc.objects;
    // vm->gc.objects = gcb;
    // vm->gc.bytes_allocated += size;
    // return gcb;
//}

void mark_gcobj(GCObject* gcobj);
// {
//     if (NULL == gcobj && gcobj->marked) return;
//     gcobj->marked = true;
//     switch (gcobj->kind)
//     {
//     case ARRAY_TYPE:
//         /* code */
//         break;
    
//     default:
//         break;
//     }
// }
void mark_object(JnObject* obj);
void mark_roots(VM* vm);
void sweep(VM* vm);
void gc_collect(VM* vm);