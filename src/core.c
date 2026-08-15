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

static int read_from_fptr(FILE* f, Jn_Buffer* B)
{
    if (NULL == f)
        return -1;
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    rewind(f);
    char* buf = malloc(sizeof(char) * (size + 1));
    if (NULL == buf)
        return -1;
    fread(buf, 1, size, f);
    buf[size] = '\0';
    fclose(f);
    Jn_buff_add_nstring(B, buf, size);
    return 0;
}

// Helper function to read file content

J_Source read_source_file(const char* filename)
{
    J_Source src;
    FILE* p_file;
    Jn_Buffer b = {0};
    Jn_buff_init(&b);
    p_file = fopen(filename, "rb");
    int err = read_from_fptr(p_file, &b);
    if (err == -1 && b.len == 0)
    {
        perror("Failed to read file.");
        exit(1);
    }
    Jn_buff_add_char(&b, '\0');
    src.filename = (const char*)strdup(filename);
    src.source = b.data;
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

JN_API J_Context* Jn_get_context(Jn_State* state) { return &state->cxt; }

JN_API Jn_Error* Jn_get_error(Jn_State* state)
{
    assert(state != NULL);
    return &state->error;
}

JN_API void Jn_set_global(Jn_State* state, char* name, JnObject* obj)
{
    environ_insert(state->globals, name, obj);
}

JN_API JnObject* Jn_get_global(Jn_State* state, char* name)
{
    Jn_environ_E* ett = environ_get(state->globals, name);
    if (ett->key == NULL || ett->value == NULL) return NULL;
    return ett->value;
}


void set_symbols(Jn_State* state, const char* str)
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

JN_API void Jn_program_init(Jn_State* state, char** argv, int argc)
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
    state->cxt.argv = argv;
    state->cxt.argc = argc;
    JnObject* arr_obj = Jn_get_global(state, "argv");
    if (argc && (arr_obj != NULL))
    {
        for (int i = 0; i < argc; ++i)
        {
            if (!argv[i]) continue;
            jn_arr_append(arr_obj, jn_obj_string(state, argv[i]));
        }
    }

}


JN_API int Jn_compile(Jn_State* state)
{
    JnSemantic sem;
    JnParser* p = state->parser;
    Jn_semantic_init(state, &sem);
    while(p->curr.type != TOK_EOF)
    {
        Jn_Node* stmt = parse_stmt(p);
        stmt = parse_stmt_check(p, stmt);
        Jn_semantic_check(&sem, stmt);
        if (sem.errors) return -1;
        compile(stmt, state->vm->chuck);
    }
    write_chuck(state->vm->chuck, OP_END);
    scope_free(sem.scope);
    return 0;
}

JN_API int Jn_exec(Jn_State* state)
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


JN_API int Jn_exec_program(Jn_State* state, const char* filename, const char* source)
{
    if (source == NULL) return -1;
    if (!filename)
        filename = "main";
    assert(state->running && "program is not initialize.");
    Jn_Lexer l;
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

JN_API int Jn_execute_main(Jn_State* state, const char* filepath)
{
    if (!filepath)
    {
        fprintf(stderr, "filepath was not provided.\n");
        exit(1);
    }
    J_Source src = read_source_file(filepath);
    assert(src.filename != NULL && src.source != NULL);
    state->cxt.source = src;
    Jn_register(state, "__FILE__", "Returns the filename or main in repl.", JN_RETURN_STRING(state, (char *)filepath));
    int exit_code = Jn_exec_program(state, filepath, src.source);
    return exit_code;
}


JN_API int Jn_exec_from_file(Jn_State* state, char* filename, FILE* fptr) 
{
    if (!fptr) return -1;
    Jn_Buffer b;
    Jn_buff_init(&b);
    read_from_fptr(fptr, &b);
    if (filename)
        state->cxt.source.filename = strdup(filename);
    state->cxt.source.source = b.data; 
    state->cxt.argv = NULL;
    state->cxt.argc = 0;
    int exit_code = Jn_exec_program(state, filename, b.data);    
    return exit_code;
}

JN_API int Jn_exec_string(Jn_State* state, const char* string)
{
    if (!string) return -1;
    Jn_Buffer b;
    Jn_buff_init(&b);
    Jn_buff_add_string(&b, string);
    Jn_buff_to_string(&b);
    state->cxt.source.source = b.data;
    state->cxt.argv = NULL;
    state->cxt.argc = 0;
    int exit_code = Jn_exec_program(state, NULL, b.data);
    return exit_code;
}

JN_API int Jn_exec_REPL(Jn_State* state, const char* source)
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


JN_API JnObject* Jn_import_module(Jn_State* state, char* path, int is_std)
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
    Jn_State st = {0};
    J_Context* cxt = Jn_get_context(state);
    J_Source old = cxt->source;
    J_Source src = read_source_file(filename);
    cxt->source = src;
    Jn_Lexer l;
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
    while(p.curr.type != TOK_EOF)
    {
        Jn_Node* stmt = parse_stmt(&p);
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


JN_API void Jn_program_close(Jn_State* state)
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
