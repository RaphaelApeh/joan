#include <stdint.h>
#include "lexer.h"

void strip_ws(lexer* l)
{
    while(!at_end(l))
    {
        char c = peek(l);
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') advance(l);
        else break;
    }
}

bool at_end(lexer* l)
{
    return *l->curr == '\0';
}

char advance(lexer* l)
{
    char c = *l->curr++;
    if (c == '\n')
    {
        l->line++;
        l->column = 0;
    } else l->column++;

    return c;
}

char peek(lexer* l)
{
    return *l->curr;
}

char peek_next(lexer* l)
{
    if (at_end(l)) return '\0';
    return l->curr[1];
}

void init_lexer(lexer* l, char* source)
{
    l->start = source;
    l->curr = source;
    l->line = 1;
    l->column = 0;
}
