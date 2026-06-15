#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <stdarg.h>

#include "parser.h"
#include "object.h"
#include "helper.h"
#include "lexer.h"
#include "token.h"
#include "ast.h"

#define _check(p, t, n)((n) ? (p)->next.type == (type) : (p)->curr.type == (type))

#define GET_LEX(p)((p)->curr.lexeme)
#define GET_TOK(p) (p)->curr

int check(joan_parser_t* p, J_TokenType type)
{
    return _check(p, type, false);
}

static bool match(joan_parser_t* p, J_TokenType type)
{
    if (p->curr.type != type) return false;
    advance_parser_c(p);
    return true;
}

static bool is_assign_token(J_TokenType type)
{
    switch (type)
    {
        case TOKEN_APLUS:
        case TOKEN_AMINUS:
        case TOKEN_ASTAR: // TODO: TOKEN_AMUL
        case TOKEN_EQUAL:
        case TOKEN_ASLASH:
        case TOKEN_APERCENTAGE:
        case TOKEN_ARSHIFT:
        case TOKEN_ALSHIFT:
        case TOKEN_ABITAC:
        case TOKEN_ABITAND:
        case TOKEN_ABITOR:
        case TOKEN_WALRUS:
            return true;
        default:
            return false;
    }
}


void advance_parser(joan_parser_t* p)
{
    p->curr = p->next;
    p->next = next_token(p->l);
}

int check_next(joan_parser_t* p, J_TokenType type)
{
    return _check(p, type, true);
}


void jn_init_parser(joan_parser_t* p, joan_lexer_t* l)
{
    assert(p != NULL && l != NULL);
    p->l = l;
    p->next = clean_token(l);
    advance_parser_c(p);
}

void J_parse_file(joan_parser_t* p, char* restrict filecontent)
{
    joan_lexer_t l;
    J_init_lexer(&l, filecontent);
    p->l = &l;
    p->next = clean_token(&l);
}

precedence get_prec(J_TokenType type)
{
    switch (type)
    {
    case TOKEN_EQEQ:
    case TOKEN_NEQ:
        return PREC_EQ;
    
    case TOKEN_GT:
    case TOKEN_LT:
    case TOKEN_GTE:
    case TOKEN_LTE:
        return PREC_COMP;
    
    case TOKEN_STAR:
    case TOKEN_SLASH:
    case TOKEN_PLUS:
    case TOKEN_RSHIFT:
    case TOKEN_LSHIFT:
    case TOKEN_PERCENTAGE:
    case TOKEN_BITAND:
    case TOKEN_BITOR:
    case TOKEN_BITAC:
    case TOKEN_MINUS:
        return PREC_TERM;
    
    case TOKEN_AND:
        return PREC_AND;
    
    case TOKEN_NOT_IN:
    case TOKEN_IN:
        return PREC_IN;

    case TOKEN_OR:
        return PREC_OR;
    
    case TOKEN_IS:
        return PREC_PRIMARY;
    
    default:
        return PREC_NONE;
    }
}

AST* parse_block(joan_parser_t* p)
{
    advance_parser_c(p);
    AST* block = new_block(p);
    while (!match(p, TOKEN_RBRACE))
        add_block(block, parse_stmt(p));
    return block;
}


void advance_parser_c(joan_parser_t* p)
{
    p->curr = p->next;
    while (true)
    {
        p->next = next_token(p->l);
        if (p->next.type != TOKEN_NEWLINE || p->next.type != TOKEN_SIMICOLON);
            break;
    }
}

AST* parse_error(joan_parser_t* p, const char* msg, ...)
{
    va_list args;
    va_start(args, msg);
    //TODO: code  
    va_end(args);
    return ast_error(p, msg);
}

static AST* parse_loop(joan_parser_t* p)
{
    advance_parser_c(p); // loop
    AST* ast = ast_create(p, AST_LOOP);
    if (check(p, TOKEN_LBRACE))
        ast->loop_stmt.block = parse_block(p);
    return ast;
}

static AST* parse_range(joan_parser_t* p, AST* node)
{
    advance_parser_c(p); // ...
    int op = 0; // None
    if (match(p, TOKEN_EQUAL))
        op = TOKEN_EQUAL;
    else if (match(p, TOKEN_LT))
        op = TOKEN_LT;
    AST* ast = ast_create(p, AST_RANGE);
    ast->range_node.start = node;
    ast->range_node.stop = parse_expr(p);
    ast->range_node.op = op;
    return ast;
}

static AST* parse_while(joan_parser_t* p)
{
    advance_parser_c(p); // while
    AST* cond = parse_expr(p);
    AST* block = NULL;
    if (check(p, TOKEN_LBRACE))
        block = parse_block(p);
    else if (match(p, TOKEN_THEN))
        block = parse_expr(p);
    else
        return parse_error(p, "Expected a while block.");
    AST* ast = ast_create(p, AST_WHILE);
    ast->while_node.cond = cond;
    ast->while_node.block = block;
    return ast;
}

static AST* parse_if(joan_parser_t* p)
{
    advance_parser_c(p); // if token
    AST* cond = parse_expr(p);
    AST* block = NULL;
    if (check(p, TOKEN_LBRACE))
        block = parse_block(p);
    else if (match(p, TOKEN_THEN))
        block = parse_expr(p);
    else return parse_error(p, "Got an invalid if statement.");
    
    AST* elsenode = NULL;
    elseif* elseifs = elseif_init();
    while(match(p, TOKEN_ELSEIF))
    {
        match(p, TOKEN_IF);
        AST* cond = parse_expr(p);
        AST* childblock;
        if (check(p, TOKEN_LBRACE))
        {
            childblock = parse_block(p);
        } else if (match(p, TOKEN_THEN))
        {
            childblock = parse_expr(p);
        }
        else{
            return parse_error(p, "Error: Expected an 'then' or '{'.");
        }
        elseif_add(elseifs, childblock, cond);
    }
    if (match(p, TOKEN_ELSE))
    {
        if (check(p, TOKEN_LBRACE))
            elsenode = parse_block(p);
        else elsenode = parse_expr(p);
    }
    return ast_if_node(
        p,
        cond,
        block,
        elseifs,
        elsenode
    );
}

static AST* parse_match(joan_parser_t* p)
{
    advance_parser_c(p); // match
    AST* stmt = parse_expr(p);
    AST* else_stmt = NULL;
    if (!match(p, TOKEN_LBRACE))
        return parse_error(p, "Expected '{'"); // TODO
    case_t* caseObj = init_case(p->arena);
    while (!match(p, TOKEN_RBRACE))
    {
        if (match(p, TOKEN_ELSE))
        {
            match(p, TOKEN_EXR);
            if (match(p, TOKEN_THEN))
                else_stmt = parse_expr(p);
            else if (check(p, TOKEN_LBRACE))
                else_stmt = parse_block(p);
        } else 
        {
            AST* sub = parse_expr(p);
            AST* block = NULL;
            if (!match(p, TOKEN_EXR))
                return parse_error(p, "Expected '=>'");
            if (match(p, TOKEN_THEN))
                block = parse_expr(p);
            else if (check(p, TOKEN_LBRACE))
                block = parse_block(p);
            push_case(caseObj, sub, block);
        }
    }
    AST* ast = ast_create(p, AST_MATCH);
    ast->match_node.def = else_stmt;
    ast->match_node.cases = caseObj;
    ast->match_node.subject = stmt;
    return ast;
}

static AST* parse_fn(joan_parser_t* p)
{
    advance_parser_c(p); // fn
    if (!check(p, TOKEN_IDENTIFIER))
        return parse_error(p, "Expected an identifier.");
    char* ident = GET_LEX(p);
    advance_parser_c(p); // ident
    char** params = malloc(sizeof(char *) * 100);
    int len = 0;
    AST* block = NULL;
    if (!match(p, TOKEN_LPARN))
        return parse_error(p, "Expected an '(");
    while (!match(p, TOKEN_RPARN))
    {
        if (!check(p, TOKEN_IDENTIFIER))
            return parse_error(p, "Expected an identifer.");
        params[len++] = GET_LEX(p);
        advance_parser_c(p);
        if (match(p, TOKEN_COMMA))
            continue;
    }
    if (match(p, TOKEN_THEN))
        block = parse_expr(p);
    else if (check(p, TOKEN_LBRACE))
        block = parse_block(p);
    else
        return parse_error(p, "No function body.");
    AST* ast = ast_create(p, AST_FUNCTION);
    ast->fn_node.block = block;
    ast->fn_node.name = ident;
    ast->fn_node.params = params;
    ast->fn_node.count = len;
    ast->fn_node.is_async = false;
    ast->fn_node.is_yield = false;
    return ast;
}

static AST* parse_assign(joan_parser_t* p)
{
    char* ident;
    bool is_const = false;
    if (match(p, TOKEN_CONST))
        is_const = true;
    if (!is_const && !match(p, TOKEN_LET))
        return parse_error(p, "Expected the 'let' keyword");
    if (check(p, TOKEN_IDENTIFIER))
    {
        ident = p->curr.lexeme;
        advance_parser_c(p);
    }else
        return parse_error(p, "Expected an identifier.");
    if (!match(p, TOKEN_EQUAL))
        return parse_error(p, "Expected an '=' operator but got '%s'.", p->curr.lexeme);
    return ast_assign(p, ident, is_const,  parse_expr(p));
}

static AST* parse_reassign(joan_parser_t* p, AST* node)
{
    if (node->type != AST_IDENTIFIER)
        return parse_error(p, "Expected an reassign operator.");
    const char* ident = node->identifier;
    J_TokenType op = p->curr.type;
    AST* ast = ast_create(p, AST_REASSIGN);
    advance_parser_c(p); // += reassign operator
    //x += 4;
    ast->reassign.ident = (char *)ident;
    ast->reassign.op = op;
    ast->reassign.value = parse_expr(p);
    return ast;
}

static AST* parse_call(joan_parser_t* p, AST* callee)
{
    //e.g main(1, None, true)
    //TODO:
    if (callee->type != AST_IDENTIFIER)
        return parse_error(p, "Expected an identifier.");
    advance_parser_c(p); // (
    AST* ast = ast_call(p, callee);
    AST* args[20]  = {0}; // TODO
    size_t len = 0;
    while(!match(p, TOKEN_RPARN))
    {
        args[len++] = parse_expr(p);
        if (match(p, TOKEN_COMMA))
            continue;
        if (match(p, TOKEN_RPARN))
            break;
    }
    ast->call.pos_args = args;
    ast->call.pos_count = len;
    return ast;
}

static AST* parse_for(joan_parser_t* p)
{
   advance_parser_c(p); // for
   if (!check(p, TOKEN_IDENTIFIER))
        return parse_error(p, "Expected an identifier.");
    char* idx = GET_LEX(p);
    char* ident = NULL;
    advance_parser_c(p);
    if (match(p, TOKEN_COMMA))
    {
        if (!check(p, TOKEN_IDENTIFIER))
            return parse_error(p, "Expected an identifier.");
        ident = GET_LEX(p);
        advance_parser_c(p);
    }

    if (!match(p, TOKEN_IN))
        return parse_error(p, "Expected an 'in' operator.");
    
    AST* iter = parse_expr(p);
    AST* block = NULL;
    if (match(p, TOKEN_THEN))
        block = parse_expr(p);
    else if (check(p, TOKEN_LBRACE))
        block = parse_block(p);
    else
        return parse_error(p, "forloop expected a block");
    assert(block != NULL);
    AST* ast = ast_create(p, AST_FOR);
    ast->for_node.iter = iter;
    ast->for_node.block = block;
    // idea:
    // for i, x in [1, 2, 3, 4, 5] -> i = index, x is the value
    // for i in [1, 2, 3, 4, 5] -> i = value
    if (NULL == ident)
    {
        ast->for_node.ident = idx;
        ast->for_node.index = NULL;
    } else {
        ast->for_node.ident = ident;
        ast->for_node.index = idx;
    }
    return ast;
}

static AST* parse_member(joan_parser_t* p, AST* obj)
{
    // ::<field>, .<field>
    bool is_setter, is_getter = false;
    AST* setter = NULL;
    J_TokenType tok = p->curr.type;
    advance_parser_c(p);
    if (!check(p, TOKEN_IDENTIFIER))
        return parse_error(p, "Expected an identifier but got '%s'.", GET_LEX(p));
    char* field = GET_LEX(p);
    advance_parser_c(p);
    if (match(p, TOKEN_EQUAL))
    {
        is_setter = true;
        setter = parse_expr(p);
    }
    AST* ast = ast_create(p, AST_MEMBER);
    ast->member.callie = obj;
    ast->member.field = field;
    ast->member.is_call = false;
    ast->member.setter = setter;
    ast->member.is_setter = is_setter;
    ast->member.tok = tok;
    return ast;
}

static AST* parse_enum(joan_parser_t* p)
{
    advance_parser_c(p); // enum
    if (!check(p, TOKEN_IDENTIFIER))
        return parse_error(p, "Expected an identifier but got '%s'.", GET_LEX(p));
    
    char* ident = GET_LEX(p);
    advance_parser_c(p);
    match(p, TOKEN_EXR);
    if (!match(p, TOKEN_LBRACE))
        return parse_error(p, "Expected an '{' but got '%s'.", GET_LEX(p));
    
    int len = 0;
    size_t capacity = 100;
    char** fields = malloc(sizeof(char *) * capacity);
    while (!match(p, TOKEN_RBRACE))
    {
        if (!check(p, TOKEN_IDENTIFIER))
            return parse_error(p, "Expected an identifier.");
        if (len >= capacity)
        {
            capacity *= 2;
            fields = realloc(fields, sizeof(char *) * capacity);
        }
        fields[len++] = GET_LEX(p);
        advance_parser_c(p);
        if (match(p, TOKEN_COMMA))
            continue;
    }
    fields[len] = NULL;
    AST* ast = ast_create(p, AST_ENUM);
    ast->enum_stmt.ident = ident;
    ast->enum_stmt.fields = fields;
    ast->enum_stmt.count = len;
    return ast; 
}
static AST* parse_inline_if(joan_parser_t* p, AST* node)
{
    advance_parser_c(p); // if
    AST* cond = parse_expr(p);
    if (!match(p, TOKEN_ELSE))
        return parse_error(p, "Expected an 'else' clause.");
    AST* elsenode = parse_expr(p);
    AST* ast = ast_create(p, AST_INLINE_IF);
    ast->inline_if_stmt.cond = cond;
    ast->inline_if_stmt.then = node;
    ast->inline_if_stmt.otherwise = elsenode;
    return ast;
}

static AST* parse_index(joan_parser_t* p, AST* arr)
{
    advance_parser_c(p); //[
    AST* ast = ast_create(p, AST_ARRAY_INDEX);
    ast->index.array = arr;
    ast->index.pos = parse_expr(p);
    ast->index.is_set = false;
    ast->index.value = NULL;
    if (!match(p, TOKEN_RBRACKET))
        return parse_error(p, "Expected an closing ']'");
    if (match(p, TOKEN_EQUAL))
    {
        ast->index.value = parse_expr(p);
        ast->index.is_set = true;
        return ast;
    }
    return ast;
}
static AST* parse_postfix(joan_parser_t* p, AST* left)
{

    while (true)
    {
        if (check(p, TOKEN_LPARN))
        {
            left = parse_call(p, left);
            continue;
        }

        if (match(p, TOKEN_AT) && check(p, TOKEN_IF))
        {
            return parse_inline_if(p, left);
        }

        if (check(p, TOKEN_RANGE))
            return parse_range(p, left);

        //TODO
        if (check(p, TOKEN_SETTER) || check(p, TOKEN_DOT))
        {
            return parse_member(p, left);
        }
        if (is_assign_token(GET_TOK(p).type))
            return parse_reassign(p, left);
    
        if (check(p, TOKEN_LBRACKET))
        {
            left = parse_index(p, left);
            if (left->type == AST_ARRAY_INDEX && left->index.is_set)
                return left; // arr[0] = 2 return
            continue; // arr[0] continue -> e.g arr[0](arg1, arg2) arr[0].method
        }
        break;
    }
    return left;
}

static AST* parse_hashmap(joan_parser_t* p)
{
    // TODO
    if (!match(p, TOKEN_LBRACE)) // {
        return parse_error(p, "Expected an opening '{'");
    AST* keys[256]; // TODO
    AST* values[256]; // TODO
    size_t len = 0;    
    AST* ast = ast_create(p, AST_HASHMAP);

    while (true)
    {
        if (match(p, TOKEN_RBRACE))
            break;
        keys[len] = parse_expr(p);
        if (!match(p, TOKEN_COLON))
            return parse_error(p, "Expected an ':'");
        values[len] = parse_expr(p);
        len++;
        if (match(p, TOKEN_COMMA))
            continue;
        if (match(p, TOKEN_RBRACE))
            break;
        return parse_error(p, "Something went wrong");
    }
    ast->hmp_node.keys = keys;
    ast->hmp_node.values = values;
    ast->hmp_node.count = len;
    return ast;
}
AST* parse_array(joan_parser_t* p)
{
    advance_parser_c(p); // [
    AST* arr = ast_array(p);
    while (true)
    {
        if (check(p, TOKEN_RBRACKET))
        {
            advance_parser_c(p);
            break;
        }
        ast_array_add(arr, parse_expr(p));
        if (check(p, TOKEN_COMMA))
        {
            advance_parser_c(p);
            continue;
        }
        if (check(p, TOKEN_RBRACKET))
        {
            advance_parser_c(p);
            break;
        }
        return parse_error(p, "<something went wrong>");
    }
    return arr;
}

static AST* parse_assert(joan_parser_t* p)
{
    advance_parser_c(p); // assert
    AST* cond = parse_expr(p);
    char* msg = NULL;
    if (match(p, TOKEN_COMMA))
    {
        if (!check(p, TOKEN_STRING)) return parse_error(p, "Expected 'string' but got %s.", GET_LEX(p));
        msg = GET_LEX(p);
        advance_parser_c(p);
    }
    AST* ast = ast_create(p, AST_ASSERT);
    ast->assert_stmt.cond = cond;
    ast->assert_stmt.msg = msg;
    return ast;
}

static AST* parse_lambda(joan_parser_t* p)
{
    /*
    Inline function
    Example:
        add :=  |a, b| => a + b
        add(32, 12)
    */
    advance_parser_c(p);
    int len = 0, cap = 20;
    char** args = malloc(sizeof(char *) * cap);
    do {
        if (match(p, TOKEN_BITOR))  break;
        if (!check(p, TOKEN_IDENTIFIER)) return parse_error(p, "lambda expect an identifer but got %s.", GET_LEX(p));
        if (len >= cap)
        {
            cap *= 2;
            args = realloc(args, sizeof(char *) * cap);
        }
        args[len++] = GET_LEX(p);
        advance_parser_c(p);
        if (match(p, TOKEN_COMMA)) continue;
        if (match(p, TOKEN_BITOR)) break;
    } while(true);
    args[len] = NULL;
    match(p, TOKEN_EXR);
    AST* expr = parse_expr(p);
    AST* ast = ast_create(p, AST_LAMBDA);
    ast->lambda_node.count = len;
    ast->lambda_node.expr = expr;
    ast->lambda_node.args = args;
    return ast;
}

AST* parse_value(joan_parser_t* p)
{
    joan_token_t t = p->curr;
    AST* ast = NULL;
    switch (t.type)
    {
        case TOKEN_INT:
            int i = *((int *) t.v);
             JnObject* v =  jn_obj_int(i);
            advance_parser_c(p);
            return ast_literal(p, v);
        case TOKEN_LPARN:
            advance_parser_c(p);
            ast = parse_expr(p);
            if (check(p, TOKEN_RPARN))
                advance_parser_c(p);
            return ast;
        case TOKEN_COMMENT:
            advance_parser_c(p);
            ast = ast_create(p, AST_COMMENT);
            ast->comment = strdup(t.lexeme);
            return ast;
        case TOKEN_FLOAT:
            double d = *((double *) t.v);
            advance_parser_c(p);
            return ast_literal(p,  jn_obj_float(d));
        case TOKEN_HASH:
            advance_parser_c(p);
            if (check(p, TOKEN_LBRACE))
                return parse_hashmap(p);
        case TOKEN_STRING:
            ast = ast_literal(p,  jn_obj_string(t.lexeme));
            advance_parser_c(p);
            return ast;
        case TOKEN_CHAR:
            int c;
            if ((c = strlen(t.lexeme)) > 1 ||
                c < 1    
            )
                return parse_error(p, "invalid char literal.");
            ast = ast_literal(
                p,
                JN_RETURN_CHAR(*t.lexeme) 
            );
            advance_parser_c(p);
            return ast;
        case TOKEN_RETURN:
            advance_parser_c(p);
            ast = ast_create(p, AST_RETURN);
            ast->return_stmt.value = parse_expr(p);
            return ast;
        case TOKEN_IDENTIFIER:
            ast = ast_identifier(p, t.lexeme);
            advance_parser_c(p);
            return ast;
        case TOKEN_ASSERT:
            return parse_assert(p);
        case TOKEN_CONTINUE:
            advance_parser_c(p);
            return ast_continue(p);
        case TOKEN_BREAK:
            advance_parser_c(p);
            return ast_break(p);
        case TOKEN_NEWLINE:
            advance_parser_c(p);
            return parse_value(p);
        case TOKEN_MINUS:
            advance_parser(p);
            ast = ast_create(p, AST_UNARY);
            ast->unary.op = TOKEN_MINUS;
            ast->unary.right = parse_value(p);
            return ast;
        case TOKEN_NOT:
            advance_parser(p);
            ast = ast_create(p, AST_UNARY);
            ast->unary.op = TOKEN_NOT;
            ast->unary.right = parse_value(p);
            return ast;
        case TOKEN_TRUE:
            ast = ast_literal(p,  jn_obj_bool(true));
            advance_parser_c(p);
            return ast;
        case TOKEN_NONE:
            ast = ast_literal(p,  jn_obj_none());
            advance_parser_c(p);
            return ast;
        case TOKEN_FALSE:
            ast = ast_literal(p,  jn_obj_bool(false));
            advance_parser_c(p);
            return ast;
        case TOKEN_LBRACKET:
            return parse_array(p);
        case TOKEN_BITOR:
            return parse_lambda(p);
        case TOKEN_PRINTLN: {
            advance_parser_c(p);
            AST* out = NULL;
            if (!check(p, TOKEN_NEWLINE))
                out = parse_expr(p);
            return ast_println(p, out);
        }
        case TOKEN_ERROR: {
            return parse_error(p, t.lexeme);
        }
        default:
            return parse_error(p, "Error: Got an invalid expression '%s'.", t.lexeme);
    }
}

AST* parse_prec(joan_parser_t* p, precedence prec)
{
    AST* left = parse_value(p);
    left = parse_postfix(p, left);
    while (prec < get_prec(p->curr.type))
    {
        J_TokenType op = p->curr.type;
        precedence op_prec = get_prec(op);
        advance_parser_c(p);
        AST* rhs = parse_prec(p, op_prec);
        left = ast_binary(p, left, op, rhs);
    }
    return  left;
}
AST* parse_expr(joan_parser_t* p)
{
    // MAIN FUNCTION
    return parse_prec(p, PREC_NONE);
}

AST* parse_stmt(joan_parser_t* p)
{
    switch (p->curr.type)
    {
        case TOKEN_CONST:
            return parse_assign(p);
        case TOKEN_LET:
            return parse_assign(p);
        case TOKEN_LBRACE:
            return parse_block(p);
        case TOKEN_ENUM:
            return parse_enum(p);
        case TOKEN_MATCH:
            return parse_match(p);
        case TOKEN_IF:
            return parse_if(p);
        case TOKEN_FOR:
            return parse_for(p);
        case TOKEN_FN:
            return parse_fn(p);
        case TOKEN_LOOP:
            return parse_loop(p);
        case TOKEN_WHILE:
            return parse_while(p);
        default:
            return parse_expr(p);
    }
}