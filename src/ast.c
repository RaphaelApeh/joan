#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>

#include "ast.h"
#include "helper.h"
#include "token.h"
#include "object.h"
#include "parse.h"


AST* ast_create(JnParser* p, AST_TYPE type)
{
    assert(p != NULL);
    assert(p->arena != NULL);
    AST* ast = arena_alloc(p->arena, sizeof(AST));
    assert(ast != NULL);
    ast->type = type;
    ast->state = p->state;
    ast->line = p->curr.line;
    ast->col = p->curr.column;
    ast->filename = (char *)p->l->filename;
    return ast;
}

AST* new_block(JnParser* p)
{
    u64 capacity = 8;
    AST* ast = ast_create(p, AST_BLOCK);

    ast->block.count = 0;
    ast->block.capacity = capacity;
    ast->block.statements = malloc(sizeof(AST *) * capacity);
    return ast;
}

void add_block(AST* ast, AST* node)
{
    if (ast->block.count >= ast->block.capacity)
    {
        ast->block.capacity *= 2;
        ast->block.statements = realloc(ast->block.statements,
            sizeof(AST *) * ast->block.capacity);
    }
    ast->block.statements[ast->block.count++] = node;
}


AST* ast_literal(JnParser* p, JnObject* object)
{
    AST* ast = ast_create(p, AST_LITERAL);

    ast->literal = object;
    return ast;
}

AST* ast_decl(JnParser* p, const char* ident, AST* value, bool is_const)
{
    
}

AST* ast_while(JnParser* p, AST* cond, AST* block)
{
    AST* ast = ast_create(p, AST_WHILE);
    ast->while_node.cond = cond;
    ast->while_node.block = block;
    return ast;
}

AST* ast_identifier(JnParser* p, const char* identifier)
{
    AST* ast = ast_create(p, AST_IDENTIFIER);

    ast->identifier = identifier;
    return ast;
}

AST* ast_unary(JnParser* p, J_TokenType op, AST* right)
{
    AST* ast = ast_create(p, AST_UNARY);

    ast->unary.op = op;
    ast->unary.right = right;
    return ast;
}

AST* ast_binary(JnParser* p, AST* lhs, J_TokenType op, AST* rhs)
{
    AST* ast = ast_create(p, AST_BINARY);

    ast->binary.left = lhs;
    ast->binary.op = op;
    ast->binary.right = rhs;
    return ast;
}

AST* ast_println(JnParser* p, AST* out)
{
    AST* ast = ast_create(p, AST_PRINTLN);

    ast->println.out = out;
    return ast;
}

AST* ast_array(JnParser* p)
{
    AST* ast = ast_create(p, AST_ARRAY);

    ast->array.count = 0;
    ast->array.capacity = 13;
    ast->array.elements = arena_alloc(
        p->arena,
        sizeof(AST *) * ast->array.capacity
    );
    return ast;
}

void ast_array_add(AST* arr, AST* element)
{
    assert(arr->type == AST_ARRAY);
    if (arr->array.count >= arr->array.capacity)
    {
        arr->array.capacity *= 2;
        arr->array.elements = realloc(
            arr->array.elements, sizeof(AST *) * arr->array.capacity
        );
    }
    arr->array.elements[arr->array.count++] = element;
}


AST* ast_assign(
    JnParser* p,
    char* name,
    bool is_const,
    AST* value
)
{
    AST* ast = ast_create(p, AST_ASSIGN);
    ast->assign.name = strdup(name);
    ast->assign.value = value;
    ast->assign.is_const = is_const;
    return ast;
}

AST* ast_if_node(JnParser* p, AST* cond, AST* then, elseif* elseif, AST* else_node)
{
    AST* ast = ast_create(p, AST_IF);
    ast->if_node.condition = cond;
    ast->if_node.then = then;
    ast->if_node.elseif = elseif;
    ast->if_node.else_node = else_node;
    return ast;
}

AST* ast_break(JnParser* p)
{
    AST* ast = ast_create(p, AST_BREAK);
    return ast;
}

AST* ast_continue(JnParser* p)
{
    AST* ast = ast_create(p, AST_CONTINUE);
    return ast;
}

AST* ast_match(JnParser* p, AST* sub, case_t* cases, AST* def)
{
    AST* ast = ast_create(p, AST_MATCH);
    ast->match_node.cases = cases;
    ast->match_node.subject = sub;
    ast->match_node.def = def;
    return ast;
}

AST* ast_call(JnParser* p, AST* callee, AST** args, size_t count)
{
    AST* ast = ast_create(p, AST_CALL);
    ast->call.callee = callee;
    ast->call.pos_count = count;
    ast->call.pos_args = args;
    return ast;
}


AST* ast_function(JnParser* p, char* ident, AST* block, int count, char** params)
{
    AST* ast = ast_create(p, AST_FUNCTION);
    ast->fn_node.block = block;
    ast->fn_node.count = count;
    ast->fn_node.params = params;
    ast->fn_node.name = ident;
    return ast;
}

AST* ast_error(JnParser* p, const char* msg)
{
    AST* ast = ast_create(p, AST_ERROR);
    ast->error_msg = msg;
    return ast;
}