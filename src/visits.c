#include <Joan.h>
#include "semantic.h"


static void visit_return(JnSemantic* sem, AST* node)
{
    if (sem->fnc_depth == 0)
        error(sem, node, "return statement found outside of a function.");

    if (node->return_stmt.value)
        visit(sem, node->return_stmt.value);
}

static void visit_var(JnSemantic* sem, AST* node){
	// TODO
}

static void visit_fn(JnSemantic* sem, AST* node)
{
    scope_insert(sem->scope, node->fn_node.name, SYMBOL_FN, true);
    JnScope* old = sem->scope;
    sem->scope = scope_new(old);
    sem->fnc_depth++;
    for (int i = 0; i < node->fn_node.count; ++i)
    {
        scope_insert(sem->scope, node->fn_node.params[i], SYMBOL_VAR, false);
    }
    visit(sem, node->fn_node.block);
    sem->fnc_depth--;
    JnScope* child = sem->scope;
    sem->scope = old;
    scope_free(child);
}

static void visit_binary(JnSemantic* sem, AST* node)
{
    Jn_visit(sem, node->binary.left);
    Jn_visit(sem, node->binary.right);
}

static void visit_break(JnSemantic* sem, AST* node)
{
    if (sem->loop_depth == 0)
    {
        error(sem, node, "Found a break statement outside of a loop.");
    }
}

static void visit_continue(JnSemantic* sem, AST* node)
{
    if (sem->loop_depth == 0)
    {
        error(sem, node, "Found a continue statement outside of a loop.");
    }
}

static void visit_while(JnSemantic* sem, AST* node)
{
    Jn_visit(sem, node->while_node.cond);
    sem->loop_depth++;
    Jn_visit(sem, node->while_node.block);
    sem->loop_depth--;
}



void Jn_visit(JnSemantic* sem, AST* node)
{
    if (NULL == node) return;
    switch (node->type)
    {
    case AST_RETURN:
        visit_return(sem, node);
        break;
    case AST_CONTINUE:
	visit_continue(sem, node); break;
    case AST_BREAK:
	visit_break(sem, node);
	break;
    case AST_WHILE:
	visit_while(sem, node); break;
    default:
        break;
    }
}
