#ifndef PARSER_H
#define PARSER_H

#include "token.h"
#include "env.h"

typedef struct Arena Arena;
typedef struct AST AST;
typedef struct lexer lexer;

typedef enum{
    PREC_NONE,
    PREC_ASSIGN,
    PREC_OR,
    PREC_AND,
    PREC_EQ,
    PREC_COMP,
    PREC_IN,
    PREC_TERM,
    PREC_UNARY,
    PREC_CALL,
    PREC_PRIMARY
} precedence;

typedef struct parser{
    lexer* l;
    env_t* env;
    Arena* arena;
    token curr;
    token next;
} parser;


parser* init_parser(lexer* l);
void advance_parser(parser* p);
precedence get_prec(TokenType type);
// Main function for the parser.
AST* parse_stmt(parser* p);
AST* parse_expr(parser* p);
AST* parse_error(parser* p, const char* msg, ...);
void advance_parser_c(parser* p);
AST* parse_value(parser* p);

#endif