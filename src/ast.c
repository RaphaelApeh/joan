#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>

#include "ast.h"
#include "helper.h"
#include "token.h"
#include "object.h"
#include "parse.h"


AST* ast_create(joan_parser_t* p, AST_TYPE type)
{
    assert(p != NULL);
    assert(p->arena != NULL);
    AST* ast = arena_alloc(p->arena, sizeof(AST));
    assert(ast != NULL);
    ast->type = type;
    ast->line = p->curr.line;
    ast->col = p->curr.column;
    return ast;
}

AST* new_block(joan_parser_t* p)
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


AST* ast_literal(joan_parser_t* p, JnObject* object)
{
    AST* ast = ast_create(p, AST_LITERAL);

    ast->literal = object;
    return ast;
}

AST* ast_identifier(joan_parser_t* p, const char* identifier)
{
    AST* ast = ast_create(p, AST_IDENTIFIER);

    ast->identifier = identifier;
    return ast;
}

AST* ast_unary(joan_parser_t* p, J_TokenType op, AST* right)
{
    AST* ast = ast_create(p, AST_UNARY);

    ast->unary.op = op;
    ast->unary.right = right;
    return ast;
}

AST* ast_binary(joan_parser_t* p, AST* lhs, J_TokenType op, AST* rhs)
{
    AST* ast = ast_create(p, AST_BINARY);

    ast->binary.left = lhs;
    ast->binary.op = op;
    ast->binary.right = rhs;
    return ast;
}

AST* ast_println(joan_parser_t* p, AST* out)
{
    AST* ast = ast_create(p, AST_PRINTLN);

    ast->println.out = out;
    return ast;
}

AST* ast_array(joan_parser_t* p)
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
    joan_parser_t* p,
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

AST* ast_if_node(joan_parser_t* p, AST* cond, AST* then, elseif* elseif, AST* else_node)
{
    AST* ast = ast_create(p, AST_IF);
    ast->if_node.condition = cond;
    ast->if_node.then = then;
    ast->if_node.elseif = elseif;
    ast->if_node.else_node = else_node;
    return ast;
}

AST* ast_break(joan_parser_t* p)
{
    AST* ast = ast_create(p, AST_BREAK);
    return ast;
}

AST* ast_continue(joan_parser_t* p)
{
    AST* ast = ast_create(p, AST_CONTINUE);
    return ast;
}

AST* ast_match(joan_parser_t* p, AST* sub, case_t* cases, AST* def)
{
    AST* ast = ast_create(p, AST_MATCH);
    ast->match_node.cases = cases;
    ast->match_node.subject = sub;
    ast->match_node.def = def;
    return ast;
}

AST* ast_struct(joan_parser_t* p,const char* ident, attr_t* attr)
{
    AST* ast = ast_create(p, AST_MATCH);
    ast->struct_node.attrs = attr;
    ast->struct_node.ident = ident;
    return ast;
}

AST* ast_class(joan_parser_t* p, const char* ident, attr_t* attr, klass_t* base)
{
    AST* ast = ast_create(p, AST_MATCH);
    ast->type = AST_CLASS;
    ast->class_node.ident = ident;
    ast->class_node.base = base;
    ast->class_node.attrs = attr;
    return ast;
}


AST* ast_call(joan_parser_t* p, AST* callee)
{
    AST* ast = ast_create(p, AST_CALL);
    ast->call.callee = callee;
    ast->call.pos_count = 0;
    //TODO
    //ast->call.pos_args = p_alloc(p, sizeof(AST *) * 8);
    //ast->call.params = param_init();
    return ast;
}

AST* ast_instance(const char* ident, param_t* param)
{
    AST* ast = malloc(sizeof(ast));
    ast->type = AST_INSTANCE;
    ast->instance_T.ident = ident;
    ast->instance_T.params = param;
    return ast;
}

AST* ast_error(joan_parser_t* p, const char* msg)
{
    AST* ast = ast_create(p, AST_ERROR);
    ast->error_msg = msg;
    return ast;
}