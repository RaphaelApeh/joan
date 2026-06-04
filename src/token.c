#include <strings.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include "token.h"
#include "lexer.h"

#define CHECK_TOK(lex, str) ((strcmp((lex), (str))) == 0)

static bool peek_advance(joan_lexer_t* l, char c)
{
    if (peek(l) != c)
        return false;
    advance(l);
    return true;
}

joan_token_t clean_token(joan_lexer_t* l)
{
    joan_token_t t;
    do {
        t = next_token(l);
    } while (t.type == TOKEN_NEWLINE);
    return t;
}

joan_token_t number(joan_lexer_t* l)
{
    if (l->curr[0] == '0' && (peek(l) == 'x' || peek(l) == 'X'))
    {
        advance(l);
        advance(l);
        while(isxdigit(peek(l)))
            advance(l);
        joan_token_t _t = make_token(l, TOKEN_INT);
        int* i = malloc(sizeof(int));
        *i = strtol(_t.lexeme, NULL, 16);
        _t.v = i;
        return _t;
    }
    while(isdigit(peek(l))) advance(l);
    if (peek(l) == '.' && isdigit(peek_next(l)))
    {
        advance(l);
        while (isdigit(peek(l))) advance(l);
        joan_token_t t = make_token(l, TOKEN_FLOAT);
        double* d = malloc(sizeof(double));
        *d = atof(t.lexeme);
        t.v = d;
        return t;
    }
    joan_token_t t = make_token(l, TOKEN_INT);
    int* i = malloc(sizeof(int));
    *i = atoi(t.lexeme);
    t.v = i;
    return t;
}

joan_token_t token_string(joan_lexer_t* l)
{
    l->start = l->curr;
    char q = l->curr[-1];
    while (*l->curr && *l->curr != q)
    {
        if (*l->curr == '\\' && *l->curr)
            l->curr += 2;
        else
            l->curr++;
    }
    if (*l->curr == '\0')
    {
        joan_token_t t;
        t.type = TOKEN_ERROR;
        t.lexeme = "error";
        int* i = malloc(sizeof(int));
        *i = -1;
        t.v = i;
        return t;
    }
    joan_token_t t = make_token(l, TOKEN_STRING);
    char c = *l->curr++;
    if (q != c)
    {
        // TODO
    }
    return t;   
}

static joan_token_t token_char(joan_lexer_t* l)
{
    //TODO
}

joan_token_t identifier(joan_lexer_t* l)
{
    while (isalnum(peek(l)) || peek(l) == '_') 
        advance(l);
    joan_token_t t = make_token(l, TOKEN_IDENTIFIER);
    if (strcmp(t.lexeme, "if") == 0)
        t.type = TOKEN_IF;
    else if (strcmp(t.lexeme, "None") == 0)
        t.type = TOKEN_NONE;
    else if (
        strcmp(t.lexeme, "true") == 0
    )
        t.type = TOKEN_TRUE;
    else if (strcmp(t.lexeme, "false") == 0)
        t.type = TOKEN_FALSE;
    else if (strcmp(t.lexeme, "for") == 0)
        t.type = TOKEN_FOR;
    else if (strcmp(t.lexeme, "loop") == 0)
        t.type  = TOKEN_LOOP;
    else if (strcmp(t.lexeme, "elseif") == 0)
        t.type = TOKEN_ELSEIF;
    else if (strcmp(t.lexeme, "else") == 0)
        t.type = TOKEN_ELSE;
    else if (strcmp(t.lexeme, "then") == 0)
        t.type = TOKEN_THEN;
    else if (strcmp(t.lexeme, "let") == 0)
        t.type = TOKEN_LET;
    else if (strcmp(t.lexeme, "not") == 0)
        t.type = TOKEN_NOT;
    else if (strcmp(t.lexeme, "of") == 0)
        t.type = TOKEN_OF;
    else if (strcmp(t.lexeme, "using") == 0)
        t.type = TOKEN_USING;
    else if (strcmp(t.lexeme, "enum") == 0)
        t.type = TOKEN_ENUM;
    else if (strcmp(t.lexeme, "as") == 0)
        t.type = TOKEN_AS;
    else if (strcmp(t.lexeme, "fn") == 0)
        t.type = TOKEN_FN;
    else if (strcmp(t.lexeme, "assert") == 0)
        t.type = TOKEN_ASSERT;
    else if (strcmp(t.lexeme, "in") == 0)
        t.type = TOKEN_IN;
    else if (strcmp(t.lexeme, "or") == 0)
        t.type = TOKEN_OR;
    else if (strcmp(t.lexeme, "and") == 0)
        t.type = TOKEN_AND;
    else if (strcmp(t.lexeme, "is") == 0)
        t.type = TOKEN_IS;
    else if (strcmp(t.lexeme, "do") == 0)
        t.type = TOKEN_DO;
    else if (strcmp(t.lexeme, "while") == 0)
        t.type = TOKEN_WHILE;
    else if (strcmp(t.lexeme, "return") == 0)
        t.type = TOKEN_RETURN;
    else if (strcmp(t.lexeme, "break") == 0)
        t.type = TOKEN_BREAK;
    else if (strcmp(t.lexeme, "continue") == 0)
        t.type = TOKEN_CONTINUE;
    else if (strcmp(t.lexeme, "class") == 0)
        t.type = TOKEN_CLASS;
    else if (strcmp(t.lexeme, "const") == 0)
        t.type = TOKEN_CONST;
    else if (strcmp(t.lexeme, "struct") == 0)
        t.type = TOKEN_STRUCT;
    else if (strcmp(t.lexeme, "match") == 0)
        t.type = TOKEN_MATCH;
    else if (strcmp(t.lexeme, "println") == 0)
        t.type = TOKEN_PRINTLN;
    return t;
}


joan_token_t make_token(joan_lexer_t* l, J_TokenType type)
{
    joan_token_t t;
    t.type = type;
    int len = l->curr - l->start;
    char* copy = malloc(len + 1);
    memcpy(copy, l->start, len);
    copy[len] = '\0';
    t.lexeme = copy;
    int* i = malloc(sizeof(int));
    *i = 0;
    t.v = i;
    t.line = l->line;
    t.column = l->column;
    return t;
}

joan_token_t make_comment(joan_lexer_t* l)
{
    l->curr += 2;
    l->start = l->curr;
    while(peek(l) != '\n' && !at_end(l))
        advance(l);
    return make_token(l, TOKEN_COMMENT);
}
joan_token_t next_token(joan_lexer_t* l)
{
    strip_ws(l);
    l->start = l->curr;
    if (at_end(l)) return make_token(l, TOKEN_EOF);
    char c = advance(l);
    if (
        isdigit(c) || 
        (c == '.' && isdigit(isdigit(peek(l))))
    ) return number(l);
    if (isalnum(c) || c == '_') return identifier(l);
    switch (c)
    {
        case '\n': // Don't need it but i will keep it for now.
            return make_token(l, TOKEN_NEWLINE);
        case '(':
            return make_token(l, TOKEN_LPARN);
        case ')':
            return make_token(l, TOKEN_RPARN);
        case '{':
            return make_token(l, TOKEN_LBRACE);
        case '}':
            return make_token(l, TOKEN_RBRACE);
        case '[':
            return make_token(l, TOKEN_LBRACKET);
        case ']':
            return make_token(l, TOKEN_RBRACKET);
        case ':':
            if (peek(l) == ':')
            {
                advance(l);
                return make_token(l, TOKEN_SETTER);
            }
            return make_token(l, TOKEN_COLON);
        case '\\':
            return make_token(l, TOKEN_BSLASH);
        case ',':
            return make_token(l, TOKEN_COMMA);
        case '"':
            return token_string(l);
        case '\'':
            return token_string(l);
        case '`':
            return token_string(l);
        case '.':
            if (peek(l) == '.')
            {
                advance(l);
                return make_token(l, TOKEN_RANGE);
            }
            return make_token(l, TOKEN_DOT);
        case '+':
            if (peek(l) == '=')
            {
                advance(l);
                return make_token(l, TOKEN_APLUS);
            }
            return make_token(l, TOKEN_PLUS);
        case '-':
            if (peek(l) == '=')
            {
                advance(l);
                return make_token(l, TOKEN_AMINUS);
            }
            return make_token(l, TOKEN_MINUS);
        case '*':
            if (peek(l) == '=')
            {
                advance(l);
                return make_token(l, TOKEN_ASTAR);
            }
            return make_token(l, TOKEN_STAR);
        case '=':
            if (peek(l) == '='){
                advance(l);
                return make_token(l, TOKEN_EQEQ);
            } else if (peek(l) == '>')
            {
                advance(l);
                return make_token(l, TOKEN_EXR);
            }
            return make_token(l, TOKEN_EQUAL);
        case '>':
            if (peek(l) == '='){
                advance(l);
                return make_token(l, TOKEN_GTE);
            } else if (peek(l) == '>'){
                advance(l);
                if (peek(l) == '=')
                {
                    advance(l);
                    return make_token(l, TOKEN_ARSHIFT);
                }
                return make_token(l, TOKEN_RSHIFT);
            }
            return make_token(l, TOKEN_GT);
        case '^':
            if (peek_advance(l, '='))
                return make_token(l, TOKEN_ABITAC);
            return make_token(l, TOKEN_BITAC);
        case '<':
            if (peek(l) == '='){
                advance(l);
                return make_token(l, TOKEN_LTE);
            }else if (peek(l) == '<'){
                advance(l);
                if (peek(l) == '=')
                {
                    advance(l);
                    return make_token(l, TOKEN_ALSHIFT);
                }
                return make_token(l, TOKEN_LSHIFT);
            }
            return make_token(l, TOKEN_LT);
        case '%':
            if (peek(l) == '=')
            {
                advance(l);
                return make_token(l, TOKEN_APERCENTAGE);
            }
            return make_token(l, TOKEN_PERCENTAGE);
        case '!':
            if (peek(l) == '=')
            {
                advance(l);
                return make_token(l, TOKEN_NEQ);
            }
            return make_token(l, TOKEN_NOT);
        case '/':
            if (peek(l) == '/')
                return make_comment(l);
            else if (peek_advance(l, '='))
                return make_token(l, TOKEN_ASLASH);
            return make_token(l, TOKEN_SLASH);
        case '?':
            return make_token(l, TOKEN_QUESTION);
        case '@':
            return make_token(l, TOKEN_AT);
        case ';':
            return make_token(l, TOKEN_SIMICOLON);
        case '&':
            if (peek(l) == '&')
            {
                advance(l);
                return make_token(l, TOKEN_AND);
            } else if (peek_advance(l, '='))
                return make_token(l, TOKEN_ABITAND);
            return make_token(l, TOKEN_BITAND);
        case '|':
            if (peek(l) == '|')
            {
                advance(l);
                return make_token(l, TOKEN_OR);
            } else if (peek_advance(l, '='))
                return make_token(l, TOKEN_ABITOR);
            return make_token(l, TOKEN_BITOR);
        case '#':
            return make_token(l, TOKEN_HASH);
    }
    return make_token(l, TOKEN_ERROR);
}
