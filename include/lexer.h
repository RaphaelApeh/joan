#ifndef JOAN_LEXER_H

#define JOAN_LEXER_H

#include <stdbool.h>

typedef struct Jn_Lexer{
    const char* filename;
    char* start, *curr; 
    size_t line;
    size_t column;
} Jn_Lexer;

void J_init_lexer(Jn_Lexer* l, char* source, const char* filename);
void strip_ws(Jn_Lexer * l);
char peek_next(Jn_Lexer* l);
char peek(Jn_Lexer* l);
bool peek_advance(Jn_Lexer* l, char c);
char advance(Jn_Lexer* l);
bool at_end(Jn_Lexer* l);

#endif //JOAN_LEXER_H