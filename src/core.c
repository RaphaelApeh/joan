#include <assert.h>
#include <stdlib.h>
#include "Joan.h"
#include "opcode.h"
#include "object.h"
#include "vm.h"
#include "gc.h"
#include "emit.h"
#include "ast.h"
#include "env.h"


J_State Jn_globalState;
static bool __set = false;

struct Module_Reg {
    JnObject* module;
    char* name;
};

struct Module_Reg module_register[300];
static int module_count = 0;
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
    state->globals = Jn_environ_init(NULL);
    state->gc->objects = NULL;
    assert(state->running && "Something went wrong");
    assert(state->globals && "Global not set...");
    assert(state->arena && "Arena not set...");
    state->vm->chuck->env = state->globals;
    state->vm->global = state->globals;
    Jnvm_init(state->vm, state->vm->chuck);
    assert(state->vm->global != NULL);
    Jn_load_Cfunctions(state);
}

JN_API int Jn_exec_program(J_State* state, char* source)
{
    if (source == NULL) return -1;
    assert(state->running && "program is not initialize.");
    joan_lexer_t l;
    state->parser->arena = state->arena;
    J_init_lexer(&l, source);
    jn_init_parser(state->parser, &l);
    state->vm->global = state->globals;
    state->vm->env = state->globals;
    state->vm->chuck->env = state->vm->env;
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
    if (i != INTERPRET_OK)
    {
        // RESET
        state->vm->chuck->count = 0;
        state->vm->ip = state->vm->chuck->code;
        Jnvm_init(state->vm, state->vm->chuck);
        return -1;
    }
    // state->globals = state->vm->env;
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
    Jn_register(state, "__FILE__", "Returns the filename or main in repl.", JN_RETURN_STRING(filepath));
    int exit_code = Jn_exec_program(state, src.source);
    return exit_code;
}

JN_API int Jn_exec_REPL(char* source)
{
    if (!source) return -1;
    J_State* state = Jn_get_state();
    state->cxt.source.filename = NULL;
    state->cxt.source.source = strdup(source);
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


static JnObject* find_module(char* name)
{
    for (int i = 0; i < module_count; ++i)
    {
        struct Module_Reg* mod = &module_register[i];
        if (strcmp(mod->name, name) == 0)
            return mod->module;
    }
    return NULL;
}


JN_API JnObject* Jn_import_module(J_State* state, char* path, int is_std)
{
    if (state == NULL)
    {
        state = Jn_get_state();
    }
    assert(state != NULL);
    char buff[100];
    char* filename;
    snprintf(buff, sizeof buff, "%s.jt", path);
    assert(JN_STD_PATH);
    char* std_path = strdup(JN_STD_PATH);
    if (is_std)
        filename = strcat(std_path, buff);
    else
        filename = strdup(buff);
    
    bool exists = file_exists(filename);
    if (!exists)
        return  JN_RAISE_EXCPETION(IMPORT_ERROR, "cannot import %s.", filename);
    JnObject* mod = find_module(filename);
    if (NULL != mod)
        return mod;
    J_State st = {0};
    J_Context* cxt = Jn_get_context();
    J_Source old = cxt->source;
    J_Source src = read_source_file(filename);
    cxt->source = src;
    joan_lexer_t l;
    J_init_lexer(&l, src.source);
    jn_init_parser(state->parser, &l);
    Jn_environ* env = Jn_environ_init(NULL);
    JnVM vm = {0};
    Chuck chuck = {0};
    chuck.env = env;
    st.globals = env;
    st.cxt = *cxt;
    vm.chuck = &chuck;
    chuck_init(&chuck);
    Jnvm_init(&vm, &chuck);
    Jn_load_Cfunctions(&st);
    while(state->parser->curr.type != TOKEN_EOF)
    {
        AST* stmt = parse_stmt(state->parser);
        compile(stmt, &chuck);
    }
    write_chuck(&chuck, OP_END);
    cxt->source = old;
    InterpretResult i = vm_run(&vm);
    if (i != INTERPRET_OK)
        return JN_RAISE_EXCPETION(SYS_ERROR, "extra error message.");
    JnObject* obj = jn_obj_module(path, filename, env);
    module_register[module_count++] = (struct Module_Reg){obj, filename};
    return obj;
}

JN_API void Jn_program_close(void)
{
    J_State* state = &Jn_globalState;
    assert(state->running && "Program has already stopped."); // TOD: better msg
    state->running = false;
    free(state->globals);
    arena_free(state->arena);
    free(state->arena);
    free(state->parser);
    chuck_free(state->vm->chuck);
    // vm_free(state->vm);
    free(state->vm->chuck);
    free(state->vm);
    free(state->gc);
    if (state->cxt.source.filename != NULL)
        free((void *)state->cxt.source.filename);
    free(state->cxt.source.source);
}