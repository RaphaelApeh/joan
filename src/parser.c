#include <stdlib.h>
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

int check(parser* p, TokenType type)
{
    return _check(p, type, false);
}

static bool match(parser* p, TokenType type)
{
    if (p->curr.type != type) return false;
    advance_parser_c(p);
    return true;
}


static bool is_assign_token(TokenType type)
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
            return true;
        default:
            return false;
    }
}


void advance_parser(parser* p)
{
    p->curr = p->next;
    p->next = next_token(p->l);
}

int check_next(parser* p, TokenType type)
{
    return _check(p, type, true);
}


parser* init_parser(lexer* l)
{
    parser* p = malloc(sizeof(parser));
    p->l = l;
    p->env = init_env(NULL);
    p->next = clean_token(l);
    return p;
}

precedence get_prec(TokenType type)
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
    
    case TOKEN_IN:
        return PREC_IN;

    case TOKEN_OR:
        return PREC_OR;
    
    case TOKEN_IS:
    case TOKEN_RANGE:
        return PREC_PRIMARY;
    
    default:
        return PREC_NONE;
    }
}

AST* parse_block(parser* p)
{
    advance_parser_c(p);
    AST* block = new_block(p->arena);
    while (!match(p, TOKEN_RBRACE))
        add_block(block, parse_stmt(p));
    return block;
}


void advance_parser_c(parser* p)
{
    p->curr = p->next;
    while (true)
    {
        p->next = next_token(p->l);
        if (p->next.type != TOKEN_NEWLINE || p->next.type != TOKEN_SIMICOLON);
            break;
    }
}

AST* parse_error(parser* p, const char* msg, ...)
{
    va_list args;
    va_start(args, msg);
    //TODO: code  
    va_end(args);
    return ast_error(p->arena, p, msg);
}

static AST* parse_loop(parser* p)
{
    advance_parser_c(p); // loop
    AST* ast = ast_create(p->arena, AST_LOOP);
    if (check(p, TOKEN_LBRACE))
        ast->loop_stmt.block = parse_block(p);
    return ast;
}

static AST* parse_while(parser* p)
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
    AST* ast = ast_create(p->arena, AST_WHILE);
    ast->while_node.cond = cond;
    ast->while_node.block = block;
    return ast;
}

static AST* parse_if(parser* p)
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
        p->arena,
        cond,
        block,
        elseifs,
        elsenode
    );
}

static AST* parse_match(parser* p)
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
    AST* ast = ast_create(p->arena, AST_MATCH);
    ast->match_node.def = else_stmt;
    ast->match_node.cases = caseObj;
    ast->match_node.subject = stmt;
    return ast;
}

static AST* parse_fn(parser* p)
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
    AST* ast = ast_create(p->arena, AST_FUNCTION);
    ast->fn_node.block = block;
    ast->fn_node.name = ident;
    ast->fn_node.params = params;
    ast->fn_node.count = len;
    ast->fn_node.is_async = false;
    ast->fn_node.is_yield = false;
    return ast;
}

static AST* parse_assign(parser* p)
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
    return ast_assign(p->arena, ident, is_const,  parse_expr(p));
}

static AST* parse_reassign(parser* p, AST* node)
{
    if (node->type != AST_IDENTIFIER)
        return parse_error(p, "Expected an reassign operator.");
    const char* ident = node->identifier;
    TokenType op = p->curr.type;
    AST* ast = ast_create(p->arena, AST_REASSIGN);
    advance_parser_c(p); // += reassign operator
    //x += 4;
    ast->reassign.ident = (char *)ident;
    ast->reassign.op = op;
    ast->reassign.value = parse_expr(p);
    return ast;
}

static AST* parse_call(parser* p, AST* callee)
{
    //e.g main(1, None, true)
    //TODO:
    if (callee->type != AST_IDENTIFIER)
        return parse_error(p, "Expected an identifier.");
    advance_parser_c(p); // (
    AST* ast = ast_call(p->arena, callee->identifier);// TODO
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

static AST* parse_member(parser* p, AST* obj)
{
    //TODO
    // advance_parser_c(p);
    // if (!check(p, TOKEN_IDENTIFIER))
    //     return parse_error(p, "Expected a field identifier but got %s", GET_LEX(p));
    // char* field = GET_LEX(p);
    // advance_parser_c(p);
    // AST* ret = ast_member(p->arena, obj, field);
    // // MEMBER CALL
    // if (check(p, TOKEN_LPARN))
    // {
    //     AST* callie = parse_call(p, field);
    //     ret->member.is_call = true;
    //     ret->member.is_getter = false;
    //     ret->member.is_setter = false;
    //     ret->member.callie = callie;
    //     return ret;
    // }
    // // MEMBER SETTER
    // else if (check(p, TOKEN_EQUAL))
    // {
    //     advance_parser(p);
    //     if (check(p, TOKEN_NEWLINE))
    //         return parse_error(p, "Expected a value");
    //     AST* setter = parse_expr(p);
    //     ret->member.is_setter = true;
    //     ret->member.is_call = false;
    //     ret->member.is_getter = false;
    //     ret->member.setter = setter;
    //     return ret;
    // }
    // // DEFAULT GETTER
    // return ret;
}

static AST* parse_inline_if(parser* p, AST* node)
{
    advance_parser_c(p); // if
    AST* cond = parse_expr(p);
    if (!match(p, TOKEN_ELSE))
        return parse_error(p, "Expected an 'else' clause.");
    AST* elsenode = parse_expr(p);
    AST* ast = ast_create(p->arena, AST_INLINE_IF);
    ast->inline_if_stmt.cond = cond;
    ast->inline_if_stmt.then = node;
    ast->inline_if_stmt.otherwise = elsenode;
    return ast;
}

static AST* parse_index(parser* p, AST* arr)
{
    advance_parser_c(p); //[
    AST* ast = ast_create(p->arena, AST_ARRAY_INDEX);
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
static AST* parse_postfix(parser* p, AST* left)
{

    while (true)
    {
        if (check(p, TOKEN_LPARN))
        {
            left = parse_call(p, left);
            continue;
        }

        // if (check(p, TOKEN_IF))
        // {
        //     return parse_inline_if(p, left);
        // }
        //TODO
        // if (check(p, TOKEN_DOT))
        // {
        //     left = parse_member(p, left);
        //     continue;
        // }
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

static AST* parse_hashmap(parser* p)
{
    // TODO
    if (!match(p, TOKEN_LBRACE)) // {
        return parse_error(p, "Expected an opening '{'");
    char* keys[1024];
    AST* values[1024];
    size_t len = 0;    
    AST* ast = ast_create(p->arena, AST_HASHMAP);

    while (true)
    {
        if (match(p, TOKEN_RBRACE))
            break;
        if (match(p, TOKEN_COMMA))
            continue;
        token tok = p->curr;
        if (!match(p, TOKEN_COLON))
            return parse_error(p, "Expected a ':'.");
        keys[len] = tok.lexeme;
        values[len] = parse_expr(p);
        len++;
        if (match(p, TOKEN_COMMA))
            continue;
        if (match(p, TOKEN_RBRACE))
            break;
    }
    memcmp(ast->hmp_node.keys, keys, len * sizeof(keys));
    memcmp(ast->hmp_node.values, values, len * sizeof(values));
    ast->hmp_node.count = len;
    return ast;
}
AST* parse_array(parser* p)
{
    advance_parser_c(p); // [
    AST* arr = ast_array(p->arena);
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

AST* parse_value(parser* p)
{
    token t = p->curr;
    AST* ast = NULL;
    switch (t.type)
    {
        case TOKEN_INT:
            int i = *((int *) t.v);
            Object* v = obj_int(i);
            advance_parser_c(p);
            return ast_literal(p->arena, v);
        case TOKEN_LPARN:
            advance_parser_c(p);
            ast = parse_expr(p);
            if (check(p, TOKEN_RPARN))
                advance_parser_c(p);
            return ast;
        case TOKEN_COMMENT:
            advance_parser_c(p);
            ast = ast_create(p->arena, AST_COMMENT);
            ast->comment = strdup(t.lexeme);
            return ast;
        case TOKEN_FLOAT:
            double d = *((double *) t.v);
            advance_parser_c(p);
            return ast_literal(p->arena, obj_float(d));
        case TOKEN_HASH:
            advance_parser_c(p);
            if (check(p, TOKEN_LBRACE))
                return parse_hashmap(p);
        case TOKEN_STRING:
            ast = ast_literal(p->arena, obj_string(t.lexeme));
            advance_parser_c(p);
            return ast;
        case TOKEN_RETURN:
            advance_parser_c(p);
            ast = ast_create(p->arena, AST_RETURN);
            ast->return_stmt.value = parse_expr(p);
            return ast;
        case TOKEN_IDENTIFIER:
            ast = ast_identifier(p->arena, t.lexeme);
            advance_parser_c(p);
            //if (check(p, TOKEN_EQUAL))
                //TODO
            return ast;
        case TOKEN_CONTINUE:
            advance_parser_c(p);
            return ast_continue(p->arena);
        case TOKEN_BREAK:
            advance_parser_c(p);
            return ast_break(p->arena);
        case TOKEN_NEWLINE:
            advance_parser_c(p);
            return parse_value(p);
        case TOKEN_MINUS:
            advance_parser(p);
            ast = ast_create(p->arena, AST_UNARY);
            ast->unary.op = TOKEN_MINUS;
            ast->unary.right = parse_value(p);
            return ast;
        case TOKEN_NOT:
            advance_parser(p);
            ast = ast_create(p->arena, AST_UNARY);
            ast->unary.op = TOKEN_NOT;
            ast->unary.right = parse_value(p);
            return ast;
        case TOKEN_TRUE:
            ast = ast_literal(p->arena, obj_bool(true));
            advance_parser_c(p);
            return ast;
        case TOKEN_NONE:
            ast = ast_literal(p->arena, obj_none());
            advance_parser_c(p);
            return ast;
        case TOKEN_FALSE:
            ast = ast_literal(p->arena, obj_bool(false));
            advance_parser_c(p);
            return ast;
        case TOKEN_LBRACKET:
            return parse_array(p);
        // case TOKEN_MINUS:
        //     advance_parser_c(p);
        //     ast = parse_value(p);
        //     return ast_unary(TOKEN_MINUS, ast);
        case TOKEN_PRINTLN: {
            advance_parser_c(p);
            AST* out = NULL;
            if (!check(p, TOKEN_NEWLINE))
                out = parse_expr(p);
            return ast_println(p->arena, out);
        }
        default:
            return parse_error(p, "Error: Got an invalid expression '%s'.", t.lexeme);
    }
}

AST* parse_prec(parser* p, precedence prec)
{
    AST* left = parse_value(p);
    while (prec < get_prec(p->curr.type))
    {
        TokenType op = p->curr.type;
        precedence op_prec = get_prec(op);
        advance_parser_c(p);
        AST* rhs = parse_prec(p, op_prec);
        left = ast_binary(p->arena, left, op, rhs);
    }
    return parse_postfix(p, left);
}
AST* parse_expr(parser* p)
{
    // MAIN FUNCTION
    return parse_prec(p, PREC_NONE);
}

AST* parse_stmt(parser* p)
{
    switch (p->curr.type)
    {
        case TOKEN_CONST:
            return parse_assign(p);
        case TOKEN_LET:
            return parse_assign(p);
        case TOKEN_LBRACE:
            return parse_block(p);
        case TOKEN_MATCH:
            return parse_match(p);
        case TOKEN_IF:
            return parse_if(p);
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