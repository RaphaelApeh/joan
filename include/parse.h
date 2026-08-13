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

typedef struct JnParser{
    J_State* state;
    Jn_Lexer* l;
    Jn_Arena* arena;
    JnToken prev;
    JnToken curr;
    JnToken next;
    bool has_newl;
    int errors;
} JnParser;


void jn_init_parser(JnParser* p, Jn_Lexer* l);
void advance_parser(JnParser* p);

JnToken next_parser(JnParser* p);
Jn_Node* parse_primary(JnParser* p);
// Main function for the parser.
Jn_Node* parse_stmt(JnParser* p);
Jn_Node* parse_stmt_check(JnParser* p, Jn_Node* stmt);
Jn_Node* parse_expr(JnParser* p);
Jn_Node* parse_error(JnParser* p, const char* msg, ...);

#endif //JN_PARSE_H