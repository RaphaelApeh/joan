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


static Jn_Node* parse_unary(JnParser* p, JnTokenType op);
static Jn_Node* parse_block(JnParser* p);

int check(JnParser* p, JnTokenType type)
{
    return _check(p, type, false);
}

static bool match(JnParser* p, JnTokenType type)
{
    if (p->curr.type != type) return false;
    next_parser(p);
    return true;
}

static bool expect(JnParser* p, JnTokenType token, const char* msg, ...)
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

JN_INLINE void consume(JnParser* p, JnTokenType token)
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

static bool is_assign_token(JnTokenType type)
{
    switch (type)
    {
        case TOK_APLUS:
        case TOK_AMINUS:
        case TOK_AMUL:
        case TOK_EQUAL:
        case TOK_ASLASH:
        case TOK_APERCENTAGE:
        case TOK_ARSHIFT:
        case TOK_ALSHIFT:
        case TOK_AXOR:
        case TOK_ABITAND:
        case TOK_ABITOR:
        case TOK_SETTER:
        case TOK_WALRUS:
            return true;
        default:
            return false;
    }
}


static bool is_stmt_end(JnParser* p)
{
    switch  (GET_TOK_TYPE(p))
    {
        case TOK_NEWLINE:
        case TOK_SEMICOLON:
        case TOK_EOF:
        case TOK_COMMA:
        case TOK_RPARN:
        case TOK_RBRACE:
        case TOK_RBRACKET:
            return true;
        default:
            return false;
    }
}

Jn_Node* parse_stmt_check(JnParser* p, Jn_Node* stmt)
{
    if (stmt->type == AST_COMMENT) // TODO
        return stmt;
    if (stmt->type == AST_ERROR) return stmt;
    if (
        ( p->prev.type !=TOK_RBRACE && p->curr.type != TOK_EOF) && !match(p, TOK_SEMICOLON)
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

JN_INLINE bool check_next(JnParser* p, JnTokenType type)
{
    return peek_parser(p).type == type;
}

JN_INLINE char* consume_token(JnParser* p, JnTokenType token)
{
    if (!check(p, token)) return NULL;
    return get_lexeme(p);
}

static char* consume_ident(JnParser* p)
{
    return consume_token(p, TOK_IDENT);
}

static char* consume_string(JnParser* p)
{
    return consume_token(p, TOK_STRING);
}

static Jn_Node* parse__body(JnParser* p)
{
    if (check(p, TOK_LBRACE))
        return parse_block(p);
    else if (match(p, TOK_THEN))
    {
        return parse_expr(p);
    }
    return parse_error(p, "No expression block found.");
}

void jn_init_parser(JnParser* p, Jn_Lexer* l)
{
    assert(p != NULL && l != NULL);
    p->l = l;
    p->next = next_token(l);
    next_parser(p);
}

void J_parse_file(JnParser* p, char* restrict filecontent)
{
    Jn_Lexer l;
    J_init_lexer(&l, filecontent, "main");
    p->l = &l;
    p->next = next_token(&l);
}

precedence get_prec(JnTokenType type)
{
    switch (type)
    {
    case TOK_EQEQ:
    case TOK_IS:
    case TOK_IS_NOT:
    case TOK_NEQ:
        return PREC_EQ;
    
    case TOK_GT:
    case TOK_LT:
    case TOK_GTE:
    case TOK_LTE:
    case TOK_IN:
    case TOK_NOT_IN:
        return PREC_COMP;
    
    case TOK_MUL:
    case TOK_SLASH:
    case TOK_PERCENTAGE:
        return PREC_FACTOR;
    
    case TOK_RSHIFT:
    case TOK_LSHIFT:
        return PREC_SHIFT;
    
    case TOK_BITAND:
        return PREC_BITAND;
    case TOK_POW:
        return PREC_POWER;
    
    case TOK_BITOR:
        return PREC_BITOR;
    case TOK_XOR:
        return PREC_BITXOR;
    
    case TOK_PLUS:
    case TOK_MINUS:
        return PREC_TERM;
    
    case TOK_AND:
        return PREC_AND;
    
    case TOK_OR:
        return PREC_OR;
        
    default:
        return PREC_NONE;
    }
}

static Jn_Node* parse_block(JnParser* p)
{
    consume(p, TOK_LBRACE);
    Jn_Node* block = new_block(p);
    while (!check(p, TOK_RBRACE) && !check(p, TOK_EOF))
    {
        add_block(block, parse_stmt_check(p, parse_stmt(p)));
    }
    match(p, TOK_RBRACE);
    return block;
}


JnToken next_parser(JnParser* p)
{
    p->prev = p->curr;
    p->curr = p->next;
    p->has_newl = false;
    do {
        p->next = next_token(p->l);
        if (p->next.type == TOK_NEWLINE)
            p->has_newl = true;
    }  while(p->next.type == TOK_NEWLINE);
    return p->curr;
}

Jn_Node* parse_error(JnParser* p, const char* msg, ...)
{
    char buffer[256];
    va_list args;
    va_start(args, msg);
    vsnprintf(buffer, sizeof(buffer), msg, args);
    va_end(args);
    Jn_Node* err = ast_error(p, strdup(buffer));
    next_parser(p);
    return err;
}

static Jn_Node* parse_loop(JnParser* p)
{
    consume(p, TOK_LOOP);
    Jn_Node* ast = ast_create(p, AST_LOOP);
    if (check(p, TOK_LBRACE))
        ast->loop_stmt.block = parse_block(p);
    else {
        return parse_error(p, "Expected loop block.");
    }
    return ast;
}

static Jn_Node* parse_range(JnParser* p, Jn_Node* node)
{
    consume(p, TOK_RANGE);
    int op = 0; // None
    if (match(p, TOK_EQUAL))
        op = TOK_EQUAL;
    else if (match(p, TOK_LT))
        op = TOK_LT;
    Jn_Node* stop = parse_expr(p);
    Jn_Node* step = NULL;
    if (match(p, TOK_COLON))
        step = parse_expr(p);
    Jn_Node* ast = ast_create(p, AST_RANGE);
    ast->range_node.start = node;
    ast->range_node.stop = stop;
    ast->range_node.step = step;
    ast->range_node.has_step = step != NULL;
    ast->range_node.op = op;
    return ast;
}

static Jn_Node* parse_while(JnParser* p)
{
    consume(p, TOK_WHILE);
    Jn_Node* cond = parse_expr(p);
    Jn_Node* block = NULL;
    block = parse_block(p);
    return ast_while(p, cond, block);
}

static Jn_Node* parse_if(JnParser* p)
{
    consume(p, TOK_IF);
    Jn_Node* cond = parse_expr(p);
    Jn_Node* block = NULL;
    if (check(p, TOK_LBRACE))
        block = parse_block(p);
    else if (match(p, TOK_THEN))
        block = parse_expr(p);
    else return parse_error(p, "Got an invalid if statement.");
    
    Jn_Node* elsenode = NULL;
    elseif* elseifs = elseif_init();
    while(match(p, TOK_ELSEIF))
    {
        match(p, TOK_IF);
        Jn_Node* cond = parse_expr(p);
        Jn_Node* childblock;
        if (check(p, TOK_LBRACE))
        {
            childblock = parse_block(p);
        } else if (match(p, TOK_THEN))
        {
            childblock = parse_expr(p);
        }
        else{
            return parse_error(p, "Error: Expected an 'then' or '{'.");
        }
        elseif_add(elseifs, childblock, cond);
    }
    if (match(p, TOK_ELSE))
    {
        if (check(p, TOK_LBRACE))
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

static Jn_Node* parse_match(JnParser* p)
{
    consume(p, TOK_MATCH);
    Jn_Node* stmt = parse_expr(p);
    Jn_Node* else_stmt = NULL;
    if (!match(p, TOK_LBRACE))
        return parse_error(p, "Expected '{'"); // TODO
    case_t* caseObj = init_case(p->arena);
    while (!match(p, TOK_RBRACE))
    {
        if (match(p, TOK_ELSE))
        {
            match(p, TOK_DCOLON);
            if (match(p, TOK_THEN))
                else_stmt = parse_expr(p);
            else if (check(p, TOK_LBRACE))
                else_stmt = parse_block(p);
        } else 
        {
            Jn_Node* sub = parse_expr(p);
            Jn_Node* block = NULL;
            if (!match(p, TOK_DCOLON))
                return parse_error(p, "Expected '=>'");
            if (match(p, TOK_THEN))
                block = parse_expr(p);
            else if (check(p, TOK_LBRACE))
                block = parse_block(p);
            push_case(caseObj, sub, block);
        }
    }
    Jn_Node* ast = ast_create(p, AST_MATCH);
    ast->match_node.def = else_stmt;
    ast->match_node.cases = caseObj;
    ast->match_node.subject = stmt;
    return ast;
}

static Jn_Node* parse_fn(JnParser* p)
{
    consume(p, TOK_FN);
    if (!check(p, TOK_IDENT))
        return parse_error(p, "Expected an identifier.");
    char* ident = GET_LEX(p);
    next_parser(p); // ident
    char** params = malloc(sizeof(char *) * 100);
    int len = 0;
    Jn_Node* block = NULL;
    if (!match(p, TOK_LPARN))
        return parse_error(p, "Expected an '(");
    while (!match(p, TOK_RPARN))
    {
        if (!check(p, TOK_IDENT))
            return parse_error(p, "Expected an identifer.");
        params[len++] = GET_LEX(p);
        next_parser(p);
        if (match(p, TOK_COMMA))
            continue;
    }
    if (!check(p, TOK_LBRACE))
    {
        Jn_Node* ast = ast_function(p, ident, NULL, len, params);
        ast->fn_node.is_defined = false;
        return ast;
    }
    if (check(p, TOK_LBRACE))
        block = parse_block(p);
    else
        return parse_error(p, "Invalid function body.");
    Jn_Node* ast = ast_function(p, ident, block, len, params);
    ast->fn_node.is_async = false;
    ast->fn_node.is_yield = false;
    ast->fn_node.is_defined = true;
    return ast;
}

static Jn_Node* parse_assign(JnParser* p)
{
    char* ident;
    bool is_const = false;
    Jn_Node* type = NULL;
    if (match(p, TOK_CONST))
        is_const = true;
    if (!is_const && !match(p, TOK_LET))
        return parse_error(p, "Expected the 'let' keyword");
    if (check(p, TOK_IDENT))
    {
        ident = p->curr.lexeme;
        next_parser(p);
    }else
        return parse_error(p, "Expected an identifier.");
    
    if (match(p, TOK_COLON))
    {
        type = parse_primary(p);
    }
    
    if (!match(p, TOK_EQUAL))
        return parse_error(p, "Expected an '=' operator but got '%s'.", p->curr.lexeme);
    
    Jn_Node* ast = ast_assign(p, ident, is_const,  parse_expr(p));
    ast->assign.type = type;
    return ast;
}

static Jn_Node* parse_reassign(JnParser* p, Jn_Node* node)
{
    JnTokenType op = p->curr.type;
    Jn_Node* ast = ast_create(p, AST_REASSIGN);
    next_parser(p); // += reassign operator
    //x += 4;
    ast->reassign.expr = node;
    ast->reassign.op = op;
    ast->reassign.value = parse_expr(p);
    return ast;
}

static Jn_Node* parse_call(JnParser* p, Jn_Node* callee)
{
    //e.g main(1, None, true)
    Jn_Node** args = arena_alloc(p->arena, sizeof(Jn_Node *) * 20);
    size_t len = 0, cap = 20;
    Jn_Node* ast = ast_create(p, AST_CALL);
    if (match(p, TOK_RPARN))
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
            args = realloc(args, sizeof(Jn_Node*) * cap);
        }
        args[len++] = parse_expr(p);
    } while (match(p, TOK_COMMA));

    if (!match(p, TOK_RPARN))
        return parse_error(p, "Invalid syntax expected ')'.");

    ast->call.callee = callee;
    ast->call.pos_args = args;
    ast->call.pos_count = len;
    return ast;
}

static Jn_Node* parse_for_each(JnParser* p)
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
    consume(p, TOK_FOR);
    if (!check(p, TOK_IDENT))
        return parse_error(p, "Expected an identifier.");
    
    char* index = get_lexeme(p);
    char* var = NULL;
    if (match(p, TOK_COMMA))
    {
        var = get_lexeme(p);
    }
    SKIP(p, TOK_IN, "Expected an 'in' token.");

    Jn_Node* iter = parse_expr(p);
    Jn_Node* block = NULL;
    if (match(p, TOK_THEN)) // TODO: parse_then(p);
        block = parse_expr(p);
    else if (match(p, TOK_LBRACE))
        block = parse_block(p);
    else
        return parse_error(p, "Expected a block body.");
    
    Jn_Node* ast = ast_create(p, AST_FOR_EACH);
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

static Jn_Node* parse_for(JnParser* p)
{
    /*
    for loop;
    for i := 0; i < 10; i += 1 {} or then
    */

    next_parser(p); // for
    Jn_Node* init = NULL, *cond = NULL, *incr = NULL;
    Jn_Node* block = NULL;
    if (check(p, TOK_LPARN))
        return parse_error(p, "Sorry, does not support parentheses like C.");
    
    if (!check(p, TOK_SEMICOLON))
    {
        if (check(p, TOK_LET) || check(p, TOK_CONST))
            init = parse_assign(p);
        else
            init = parse_expr(p);
    }
    SKIP(p, TOK_SEMICOLON, "you forgot to add ';' in the forloop.");
    if (!check(p, TOK_SEMICOLON))
        cond = parse_expr(p);
    SKIP(p, TOK_SEMICOLON, "Yes, you need to add ';' after the loop condition.");
    if (!check(p, TOK_LBRACE) && !check(p, TOK_THEN))
        incr = parse_expr(p);

    if (match(p, TOK_THEN))
    {
        block = parse_expr(p);
    } else if (check(p, TOK_LBRACE))
    {
        block = parse_block(p);
    } else 
        return parse_error(p, "fooloop has no block.");

    Jn_Node* ast = ast_create(p, AST_FOR);
    ast->for_node.block = block;
    ast->for_node.cond = cond;
    ast->for_node.init = init;
    ast->for_node.incr = incr;
    return ast;
}

static Jn_Node* parse_instance(JnParser* p, Jn_Node* instance_obj);

static Jn_Node* parse_member(JnParser* p, Jn_Node* obj)
{
    // obj.field or obj.field()
    JnTokenType tok = p->curr.type;
    next_parser(p);

    if (check(p, TOK_LBRACE))
        return parse_instance(p, obj);

    char* ident = consume_ident(p);
    if (NULL == ident)
        return parse_error(p, "Expected identifier but got (%s).", GET_LEX(p));
    Jn_Node* field = ast_identifier(p, ident);
    Jn_Node* ast = ast_create(p, AST_MEMBER);
    ast->member.callie = obj;
    ast->member.field = field;
    ast->member.tok = tok;
    ast->member.setter = NULL;
    return ast;
}

static Jn_Node* parse_enum(JnParser* p)
{
    next_parser(p); // enum
    if (!check(p, TOK_IDENT))
        return parse_error(p, "Expected an identifier but got '%s'.", GET_LEX(p));
    
    char* ident = GET_LEX(p);
    next_parser(p);
    match(p, TOK_DCOLON);
    if (!match(p, TOK_LBRACE))
        return parse_error(p, "Expected an '{' but got '%s'.", GET_LEX(p));
    
    int len = 0;
    size_t capacity = 100;
    char** fields = malloc(sizeof(char *) * capacity);
    while (!match(p, TOK_RBRACE))
    {
        if (!check(p, TOK_IDENT))
            return parse_error(p, "Expected an identifier.");
        if (len >= capacity)
        {
            capacity *= 2;
            fields = realloc(fields, sizeof(char *) * capacity);
        }
        fields[len++] = GET_LEX(p);
        next_parser(p);
        if (match(p, TOK_COMMA))
            continue;
    }
    fields[len] = NULL;
    Jn_Node* ast = ast_create(p, AST_ENUM);
    ast->enum_stmt.ident = ident;
    ast->enum_stmt.fields = fields;
    ast->enum_stmt.count = len;
    return ast; 
}
static Jn_Node* parse_inline_if(JnParser* p, Jn_Node* node)
{
    next_parser(p); // if
    Jn_Node* cond = parse_expr(p);
    if (!match(p, TOK_ELSE))
        return parse_error(p, "Expected an 'else' clause.");
    Jn_Node* elsenode = parse_expr(p);
    Jn_Node* ast = ast_create(p, AST_INLINE_IF);
    ast->inline_if_stmt.cond = cond;
    ast->inline_if_stmt.then = node;
    ast->inline_if_stmt.otherwise = elsenode;
    return ast;
}

static Jn_Node* parse_index(JnParser* p, Jn_Node* arr)
{
    next_parser(p); //[
    Jn_Node* ast = ast_create(p, AST_ARRAY_INDEX);
    ast->index.array = arr;
    ast->index.pos = parse_expr(p);
    ast->index.is_set = false;
    ast->index.value = NULL;
    if (!match(p, TOK_RBRACKET))
        return parse_error(p, "Expected an closing ']'");
    return ast;
}


static bool allow_instance(JnParser* p, Jn_Node* node)
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
    if (ret && check(p, TOK_DOT))
    {
        next_parser(p);
    }
    return ret;
}

static Jn_Node* parse_postfix(JnParser* p, Jn_Node* left)
{
    if (check(p, TOK_PLUS_PLUS))
        return parse_unary(p, TOK_PLUS_PLUS);
    while (true)
    {
        if (match(p, TOK_LPARN))
        {
            left = parse_call(p, left);
            continue;
        }

        if (check(p, TOK_DOT))
        {
            left =  parse_member(p, left);
            continue;
        }

        if (check(p, TOK_IF))
        {
            return parse_inline_if(p, left);
        }

        if (check(p, TOK_RANGE))
        {
            left = parse_range(p, left);
            continue;
        }
    
        if (check(p, TOK_LBRACKET))
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

static Jn_Node* parse_hashmap(JnParser* p)
{
    skip(p, TOK_LBRACE, "Expected an opening '{'");
    Jn_Node** keys = arena_alloc(p->arena, sizeof(Jn_Node *) * 30);
    Jn_Node** values = arena_alloc(p->arena, sizeof(Jn_Node *) * 30);
    size_t len= 0, cap = 30;    
    Jn_Node* ast = ast_create(p, AST_HASHMAP);

    while (true)
    {
        if (match(p, TOK_RBRACE))
            break;
        if (len == 0 && match(p, TOK_NONE) && match(p, TOK_RBRACE))
            break;
        if (len > cap)
        {
            cap *= 2;
            keys = realloc(keys, sizeof(Jn_Node *) * cap);
            values = realloc(values, sizeof(Jn_Node *) * cap);
        }
        keys[len] = parse_expr(p);
        if (!match(p, TOK_COLON))
            return parse_error(p, "Expected an ':'");
        values[len] = parse_expr(p);
        len++;
        if (match(p, TOK_COMMA))
            continue;
        if (match(p, TOK_RBRACE))
            break;
        return parse_error(p, "expected a closing '}'.");
    }
    match(p, TOK_SEMICOLON); // TODO
    ast->hmp_node.keys = keys;
    ast->hmp_node.values = values;
    ast->hmp_node.count = len;
    return ast;
}


Jn_Node* parse_array(JnParser* p)
{
    SKIP(p, TOK_LBRACKET, "Expected an opening '['.");
    Jn_Node* arr = ast_array(p);
    if (match(p, TOK_RBRACKET)) return arr;

    for (;;)
    {

        if (check(p, TOK_COMMA))
        {
            return parse_error(p, "Expected an expression before ','.");
        }
        
        ast_array_add(arr, parse_expr(p));

        if (!match(p, TOK_COMMA)) break;

        if (check(p, TOK_RBRACKET))   break;
    }
    SKIP(p, TOK_RBRACKET, "Expected a closing ']'.");
    return arr;
}

static Jn_Node* parse_lambda(JnParser* p)
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
    consume(p, TOK_BITOR);
    int len = 0, cap = 20;
    char** args = malloc(sizeof(char *) * cap);
    do {
        if (match(p, TOK_BITOR))  break;
        if (len == 0 && match(p, TOK_NONE))
        {
            if (!check(p, TOK_BITOR))
                return parse_error(p, "Expected a closing '|'.");
            next_parser(p);
            break;
        }
        if (!check(p, TOK_IDENT)) return parse_error(p, "lambda expect an identifer but got %s.", GET_LEX(p));
        if (len >= cap)
        {
            cap *= 2;
            args = realloc(args, sizeof(char *) * cap);
        }
        args[len++] = GET_LEX(p);
        next_parser(p);
        if (match(p, TOK_COMMA)) continue;
        if (match(p, TOK_BITOR)) break;
    } while(true);
    args[len] = NULL;
    match(p, TOK_DCOLON);
    Jn_Node* expr = NULL;
    
    if (check(p, TOK_LBRACE))
    {
        expr = parse_block(p);
    } else {    
        expr = parse_expr(p);
    }
    Jn_Node* ast = ast_create(p, AST_LAMBDA);
    ast->lambda_node.count = len;
    ast->lambda_node.expr = expr;
    ast->lambda_node.args = args;
    return ast;
}

static Jn_Node* parse_import(JnParser* p)
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
    if (!check(p, TOK_STRING))
        return parse_error(p, "Expected an import path.");
    import_path = get_lexeme(p);
    if (match(p, TOK_LBRACE))
    {
        while (true)
        {
            if (!check(p, TOK_IDENT)) return parse_error(p, "Expected an identifier.");
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
            if (match(p, TOK_COMMA)) continue;
            if (match(p, TOK_RBRACE)) break;
            return parse_error(p, "Expected ',' or '}' from expression.");
        }
    }
    fields[len] = NULL;
    Jn_Node* ast = ast_create(p, AST_IMPORT);
    ast->import_node.lib = import_path;
    ast->import_node.fields = fields;
    ast->import_node.count = len;
    return ast;
}

static Jn_Node* parse_struct(JnParser* p)
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

    consume(p, TOK_STRUCT);
    char** fields = arena_alloc(p->arena, sizeof(char *) * 10);
    int len = 0, cap = 10;
    SKIP(p, TOK_LBRACE, "To initalize a struct you need '{'.");
    do {
        if (check(p, TOK_RBRACE))
            break;
        if (len == 0 && match(p, TOK_NONE))
            break;
        if (!check(p, TOK_IDENT))
            return parse_error(p, "Expected an identifier but (got '%s').", GET_LEX(p));
        if (len >= cap)
        {
            cap *= 2;
            fields = realloc(fields, sizeof(Jn_Node *) * cap);
        }
        fields[len++] = get_lexeme(p);
        if (match(p, TOK_COLON))
            parse_expr(p); // For readablity type does nothing.            

    } while (match(p, TOK_COMMA) || match(p, TOK_SEMICOLON));
    SKIP(p, TOK_RBRACE, "Expected an closing '}'");
    fields[len] = NULL;
    Jn_Node* ast = ast_create(p, AST_STRUCT);
    ast->struct_node.fields = fields;
    ast->struct_node.ident = NULL;
    ast->struct_node.count = len;
    return ast;
}

static Jn_Node* parse_instance(JnParser* p, Jn_Node* instance_obj)
{
    SKIP(p, TOK_LBRACE, "To initalize a instance you need '{'.");
    char** fields = arena_alloc(p->arena, sizeof(char *) * 10);
    Jn_Node** values = arena_alloc(p->arena, sizeof(Jn_Node *) * 10);
    int len = 0, cap = 10;
    bool contains_kwargs = false;
    for (;;)
    {
        if (match(p, TOK_RBRACE)) break;
        if (len >= cap)
        {
            cap *= 2;
            fields = realloc(fields, sizeof(char *) * cap);
            values = realloc(values, sizeof(Jn_Node *) * cap);
        }
        if (match(p, TOK_DOT)){
            contains_kwargs = true;
            fields[len] = get_lexeme(p);
            if (!match(p, TOK_EQUAL)) return parse_error(p, "expected '='");
            values[len] = parse_expr(p);
        } else {
            if (contains_kwargs)
                return parse_error(p, "Found a positional argument after a keyword argument.");
            fields[len] = NULL;
            values[len] = parse_expr(p);
        }
        len++;
        if (match(p, TOK_COMMA)) continue;
        if (match(p, TOK_RBRACE)) break;
        return parse_error(p, "Expected a closing '}' or ','.");
    }

    Jn_Node* ast = ast_create(p, AST_INSTANCE);
    ast->instance_node.fields = fields;
    ast->instance_node.values = values;
    ast->instance_node.count = len;
    ast->instance_node.object = instance_obj;
    return ast;
}


static Jn_Node* parse_multi_var(JnParser* p, Jn_Node* first)
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
        if (check(p, TOK_WALRUS) || check(p, TOK_SETTER))
        break;
        if (!check(p, TOK_IDENT))
            return parse_error(p, "Expected an identifier.");
        if (len > cap)
        {
            cap *= 2;
            // TODO
            idents = arena_realloc(p->arena, idents, sizeof(idents), sizeof(Jn_Node *) * cap);
        }
        idents[len++] =  GET_LEX(p);
        next_parser(p);
        
        if (match(p, TOK_COMMA))
            continue;
        
        if (check(p, TOK_WALRUS) || check(p, TOK_SETTER))
            break;
        return parse_error(p, "Got an invalid token.");
    }
    int op = GET_TOK(p).type;
    next_parser(p);
    Jn_Node* expr = parse_expr(p);
    // TODO: a, b := [1, 2]
    Jn_Node* ast = ast_create(p, AST_MULTI_VAR);
    ast->assign_multiple.count = len;
    ast->assign_multiple.idents = idents;
    ast->assign_multiple.value = expr;
    ast->assign_multiple.op = op;
    return ast;
}

static Jn_Node* parse_tuple(JnParser* p)
{
    consume(p, TOK_LPARN);
    Jn_Node* node = ast_create(p, AST_TUPLE);
    size_t len = 0, cap = 100;
    if (match(p, TOK_RPARN))
    {
        node->tuple.count = 0;
        node->tuple.elements = NULL;
        return node;
    }
    Jn_Node* first = parse_expr(p);
    if (!match(p, TOK_COMMA))
    {
        SKIP(p, TOK_RPARN, "Expected ')'.");
        return first;
    }
    Jn_Node** items = arena_alloc(p->arena, sizeof(Jn_Node *) * cap);
    items[len++] = first;
    do {
        if (check(p, TOK_RPARN)) break;
        if (len > cap)
        {
            cap *= 2;
            items = arena_realloc(p->arena, items, sizeof(Jn_Node *) * len, sizeof(Jn_Node *) * cap);
        }
        items[len++] = parse_expr(p);
    } while (match(p, TOK_COMMA));
    
    SKIP(p, TOK_RPARN, "Expected ')'.");
    items[len] = NULL;
    node->tuple.elements = items;
    node->tuple.count = len;
    return node;
}


static Jn_Node* parse_unary(JnParser* p, JnTokenType op)
{
    consume(p, op);
    Jn_Node* ast = ast_create(p, AST_UNARY);
    ast->unary.op = op;
    ast->unary.right = parse_expr(p); 
    return ast;
}

static Jn_Node* parse_string(JnParser* p)
{
    size_t len = strlen(GET_LEX(p)), cap = len + 1;
    char* buff = malloc(sizeof(char) * cap);
    memcpy(buff, GET_LEX(p), cap);
    next_parser(p);

    while (check(p, TOK_STRING))
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
    Jn_Node* ast = ast_literal(p,  jn_obj_string(p->state, buff));
    free(buff);
    return ast;   
}

static Jn_Node* parse_literal(JnParser* p, JnObject* obj)
{
    next_parser(p);
    return ast_literal(p, obj);
}
static Jn_Node* parse_node(JnParser* p, Jn_Node* node)
{
    next_parser(p);
    return node;
}

static Jn_Node* parse_yield(JnParser* p)
{
    consume(p, TOK_YIELD);
    if (is_smt_end(p))
    {
        return ast_yield(p, NULL);
    }
    return ast_yield(p, parse_expr(p));
}

Jn_Node* parse_primary(JnParser* p)
{
    JnToken t = p->curr;
    Jn_Node* ast = NULL;
    char* msg = t.lexeme;
    switch (t.type)
    {
        case TOK_INT:
            long i = t.i;
            JnObject* v =  jn_obj_int(p->state, i);
            return parse_literal(p, v);
        case TOK_LPARN:
            return parse_tuple(p);
        case TOK_COMMENT:
            next_parser(p);
            ast = ast_create(p, AST_COMMENT);
            ast->comment = strdup(t.lexeme);
            return ast;
        case TOK_FLOAT:
            double d = t.d;
            return parse_literal(p, jn_obj_float(p->state, d));
        case TOK_HASH:
            consume(p, TOK_HASH);
            if (check(p, TOK_LBRACE))
                return parse_hashmap(p);
            else if (check(p, TOK_FOR))
                return parse_for_each(p);
            return parse_error(p, "Error invalid expression");
        case TOK_STRING:
            return parse_string(p);
        case TOK_CHAR:
            char c = t.c;
            return parse_literal(p, JN_RETURN_CHAR(p->state, c));
        case TOK_PLUS_PLUS:
            return parse_unary(p, TOK_PLUS_PLUS);
        case TOK_RETURN:
            ast = ast_create(p, AST_RETURN);
            next_parser(p);
            if (is_stmt_end(p) || p->has_newl)
            {
                match(p, TOK_SEMICOLON);
                ast->return_stmt.value = NULL;
                return ast;
            }
            ast->return_stmt.value = parse_expr(p);
            return ast;
        case TOK_IDENT:
            return parse_node(p, ast_identifier(p, t.lexeme));
        case TOK_CONTINUE:
            return parse_node(p, ast_continue(p));
        case TOK_BREAK:
            return parse_node(p, ast_break(p));
        case TOK_NEWLINE:
            next_parser(p);
            return parse_primary(p);
        case TOK_MINUS:
            return parse_unary(p, TOK_MINUS);
        case TOK_TILDE:
            return parse_unary(p, TOK_TILDE);
        case TOK_NOT:
            return parse_unary(p, TOK_NOT);
        case TOK_TRUE:
            return parse_literal(p, JN_RETURN_TRUE(p->state));
        case TOK_NONE:
            return parse_literal(p, JN_RETURN_NONE);
        case TOK_FALSE:
            return parse_literal(p, JN_RETURN_FALSE(p->state));
        case TOK_YIELD:
            return parse_yield(p);
        case TOK_LBRACKET:
            return parse_array(p);
        case TOK_BITOR:
            return parse_lambda(p);
        case TOK_IMPORT:
            return parse_import(p);
        case TOK_STRUCT:
            return parse_struct(p);
        case TOK_ERROR: {
            return parse_error(p, msg);
        }
        default:
            return parse_error(p, "Error: Got an invalid expression '%s'.", msg);
    }
}

Jn_Node* parse_prec(JnParser* p, precedence prec)
{
    Jn_Node* left = parse_primary(p);
    while (true)
    {
        left = parse_postfix(p, left);
        precedence next_pr;
        JnTokenType op;
        bool comp = false;
        if (p->curr.type == TOK_IN && p->next.type == TOK_NOT)
            return parse_error(p, "Invalid operator order: did you mean 'not in'?.");
        if (p->curr.type == TOK_NOT && p->next.type == TOK_IS)
            return parse_error(p, "Invalid operator order: did you mean 'is not'?.");
        if (p->curr.type == TOK_IS && p->next.type == TOK_NOT)
        {
            op = TOK_IS_NOT;
            next_pr = PREC_EQ;
            comp = true;
        } else if (p->curr.type == TOK_NOT && p->next.type == TOK_IN)
        {
            op = TOK_NOT_IN;
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
        Jn_Node* right = parse_prec(p, op == TOK_POW ? next_pr - 1 : next_pr);
        left = ast_binary(p, left, op, right);
    }
    return  left;
}
Jn_Node* parse_expr(JnParser* p)
{
    // MAIN FUNCTION
    return parse_prec(p, PREC_NONE);
}


Jn_Node* parse_stmt(JnParser* p)
{
    Jn_Node* stmt;
    switch (GET_TOK_TYPE(p))
    {
        case TOK_CONST:
            stmt = parse_assign(p);
            break;
        case TOK_LET:
            stmt = parse_assign(p);
            break;
        case TOK_LBRACE:
            stmt = parse_block(p);
            break;
        case TOK_ENUM:
            stmt = parse_enum(p);
            break;
        case TOK_MATCH:
            stmt = parse_match(p);
            break;
        case TOK_IF:
            stmt = parse_if(p);
            break;
        case TOK_FOR:
            stmt = parse_for(p);
            break;
        case TOK_FN:
            stmt = parse_fn(p);
            break;
        case TOK_LOOP:
            stmt = parse_loop(p);
            break;
        case TOK_WHILE:
            stmt = parse_while(p);
            break;
        default:
            stmt = parse_expr(p);
            break;
    }
    return stmt;
}