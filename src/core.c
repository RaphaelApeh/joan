#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <Joan.h>

#include "opcode.h"
#include "object.h"
#include "semantic.h"
#include "vm.h"
#include "gc.h"
#include "emit.h"
#include "ast.h"
#include "env.h"


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
JN_API void* Jn_alloc(size_t size)
{
    void* m = malloc(size);
    assert(m != NULL);
    return m;
}

JN_API void* Jn_realloc(void* ptr, size_t size)
{
    if (NULL == ptr) return NULL;
    if (size == 0)
    {
        free(ptr);
        return NULL;
    }
    void* re_ptr = realloc(ptr, size);
    return re_ptr;
}

JN_API void* Jn_alloc_dup(void* ptr, size_t size)
{
    void* new_ptr = malloc(size);
    memmove(new_ptr, ptr, size);
    return new_ptr;
}

JN_API void Jn_free(void* ptr)
{
    if (NULL == ptr) return;
    free(ptr);
}

JN_API void Jn_mem_zero(void* ptr, size_t size) 
{
    if (NULL == ptr || size == 0) return; 
    memset(ptr, 0, size);
}


JN_API J_Context* Jn_get_context(J_State* state) { return &state->cxt; }

JN_API Jn_Error* Jn_get_error(J_State* state)
{
    assert(state != NULL);
    return &state->error;
}

void set_symbols(J_State* state, const char* str)
{
    assert(state != NULL);
    assert(state->symbols != NULL);
    if (state->symbols_count >= state->symbols_capacity)
    {
        state->symbols_capacity *= 2;
        state->symbols = realloc(state->symbols, sizeof(char *) * state->symbols_capacity);
    }
    state->symbols[state->symbols_count++] = str;
}

JN_API void Jn_program_init(J_State* state)
{
    state->vm = malloc(sizeof(JnVM));
    state->gc = malloc(sizeof(Jn_GC));
    memset(&state->error, 0, sizeof(Jn_Error));
    // memset(state->intern_pool, 0, sizeof(JnInternEntry));
    state->symbols_count = 0;
    state->symbols_capacity = 56;
    state->symbols = malloc(sizeof(char *) * 56);
    assert(state->vm != NULL);
    assert(state->gc != NULL);
    state->arena = malloc(sizeof(Jn_Arena));
    assert(state->arena);
    arena_init(state->arena);
    state->vm->chuck = malloc(sizeof(struct Chuck));
    assert(state->vm->chuck != NULL);
    chuck_init(state->vm->chuck);
    state->running = true;
    state->parser = malloc(sizeof(JnParser));
    state->gc->next_gc = 1024 * 1024;
    state->gc->bytes_allocated = 0;
    state->gc->object_count = 0;
    state->globals = Jn_environ_init(NULL);
    state->gc->objects = NULL;
    assert(state->running && "Something went wrong");
    assert(state->globals && "Global not set...");
    assert(state->arena && "Jn_Arena not set...");
    state->vm->chuck->env = state->globals;
    state->vm->global = state->globals;
    state->parser->state = state;
    Jnvm_init(state->vm, state->vm->chuck);
    assert(state->vm->chuck->lines);
    assert(state->vm->chuck->lines);
    assert(state->vm->global != NULL);
    Jn_load_Cfunctions(state);
}


JN_API int Jn_compile(J_State* state)
{
    JnSemantic sem;
    JnParser* p = state->parser;
    Jn_semantic_init(state, &sem);
    while(p->curr.type != TOKEN_EOF)
    {
        AST* stmt = parse_stmt(p);
        stmt = parse_stmt_check(p, stmt);
        Jn_semantic_check(&sem, stmt);
        if (sem.errors) return -1;
        compile(stmt, state->vm->chuck);
    }
    write_chuck(state->vm->chuck, OP_END);
    scope_free(sem.scope);
    return 0;
}

JN_API int Jn_exec(J_State* state)
{
    int i = vm_run(state, state->vm);
    if (
        i == JN_INTERPRET_RUNTIME_ERROR || 
        i == JN_INTERPRET_ERROR
    )
    {
#if JOAN_DEBUG
        JN_LOG("Resetting VM.");
#endif
        reset_vm(state->vm);
        return -1;
    }
    return state->vm->exit_code;   
}


JN_API int Jn_exec_program(J_State* state, const char* filename, const char* source)
{
    if (source == NULL) return -1;
    if (!filename)
        filename = "main";
    assert(state->running && "program is not initialize.");
    joan_lexer_t l;
    state->parser->arena = state->arena;
    J_init_lexer(&l, (char *)source, filename);
    jn_init_parser(state->parser, &l);
    state->vm->global = state->globals;
    state->vm->env = state->globals;
    state->vm->chuck->env = state->vm->env;
    if (!state->cxt.source.source)
        state->cxt.source.source = strdup(source);
    if (!state->cxt.source.filename)
        state->cxt.source.filename = strdup(filename);
    assert(state->parser->arena && "Jn_Arena not set ...");
    assert(state->parser && "Parser not set ...");
    assert(state->vm->chuck && "VM Chuck is NULL ....");
    int exit_code = Jn_compile(state);
    if (exit_code < 0)
    {
        Jn_Error* err = Jn_get_error(state);
        Jn_error_printf(
            "%s:%d:%d Error [%s] %s\n",
            err->filename,
            err->line,
            err->col,
            JN_ERROR_PRINT(err->type),
            err->error_msg
        );
        return -1;
    }
    exit_code = Jn_exec(state);
    return exit_code;
}

JN_API int Jn_execute_main(J_State* state, const char* filepath, char** argv, int argc)
{
    if (!filepath)
    {
        fprintf(stderr, "filepath was not provided.\n");
        exit(1);
    }
    J_Source src = read_source_file(filepath);
    assert(src.filename != NULL && src.source != NULL);
    state->cxt.source = src;
    state->cxt.argv = argv;
    state->cxt.argc = argc;
    Jn_register(state, "__FILE__", "Returns the filename or main in repl.", JN_RETURN_STRING(state, (char *)filepath));
    int exit_code = Jn_exec_program(state, filepath, src.source);
    return exit_code;
}


JN_API int Jn_exec_from_file(J_State* state, FILE* fptr) 
{
    // TODO
    assert(false && "Not yet Implemented.");
}

JN_API int Jn_exec_string(J_State* state, const char* string)
{
    // TODO:
    assert(false && "Not yet Implemented.");
}

JN_API int Jn_exec_REPL(J_State* state, const char* source)
{
    if (!source) return -1;
    state->cxt.source.filename = NULL;
    state->cxt.source.source = strdup(source);
    Jn_load_repl_functions(state);
    int exrt = Jn_exec_program(state, "main", source);
    if (exrt < 0)
        return exrt;
    if (state->vm->sp > state->vm->stack)
    {
        JnObject* obj = (JnObject *)state->vm->sp[-1];
        if (obj == NULL) return 0;
        jn_obj_print(obj);
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
        return  JN_RAISE_EXCPETION(state, IMPORT_ERROR, "cannot import %s.", filename);
    JnObject* mod = find_module(filename);
    if (NULL != mod)
        return mod;
    J_State st = {0};
    J_Context* cxt = Jn_get_context(state);
    J_Source old = cxt->source;
    J_Source src = read_source_file(filename);
    cxt->source = src;
    joan_lexer_t l;
    JnParser p = {0};
    p.arena = state->arena;
    J_init_lexer(&l, src.source, path);
    jn_init_parser(&p, &l);
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
    while(p.curr.type != TOKEN_EOF)
    {
        AST* stmt = parse_stmt(&p);
        compile(stmt, &chuck);
    }
    write_chuck(&chuck, OP_END);
    cxt->source = old;
    JnVMInterpretResult i = vm_run(state, &vm);
    if (i != JN_INTERPRET_OK)
        return JN_RAISE_EXCPETION(state, SYS_ERROR, "extra error message.");
    JnObject* obj = jn_obj_module(state, path, filename, env);
    module_register[module_count++] = (struct Module_Reg){obj, filename};
    return obj;
}

JN_API void Jn_program_close(J_State* state)
{
    assert(state->running && "Program has already stopped."); // TOD: better msg
    state->running = false;
    gc_collect(state);
    free(state->globals);
    arena_free(state->arena);
    free(state->arena);
    free(state->parser);
    chuck_free(state->vm->chuck);
    free(state->vm->chuck);
    free(state->vm);
    free(state->gc);
    free(state->symbols);
    if (state->cxt.source.filename != NULL)
        free((void *)state->cxt.source.filename);
    free(state->cxt.source.source);
}
