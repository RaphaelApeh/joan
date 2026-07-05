#include <strings.h>
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include "token.h"
#include "lexer.h"

#define CHECK_TOK(lex, str) ((strcmp((lex), (str))) == 0)

static joan_token_t make_error(joan_lexer_t* l, char* msg, ...);

static bool peek_advance(joan_lexer_t* l, char c)
{
    if (peek(l) != c)
        return false;
    advance(l);
    return true;
}

static char* rm_num_sep(const char* s)
{
    size_t len = strlen(s);
    char* out = malloc(sizeof(char) * (len + 1));
    char* d = out;
    while (*s)
    {
        if (*s != '_')
            *d++ = *s;
        s++;
    }
    *d = '\0';
    return out;
}

static bool equal(char* src, char* src2)
{
    return strcmp(src, src2) == 0;
}

joan_token_t clean_token(joan_lexer_t* l)
{
    joan_token_t t;
    do {
        t = next_token(l);
    } while (t.type == TOKEN_NEWLINE);
    return t;
}

joan_token_t number_token(joan_lexer_t* l)
{
    char* buff;
    bool prev_us = false;
    if (l->start[0] == '0' && (peek(l) == 'x' || peek(l) == 'X'))
    {
        advance(l);
        if (!isxdigit(peek(l)))
            return make_error(l, "token is not an hex.");
        
        while(isxdigit(peek(l)) || peek(l) == '_')
        {
            if (peek(l) == '_')
            {
                if (prev_us)
                    return make_error(l, "consecutive '_' in int.");
                prev_us = true;
            } else 
                prev_us = false;
            advance(l);
        }
        if (l->curr[-1] == '_')
            return make_error(l, "int cannot end with '_'.");
        joan_token_t _t = make_token(l, TOKEN_INT);
        buff = rm_num_sep(_t.lexeme);
        _t.i = strtol(buff, NULL, 0);
        free(buff);
        return _t;
    }
    prev_us = false; // restore default
    while(isdigit(peek(l)) || peek(l) == '_') 
    {
        if (peek(l) == '_')
        {
            if (prev_us)
                return make_error(l, "consecutive '_' in int.");
            prev_us = true;
        } else
            prev_us = false;
        advance(l);
    }
    if (l->curr[-1] == '_')
        return make_error(l, "int cannot end with '_'.");
    if (peek(l) == '.' && isdigit(peek_next(l)))
    {
        advance(l);
        prev_us = false;
        while (isdigit(peek(l)) || peek(l) == '_' || peek(l) == 'f')
        {
            if (peek(l) == '_')
            {
                if (prev_us)
                    return make_error(l, "consecutive '_' in int.");
                prev_us = true;
            } else 
                prev_us = false;
            advance(l);
        }
        if (l->curr[-1] == '_')
            return make_error(l, "float cannot end with '_'.");
        joan_token_t t = make_token(l, TOKEN_FLOAT);
        // *d = atof(t.lexeme);
        buff = rm_num_sep(t.lexeme);
        t.d = strtod(buff, NULL);
        free(buff);
        return t;
    }
    joan_token_t t = make_token(l, TOKEN_INT);
    // *i = atoi(t.lexeme);
    buff = rm_num_sep(t.lexeme);
    t.i = strtol(buff, NULL, 10);
    free(buff);
    return t;
}

joan_token_t token_string(joan_lexer_t* l)
{
    l->start = l->curr;
    char q = l->curr[-1];
    while (*l->curr && *l->curr != q)
    {
        if (*l->curr == '\\' && *l->curr)
        {
            l->curr += 2;
            l->column += 2;
        }
        else
        {
            l->curr++;
            l->column++;
        }
    }
    if (*l->curr == '\0')
    {
        joan_token_t t;
        t.type = TOKEN_ERROR;
        t.lexeme = "unterminated string literal.";
        t.v = NULL;
        t.line = l->line;
        t.column = l->column;
        return t;
    }
    joan_token_t t = make_token(l, TOKEN_STRING);
    char c = *l->curr++;
    l->column++;
    if (q != c)
        return make_token(l, TOKEN_ERROR);
    return t;   
}

static joan_token_t token_char(joan_lexer_t* l)
{
    char c;
    l->start = l->curr;
    joan_token_t t;
    if (*l->curr == '\0')
    {
        t.type = TOKEN_ERROR;
        t.lexeme = strdup("unterminated char literal.");
        t.line = l->line;
        t.column = l->column;
        t.v = NULL;
        return t;
    }
    if (*l->curr == '\\')
    {
        advance(l);
        switch(*l->curr)
        {
            case 'n': c = '\n'; break;
            case 't': c = '\t'; break;
            case 'r': c = '\r'; break;
            case '\\': c = '\\'; break;
            case '\'': c = '\''; break;
            case '0': c = '\0'; break;
            default:
            t.type = TOKEN_ERROR;
            t.lexeme = strdup("invalid escape sequence.");
            t.v = NULL;
            t.line = l->line;
            t.column = l->column;
            return t;
        }
        advance(l);
    }else {
        c = *l->curr;
        advance(l);
    }
    if (*l->curr != '\'')
    {
        t.type = TOKEN_ERROR;
        t.lexeme = strdup("char literal contains more than one character.");
        t.v = NULL;
        t.line = l->line;
        t.column = l->column;
        return t;
    }
    advance(l);
    t = make_token(l, TOKEN_CHAR);
    t.c = c;
    return t;
}

joan_token_t identifier(joan_lexer_t* l)
{
    joan_token_t tmp;
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
    else if(strcmp(t.lexeme, "not_in") == 0)
        t.type = TOKEN_NOT_IN;
    else if (strcmp(t.lexeme, "not") == 0)
        t.type = TOKEN_NOT;
    else if (strcmp(t.lexeme, "of") == 0)
        t.type = TOKEN_OF;
    else if (equal(t.lexeme, "import"))
        t.type = TOKEN_IMPORT;    
    else if (strcmp(t.lexeme, "enum") == 0)
        t.type = TOKEN_ENUM;
    else if (strcmp(t.lexeme, "as") == 0)
        t.type = TOKEN_AS;
    else if (strcmp(t.lexeme, "fn") == 0)
        t.type = TOKEN_FN;
    else if (strcmp(t.lexeme, "assert") == 0)
        t.type = TOKEN_ASSERT;
    else if (equal(t.lexeme, "c_define"))
        t.type = TOKEN_DEFINE;
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


static joan_token_t make_error(joan_lexer_t* l, char* msg, ...)
{
    joan_token_t t;
    char buffer[256];
    va_list arg; va_start(arg, msg);
    vsnprintf(buffer, sizeof(buffer), msg, arg);
    va_end(arg);

    t.type = TOKEN_ERROR;
    t.lexeme = strdup(buffer);
    t.v = NULL;
    t.line = l->line;
    t.column = l->column;
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
    t.v = NULL;
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

joan_token_t make_comment_block(joan_lexer_t* l)
{
    l->curr += 2;
    l->start = l->curr;
    advance(l);
    while (!(peek(l) == '*' && peek_next(l) == '/') && !at_end(l))
        advance(l);
    advance(l);
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
        ( c == '.' && isdigit(peek(l)) )
    )
    return number_token(l);
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
            if (peek_advance(l, '='))
                return make_token(l, TOKEN_WALRUS);
            return make_token(l, TOKEN_COLON);
        case '\\':
            return make_token(l, TOKEN_BSLASH);
        case ',':
            return make_token(l, TOKEN_COMMA);
        case '"':
            return token_string(l);
        case '\'':
            return token_char(l);
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
            } else if (peek_advance(l, '*'))
                return make_token(l, TOKEN_POW);
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
            else if (peek(l) == '*')
                return make_comment_block(l);
            return make_token(l, TOKEN_SLASH);
        case '?':
            return make_token(l, TOKEN_QUESTION);
        case '@':
            return make_token(l, TOKEN_AT);
        case ';':
            return make_token(l, TOKEN_SEMICOLON);
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
        default:
            return make_error(l, "Invalid token '%c'.", c);
    }
    return make_error(l, "Invalid token");
}
