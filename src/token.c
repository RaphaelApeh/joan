#include <strings.h>
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>

#include "token.h"
#include "lexer.h"

#define CHECK_TOK(lex, str) ((strcmp((lex), (str))) == 0)

static JnToken make_error(joan_lexer_t* l, char* msg, ...);

#define KEYWORD_S struct {const char* keyword; J_TokenType token; }

KEYWORD_S Keywords[] = {
    // Keywords
    {"if", TOKEN_IF},
    {"elseif", TOKEN_ELSEIF},
    {"else", TOKEN_ELSE},
    {"fn", TOKEN_FN},
    {"struct", TOKEN_STRUCT},
    {"break", TOKEN_BREAK},
    {"continue", TOKEN_CONTINUE},
    {"return", TOKEN_RETURN},
    {"is", TOKEN_IS},
    {"in", TOKEN_IN},
    {"and", TOKEN_AND},
    {"or", TOKEN_OR},
    {"not", TOKEN_NOT},
    {"const", TOKEN_CONST}, // Deprecated: remove soon.
    {"let", TOKEN_LET}, // Deprecated: remove soon
    {"while", TOKEN_WHILE},
    {"for", TOKEN_FOR},
    {"loop", TOKEN_LOOP}, // Deprecated: remove soon
    {"then", TOKEN_THEN},
    {"println", TOKEN_PRINTLN}, // Deprecated: remove soon.
    {"class", TOKEN_CLASS},
    {"match", TOKEN_MATCH},

    // Type keywords
    {"None", TOKEN_NONE},
    {"true", TOKEN_TRUE},
    {"false", TOKEN_FALSE},
};


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

static bool equal(const char* src, const char* src2)
{
    return strcmp(src, src2) == 0;
}

JnToken clean_token(joan_lexer_t* l)
{
    JnToken t;
    do {
        t = next_token(l);
    } while (t.type == TOKEN_NEWLINE);
    return t;
}

JnToken number_token(joan_lexer_t* l)
{
    char* buff;
    bool is_float = false;
    bool prev_us = false;
    short base = 0;
    if (l->start[0] == '0')
    {
        if (peek(l) == 'x' || peek(l) == 'X')
        {
            base = 16;
            advance(l);
        }
        else if (peek(l) == 'b' || peek(l) == 'B')
        {
            base = 2;
            advance(l);
        }
        if (base)
        {
            if (base == 16)
            {
                if (!isxdigit(peek(l)))
                    return make_error(l, "token is not an hexadecimal.");
            } else
            {
                if (peek(l) != '0' && peek(l) != '1')
                    return make_error(l, "invalid digit '%c' in binary literal.", peek(l));
            }
            for (;;)
            {
                char c = peek(l);
                bool _valid = (
                    base == 16 && isxdigit(c))  || (base == 2 && (c == '0' || c == '1')
                );
                if (_valid)
                {
                    prev_us = false;
                    advance(l);
                    continue;
                }
                if (c == '_')
                {
                    if (prev_us)    return make_error(l, "consecutive '_' in int.");
                    prev_us = true;
                    advance(l);
                    continue;
                }
                if (isalnum((unsigned char)c))
                {
                    if (base == 2)
                    return make_error(l, "invalid digit '%c' in binary literal.", c);
                    else
                    return make_error(l, "invalid digit '%c' in hexdecimal literal.", c);
                }
                break;
            }
            if (l->curr[-1] == '_')
                return make_error(l, "'_' found in an integer.");
            JnToken t = make_token(l, TOKEN_INT);
            buff = rm_num_sep(t.lexeme);
            if (base == 2)
                t.i = strtoll(buff + 2, NULL, base);
            else
                t.i = strtoll(buff, NULL, base);
            free(buff);
            buff = NULL;
            return t;
        }
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
        JnToken _t = make_token(l, TOKEN_INT);
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
        is_float = true;
        advance(l);
        prev_us = false;
        while (isdigit(peek(l)) || peek(l) == '_')
        {
            if (peek(l) == '_')
            {
                if (prev_us)
                    return make_error(l, "consecutive '_' in number.");
                prev_us = true;
            }
            else prev_us = false;
            advance(l);
        }
        if (l->curr[-1] == '_') return make_error(l, "Float cannot end with '_'.");
    }
    if (is_float && (peek(l) == 'f' || peek(l) == 'F'))
    {
        advance(l);
    }    
    if (peek(l) == 'e' || peek(l) == 'E')
    {
        is_float = true;
        advance(l);
        if (peek(l) == '+' || peek(l) == '-')
            advance(l);
        if (!isdigit(peek(l))) return make_error(l, "expected digits after exponent.");
        prev_us = false;
        while (isdigit(peek(l)) || peek(l) == '_')
        {
            if (peek(l) == '_')
            {
                if (prev_us)    return make_error(l, "consecutive '_' in exponent.");
                prev_us = true;
            } else prev_us = false;
            advance(l);
        }
        if (l->curr[-1] == '_') return make_error(l, "exponent cannot end with '_'.");
    }
    if (is_float)
    {
        JnToken t = make_token(l, TOKEN_FLOAT);
        buff = rm_num_sep(t.lexeme);
        t.d = strtod(buff, NULL);
        free(buff);
        return t;
    }
    JnToken t = make_token(l, TOKEN_INT);
    buff = rm_num_sep(t.lexeme);
    t.i = strtol(buff, NULL, 10);
    free(buff);
    return t;
}

JnToken token_string(joan_lexer_t* l)
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
        JnToken t;
        t.type = TOKEN_ERROR;
        t.lexeme = "unterminated string literal.";
        t.v = NULL;
        t.line = l->line;
        t.column = l->column;
        return t;
    }
    JnToken t = make_token(l, TOKEN_STRING);
    char c = *l->curr++;
    l->column++;
    if (q != c)
        return make_token(l, TOKEN_ERROR);
    return t;   
}

static JnToken token_char(joan_lexer_t* l)
{
    char c;
    l->start = l->curr;
    JnToken t;
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

JnToken token_identifier(joan_lexer_t* l)
{
    while (isalnum(peek(l)) || peek(l) == '_') 
        advance(l);
    JnToken t = make_token(l, TOKEN_IDENTIFIER);

    for (int i = 0; i < (int)(sizeof(Keywords) / sizeof(Keywords[0])); ++i)
    {
        if (equal(t.lexeme, Keywords[i].keyword))
        {
            t.type = Keywords[i].token;
            break;
        }
    }
    return t;
}


static JnToken make_error(joan_lexer_t* l, char* msg, ...)
{
    JnToken t;
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

JnToken make_token(joan_lexer_t* l, J_TokenType type)
{
    JnToken t;
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

JnToken make_comment(joan_lexer_t* l)
{
    l->curr += 2;
    l->start = l->curr;
    while(peek(l) != '\n' && !at_end(l))
        advance(l);
    return make_token(l, TOKEN_COMMENT);
}

JnToken make_comment_block(joan_lexer_t* l)
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

JnToken next_token(joan_lexer_t* l)
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
    if (isalnum(c) || c == '_') return token_identifier(l);
    // TODO: redefine token.
    switch (c)
    {
        case '\n':
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
                if (peek_advance(l, '.'))
                    return make_token(l, TOKEN_ELLIPSIS);
                return make_token(l, TOKEN_RANGE);
            }
            return make_token(l, TOKEN_DOT);
        case '+':
            if (peek(l) == '=')
            {
                advance(l);
                return make_token(l, TOKEN_APLUS);
            } else if (peek_advance(l, '+'))
                return make_token(l, TOKEN_PLUS_PLUS);
            return make_token(l, TOKEN_PLUS);
        case '-':
            if (peek(l) == '=')
            {
                advance(l);
                return make_token(l, TOKEN_AMINUS);
            }

            if (peek_advance(l, '>'))
                return make_token(l, TOKEN_ARROW);
            return make_token(l, TOKEN_MINUS);
        case '*':
            if (peek(l) == '=')
            {
                advance(l);
                return make_token(l, TOKEN_AMUL);
            } else if (peek_advance(l, '*'))
            {
                if (peek_advance(l, '='))
                    return make_token(l, TOKEN_APOW);
                return make_token(l, TOKEN_POW);
            }
            return make_token(l, TOKEN_MUL);
        case '=':
            if (peek(l) == '='){
                advance(l);
                return make_token(l, TOKEN_EQEQ);
            } else if (peek(l) == '>')
            {
                advance(l);
                return make_token(l, TOKEN_DCOLON);
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
                return make_token(l, TOKEN_AXOR);
            return make_token(l, TOKEN_XOR);
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
        case '~':
            return make_token(l, TOKEN_TILDE);
        default:
            return make_error(l, "Invalid token '%c'.", c);
    }
    return make_error(l, "Invalid token");
}
