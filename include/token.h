#ifndef JOAN_TOK_H

#define JOAN_TOK_H

#include "lexer.h"

typedef enum {  TOK_EOF, TOK_NEWLINE, 
    
    TOK_INT,
    TOK_FLOAT,
    TOK_STRING,
    TOK_CHAR,
    TOK_IDENT,
    
    TOK_LPARN,
    TOK_RPARN,
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_LBRACKET,
    TOK_RBRACKET,

    TOK_PLUS,
    TOK_MINUS,
    TOK_MUL,
    TOK_SLASH,
    TOK_POW,
    TOK_COMMA,
    TOK_PLUS_PLUS,
    TOK_SEMICOLON,
    TOK_DOT,
    TOK_QUOTE,
    TOK_SQUOTE,
    TOK_BSLASH,
    TOK_GT_EQ,
    TOK_BITOR,
    TOK_BITAND,
    TOK_XOR,
    TOK_HASH, // #
    TOK_PI,
    TOK_AT,
    TOK_RANGE,
    TOK_DCOLON,
    TOK_TILDE,
    TOK_ELLIPSIS,

    TOK_APLUS, // +=
    TOK_AMINUS, // -=
    TOK_AMUL, // *=
    TOK_ARSHIFT, // >>=
    TOK_ALSHIFT, // <<=
    TOK_APERCENTAGE, // %=
    TOK_AXOR, // ^=
    TOK_ASLASH, // /=
    TOK_ABITAND, // &=
    TOK_ABITOR, // |=
    TOK_APOW, // **=

    TOK_EQUAL,
    TOK_EQEQ,
    TOK_GT,
    TOK_LT,
    TOK_GTE,
    TOK_LTE,
    TOK_NEQ,
    TOK_RSHIFT,
    TOK_LSHIFT,
    TOK_COLON, // :
    TOK_WALRUS, // :=
    TOK_QUESTION, // ?
    TOK_PERCENTAGE, // %
    TOK_SETTER,
    TOK_NONE,
    TOK_TRUE,
    TOK_FALSE,

    TOK_LET,
    TOK_IF,
    TOK_ELSEIF,
    TOK_ELSE,
    TOK_THEN,
    TOK_FOR,
    TOK_LOOP,
    TOK_WHILE,
    TOK_OR,
    TOK_IS,
    TOK_IS_NOT,
    TOK_IN,
    TOK_NOT,
    TOK_NOT_IN,
    TOK_AND,
    TOK_ARROW, // ->

    TOK_FN,
    TOK_IMPORT,
    TOK_ENUM,
    TOK_CONST,
    TOK_RETURN,
    TOK_YIELD,
    TOK_CLASS,
    TOK_STRUCT,
    TOK_MATCH,
    TOK_CONTINUE,
    TOK_BREAK,
    
    TOK_COMMENT,

    TOK_ERROR,
} Jn_TokenType;

typedef struct Jn_Token{
    char* lexeme;
    Jn_TokenType type;
    union{
        double d;
        long i;
        char c;
    };
    void* v;
    int line;
    int column;
} Jn_Token;

Jn_Token clean_token(Jn_Lexer * l);

Jn_Token make_token(Jn_Lexer * l, Jn_TokenType type);

Jn_Token next_token(Jn_Lexer * l);

#endif