#ifndef JN_PARSE_H
#define JN_PARSE_H

#include <Joan.h>
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

    PREC_BITOR,
    PREC_BITXOR,
    PREC_BITAND,

    PREC_EQ,

    PREC_SHIFT,

    PREC_COMP,
    
    PREC_IN,
    
    PREC_TERM,
    PREC_FACTOR,
    PREC_POWER,
    PREC_UNARY,
    
    PREC_CALL,

    PREC_PRIMARY
} precedence;

typedef struct joan_parser_t{
    J_State* state;
    joan_lexer_t* l;
    Arena* arena;
    joan_token_t curr;
    joan_token_t next;
} joan_parser_t;


void jn_init_parser(joan_parser_t* p, joan_lexer_t* l);
void advance_parser(joan_parser_t* p);

void advance_parser_c(joan_parser_t* p);
AST* parse_value(joan_parser_t* p);
// Main function for the parser.
AST* parse_stmt(joan_parser_t* p);
AST* parse_expr(joan_parser_t* p);
AST* parse_error(joan_parser_t* p, const char* msg, ...);

#endif //JN_PARSE_H