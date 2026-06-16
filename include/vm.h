#ifndef JOAN_VM_H
#define JOAN_VM_H
#include <stdint.h>
//#include "gc.h"
#include "object.h"
#include "env.h"
#include "parser.h"

#define _STACK_MAX 1024
#define _LOOP_MAX 256
#define _FRAME_MAX 64

typedef struct Chuck Chuck;
typedef struct AST AST;

typedef struct Chuck{
    uint8_t* code;
    size_t count;
    size_t capacity;

    int lines[256]; // TODO
    int columns[256]; // TODO
    JnObject** constants;
    int constants_count;
    int constants_capacity;

    char** idents;
    size_t ident_count;
    size_t ident_capacity;

    Jn_environ* env;
} Chuck;


typedef enum 
{
    INTERPRET_OK,
    INTERPRET_RUNTIME_ERROR,
    INTERPRET_ERROR,
} InterpretResult;

typedef struct {
    JnObject** slots;
    JnFunctionObject* fn;
    uint8_t* ip;
    Jn_environ* env;
} CallFrame;

typedef struct JnVM{
    Chuck* chuck;
    uint8_t* ip;

    JnObject* stack[_STACK_MAX];
    JnObject** sp;

    Jn_environ* global;
    Jn_environ* env;

    CallFrame frames[_FRAME_MAX];
    int frame_count;

    //GC gc;
} JnVM;

typedef struct {
    int loop_offset;
    
    int breaks[_LOOP_MAX];
    int break_count;

    int continues[_LOOP_MAX];
    int continue_count;

    int returns[_LOOP_MAX];
    int return_count;
} LoopContext;

void Jnvm_init(JnVM* vm, Chuck* chuck);
void chuck_init(Chuck* chuck);
void compile(AST* node, Chuck* chuck);
void vm_free(JnVM* vm);
void chuck_free(Chuck* chuck);

InterpretResult vm_run(JnVM* vm);

#endif