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

static JnToken make_error(Jn_Lexer* l, char* msg, ...);

#define KEYWORD_S struct {const char* keyword; JnTokenType token; }

KEYWORD_S Keywords[] = {
    // Keywords
    {"if", TOK_IF},
    {"elseif", TOK_ELSEIF},
    {"else", TOK_ELSE},
    {"fn", TOK_FN},
    {"struct", TOK_STRUCT},
    {"break", TOK_BREAK},
    {"continue", TOK_CONTINUE},
    {"return", TOK_RETURN},
    {"is", TOK_IS},
    {"in", TOK_IN},
    {"and", TOK_AND},
    {"or", TOK_OR},
    {"not", TOK_NOT},
    {"const", TOK_CONST},
    {"let", TOK_LET},
    {"while", TOK_WHILE},
    {"for", TOK_FOR},
    {"loop", TOK_LOOP}, // Deprecated: remove soon
    {"then", TOK_THEN},
    {"yield", TOK_YIELD},
    {"class", TOK_CLASS},
    {"match", TOK_MATCH},

    // Type keywords
    {"None", TOK_NONE},
    {"true", TOK_TRUE},
    {"false", TOK_FALSE},
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

JnToken clean_token(Jn_Lexer* l)
{
    JnToken t;
    do {
        t = next_token(l);
    } while (t.type == TOK_NEWLINE);
    return t;
}

JnToken number_token(Jn_Lexer* l)
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
            JnToken t = make_token(l, TOK_INT);
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
        JnToken _t = make_token(l, TOK_INT);
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
    } else if (peek(l) == 'f' || peek(l) == 'F')
        return make_error(l, "Found a 'f' suffix in a nor float literal.");
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
        JnToken t = make_token(l, TOK_FLOAT);
        buff = rm_num_sep(t.lexeme);
        t.d = strtod(buff, NULL);
        free(buff);
        return t;
    }
    JnToken t = make_token(l, TOK_INT);
    buff = rm_num_sep(t.lexeme);
    t.i = strtol(buff, NULL, 10);
    free(buff);
    return t;
}

JnToken token_string(Jn_Lexer* l)
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
        t.type = TOK_ERROR;
        t.lexeme = "unterminated string literal.";
        t.v = NULL;
        t.line = l->line;
        t.column = l->column;
        return t;
    }
    JnToken t = make_token(l, TOK_STRING);
    char c = *l->curr++;
    l->column++;
    if (q != c)
        return make_token(l, TOK_ERROR);
    return t;   
}

static JnToken token_char(Jn_Lexer* l)
{
    char c;
    l->start = l->curr;
    JnToken t;
    if (*l->curr == '\0')
    {
        t.type = TOK_ERROR;
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
            t.type = TOK_ERROR;
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
        t.type = TOK_ERROR;
        t.lexeme = strdup("char literal contains more than one character.");
        t.v = NULL;
        t.line = l->line;
        t.column = l->column;
        return t;
    }
    advance(l);
    t = make_token(l, TOK_CHAR);
    t.c = c;
    return t;
}

JnToken token_identifier(Jn_Lexer* l)
{
    while (isalnum(peek(l)) || peek(l) == '_') 
        advance(l);
    JnToken t = make_token(l, TOK_IDENT);

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


static JnToken make_error(Jn_Lexer* l, char* msg, ...)
{
    JnToken t;
    char buffer[256];
    va_list arg; va_start(arg, msg);
    vsnprintf(buffer, sizeof(buffer), msg, arg);
    va_end(arg);

    t.type = TOK_ERROR;
    t.lexeme = strdup(buffer);
    t.v = NULL;
    t.line = l->line;
    t.column = l->column;
    return t;
}

JnToken make_token(Jn_Lexer* l, JnTokenType type)
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

JnToken make_comment(Jn_Lexer* l)
{
    l->curr += 2;
    l->start = l->curr;
    while(peek(l) != '\n' && !at_end(l))
        advance(l);
    return make_token(l, TOK_COMMENT);
}

JnToken make_comment_block(Jn_Lexer* l)
{
    l->curr += 2;
    l->start = l->curr;
    advance(l);
    while (!(peek(l) == '*' && peek_next(l) == '/') && !at_end(l))
        advance(l);
    advance(l);
    advance(l);
    return make_token(l, TOK_COMMENT);
}

JnToken next_token(Jn_Lexer* l)
{
    strip_ws(l);
    l->start = l->curr;
    if (at_end(l)) return make_token(l, TOK_EOF);
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
            return make_token(l, TOK_NEWLINE);
        case '(':
            return make_token(l, TOK_LPARN);
        case ')':
            return make_token(l, TOK_RPARN);
        case '{':
            return make_token(l, TOK_LBRACE);
        case '}':
            return make_token(l, TOK_RBRACE);
        case '[':
            return make_token(l, TOK_LBRACKET);
        case ']':
            return make_token(l, TOK_RBRACKET);
        case ':':
            if (peek(l) == ':')
            {
                advance(l);
                return make_token(l, TOK_SETTER);
            }
            if (peek_advance(l, '='))
                return make_token(l, TOK_WALRUS);
            return make_token(l, TOK_COLON);
        case '\\':
            return make_token(l, TOK_BSLASH);
        case ',':
            return make_token(l, TOK_COMMA);
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
                    return make_token(l, TOK_ELLIPSIS);
                return make_token(l, TOK_RANGE);
            }
            return make_token(l, TOK_DOT);
        case '+':
            if (peek(l) == '=')
            {
                advance(l);
                return make_token(l, TOK_APLUS);
            } else if (peek_advance(l, '+'))
                return make_token(l, TOK_PLUS_PLUS);
            return make_token(l, TOK_PLUS);
        case '-':
            if (peek(l) == '=')
            {
                advance(l);
                return make_token(l, TOK_AMINUS);
            }

            if (peek_advance(l, '>'))
                return make_token(l, TOK_ARROW);
            return make_token(l, TOK_MINUS);
        case '*':
            if (peek(l) == '=')
            {
                advance(l);
                return make_token(l, TOK_AMUL);
            } else if (peek_advance(l, '*'))
            {
                if (peek_advance(l, '='))
                    return make_token(l, TOK_APOW);
                return make_token(l, TOK_POW);
            }
            return make_token(l, TOK_MUL);
        case '=':
            if (peek(l) == '='){
                advance(l);
                return make_token(l, TOK_EQEQ);
            } else if (peek(l) == '>')
            {
                advance(l);
                return make_token(l, TOK_DCOLON);
            }
            return make_token(l, TOK_EQUAL);
        case '>':
            if (peek(l) == '='){
                advance(l);
                return make_token(l, TOK_GTE);
            } else if (peek(l) == '>'){
                advance(l);
                if (peek(l) == '=')
                {
                    advance(l);
                    return make_token(l, TOK_ARSHIFT);
                }
                return make_token(l, TOK_RSHIFT);
            }
            return make_token(l, TOK_GT);
        case '^':
            if (peek_advance(l, '='))
                return make_token(l, TOK_AXOR);
            return make_token(l, TOK_XOR);
        case '<':
            if (peek(l) == '='){
                advance(l);
                return make_token(l, TOK_LTE);
            }else if (peek(l) == '<'){
                advance(l);
                if (peek(l) == '=')
                {
                    advance(l);
                    return make_token(l, TOK_ALSHIFT);
                }
                return make_token(l, TOK_LSHIFT);
            }
            return make_token(l, TOK_LT);
        case '%':
            if (peek(l) == '=')
            {
                advance(l);
                return make_token(l, TOK_APERCENTAGE);
            }
            return make_token(l, TOK_PERCENTAGE);
        case '!':
            if (peek(l) == '=')
            {
                advance(l);
                return make_token(l, TOK_NEQ);
            }
            return make_token(l, TOK_NOT);
        case '/':
            if (peek(l) == '/')
                return make_comment(l);
            else if (peek_advance(l, '='))
                return make_token(l, TOK_ASLASH);
            else if (peek(l) == '*')
                return make_comment_block(l);
            return make_token(l, TOK_SLASH);
        case '?':
            return make_token(l, TOK_QUESTION);
        case '@':
            return make_token(l, TOK_AT);
        case ';':
            return make_token(l, TOK_SEMICOLON);
        case '&':
            if (peek(l) == '&')
            {
                advance(l);
                return make_token(l, TOK_AND);
            } else if (peek_advance(l, '='))
                return make_token(l, TOK_ABITAND);
            return make_token(l, TOK_BITAND);
        case '|':
            if (peek(l) == '|')
            {
                advance(l);
                return make_token(l, TOK_OR);
            } else if (peek_advance(l, '='))
                return make_token(l, TOK_ABITOR);
            return make_token(l, TOK_BITOR);
        case '#':
            return make_token(l, TOK_HASH);
        case '~':
            return make_token(l, TOK_TILDE);
        default:
            return make_error(l, "Invalid token '%c'.", c);
    }
    return make_error(l, "Invalid token");
}
