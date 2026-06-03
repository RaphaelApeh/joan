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

    JnObject** constants;
    int constants_count;
    int constants_capacity;

    char** idents;
    size_t ident_count;
    size_t ident_capacity;

    env_t* env;
} Chuck;


typedef enum 
{
    INTERPRET_OK,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;

typedef struct {
    JnFunctionObject* fn;
    uint8_t* ip;
    env_t* env;
} CallFrame;

typedef struct JnVM{
    Chuck* chuck;
    uint8_t* ip;

    JnObject* stack[_STACK_MAX];
    JnObject** sp;

    joan_parser_t* p;

    env_t* global;
    env_t* env;

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
// Jn_progrm_t rt;
// jn_program_init(&rt);
// jn_exec_program("println \"Hello \" ");
// jn_program_close(&rt);
void jn_program_init(void* rt);
int Jn_exec_program(char* restrict source);
void jn_program_close(void* rt);

void chuck_init(Chuck* chuck);
void compile(AST* node, Chuck* chuck);

InterpretResult vm_run(JnVM* vm);

#endif