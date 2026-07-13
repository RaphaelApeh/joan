#ifndef JOAN_LEXER_H

#define JOAN_LEXER_H

#include <stdbool.h>

typedef struct joan_lexer_t{
    char* start, *curr, *filename;
    size_t line;
    size_t column;
} joan_lexer_t;

void J_init_lexer(joan_lexer_t* l, char* source);
void strip_ws(joan_lexer_t * l);
char peek_next(joan_lexer_t* l);
char peek(joan_lexer_t* l);
char advance(joan_lexer_t* l);
bool at_end(joan_lexer_t* l);

#endif //JOAN_LEXER_H