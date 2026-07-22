#include <assert.h>
#include <Joan.h>
#include "semantic.h"

#ifndef C_STRING_H
#include "optionals/c_string.h"
#endif


void Jn_semantic_init(J_State* state, JnSemantic* sem)
{
    assert(sem);
    sem->scope = scope_new(NULL);
    sem->fnc_depth = 0;
    sem->fnc_depth = 0;
    sem->errors = 0;
    sem->warnings = 0;
    sem->state = state;
}


void Jn_semantic_check(JnSemantic* sem, AST* node)
{
    Jn_visit(sem, node);
    if (sem->errors)
    {
        fprintf(stdout, "(%d) semantic error(s).\n", sem->errors);
    }
    if (sem->warnings)
    {
        print_source_line(sem->state->cxt.source.source, sem->state->cxt.cur_line, sem->state->cxt.column);
        fprintf(stdout, "(%d) semantice warning(s).\n", sem->warnings);
    }
}

void error(JnSemantic* sem, AST* node, const char* msg, ...)
{
    print_source_lines(sem->state->cxt.source.source, node->line, node->col, 2);
    putc('\n', stdout);
    sem->errors++;
    sem->state->error.error_msg = msg;
    sem->state->error.line = node->line;
    sem->state->error.col = node->col;
    sem->state->error.type = SYNTAX_ERROR;
    sem->state->error.code = -1;
    sem->state->error.filename = node->filename;
}
void warning(JnSemantic* sem, AST* node, const char* msg, ...)
{
    sem->warnings++;
    print_source_lines(sem->state->cxt.source.source, node->line, node->col, 2);
    fprintf(
        stderr, 
        "%s:%d:%d Warning: %s\n", 
        node->filename, 
        node->line, 
        node->col, 
        msg
    );
}

// Scope


JnScope* scope_new(JnScope* parent)
{
    JnScope* scope = Jn_alloc(sizeof(JnScope));
    scope->parent = parent;
    scope->symbols = NULL; // default
    return scope;
}

void scope_free(JnScope* scope)
{
    JnSymbol* sym = scope->symbols;
    while (sym)
    {
        JnSymbol* next = sym->next;
        free(sym);
        sym = next;
    }
    free(scope);
    scope = NULL;
}

bool scope_insert(JnScope* scope, const char* name, int Kind, bool is_const)
{
    if (scope_lookup_current(scope, name))
        return false;
    JnSymbol* sym = Jn_alloc(sizeof(JnSymbol));
    sym->name = (char *)name;
    sym->kind = Kind;
    sym->is_const = is_const;
    sym->type = NULL;
    sym->next = scope->symbols;
    scope->symbols = sym;
    return true;
}

JnSymbol* scope_lookup(JnScope* scope, const char* name)
{
    while (scope)
    {
        JnSymbol* sym = scope_lookup_current(scope, name);
        if (sym)
            return sym;
        scope = scope->parent;
    }
    return NULL;
}

bool symbol_lookup(JnSemantic* sem, JnScope* scope, const char* name)
{
    for (int i = 0; i < sem->state->symbols_count; ++i)
        if (strcmp(sem->state->symbols[i], name) == 0)
            return true;

    
    struct FuzzMatch matches[300];    
    int n = fuzzy_match(
        name,
        (char **)sem->state->symbols,
        sem->state->symbols_count,
        matches
    );
    if (n > 0)
    {
        printf("Did you mean: ");
        for (int i = 0; i < n; ++i)
        {
            if (i > 0)
                putchar(',');
            printf(" %s", matches[i].word);
        }
        printf("\n");
    }
    return false;
}

bool symbol_insert(JnSemantic* sem, JnScope* scope, const char* name, int kind, bool is_const)
{
    if (symbol_lookup(sem, scope, name)) return true;

    set_symbols(sem->state, name);
    JnSymbol* sym = Jn_alloc(sizeof(JnSymbol));
    sym->name = (char *)name;
    sym->kind = kind;
    sym->is_const = is_const;
    sym->type = NULL;
    sym->next = scope->symbols;
    scope->symbols = sym;
    return false;
}

JnSymbol* scope_lookup_current(JnScope* scope, const char* name)
{
    for (JnSymbol* s = scope->symbols; s != NULL; s = s->next)
    {
        if (strcmp(s->name, name) == 0)
            return s;
    }
    return NULL;
}
