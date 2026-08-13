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
#define GET_TOK_TYPE(p) (p)->curr.type

#define SKIP(p, t, msg, ...) do {\
    if (!check(p, t))          \
        return parse_error(p, msg, ##__VA_ARGS__);\
    match(p, t);                    \
} while(false)

// lower case
#define skip SKIP


static AST* parse_unary(JnParser* p, J_TokenType op);
static AST* parse_block(JnParser* p);

int check(JnParser* p, J_TokenType type)
{
    return _check(p, type, false);
}

static bool match(JnParser* p, J_TokenType type)
{
    if (p->curr.type != type) return false;
    next_parser(p);
    return true;
}

static bool expect(JnParser* p, J_TokenType token, const char* msg, ...)
{
    if (check(p, token)) return true;
    char buffer[256];
    va_list arg; va_start(arg, msg);
    vsnprintf(buffer, sizeof(buffer), msg, arg);
    va_end(arg);
    return false;
}

JN_INLINE JnToken peek_parser(JnParser* p)
{
    return p->next;
}

JN_INLINE JnToken previous(JnParser* p)
{
    return p->prev;
}

JN_INLINE void consume(JnParser* p, J_TokenType token)
{
    if (!check(p, token)) 
    {
        JN_LOG("consume() does not match token.");
        return;
    }
    next_parser(p);
}

static char* get_lexeme(JnParser* p)
{
    char* lex = GET_LEX(p);
    next_parser(p);
    return lex;
}

static bool is_assign_token(J_TokenType type)
{
    switch (type)
    {
        case TOKEN_APLUS:
        case TOKEN_AMINUS:
        case TOKEN_AMUL:
        case TOKEN_EQUAL:
        case TOKEN_ASLASH:
        case TOKEN_APERCENTAGE:
        case TOKEN_ARSHIFT:
        case TOKEN_ALSHIFT:
        case TOKEN_AXOR:
        case TOKEN_ABITAND:
        case TOKEN_ABITOR:
        case TOKEN_SETTER:
        case TOKEN_WALRUS:
            return true;
        default:
            return false;
    }
}


static bool is_stmt_end(JnParser* p)
{
    switch  (GET_TOK_TYPE(p))
    {
        case TOKEN_NEWLINE:
        case TOKEN_SEMICOLON:
        case TOKEN_EOF:
        case TOKEN_COMMA:
        case TOKEN_RPARN:
        case TOKEN_RBRACE:
        case TOKEN_RBRACKET:
            return true;
        default:
            return false;
    }
}

AST* parse_stmt_check(JnParser* p, AST* stmt)
{
    if (stmt->type == AST_COMMENT) // TODO
        return stmt;
    if (stmt->type == AST_ERROR) return stmt;
    if (
        ( p->prev.type !=TOKEN_RBRACE && p->curr.type != TOKEN_EOF) && !match(p, TOKEN_SEMICOLON)
        && !p->has_newl
    )
    {
        return parse_error(p, "Expected semicolon after a statement."); // TODO
    }
    return stmt;
}


void advance_parser(JnParser* p)
{
    p->prev =  p->curr;
    p->curr = p->next;
    p->next = next_token(p->l);
}

JN_INLINE bool check_next(JnParser* p, J_TokenType type)
{
    return peek_parser(p).type == type;
}

JN_INLINE char* consume_token(JnParser* p, J_TokenType token)
{
    if (!check(p, token)) return NULL;
    return get_lexeme(p);
}

static char* consume_ident(JnParser* p)
{
    return consume_token(p, TOKEN_IDENTIFIER);
}

static char* consume_string(JnParser* p)
{
    return consume_token(p, TOKEN_STRING);
}

static AST* parse__body(JnParser* p)
{
    if (check(p, TOKEN_LBRACE))
        return parse_block(p);
    else if (match(p, TOKEN_THEN))
    {
        return parse_expr(p);
    }
    return parse_error(p, "No expression block found.");
}

void jn_init_parser(JnParser* p, joan_lexer_t* l)
{
    assert(p != NULL && l != NULL);
    p->l = l;
    p->next = next_token(l);
    next_parser(p);
}

void J_parse_file(JnParser* p, char* restrict filecontent)
{
    joan_lexer_t l;
    J_init_lexer(&l, filecontent, "main");
    p->l = &l;
    p->next = next_token(&l);
}

precedence get_prec(J_TokenType type)
{
    switch (type)
    {
    case TOKEN_EQEQ:
    case TOKEN_IS:
    case TOKEN_IS_NOT:
    case TOKEN_NEQ:
        return PREC_EQ;
    
    case TOKEN_GT:
    case TOKEN_LT:
    case TOKEN_GTE:
    case TOKEN_LTE:
    case TOKEN_IN:
    case TOKEN_NOT_IN:
        return PREC_COMP;
    
    case TOKEN_MUL:
    case TOKEN_SLASH:
    case TOKEN_PERCENTAGE:
        return PREC_FACTOR;
    
    case TOKEN_RSHIFT:
    case TOKEN_LSHIFT:
        return PREC_SHIFT;
    
    case TOKEN_BITAND:
        return PREC_BITAND;
    case TOKEN_POW:
        return PREC_POWER;
    
    case TOKEN_BITOR:
        return PREC_BITOR;
    case TOKEN_XOR:
        return PREC_BITXOR;
    
    case TOKEN_PLUS:
    case TOKEN_MINUS:
        return PREC_TERM;
    
    case TOKEN_AND:
        return PREC_AND;
    
    case TOKEN_OR:
        return PREC_OR;
        
    default:
        return PREC_NONE;
    }
}

static AST* parse_block(JnParser* p)
{
    consume(p, TOKEN_LBRACE);
    AST* block = new_block(p);
    while (!check(p, TOKEN_RBRACE) && !check(p, TOKEN_EOF))
    {
        add_block(block, parse_stmt_check(p, parse_stmt(p)));
    }
    match(p, TOKEN_RBRACE);
    return block;
}


JnToken next_parser(JnParser* p)
{
    p->prev = p->curr;
    p->curr = p->next;
    p->has_newl = false;
    do {
        p->next = next_token(p->l);
        if (p->next.type == TOKEN_NEWLINE)
            p->has_newl = true;
    }  while(p->next.type == TOKEN_NEWLINE);
    return p->curr;
}

AST* parse_error(JnParser* p, const char* msg, ...)
{
    char buffer[256];
    va_list args;
    va_start(args, msg);
    vsnprintf(buffer, sizeof(buffer), msg, args);
    va_end(args);
    AST* err = ast_error(p, strdup(buffer));
    next_parser(p);
    return err;
}

static AST* parse_loop(JnParser* p)
{
    consume(p, TOKEN_LOOP);
    AST* ast = ast_create(p, AST_LOOP);
    if (check(p, TOKEN_LBRACE))
        ast->loop_stmt.block = parse_block(p);
    else {
        return parse_error(p, "Expected loop block.");
    }
    return ast;
}

static AST* parse_range(JnParser* p, AST* node)
{
    consume(p, TOKEN_RANGE);
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

static AST* parse_while(JnParser* p)
{
    consume(p, TOKEN_WHILE);
    AST* cond = parse_expr(p);
    AST* block = NULL;
    block = parse_block(p);
    return ast_while(p, cond, block);
}

static AST* parse_if(JnParser* p)
{
    consume(p, TOKEN_IF);
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

static AST* parse_match(JnParser* p)
{
    consume(p, TOKEN_MATCH);
    AST* stmt = parse_expr(p);
    AST* else_stmt = NULL;
    if (!match(p, TOKEN_LBRACE))
        return parse_error(p, "Expected '{'"); // TODO
    case_t* caseObj = init_case(p->arena);
    while (!match(p, TOKEN_RBRACE))
    {
        if (match(p, TOKEN_ELSE))
        {
            match(p, TOKEN_DCOLON);
            if (match(p, TOKEN_THEN))
                else_stmt = parse_expr(p);
            else if (check(p, TOKEN_LBRACE))
                else_stmt = parse_block(p);
        } else 
        {
            AST* sub = parse_expr(p);
            AST* block = NULL;
            if (!match(p, TOKEN_DCOLON))
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

static AST* parse_fn(JnParser* p)
{
    consume(p, TOKEN_FN);
    if (!check(p, TOKEN_IDENTIFIER))
        return parse_error(p, "Expected an identifier.");
    char* ident = GET_LEX(p);
    next_parser(p); // ident
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
        next_parser(p);
        if (match(p, TOKEN_COMMA))
            continue;
    }
    if (!check(p, TOKEN_LBRACE))
    {
        AST* ast = ast_function(p, ident, NULL, len, params);
        ast->fn_node.is_defined = false;
        return ast;
    }
    if (check(p, TOKEN_LBRACE))
        block = parse_block(p);
    else
        return parse_error(p, "Invalid function body.");
    AST* ast = ast_function(p, ident, block, len, params);
    ast->fn_node.is_async = false;
    ast->fn_node.is_yield = false;
    ast->fn_node.is_defined = true;
    return ast;
}

static AST* parse_assign(JnParser* p)
{
    char* ident;
    bool is_const = false;
    AST* type = NULL;
    if (match(p, TOKEN_CONST))
        is_const = true;
    if (!is_const && !match(p, TOKEN_LET))
        return parse_error(p, "Expected the 'let' keyword");
    if (check(p, TOKEN_IDENTIFIER))
    {
        ident = p->curr.lexeme;
        next_parser(p);
    }else
        return parse_error(p, "Expected an identifier.");
    
    if (match(p, TOKEN_COLON))
    {
        type = parse_primary(p);
    }
    
    if (!match(p, TOKEN_EQUAL))
        return parse_error(p, "Expected an '=' operator but got '%s'.", p->curr.lexeme);
    
    AST* ast = ast_assign(p, ident, is_const,  parse_expr(p));
    ast->assign.type = type;
    return ast;
}

static AST* parse_reassign(JnParser* p, AST* node)
{
    J_TokenType op = p->curr.type;
    AST* ast = ast_create(p, AST_REASSIGN);
    next_parser(p); // += reassign operator
    //x += 4;
    ast->reassign.expr = node;
    ast->reassign.op = op;
    ast->reassign.value = parse_expr(p);
    return ast;
}

static AST* parse_call(JnParser* p, AST* callee)
{
    //e.g main(1, None, true)
    AST** args = arena_alloc(p->arena, sizeof(AST *) * 20);
    size_t len = 0, cap = 20;
    AST* ast = ast_create(p, AST_CALL);
    if (match(p, TOKEN_RPARN))
    {
        ast->call.callee = callee;
        ast->call.pos_args = args;
        ast->call.pos_count = 0;
        return ast;
    }
    do {
        if (len >= cap)
        {
            cap *= 2;
            args = realloc(args, sizeof(AST*) * cap);
        }
        args[len++] = parse_expr(p);
    } while (match(p, TOKEN_COMMA));

    if (!match(p, TOKEN_RPARN))
        return parse_error(p, "Invalid syntax expected ')'.");

    ast->call.callee = callee;
    ast->call.pos_args = args;
    ast->call.pos_count = len;
    return ast;
}

static AST* parse_for_each(JnParser* p)
{
    /*
    Example:
        #for x in arr{
            printf("Hello World")
        }
        #for i, x in arr{
            printf("Hello World")
        }
    */
    next_parser(p); // for

    if (!check(p, TOKEN_IDENTIFIER))
        return parse_error(p, "Expected an identifier.");
    
    char* index = get_lexeme(p);
    char* var = NULL;
    if (match(p, TOKEN_COMMA))
    {
        var = get_lexeme(p);
    }
    SKIP(p, TOKEN_IN, "Expected an 'in' token.");

    AST* iter = parse_expr(p);
    AST* block = NULL;
    if (match(p, TOKEN_THEN)) // TODO: parse_then(p);
        block = parse_expr(p);
    else if (match(p, TOKEN_LBRACE))
        block = parse_block(p);
    else
        return parse_error(p, "Expected a block body.");
    
    AST* ast = ast_create(p, AST_FOR_EACH);
    ast->foreach_node.block = block;
    if (NULL == var)
    {
        ast->foreach_node.ident = index;
        ast->foreach_node.index = NULL;
    }else {
        ast->foreach_node.ident = var;
        ast->foreach_node.index = index;
    }
    return ast;
}

static AST* parse_for(JnParser* p)
{
    /*
    for loop;
    for i := 0; i < 10; i += 1 {} or then
    */

    next_parser(p); // for
    AST* init = NULL, *cond = NULL, *incr = NULL;
    AST* block = NULL;
    if (check(p, TOKEN_LPARN))
        return parse_error(p, "Sorry, does not support parentheses like C.");
    
    if (!check(p, TOKEN_SEMICOLON))
    {
        if (check(p, TOKEN_LET) || check(p, TOKEN_CONST))
            init = parse_assign(p);
        else
            init = parse_expr(p);
    }
    SKIP(p, TOKEN_SEMICOLON, "you forgot to add ';' in the forloop.");
    if (!check(p, TOKEN_SEMICOLON))
        cond = parse_expr(p);
    SKIP(p, TOKEN_SEMICOLON, "Yes, you need to add ';' after the loop condition.");
    if (!check(p, TOKEN_LBRACE) && !check(p, TOKEN_THEN))
        incr = parse_expr(p);

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

static AST* parse_instance(JnParser* p, AST* instance_obj);

static AST* parse_member(JnParser* p, AST* obj)
{
    // obj.field or obj.field()
    J_TokenType tok = p->curr.type;
    next_parser(p);

    if (check(p, TOKEN_LBRACE))
        return parse_instance(p, obj);

    char* ident = consume_ident(p);
    if (NULL == ident)
        return parse_error(p, "Expected identifier but got (%s).", GET_LEX(p));
    AST* field = ast_identifier(p, ident);
    AST* ast = ast_create(p, AST_MEMBER);
    ast->member.callie = obj;
    ast->member.field = field;
    ast->member.tok = tok;
    ast->member.setter = NULL;
    return ast;
}

static AST* parse_enum(JnParser* p)
{
    next_parser(p); // enum
    if (!check(p, TOKEN_IDENTIFIER))
        return parse_error(p, "Expected an identifier but got '%s'.", GET_LEX(p));
    
    char* ident = GET_LEX(p);
    next_parser(p);
    match(p, TOKEN_DCOLON);
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
        next_parser(p);
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
static AST* parse_inline_if(JnParser* p, AST* node)
{
    next_parser(p); // if
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

static AST* parse_index(JnParser* p, AST* arr)
{
    next_parser(p); //[
    AST* ast = ast_create(p, AST_ARRAY_INDEX);
    ast->index.array = arr;
    ast->index.pos = parse_expr(p);
    ast->index.is_set = false;
    ast->index.value = NULL;
    if (!match(p, TOKEN_RBRACKET))
        return parse_error(p, "Expected an closing ']'");
    return ast;
}


static bool allow_instance(JnParser* p, AST* node)
{
    bool ret = false;
    switch (node->type)
    {
        case AST_IDENTIFIER:
        case AST_MEMBER:
        case AST_CALL:
            ret = true;
            break;
        default:
            ret = false;
    }
    if (ret && check(p, TOKEN_DOT))
    {
        next_parser(p);
    }
    return ret;
}

static AST* parse_postfix(JnParser* p, AST* left)
{
    if (check(p, TOKEN_PLUS_PLUS))
        return parse_unary(p, TOKEN_PLUS_PLUS);
    while (true)
    {
        if (match(p, TOKEN_LPARN))
        {
            left = parse_call(p, left);
            continue;
        }

        if (check(p, TOKEN_DOT))
        {
            left =  parse_member(p, left);
            continue;
        }

        if (check(p, TOKEN_IF))
        {
            return parse_inline_if(p, left);
        }

        if (check(p, TOKEN_RANGE))
        {
            left = parse_range(p, left);
            continue;
        }
    
        if (check(p, TOKEN_LBRACKET))
        {
            left = parse_index(p, left);
            continue;
        }
        break;
    }
    if (is_assign_token(GET_TOK(p).type))
        return parse_reassign(p, left);
    return left;
}

static AST* parse_hashmap(JnParser* p)
{
    skip(p, TOKEN_LBRACE, "Expected an opening '{'");
    AST** keys = arena_alloc(p->arena, sizeof(AST *) * 30);
    AST** values = arena_alloc(p->arena, sizeof(AST *) * 30);
    size_t len= 0, cap = 30;    
    AST* ast = ast_create(p, AST_HASHMAP);

    while (true)
    {
        if (match(p, TOKEN_RBRACE))
            break;
        if (len == 0 && match(p, TOKEN_NONE) && match(p, TOKEN_RBRACE))
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
    match(p, TOKEN_SEMICOLON); // TODO
    ast->hmp_node.keys = keys;
    ast->hmp_node.values = values;
    ast->hmp_node.count = len;
    return ast;
}


AST* parse_array(JnParser* p)
{
    SKIP(p, TOKEN_LBRACKET, "Expected an opening '['.");
    AST* arr = ast_array(p);
    if (match(p, TOKEN_RBRACKET)) return arr;

    for (;;)
    {

        if (check(p, TOKEN_COMMA))
        {
            return parse_error(p, "Expected an expression before ','.");
        }
        
        ast_array_add(arr, parse_expr(p));

        if (!match(p, TOKEN_COMMA)) break;

        if (check(p, TOKEN_RBRACKET))   break;
    }
    SKIP(p, TOKEN_RBRACKET, "Expected a closing ']'.");
    return arr;
}

static AST* parse_lambda(JnParser* p)
{
    /*
    Inline function
    Example:
        add :=  |a, b| => a + b;
        no_arg := |None| => something;
        block := |a, b| => {
            return "block statement";
        }
        add(32, 12);
    */
    consume(p, TOKEN_BITOR);
    int len = 0, cap = 20;
    char** args = malloc(sizeof(char *) * cap);
    do {
        if (match(p, TOKEN_BITOR))  break;
        if (len == 0 && match(p, TOKEN_NONE))
        {
            if (!check(p, TOKEN_BITOR))
                return parse_error(p, "Expected a closing '|'.");
            next_parser(p);
            break;
        }
        if (!check(p, TOKEN_IDENTIFIER)) return parse_error(p, "lambda expect an identifer but got %s.", GET_LEX(p));
        if (len >= cap)
        {
            cap *= 2;
            args = realloc(args, sizeof(char *) * cap);
        }
        args[len++] = GET_LEX(p);
        next_parser(p);
        if (match(p, TOKEN_COMMA)) continue;
        if (match(p, TOKEN_BITOR)) break;
    } while(true);
    args[len] = NULL;
    match(p, TOKEN_DCOLON);
    AST* expr = NULL;
    
    if (check(p, TOKEN_LBRACE))
    {
        expr = parse_block(p);
    } else {    
        expr = parse_expr(p);
    }
    AST* ast = ast_create(p, AST_LAMBDA);
    ast->lambda_node.count = len;
    ast->lambda_node.expr = expr;
    ast->lambda_node.args = args;
    return ast;
}

static AST* parse_import(JnParser* p)
{
    /*
    Example:
        import "conf" // import everything
        OR
        import "conf"{version} // import only version
    */
    next_parser(p);
    char* import_path;
    char** fields = arena_alloc(p->arena, sizeof(char *) * 100);
    int len = 0, cap = 100;
    if (!check(p, TOKEN_STRING))
        return parse_error(p, "Expected an import path.");
    import_path = get_lexeme(p);
    if (match(p, TOKEN_LBRACE))
    {
        while (true)
        {
            if (!check(p, TOKEN_IDENTIFIER)) return parse_error(p, "Expected an identifier.");
            if (len > cap)
            {
                cap *= 2;
                fields = arena_realloc(
                    p->arena, 
                    fields, 
                    sizeof(char *) * len, 
                    sizeof(char *) * cap
                );
            }
            fields[len++] = get_lexeme(p);
            if (match(p, TOKEN_COMMA)) continue;
            if (match(p, TOKEN_RBRACE)) break;
            return parse_error(p, "Expected ',' or '}' from expression.");
        }
    }
    fields[len] = NULL;
    AST* ast = ast_create(p, AST_IMPORT);
    ast->import_node.lib = import_path;
    ast->import_node.fields = fields;
    ast->import_node.count = len;
    return ast;
}

static AST* parse_struct(JnParser* p)
{
    /*
    Example:
        Point := struct {
            x: int, y: float
        }
        Person := struct {
            name, age
        }
    PS: both do the same thing. No types
    */

    consume(p, TOKEN_STRUCT);
    char** fields = arena_alloc(p->arena, sizeof(char *) * 10);
    int len = 0, cap = 10;
    SKIP(p, TOKEN_LBRACE, "To initalize a struct you need '{'.");
    do {
        if (check(p, TOKEN_RBRACE))
            break;
        if (len == 0 && match(p, TOKEN_NONE))
            break;
        if (!check(p, TOKEN_IDENTIFIER))
            return parse_error(p, "Expected an identifier but (got '%s').", GET_LEX(p));
        if (len >= cap)
        {
            cap *= 2;
            fields = realloc(fields, sizeof(AST *) * cap);
        }
        fields[len++] = get_lexeme(p);
        if (match(p, TOKEN_COLON))
            parse_expr(p); // For readablity type does nothing.            

    } while (match(p, TOKEN_COMMA) || match(p, TOKEN_SEMICOLON));
    SKIP(p, TOKEN_RBRACE, "Expected an closing '}'");
    fields[len] = NULL;
    AST* ast = ast_create(p, AST_STRUCT);
    ast->struct_node.fields = fields;
    ast->struct_node.ident = NULL;
    ast->struct_node.count = len;
    return ast;
}

static AST* parse_instance(JnParser* p, AST* instance_obj)
{
    SKIP(p, TOKEN_LBRACE, "To initalize a instance you need '{'.");
    char** fields = arena_alloc(p->arena, sizeof(char *) * 10);
    AST** values = arena_alloc(p->arena, sizeof(AST *) * 10);
    int len = 0, cap = 10;
    bool contains_kwargs = false;
    for (;;)
    {
        if (match(p, TOKEN_RBRACE)) break;
        if (len >= cap)
        {
            cap *= 2;
            fields = realloc(fields, sizeof(char *) * cap);
            values = realloc(values, sizeof(AST *) * cap);
        }
        if (match(p, TOKEN_DOT)){
            contains_kwargs = true;
            fields[len] = get_lexeme(p);
            if (!match(p, TOKEN_EQUAL)) return parse_error(p, "expected '='");
            values[len] = parse_expr(p);
        } else {
            if (contains_kwargs)
                return parse_error(p, "Found a positional argument after a keyword argument.");
            fields[len] = NULL;
            values[len] = parse_expr(p);
        }
        len++;
        if (match(p, TOKEN_COMMA)) continue;
        if (match(p, TOKEN_RBRACE)) break;
        return parse_error(p, "Expected a closing '}' or ','.");
    }

    AST* ast = ast_create(p, AST_INSTANCE);
    ast->instance_node.fields = fields;
    ast->instance_node.values = values;
    ast->instance_node.count = len;
    ast->instance_node.object = instance_obj;
    return ast;
}


static AST* parse_multi_var(JnParser* p, AST* first)
{

    /*
     Example:
    a, b, c := 3
    OR (later)
    a, b := [1, 2]
    a // 1
    b // 2
    */
    next_parser(p);

    char** idents = arena_alloc(p->arena, sizeof(char *) * 100);
    int len = 0, cap = 100;
    idents[len++] = (char *)first->identifier;
    while (true)
    {
        if (check(p, TOKEN_WALRUS) || check(p, TOKEN_SETTER))
        break;
        if (!check(p, TOKEN_IDENTIFIER))
            return parse_error(p, "Expected an identifier.");
        if (len > cap)
        {
            cap *= 2;
            // TODO
            idents = arena_realloc(p->arena, idents, sizeof(idents), sizeof(AST *) * cap);
        }
        idents[len++] =  GET_LEX(p);
        next_parser(p);
        
        if (match(p, TOKEN_COMMA))
            continue;
        
        if (check(p, TOKEN_WALRUS) || check(p, TOKEN_SETTER))
            break;
        return parse_error(p, "Got an invalid token.");
    }
    int op = GET_TOK(p).type;
    next_parser(p);
    AST* expr = parse_expr(p);
    // TODO: a, b := [1, 2]
    AST* ast = ast_create(p, AST_MULTI_VAR);
    ast->assign_multiple.count = len;
    ast->assign_multiple.idents = idents;
    ast->assign_multiple.value = expr;
    ast->assign_multiple.op = op;
    return ast;
}

static AST* parse_tuple(JnParser* p)
{
    consume(p, TOKEN_LPARN);
    AST* node = ast_create(p, AST_TUPLE);
    size_t len = 0, cap = 100;
    if (match(p, TOKEN_RPARN))
    {
        node->tuple.count = 0;
        node->tuple.elements = NULL;
        return node;
    }
    AST* first = parse_expr(p);
    if (!match(p, TOKEN_COMMA))
    {
        SKIP(p, TOKEN_RPARN, "Expected ')'.");
        return first;
    }
    AST** items = arena_alloc(p->arena, sizeof(AST *) * cap);
    items[len++] = first;
    do {
        if (check(p, TOKEN_RPARN)) break;
        if (len > cap)
        {
            cap *= 2;
            items = arena_realloc(p->arena, items, sizeof(AST *) * len, sizeof(AST *) * cap);
        }
        items[len++] = parse_expr(p);
    } while (match(p, TOKEN_COMMA));
    
    SKIP(p, TOKEN_RPARN, "Expected ')'.");
    items[len] = NULL;
    node->tuple.elements = items;
    node->tuple.count = len;
    return node;
}


static AST* parse_unary(JnParser* p, J_TokenType op)
{
    consume(p, op);
    AST* ast = ast_create(p, AST_UNARY);
    ast->unary.op = op;
    ast->unary.right = parse_expr(p); 
    return ast;
}

static AST* parse_string(JnParser* p)
{
    size_t len = strlen(GET_LEX(p)), cap = len + 1;
    char* buff = malloc(sizeof(char) * cap);
    memcpy(buff, GET_LEX(p), cap);
    next_parser(p);

    while (check(p, TOKEN_STRING))
    {
        char* next = GET_LEX(p);
        size_t next_len = strlen(next);
        if (len + next_len + 1 > cap)
        {
            cap = (len + next_len + 1) * 2;
            buff = realloc(buff, cap);
        }
        memcpy(buff + len, next, next_len + 1);
        len += next_len;
        next_parser(p);
    }
    AST* ast = ast_literal(p,  jn_obj_string(p->state, buff));
    free(buff);
    return ast;   
}

static AST* parse_literal(JnParser* p, JnObject* obj)
{
    next_parser(p);
    return ast_literal(p, obj);
}

AST* parse_primary(JnParser* p)
{
    JnToken t = p->curr;
    AST* ast = NULL;
    char* msg = t.lexeme;
    switch (t.type)
    {
        case TOKEN_INT:
            long i = t.i;
            JnObject* v =  jn_obj_int(p->state, i);
            return parse_literal(p, v);
        case TOKEN_LPARN:
            return parse_tuple(p);
        case TOKEN_COMMENT:
            next_parser(p);
            ast = ast_create(p, AST_COMMENT);
            ast->comment = strdup(t.lexeme);
            return ast;
        case TOKEN_FLOAT:
            double d = t.d;
            return parse_literal(p, jn_obj_float(p->state, d));
        case TOKEN_HASH:
            next_parser(p);
            if (check(p, TOKEN_LBRACE))
                return parse_hashmap(p);
            else if (check(p, TOKEN_FOR))
                return parse_for_each(p);
            return parse_error(p, "Error invalid expression");
        case TOKEN_STRING:
            return parse_string(p);
        case TOKEN_CHAR:
            char c = t.c;
            return parse_literal(p, JN_RETURN_CHAR(p->state, c));
        case TOKEN_PLUS_PLUS:
            return parse_unary(p, TOKEN_PLUS_PLUS);
        case TOKEN_RETURN:
            ast = ast_create(p, AST_RETURN);
            next_parser(p);
            if (is_stmt_end(p) || p->has_newl)
            {
                match(p, TOKEN_SEMICOLON);
                ast->return_stmt.value = NULL;
                return ast;
            }
            ast->return_stmt.value = parse_expr(p);
            return ast;
        case TOKEN_IDENTIFIER:
            ast = ast_identifier(p, t.lexeme);
            next_parser(p);
            return ast;
        case TOKEN_CONTINUE:
            next_parser(p);
            return ast_continue(p);
        case TOKEN_BREAK:
            next_parser(p);
            return ast_break(p);
        case TOKEN_NEWLINE:
            next_parser(p);
            return parse_primary(p);
        case TOKEN_MINUS:
            return parse_unary(p, TOKEN_MINUS);
        case TOKEN_TILDE:
            return parse_unary(p, TOKEN_TILDE);
        case TOKEN_NOT:
            return parse_unary(p, TOKEN_NOT);
        case TOKEN_TRUE:
            return parse_literal(p, JN_RETURN_TRUE(p->state));
        case TOKEN_NONE:
            return parse_literal(p, JN_RETURN_NONE);
        case TOKEN_FALSE:
            return parse_literal(p, JN_RETURN_FALSE(p->state));
        case TOKEN_LBRACKET:
            return parse_array(p);
        case TOKEN_BITOR:
            return parse_lambda(p);
        case TOKEN_IMPORT:
            return parse_import(p);
        case TOKEN_PRINTLN: {
            next_parser(p);
            AST* out = NULL;
            if (!check(p, TOKEN_NEWLINE))
                out = parse_expr(p);
            return ast_println(p, out);
        }
        case TOKEN_STRUCT:
            return parse_struct(p);
        case TOKEN_ERROR: {
            return parse_error(p, msg);
        }
        default:
            return parse_error(p, "Error: Got an invalid expression '%s'.", msg);
    }
}

AST* parse_prec(JnParser* p, precedence prec)
{
    AST* left = parse_primary(p);
    while (true)
    {
        left = parse_postfix(p, left);
        precedence next_pr;
        J_TokenType op;
        bool comp = false;
        if (p->curr.type == TOKEN_IN && p->next.type == TOKEN_NOT)
            return parse_error(p, "Invalid operator order: did you mean 'not in'?.");
        if (p->curr.type == TOKEN_NOT && p->next.type == TOKEN_IS)
            return parse_error(p, "Invalid operator order: did you mean 'is not'?.");
        if (p->curr.type == TOKEN_IS && p->next.type == TOKEN_NOT)
        {
            op = TOKEN_IS_NOT;
            next_pr = PREC_EQ;
            comp = true;
        } else if (p->curr.type == TOKEN_NOT && p->next.type == TOKEN_IN)
        {
            op = TOKEN_NOT_IN;
            next_pr = PREC_COMP;
            comp = true;
        } else {
            op = p->curr.type;
            next_pr = get_prec(op);
        }
        if (prec >= next_pr)
            break;
        if (comp)
        {
            next_parser(p);
            next_parser(p);
        } else {
            next_parser(p);
        }
        AST* right = parse_prec(p, op == TOKEN_POW ? next_pr - 1 : next_pr);
        left = ast_binary(p, left, op, right);
    }
    return  left;
}
AST* parse_expr(JnParser* p)
{
    // MAIN FUNCTION
    return parse_prec(p, PREC_NONE);
}


AST* parse_stmt(JnParser* p)
{
    AST* stmt;
    switch (GET_TOK_TYPE(p))
    {
        case TOKEN_CONST:
            stmt = parse_assign(p);
            break;
        case TOKEN_LET:
            stmt = parse_assign(p);
            break;
        case TOKEN_LBRACE:
            stmt = parse_block(p);
            break;
        case TOKEN_ENUM:
            stmt = parse_enum(p);
            break;
        case TOKEN_MATCH:
            stmt = parse_match(p);
            break;
        case TOKEN_IF:
            stmt = parse_if(p);
            break;
        case TOKEN_FOR:
            stmt = parse_for(p);
            break;
        case TOKEN_FN:
            stmt = parse_fn(p);
            break;
        case TOKEN_LOOP:
            stmt = parse_loop(p);
            break;
        case TOKEN_WHILE:
            stmt = parse_while(p);
            break;
        default:
            stmt = parse_expr(p);
            break;
    }
    return stmt;
}