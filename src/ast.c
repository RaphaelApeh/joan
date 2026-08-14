#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>

#include "ast.h"
#include "helper.h"
#include "token.h"
#include "object.h"
#include "parse.h"


Jn_Node* ast_create(JnParser* p, Jn_NodeType type)
{
    assert(p != NULL);
    assert(p->arena != NULL);
    Jn_Node* ast = arena_alloc(p->arena, sizeof(Jn_Node));
    assert(ast != NULL);
    ast->type = type;
    ast->state = p->state;
    ast->line = p->curr.line;
    ast->col = p->curr.column;
    ast->filename = (char *)p->l->filename;
    return ast;
}

Jn_Node* new_block(JnParser* p)
{
    u64 capacity = 8;
    Jn_Node* ast = ast_create(p, AST_BLOCK);

    ast->block.count = 0;
    ast->block.capacity = capacity;
    ast->block.statements = malloc(sizeof(Jn_Node *) * capacity);
    return ast;
}

void add_block(Jn_Node* ast, Jn_Node* node)
{
    if (ast->block.count >= ast->block.capacity)
    {
        ast->block.capacity *= 2;
        ast->block.statements = realloc(ast->block.statements,
            sizeof(Jn_Node *) * ast->block.capacity);
    }
    ast->block.statements[ast->block.count++] = node;
}


Jn_Node* ast_literal(JnParser* p, JnObject* object)
{
    Jn_Node* ast = ast_create(p, AST_LITERAL);

    ast->literal = object;
    return ast;
}

Jn_Node* ast_decl(JnParser* p, const char* ident, Jn_Node* value, bool is_const)
{
    
}

Jn_Node* ast_while(JnParser* p, Jn_Node* cond, Jn_Node* block)
{
    Jn_Node* ast = ast_create(p, AST_WHILE);
    ast->while_node.cond = cond;
    ast->while_node.block = block;
    return ast;
}

Jn_Node* ast_identifier(JnParser* p, const char* identifier)
{
    Jn_Node* ast = ast_create(p, AST_IDENTIFIER);

    ast->identifier = identifier;
    return ast;
}

Jn_Node* ast_unary(JnParser* p, JnTokenType op, Jn_Node* right)
{
    Jn_Node* ast = ast_create(p, AST_UNARY);

    ast->unary.op = op;
    ast->unary.right = right;
    return ast;
}

Jn_Node* ast_binary(JnParser* p, Jn_Node* lhs, JnTokenType op, Jn_Node* rhs)
{
    Jn_Node* ast = ast_create(p, AST_BINARY);

    ast->binary.left = lhs;
    ast->binary.op = op;
    ast->binary.right = rhs;
    return ast;
}


Jn_Node* ast_array(JnParser* p)
{
    Jn_Node* ast = ast_create(p, AST_ARRAY);

    ast->array.count = 0;
    ast->array.capacity = 13;
    ast->array.elements = arena_alloc(
        p->arena,
        sizeof(Jn_Node *) * ast->array.capacity
    );
    return ast;
}

void ast_array_add(Jn_Node* arr, Jn_Node* element)
{
    assert(arr->type == AST_ARRAY);
    if (arr->array.count >= arr->array.capacity)
    {
        arr->array.capacity *= 2;
        arr->array.elements = realloc(
            arr->array.elements, sizeof(Jn_Node *) * arr->array.capacity
        );
    }
    arr->array.elements[arr->array.count++] = element;
}

Jn_Node* ast_program(JnParser* p)
{
    return NULL;
}


Jn_Node* ast_assign(
    JnParser* p,
    char* name,
    bool is_const,
    Jn_Node* value
)
{
    Jn_Node* ast = ast_create(p, AST_ASSIGN);
    ast->assign.name = strdup(name);
    ast->assign.value = value;
    ast->assign.is_const = is_const;
    return ast;
}

Jn_Node* ast_if_node(JnParser* p, Jn_Node* cond, Jn_Node* then, elseif* elseif, Jn_Node* else_node)
{
    Jn_Node* ast = ast_create(p, AST_IF);
    ast->if_node.condition = cond;
    ast->if_node.then = then;
    ast->if_node.elseif = elseif;
    ast->if_node.else_node = else_node;
    return ast;
}

Jn_Node* ast_break(JnParser* p)
{
    Jn_Node* ast = ast_create(p, AST_BREAK);
    return ast;
}

Jn_Node* ast_continue(JnParser* p)
{
    Jn_Node* ast = ast_create(p, AST_CONTINUE);
    return ast;
}

Jn_Node* ast_match(JnParser* p, Jn_Node* sub, case_t* cases, Jn_Node* def)
{
    Jn_Node* ast = ast_create(p, AST_MATCH);
    ast->match_node.cases = cases;
    ast->match_node.subject = sub;
    ast->match_node.def = def;
    return ast;
}

Jn_Node* ast_call(JnParser* p, Jn_Node* callee, Jn_Node** args, size_t count)
{
    Jn_Node* ast = ast_create(p, AST_CALL);
    ast->call.callee = callee;
    ast->call.pos_count = count;
    ast->call.pos_args = args;
    return ast;
}


Jn_Node* ast_function(JnParser* p, char* ident, Jn_Node* block, int count, char** params)
{
    Jn_Node* ast = ast_create(p, AST_FUNCTION);
    ast->fn_node.block = block;
    ast->fn_node.count = count;
    ast->fn_node.params = params;
    ast->fn_node.name = ident;
    return ast;
}

Jn_Node* ast_error(JnParser* p, const char* msg)
{
    Jn_Node* ast = ast_create(p, AST_ERROR);
    ast->error_msg = msg;
    return ast;
}