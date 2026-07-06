#include <assert.h>
#include <Joan.h>
#include "semantic.h"

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
        fprintf(stdout, "(%d) semantice warning(s).\n", sem->warnings);
    }
}

void error(JnSemantic* sem, AST* node, const char* msg, ...)
{
    sem->errors++;
    sem->state->error.error_msg = msg;
    sem->state->error.line = node->line;
    sem->state->error.col = node->col;
    sem->state->error.type = SYNTAX_ERROR; // TODO
    sem->state->error.code = -1;
    sem->state->error.filename = node->filename;
}
void warning(JnSemantic* sem, AST* node, const char* msg, ...)
{
    sem->warnings++;
    sem->state->error.error_msg = msg;
    sem->state->error.line = node->line;
    sem->state->error.col = node->col;
    sem->state->error.type = SYNTAX_ERROR; // TODO
    sem->state->error.code = -1;
    sem->state->error.filename = node->filename;
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

JnSymbol* scope_lookup_current(JnScope* scope, const char* name)
{
    for (JnSymbol* s = scope->symbols; s != NULL; s = s->next)
        if (!strcmp(s->name, name))
            return s;
    return NULL;
}
