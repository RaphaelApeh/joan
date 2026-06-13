#ifndef JOAN_TOKEN_H

#define JOAN_TOKEN_H

#include "lexer.h"

typedef enum {
    TOKEN_EOF,
    TOKEN_NEWLINE,

    TOKEN_INT,
    TOKEN_FLOAT,
    TOKEN_STRING,
    TOKEN_CHAR,
    TOKEN_IDENTIFIER,
    
    TOKEN_LPARN,
    TOKEN_RPARN,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_LBRACKET,
    TOKEN_RBRACKET,

    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_COMMA,
    TOKEN_SIMICOLON,
    TOKEN_DOT,
    TOKEN_QUOTE,
    TOKEN_SQUOTE,
    TOKEN_BSLASH,
    TOKEN_GT_EQ,
    TOKEN_BITOR,
    TOKEN_BITAND,
    TOKEN_BITAC,
    TOKEN_HASH, // #
    TOKEN_PI,
    TOKEN_AT,
    TOKEN_RANGE,
    TOKEN_EXR,

    TOKEN_APLUS, // +=
    TOKEN_AMINUS, // -=
    TOKEN_ASTAR, // *=
    TOKEN_ARSHIFT, // >>=
    TOKEN_ALSHIFT, // <<=
    TOKEN_APERCENTAGE, // %=
    TOKEN_ABITAC, // ^=
    TOKEN_ASLASH, // /=
    TOKEN_ABITAND, // &=
    TOKEN_ABITOR, // |= TODO: name change

    TOKEN_EQUAL,
    TOKEN_EQEQ,
    TOKEN_GT,
    TOKEN_LT,
    TOKEN_GTE,
    TOKEN_LTE,
    TOKEN_NEQ,
    TOKEN_RSHIFT,
    TOKEN_LSHIFT,
    TOKEN_COLON, // :
    TOKEN_WALRUS, // :=
    TOKEN_QUESTION, // ?
    TOKEN_PERCENTAGE, // %
    TOKEN_SETTER,
    TOKEN_NONE,
    TOKEN_TRUE,
    TOKEN_FALSE,

    TOKEN_LET,
    TOKEN_OF,
    TOKEN_IF,
    TOKEN_ELSEIF,
    TOKEN_ELSE,
    TOKEN_THEN,
    TOKEN_FOR,
    TOKEN_LOOP,
    TOKEN_WHILE,
    TOKEN_OR,
    TOKEN_AS,
    TOKEN_IS,
    TOKEN_IN,
    TOKEN_NOT,
    TOKEN_NOT_IN,
    TOKEN_AND,

    TOKEN_FN,
    TOKEN_DO,
    TOKEN_LAMBDA,
    TOKEN_USING,
    TOKEN_ENUM,
    TOKEN_CONST,
    TOKEN_RETURN,
    TOKEN_CLASS,
    TOKEN_ASSERT,
    TOKEN_STRUCT,
    TOKEN_MATCH,
    TOKEN_CONTINUE,
    TOKEN_BREAK,
    TOKEN_PRINTLN,
    
    TOKEN_COMMENT,

    TOKEN_ERROR,
} J_TokenType;

typedef struct joan_token_t{
    J_TokenType type;
    char* lexeme;
    void* v;
    int line;
    int column;
} joan_token_t;

joan_token_t clean_token(joan_lexer_t * l);

joan_token_t make_token(joan_lexer_t * l, J_TokenType type);

joan_token_t number(joan_lexer_t * l);

joan_token_t identifier(joan_lexer_t * l);
// TODO: improve on token_string function
joan_token_t token_string(joan_lexer_t * l);

joan_token_t next_token(joan_lexer_t * l);

#endif