#ifndef JOAN_VM_H
#define JOAN_VM_H
#include <stdint.h>
#include "object.h"
#include "env.h"
#include "parse.h"

typedef struct Chuck Chuck;
typedef struct Jn_Node Jn_Node;

#define DEFAULT_VM_EXIT_CODE 1
typedef struct Chuck{
    uint8_t* code;
    size_t count;
    size_t capacity;

    int* lines;
    int* columns;
    JnObject** constants;
    int constants_count;
    int constants_capacity;

    char** idents;
    size_t ident_count;
    size_t ident_capacity;

    Jn_environ* env;
} Chuck;


typedef struct {
    JnObject** slots;
    JnFunctionObject* fn;
    uint8_t* ip;
    Jn_environ* env;
} CallFrame;

typedef struct JnVM{
    Chuck* chuck;
    uint8_t* ip;

    JnObject* stack[JN_STACK_MAX];
    JnObject** sp;

    Jn_environ* global;
    Jn_environ* env;

    JnObject* yielded;
    CallFrame frames[JN_FRAME_MAX];
    int frame_count;

    int exit_code;
    bool want_exit;
} JnVM;

typedef struct {
    int loop_offset;
    
    int breaks[JN_LOOP_MAX];
    int break_count;

    int continues[JN_LOOP_MAX];
    int continue_count;

    int returns[JN_LOOP_MAX];
    int return_count;
} LoopContext;

void Jnvm_init(JnVM* vm, Chuck* chuck);
void chuck_init(Chuck* chuck);
void compile(Jn_Node* node, Chuck* chuck);
void vm_free(JnVM* vm);
void reset_vm(JnVM* vm);
void chuck_free(Chuck* chuck);

int vm_run(Jn_State* state, JnVM* vm);

#endif