#include <assert.h>
#include <stdlib.h>
#include "Joan.h"
#include "opcode.h"
#include "object.h"
#include "vm.h"
#include "gc.h"
#include "emit.h"
#include "ast.h"

J_State Jn_globalState;
static bool __set = false;


// Helper function to read file content


J_Source read_source_file(const char* filename)
{
    J_Source src;
    FILE* p_file;
    p_file = fopen(filename, "rb");
    if (NULL == p_file)
    {
        perror("Filename does not exists.");
        exit(1);
    }
    fseek(p_file, 0, SEEK_END);
    size_t size = ftell(p_file);
    rewind(p_file);
    char* buf = malloc(sizeof(char) * (size + 1));
    if (NULL == buf)
    {
        perror("memory failed.");
        exit(1);
    }
    fread(buf, 1, size, p_file);
    buf[size] = '\0';
    fclose(p_file);
    src.filename = (const char*)strdup(filename);
    src.source = buf;
    return src;
}


// Main allocation function
void* Jn_alloc(size_t size)
{
    void* m = malloc(size);
    assert(m != NULL);
    memset(m, 0, size);
    return m;
}


JN_API J_State* Jn_get_state(void)
{

    J_State* state = &Jn_globalState;
    assert(state->running);
    return state;
}

JN_API J_Context* Jn_get_context(void)
{
    J_State* state = Jn_get_state();
    return &state->cxt;
}


JN_API void Jn_program_init(void)
{
    assert(!__set && "program is already initialize.");
    memset(&Jn_globalState, 0, sizeof(J_State));
    J_State* state = &Jn_globalState;
    __set = true;
    state->vm = malloc(sizeof(JnVM));
    state->gc = malloc(sizeof(GC));
    assert(state->vm != NULL);
    assert(state->gc != NULL);
    state->arena = malloc(sizeof(Arena));
    assert(state->arena);
    arena_init(state->arena);
    state->vm->chuck = malloc(sizeof(struct Chuck));
    assert(state->vm->chuck != NULL);
    chuck_init(state->vm->chuck);
    state->running = true;
    state->parser = malloc(sizeof(joan_parser_t));
    state->gc->next_gc = 1024 * 1024;
    state->gc->bytes_allocated = 0;
    state->gc->object_count = 0;
    state->globals = init_env(NULL); // Jn_global_init(NULL)
    state->gc->objects = NULL;
    assert(state->running && "Something went wrong"); // TODO
    assert(state->globals && "Global not set...");
    assert(state->arena && "Arena not set...");
    state->vm->chuck->env = state->globals;
    Jnvm_init(state->vm, state->vm->chuck);
    assert(state->vm->global != NULL);
}

JN_API int Jn_exec_program(J_State* state, char* source)
{
    if (source == NULL) return -1;
    assert(state->running && "program is not initialize.");
    joan_lexer_t l;
    state->parser->arena = state->arena;
    J_init_lexer(&l, source);
    jn_init_parser(state->parser, &l);
    state->vm->env = state->globals;
    state->vm->global = state->globals;
    assert(state->parser->arena && "Arena not set ...");
    assert(state->parser && "Parser not set ...");
    assert(state->vm->chuck && "VM Chuck is NULL ....");
    while(state->parser->curr.type != TOKEN_EOF)
    {
        AST* stmt = parse_stmt(state->parser);
        compile(stmt, state->vm->chuck);
    }
    write_chuck(state->vm->chuck, OP_END);
    InterpretResult i = vm_run(state->vm);
    // Clean-Up
    gc_collect(state);
    if (i == INTERPRET_RUNTIME_ERROR)
        return -1;
    state->globals = state->vm->env;
    return 0;
}

JN_API int Jn_execute_main(char* filepath)
{
    if (!filepath)
    {
        fprintf(stderr, "filepath was not provided.\n");
        exit(1);
    }
    J_Source src = read_source_file(filepath);
    assert(src.filename != NULL && src.source != NULL);
    J_State* state = Jn_get_state();
    state->cxt.source = src;
    int exit_code = Jn_exec_program(state, src.source);
    return exit_code;
}

JN_API int Jn_exec_REPL(char* source)
{
    if (!source) return -1;
    J_State* state = Jn_get_state();
    int exrt = Jn_exec_program(state, source);
    if (exrt < 0)
        return exrt;
    if (state->vm->sp > state->vm->stack)
    {
        JnObject* obj = (JnObject *)state->vm->sp[-1];
        if (obj == NULL) return 0;
        print_JnObject(obj);
        putchar('\n');
        state->vm->sp[-1] = NULL;
    }
    return 0;
}

JN_API void Jn_program_close(void)
{
    J_State* state = &Jn_globalState;
    assert(state->running && "Program has already stopped."); // TOD: better msg
    state->running = false;
    free(state->globals);
    arena_free(state->arena);
    free(state->arena);
    free(state->globals);
    free(state->parser);
    free(state->vm->chuck);
    free(state->vm);
    free(state->gc);
    free((void *)state->cxt.source.filename);
    free(state->cxt.source.source);
}