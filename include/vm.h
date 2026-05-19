#ifndef VM_H
#define VM_H
#include <stdint.h>
//#include "gc.h"
#include "object.h"
#include "env.h"
#include "parser.h"

#define _STACK_MAX 1024
#define _LooP_MAX 256

typedef struct Chuck Chuck;
typedef struct AST AST;

typedef enum 
{
    INTERPRET_OK,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;

typedef struct VM{
    Chuck* chuck;
    uint8_t* ip;

    Object* stack[_STACK_MAX];
    Object** sp;

    parser* p;

    env_t* global;
    env_t* env;
    //GC gc;
} VM;

typedef struct {
    int loop_offset;
    
    int breaks[_LooP_MAX];
    int break_count;

    int continues[_LooP_MAX];
    int continue_count;

    int returns[_LooP_MAX];
    int return_count;
} LoopContext;

void compile(AST* node, Chuck* chuck);

 InterpretResult vm_run(VM* vm);

#endif