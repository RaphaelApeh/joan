#ifndef JN_PARSE_H
#define JN_PARSE_H

#include <Joan.h>
#include "token.h"
#include "env.h"

typedef struct Jn_Arena Jn_Arena;
typedef struct Jn_Node Jn_Node;
typedef struct Jn_Lexer Jn_Lexer;

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

typedef struct Jn_Parser{
    Jn_State* state;
    Jn_Lexer* l;
    Jn_Arena* arena;
    Jn_Token prev;
    Jn_Token curr;
    Jn_Token next;
    int yields, errors;
    bool has_newl, skip_comma;
} Jn_Parser;


void jn_init_parser(Jn_Parser* p, Jn_Lexer* l);
void advance_parser(Jn_Parser* p);

Jn_Token next_parser(Jn_Parser* p);
Jn_Node* parse_primary(Jn_Parser* p);
// Main function for the parser.
Jn_Node* parse_stmt(Jn_Parser* p);
Jn_Node* parse_stmt_check(Jn_Parser* p, Jn_Node* stmt);
Jn_Node* parse_expr(Jn_Parser* p);
Jn_Node* parse_error(Jn_Parser* p, const char* msg, ...);

#endif //JN_PARSE_H