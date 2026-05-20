#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>

#include "ast.h"
#include "helper.h"
#include "token.h"
#include "object.h"
#include "parser.h"


AST* ast_create(Arena* arena, AST_TYPE type)
{
    //AST* ast = malloc(sizeof(ast));
    AST* ast = arena_alloc(arena, sizeof(AST));
    ast->type = type;
    return ast;
}

AST* new_block(Arena* arena)
{
    u64 capacity = 8;
    AST* ast = ast_create(arena, AST_BLOCK);
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


AST* ast_literal(Arena* arena, Object* object)
{
    AST* ast = ast_create(arena, AST_LITERAL);
    ast->literal = object;
    return ast;
}

AST* ast_identifier(Arena* arena, const char* identifier)
{
    AST* ast = ast_create(arena, AST_IDENTIFIER);
    ast->identifier = identifier;
    return ast;
}

AST* ast_unary(Arena* arena, TokenType op, AST* right)
{
    AST* ast = ast_create(arena, AST_UNARY);
    ast->unary.op = op;
    ast->unary.right = right;
    return ast;
}

AST* ast_binary(Arena* arena, AST* lhs, TokenType op, AST* rhs)
{
    AST* ast = ast_create(arena, AST_BINARY);
    ast->binary.left = lhs;
    ast->binary.op = op;
    ast->binary.right = rhs;
    return ast;
}

AST* ast_println(Arena* arena, AST* out)
{
    AST* ast = ast_create(arena, AST_PRINTLN);
    ast->println.out = out;
    return ast;
}

AST* ast_array(Arena* arena)
{
    AST* ast = ast_create(arena, AST_ARRAY);
    ast->array.count = 0;
    ast->array.capacity = 13;
    ast->array.elements = arena_alloc(
        arena,
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

AST* ast_for(Arena* arena, const char* ident, AST* iter, AST* block)
{
    AST* ast = ast_create(arena, AST_FOR);
    ast->for_node.block = block;
    ast->for_node.ident = ident;
    ast->for_node.iter = iter;
    return ast;
}

AST* ast_assign(
    Arena* arena,
    char* name,
    bool is_const,
    AST* value
)
{
    AST* ast = ast_create(arena, AST_ASSIGN);
    ast->assign.name = strdup(name);
    ast->assign.value = value;
    ast->assign.is_const = is_const;
    return ast;
}

AST* ast_if_node(Arena* arena, AST* cond, AST* then, elseif* elseif, AST* else_node)
{
    AST* ast = ast_create(arena, AST_IF);
    ast->if_node.condition = cond;
    ast->if_node.then = then;
    ast->if_node.elseif = elseif;
    ast->if_node.else_node = else_node;
    return ast;
}

AST* ast_break(Arena* arena)
{
    AST* ast = ast_create(arena, AST_BREAK);
    return ast;
}

AST* ast_continue(Arena* arena)
{
    AST* ast = ast_create(arena, AST_CONTINUE);
    return ast;
}

AST* ast_fn(Arena* arena, AST* block, const char* name, param_t* params)
{
    AST* ast = ast_create(arena, AST_FUNCTION);
    ast->fn_node.name = name;
    ast->fn_node.params = params;
    ast->fn_node.block = block;
    return ast;
}

AST* ast_match(Arena* arena, AST* sub, case_t* cases, AST* def)
{
    AST* ast = ast_create(arena, AST_MATCH);
    ast->match_node.cases = cases;
    ast->match_node.subject = sub;
    ast->match_node.def = def;
    return ast;
}

AST* ast_struct(Arena* arena,const char* ident, attr_t* attr)
{
    AST* ast = ast_create(arena, AST_MATCH);
    ast->struct_node.attrs = attr;
    ast->struct_node.ident = ident;
    return ast;
}

AST* ast_class(Arena* arena, const char* ident, attr_t* attr, klass_t* base)
{
    AST* ast = ast_create(arena, AST_MATCH);
    ast->type = AST_CLASS;
    ast->class_node.ident = ident;
    ast->class_node.base = base;
    ast->class_node.attrs = attr;
    return ast;
}

AST* ast_member(Arena* arena, AST* obj, const char* field)
{
    AST* ast = ast_create(arena, AST_MEMBER);
    ast->member.obj = obj;
    ast->member.field = field;
    //default
    ast->member.is_call = false;
    ast->member.is_getter = false;
    ast->member.is_setter = false;
    return ast;
}


AST* ast_call(Arena* arena, const char* callee)
{
    AST* ast = ast_create(arena, AST_CALL);
    ast->call.callee = callee;
    ast->call.pos_count = 0;
    //TODO
    //ast->call.pos_args = arena_alloc(arena, sizeof(AST *) * 8);
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

AST* ast_error(Arena* arena, parser* p, const char* msg)
{
    AST* ast = ast_create(arena, AST_ERROR);
    ast->error_msg = msg;
    return ast;
}