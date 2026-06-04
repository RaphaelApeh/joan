#ifndef PARSER_H
#define PARSER_H

#include "token.h"
#include "env.h"

typedef struct Arena Arena;
typedef struct AST AST;
typedef struct joan_lexer_t joan_lexer_t;

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

typedef struct joan_parser_t{
    joan_lexer_t* l;
    Arena* arena;
    joan_token_t curr;
    joan_token_t next;
} joan_parser_t;


joan_parser_t* jn_init_parser(joan_lexer_t* l);
void advance_parser(joan_parser_t* p);
precedence get_prec(J_TokenType type);
// Main function for the parser.
AST* parse_stmt(joan_parser_t* p);
AST* parse_expr(joan_parser_t* p);
AST* parse_error(joan_parser_t* p, const char* msg, ...);
void advance_parser_c(joan_parser_t* p);
AST* parse_value(joan_parser_t* p);

#endif