#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <stdarg.h>

#include "parse.h"
#include "object.h"
#include "helper.h"
#include "lexer.h"
#include "token.h"
#include "ast.h"

#define _check(p, t, n)((n) ? (p)->next.type == (type) : (p)->curr.type == (type))

#define GET_LEX(p)((p)->curr.lexeme)
#define GET_TOK(p) (p)->curr

#define SKIP(p, t, msg, ...) do {\
    if (!check(p, t))          \
        return parse_error(p, msg);\
    match(p, t);                    \
} while(false)

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
    while (!check(p, TOKEN_RBRACE) && !check(p, TOKEN_EOF))
    {
        add_block(block, parse_stmt(p));
    }
    match(p, TOKEN_RBRACE);
    return block;
}


void advance_parser_c(joan_parser_t* p)
{
    p->curr = p->next;
    do {
        p->next = next_token(p->l);
    } while(p->next.type == TOKEN_NEWLINE || p->next.type == TOKEN_SIMICOLON);
}

AST* parse_error(joan_parser_t* p, const char* msg, ...)
{
    char buffer[256];
    va_list args;
    va_start(args, msg);
    vsnprintf(buffer, sizeof(buffer), msg, args);
    va_end(args);
    AST* err = ast_error(p, strdup(buffer));
    advance_parser_c(p);
    return err;
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
    advance_parser_c(p); // ..
    int op = 0; // None
    if (match(p, TOKEN_EQUAL))
        op = TOKEN_EQUAL;
    else if (match(p, TOKEN_LT))
        op = TOKEN_LT;
    AST* stop = parse_expr(p);
    AST* step = NULL;
    if (match(p, TOKEN_COLON))
        step = parse_expr(p);
    AST* ast = ast_create(p, AST_RANGE);
    ast->range_node.start = node;
    ast->range_node.stop = stop;
    ast->range_node.step = step;
    ast->range_node.has_step = step != NULL;
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
    AST* args[20]; // TODO
    size_t len = 0;
    while(true)
    {
        printf("TOKEN: %d\n", p->curr.type);
        args[len++] = parse_expr(p);
        if (match(p, TOKEN_COMMA))
            continue;
        if (match(p, TOKEN_RPARN)) break;
    }
    printf("END TOKEN: %d\n", p->curr.type);
    AST* ast = ast_create(p, AST_CALL);
    ast->call.callee = callee;
    ast->call.params = NULL;
    ast->call.pos_args = args;
    ast->call.pos_count = len;
    return ast;
}

static AST* parse_for(joan_parser_t* p)
{
    /*
    for loop;
    for i := 0; i < 10; i += 1 {} or then
    */
   advance_parser_c(p); // for
    AST* init = parse_expr(p);
    AST* cond = parse_expr(p);
    AST* incr = parse_expr(p);
    AST* block = NULL;
    if (match(p, TOKEN_THEN))
    {
        block = parse_expr(p);
    } else if (check(p, TOKEN_LBRACE))
    {
        block = parse_block(p);
    } else 
        return parse_error(p, "fooloop has no block.");
    
    AST* ast = ast_create(p, AST_FOR);
    ast->for_node.block = block;
    ast->for_node.cond = cond;
    ast->for_node.init = init;
    ast->for_node.incr = incr;
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
        if (match(p, TOKEN_LPARN))
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
    AST** keys = malloc(sizeof(AST *) * 30);
    AST** values = malloc(sizeof(AST *) * 30);
    size_t len= 0, cap = 30;    
    AST* ast = ast_create(p, AST_HASHMAP);

    while (true)
    {
        if (match(p, TOKEN_RBRACE))
            break;
        if (len > cap)
        {
            cap *= 2;
            keys = realloc(keys, sizeof(AST *) * cap);
            values = realloc(values, sizeof(AST *) * cap);
        }
        keys[len] = parse_expr(p);
        if (!match(p, TOKEN_COLON))
            return parse_error(p, "Expected an ':'");
        values[len] = parse_expr(p);
        len++;
        if (match(p, TOKEN_COMMA))
            continue;
        if (match(p, TOKEN_RBRACE))
            break;
        return parse_error(p, "expected a closing '}'.");
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
        return parse_error(p, "Expected a closing bracket '['.");
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
        no_arg := |None| => something
        add(32, 12)
    */
    advance_parser_c(p);
    int len = 0, cap = 20;
    char** args = malloc(sizeof(char *) * cap);
    do {
        if (match(p, TOKEN_BITOR))  break;
        if (len == 0 && match(p, TOKEN_NONE))
        {
            if (!check(p, TOKEN_BITOR))
                return parse_error(p, "Expected a closing '|'.");
            advance_parser_c(p);
            break;
        }
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

static AST* parse_import(joan_parser_t* p)
{
    advance_parser_c(p);
    if (!check(p, TOKEN_STRING)) return parse_error(p, "Expected an import path.");
    char* lib = GET_LEX(p);
    advance_parser_c(p);
    AST* ast = ast_create(p, AST_IMPORT);
    ast->import_node.lib = lib;
    ast->import_node.alias = NULL; // TODO
    return ast;
}

AST* parse_value(joan_parser_t* p)
{
    joan_token_t t = p->curr;
    AST* ast = NULL;
    char* msg = t.lexeme;
    switch (t.type)
    {
        case TOKEN_INT:
            long i = t.i;
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
            double d = t.d;
            advance_parser_c(p);
            return ast_literal(p,  jn_obj_float(d));
        case TOKEN_HASH:
            advance_parser_c(p);
            if (!check(p, TOKEN_LBRACE))
                return parse_error(p, "Expected '}'.");
            return parse_hashmap(p);
        case TOKEN_STRING:
            ast = ast_literal(p,  jn_obj_string(t.lexeme));
            advance_parser_c(p);
            return ast;
        case TOKEN_CHAR:
            char c = t.c;
            ast = ast_literal(
                p,
                JN_RETURN_CHAR(c) 
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
            return parse_error(p, msg);
        }
        default:
            return parse_error(p, "Error: Got an invalid expression '%s'.", msg);
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