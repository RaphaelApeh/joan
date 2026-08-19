#include <stdint.h>
#include <Joan.h>
#include "lexer.h"
#include "token.h"


JN_API bool Jn_get_next_token(Jn_Lexer* lex, Jn_Token* tok)
{
    if (NULL == tok) return false;
    *tok = next_token(lex);
    return tok->type != TOK_EOF && tok->type != TOK_ERROR;
}

void strip_ws(Jn_Lexer* l)
{
    while(!at_end(l))
    {
        char c = peek(l);
        switch (c)
        {
            case ' ':
            case '\t':
            case '\r':
                advance(l);
                break;
            default:
                return;
        }
    }
}

bool at_end(Jn_Lexer* l)
{
    return *l->curr == '\0';
}

char advance(Jn_Lexer* l)
{
    char c = *l->curr++;
    if (c == '\n')
    {
        l->line++;
        l->column = 1;
    } else l->column++;

    return c;
}

char peek(Jn_Lexer* l)
{
    return *l->curr;
}

char peek_next(Jn_Lexer* l)
{
    if (at_end(l)) return '\0';
    return l->curr[1];
}

bool peek_advance(Jn_Lexer* l, char c)
{
    if (peek(l) != c)
        return false;
    advance(l);
    return true;
}


void J_init_lexer(Jn_Lexer* l, char* source, const char* filename)
{
    l->start = source;
    l->curr = source;
    l->line = 1;
    l->column = 1;
    l->filename = filename;
}
