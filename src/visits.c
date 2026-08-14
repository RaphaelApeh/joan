#include <assert.h>
#include <Joan.h>
#include "semantic.h"


static void visit_return(JnSemantic* sem, Jn_Node* node)
{
    if (sem->fnc_depth == 0)
        error(sem, node, "return statement found outside of a function.");

    if (node->return_stmt.value)
        Jn_visit(sem, node->return_stmt.value);
}


static void visit_ident(JnSemantic* sem, Jn_Node* node)
{
    if (!symbol_lookup(sem, sem->scope, node->identifier))
    {
        error(sem, node, "undefine variable.");
    }
}

static void visit_reassign(JnSemantic* sem, Jn_Node* node)
{
    if (node->reassign.op == TOK_WALRUS || node->reassign.op == TOK_DCOLON)
        return;
    if (node->reassign.expr->type == AST_CALL)
    {
        error(sem, node, "WTF. reasigning a call expression.");
        return;
    }
    if (node->reassign.expr->type != AST_IDENTIFIER) return;

    bool sym = symbol_lookup(sem, sem->scope, node->reassign.expr->identifier);
    if (!sym)
    {
        error(sem, node, "undefined variable.");
        return;
    }
    // if (sym->is_const)
    // {
    //     error(sem, node, "cannot modify const.");
    //     return;
    // }
    Jn_visit(sem, node->reassign.value);
}

static void visit_for(JnSemantic* sem, Jn_Node* node)
{
    if (node->for_node.cond)
        Jn_visit(sem, node->for_node.cond);
    if (node->for_node.incr)
        Jn_visit(sem, node->for_node.incr);
    if (node->for_node.init)
        Jn_visit(sem, node->for_node.init);
    
    sem->loop_depth++;
    Jn_visit(sem, node->for_node.block);
    sem->loop_depth--;
}

static void visit_var(JnSemantic* sem, Jn_Node* node)
{
	Jn_visit(sem, node->assign.value);

    bool ret = symbol_insert(sem, sem->scope, node->assign.name, SYMBOL_VAR, node->assign.is_const);        
    if (ret) error(sem, node, "variable already declared.");
}

static void visit_fn(JnSemantic* sem, Jn_Node* node)
{
    scope_insert(sem->scope, node->fn_node.name, SYMBOL_FN, true);
    JnScope* old = sem->scope;
    sem->scope = scope_new(old);
    sem->fnc_depth++;
    for (int i = 0; i < node->fn_node.count; ++i)
    {
        scope_insert(sem->scope, node->fn_node.params[i], SYMBOL_VAR, false);
    }
    Jn_visit(sem, node->fn_node.block);
    sem->fnc_depth--;
    JnScope* child = sem->scope;
    sem->scope = old;
    scope_free(child);
}

static void visit_block(JnSemantic* sem, Jn_Node* node)
{
    JnScope* old = sem->scope;
    sem->scope = scope_new(old);

    for (size_t i = 0; i < node->block.count; ++i)
        Jn_visit(sem, node->block.statements[i]);

    JnScope* child = sem->scope;
    sem->scope = old;
    scope_free(child);
}

static void visit_binary(JnSemantic* sem, Jn_Node* node)
{
    Jn_visit(sem, node->binary.left);
    Jn_visit(sem, node->binary.right);
}

static void visit_break(JnSemantic* sem, Jn_Node* node)
{
    if (sem->loop_depth == 0)
    {
        error(sem, node, "Found a break statement outside of a loop.");
    }
}

static void visit_continue(JnSemantic* sem, Jn_Node* node)
{
    if (sem->loop_depth == 0)
    {
        error(sem, node, "Found a continue statement outside of a loop.");
    }
}

static void visit_while(JnSemantic* sem, Jn_Node* node)
{
    Jn_visit(sem, node->while_node.cond);
    sem->loop_depth++;
    Jn_visit(sem, node->while_node.block);
    sem->loop_depth--;
}

static void visit_lambda(JnSemantic* sem, Jn_Node* node)
{
    sem->fnc_depth++;
    assert(false && "TODO");
}

void Jn_visit(JnSemantic* sem, Jn_Node* node)
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
	visit_break(sem, node); break;
    // case AST_FOR:
    //     visit_for(sem, node); break;
    case AST_BLOCK:
        visit_block(sem, node); break;
    // case AST_FUNCTION:
    //     visit_fn(sem, node); break;
    case AST_IDENTIFIER:
        visit_ident(sem, node); break;
    case AST_ASSIGN:
        visit_var(sem, node); break;
    case AST_REASSIGN:
        visit_reassign(sem, node); break;
    case AST_BINARY:
        visit_binary(sem, node); break;
    case AST_WHILE:
	visit_while(sem, node); break;
    case AST_ERROR:
        error(sem, node, node->error_msg); break;
    default:
        break;
    }
}
