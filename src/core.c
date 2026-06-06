#include <assert.h>
#include <stdlib.h>
#include "Joan.h"
#include "opcode.h"
#include "ast.h"


typedef struct J_Context {
    int error_code;
} J_Context;

typedef struct J_State
{
    JnVM vm;
    J_Context cxt;
    Arena* arena;
    joan_parser_t* parser;
    JnObject** objects;
    size_t object_count;
    size_t object_capacity;
    InternEntry* intern_pool[INTER_SIZE];
    JnObject_Alloc alloc_fn;
    env_t* globals;
    size_t bytes_allocated;
    size_t next_gc;
    bool running;
} J_State;

J_State Jn_globalState;

static void* alloc_object(size_t size, JnTypeObject type)
{
    assert(Jn_globalState.running && "Program not initialized.");
    J_State* state = &Jn_globalState;
    JnObject* obj = malloc(size);
    assert(obj != NULL);
    memset(obj, 0, sizeof(*obj));
    obj->type = type;
    state->objects[state->object_count] = obj;
    state->bytes_allocated += size;
    return obj;
}
JN_API void Jn_program_init(void)
{
    memset(&Jn_globalState, 0, sizeof(J_State));
    J_State* state = &Jn_globalState;
    state->vm = (JnVM){0};
    state->arena = malloc(sizeof(Arena));
    assert(state->arena);
    arena_init(state->arena);
    state->vm.chuck = malloc(sizeof(struct Chuck));
    assert(state->vm.chuck != NULL);
    chuck_init(state->vm.chuck);
    state->running = true;
    state->object_capacity = 1000;
    state->object_count = 0;
    state->alloc_fn = alloc_object;
    state->next_gc = 1024 * 1024;
    state->globals = init_env(NULL); // Jn_global_init(NULL)
    state->objects = malloc(sizeof(JnObject* ) * state->object_capacity);
    assert(state->running && "Something went wrong"); // TODO
    assert(state->objects != NULL && "malloc failed.");
    assert(state->globals && "Global not set...");
    assert(state->arena && "Arena not set...");
    state->vm.chuck->env = state->globals;
    Jnvm_init(&state->vm, state->vm.chuck);
    assert(state->vm.global != NULL);
}

JN_API int Jn_exec_program(char* source)
{
    if (source == NULL) return -1;
    J_State* state = &Jn_globalState;
    assert(state->running && "program is not initialize.");
    joan_lexer_t l;
    J_init_lexer(&l, source);
    joan_parser_t* p = jn_init_parser(&l);
    state->parser = p;
    p->arena = state->arena;
    state->vm.p = p;
    state->vm.env = state->globals;
    state->vm.global = state->globals;
    state->vm.chuck->count = 0;
    assert(p->arena && "Arena not set ...");
    assert(state->parser && "Parser not set ...");
    assert(state->vm.chuck && "VM Chuck is NULL ....");
    while(p->curr.type != TOKEN_EOF)
    {
        AST* stmt = parse_stmt(p);
        compile(stmt, state->vm.chuck);
    }
    write_chuck(state->vm.chuck, OP_END);
    InterpretResult i = vm_run(&state->vm);
    if (i == INTERPRET_RUNTIME_ERROR)
        return -1;
    return 0;
}

JN_API void Jn_program_close(void)
{
    J_State* state = &Jn_globalState;
    assert(state->running && "Program has already stopped."); // TOD: better msg
    state->running = false;
    for (int i = 0; i < state->object_count; i++)
        free(state->objects[i]); // TODO JnObjectFree(obj)
    free(state->objects);
    arena_free(state->arena);
    free(state->globals);
    free(state->parser);
}