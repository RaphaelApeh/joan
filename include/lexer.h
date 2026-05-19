#ifndef LEXER_H

#define LEXER_H

#include <stdbool.h>

typedef struct lexer{
    char* start;
    char* curr;
    size_t line;
    size_t column;
} lexer;

void strip_ws(lexer* l);

void init_lexer(lexer* l, char* source);

char peek(lexer* l);

char peek_next(lexer* l);

bool at_end(lexer* l);

char advance(lexer* l);

#endif