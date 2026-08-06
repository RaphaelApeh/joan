#ifndef JOAN_LEXER_H

#define JOAN_LEXER_H

#include <stdbool.h>

typedef struct joan_lexer_t{
    const char* filename;
    char* start, *curr; 
    size_t line;
    size_t column;
} joan_lexer_t;

void J_init_lexer(joan_lexer_t* l, char* source, const char* filename);
void strip_ws(joan_lexer_t * l);
char peek_next(joan_lexer_t* l);
char peek(joan_lexer_t* l);
bool peek_advance(joan_lexer_t* l, char c);
char advance(joan_lexer_t* l);
bool at_end(joan_lexer_t* l);

#endif //JOAN_LEXER_H