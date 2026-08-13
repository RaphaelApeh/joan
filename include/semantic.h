#ifndef JN_SEMANTIC_H
#define JN_SEMANTIC_H

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "ast.h"


typedef struct JnSymbol JnSymbol;
typedef struct JnScope JnScope;
typedef struct JnSemantic JnSemantic;
typedef enum { SYMBOL_VAR, SYMBOL_CONST, SYMBOL_FN, SYMBOL_STRUCT} JnSymbolKind;

struct JnSymbol {
    Jn_Node* type;
    struct JnSymbol* next;
    char* name;
    JnSymbolKind kind;
    bool is_const;
};

struct JnScope {
    struct JnSymbol* symbols;
    struct JnScope* parent;
};

struct JnSemantic {
    J_State* state;
    JnScope* scope;
    int loop_depth, fnc_depth, errors, warnings;
};

// Visits
void Jn_visit(JnSemantic*, Jn_Node*);

// Semantic
void error(JnSemantic* sem, Jn_Node* node, const char* msg, ...);
void warning(JnSemantic* sem, Jn_Node* node, const char* msg, ...);
void Jn_semantic_init(J_State*, JnSemantic*);
void Jn_semantic_check(JnSemantic* sem, Jn_Node* node);


// TODO
bool symbol_lookup(JnSemantic* sem, JnScope* scope, const char* name);
bool symbol_insert(JnSemantic* sem, JnScope* scope, const char* name, int kind, bool is_const);


// Scope
JnScope* scope_new(JnScope* parent);
void scope_free(JnScope* scope);
bool scope_insert(JnScope* scope, const char* name, int Kind, bool is_const);
JnSymbol* scope_lookup(JnScope* scope, const char* name);
JnSymbol* scope_lookup_current(JnScope* scope, const char* name);
#endif // JN_SEMANTIC_H