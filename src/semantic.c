#include <assert.h>
#include <Joan.h>
#include "semantic.h"

void semantic_init(JnSemantic* sem)
{
    assert(sem);
    sem->scope = scope_new(NULL);
    sem->fnc_depth = 0;
    sem->fnc_depth = 0;
    sem->errors = 0;
    sem->warnings = 0;
}


void semantic_check(JnSemantic* sem, AST* node)
{
    assert(false);
}

void error(JnSemantic* sem, AST* node, const char* msg, ...)
{
    sem->errors++;
}
void warning(JnSemantic* sem, AST* node, const char* msg, ...)
{
    sem->warnings++;
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

}

JnSymbol* scope_lookup(JnScope* scope, const char* name)
{

}

JnSymbol* scope_lookup_current(JnScope* scope, const char* name)
{

}
