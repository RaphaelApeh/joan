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

static Jn_Token make_error(Jn_Lexer* l, char* msg, ...);

#define KEYWORD_S struct {const char* keyword; Jn_TokenType token; }

char* tok_string[] = {
    [TOK_EOF] = "TOK_EOF", 
    [TOK_NEWLINE] = "TOK_NEWLINE",
    [TOK_INT] = "TOK_INT",
    [TOK_FLOAT] = "TOK_FLOAT",
    [TOK_STRING] = "TOK_STRING",
    [TOK_CHAR] = "TOK_CHAR",
    [TOK_IDENT] = "TOK_IDENT",
    [TOK_LPARN] = "TOK_LPARN",
    [TOK_RPARN] = "TOK_RPARN",
    [TOK_LBRACE] = "TOK_LBRACE",
    [TOK_RBRACE] = "TOK_RBRACE",
    [TOK_LBRACKET] = "TOK_LBRACKET",
    [TOK_RBRACKET] = "TOK_RBRACKET",
    [TOK_PLUS] = "TOK_PLUS",
    [TOK_MINUS] = "TOK_MINUS",
    [TOK_MUL] = "TOK_MUL",
    [TOK_SLASH] = "TOK_SLASH",
    [TOK_POW] = "TOK_POW",
    [TOK_COMMA] = "TOK_COMMA",
    [TOK_PLUS_PLUS] = "TOK_PLUS_PLUS",
    [TOK_SEMICOLON] = "TOK_SEMICOLON",
    [TOK_DOT] = "TOK_QUOTE",
    [TOK_QUOTE] = "TOK_QUOTE",
    [TOK_SQUOTE] = "TOK_SQUOTE",
    [TOK_BSLASH] = "TOK_BSLASH",
    [TOK_GT_EQ] = "TOK_GT_EQ",
    [TOK_BITOR] = "TOK_BITOR",
    [TOK_BITAND] = "TOK_BITAND",
    [TOK_XOR] = "TOK_XOR",
    [TOK_HASH] = "TOK_HASH",
    [TOK_PI] = "TOK_PI",
    [TOK_AT] = "TOK_AT",
    [TOK_RANGE] = "TOK_RANGE",
    [TOK_EQ_GT] = "TOK_EQ_GT",
    [TOK_TILDE] = "TOK_TILDE",
    [TOK_ELLIPSIS] = "TOK_ELLIPSIS",
    [TOK_EQ_PLUS] = "TOK_EQ_PLUS",
    [TOK_EQ_MINUS] = "TOK_EQ_MINUS",
    [TOK_EQ_MUL] = "TOK_EQ_MUL",
    [TOK_EQ_RSHIFT] = "TOK_EQ_RSHIFT",
    [TOK_EQ_LSHIFT] = "TOK_EQ_LSHIFT",
    [TOK_EQ_PERCENTAGE] = "TOK_EQ_PERCENTAGE",
    [TOK_EQ_XOR] = "TOK_EQ_XOR",
    [TOK_EQ_SLASH] = "TOK_EQ_SLASH",
    [TOK_EQ_BITAND] = "TOK_EQ_BITAND",
    [TOK_EQ_BITOR] = "TOK_EQ_BITOR",
    [TOK_EQ_POW] = "TOK_EQ_POW",
    [TOK_EQUAL] = "TOK_EQUAL",
    [TOK_EQEQ] = "TOK_EQEQ",
    [TOK_GT] = "TOK_GT",
    [TOK_LT] = "TOK_LT",
    [TOK_GTE] = "TOK_GTE",
    [TOK_LTE] = "TOK_LTE",
    [TOK_NEQ] = "TOK_NEQ",
    [TOK_RSHIFT] = "TOK_RSHIFT",
    [TOK_LSHIFT] = "TOK_LSHIFT",
    [TOK_COLON] = "TOK_COLON",
    [TOK_WALRUS] = "TOK_WALRUS",
    [TOK_QUESTION] = "TOK_QUESTION",
    [TOK_PERCENTAGE] = "TOK_PERCENTAGE",
    [TOK_DCOLON] = "TOK_DCOLON",
    [TOK_NONE] = "TOK_NONE",
    [TOK_TRUE] = "TOK_TRUE",
    [TOK_FALSE] = "TOK_FALSE",
    [TOK_LET] = "TOK_LET",
    [TOK_IF] = "TOK_IF",
    [TOK_ELSEIF] = "TOK_ELSEIF",
    [TOK_ELSE] = "TOK_ELSE",
    [TOK_THEN] = "TOK_THEN",
    [TOK_FOR] = "TOK_FOR",
    [TOK_LOOP] = "TOK_LOOP",
    [TOK_WHILE] = "TOK_WHILE",
    [TOK_OR] = "TOK_OR",
    [TOK_IS] = "TOK_IS",
    [TOK_IS_NOT] = "TOK_IS_NOT",
    [TOK_IN] = "TOK_IN",
    [TOK_NOT] = "TOK_NOT",
    [TOK_AND] = "TOK_AND",
    [TOK_ARROW] = "TOK_ARROW",
    [TOK_FN] = "TOK_FN",
    [TOK_IMPORT] = "TOK_IMPORT",
    [TOK_ENUM] = "TOK_ENUM",
    [TOK_CONST] = "TOK_CONST",
    [TOK_RETURN] = "TOK_RETURN",
    [TOK_YIELD] = "TOK_YIELD",
    [TOK_RAISE] = "TOK_RAISE",
    [TOK_CLASS] = "TOK_CLASS",
    [TOK_STRUCT] = "TOK_STRUCT",
    [TOK_MATCH] = "TOK_MATCH",
    [TOK_CONTINUE] = "TOK_CONTINUE",
    [TOK_BREAK] = "TOK_BREAK",
    [TOK_COMMENT] = "TOK_COMMENT",
    [TOK_ERROR] = "TOK_ERROR",
};

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
    {"raise", TOK_RAISE},
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

static bool equal(const char* src, const char* src2){return strcmp(src, src2) == 0;}

Jn_Token clean_token(Jn_Lexer* l)
{
    Jn_Token t;
    do {
        t = next_token(l);
    } while (t.type == TOK_NEWLINE);
    return t;
}

Jn_Token number_token(Jn_Lexer* l)
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
            Jn_Token t = make_token(l, TOK_INT);
            buff = rm_num_sep(t.lexeme);
            if (base == 2)
                t.i = strtoll(buff + 2, NULL, base);
            else
                t.i = strtoll(buff, NULL, base);
            free(buff);
            buff = NULL;
            return t;
        }
        else if (isxdigit(peek(l)) || peek(l) == '_')
            advance(l);
        else 
        {
            Jn_Token t = make_token(l, TOK_INT);
            t.i = 0;
            return t;
        }
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
        Jn_Token _t = make_token(l, TOK_INT);
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
        Jn_Token t = make_token(l, TOK_FLOAT);
        buff = rm_num_sep(t.lexeme);
        t.d = strtod(buff, NULL);
        free(buff);
        return t;
    }
    Jn_Token t = make_token(l, TOK_INT);
    buff = rm_num_sep(t.lexeme);
    t.i = strtol(buff, NULL, 10);
    free(buff);
    return t;
}

Jn_Token token_string(Jn_Lexer* l)
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
        Jn_Token t;
        t.type = TOK_ERROR;
        t.lexeme = "unterminated string literal.";
        t.v = NULL;
        t.line = l->line;
        t.column = l->column;
        return t;
    }
    Jn_Token t = make_token(l, TOK_STRING);
    char c = *l->curr++;
    l->column++;
    if (q != c)
        return make_token(l, TOK_ERROR);
    return t;   
}

static Jn_Token token_char(Jn_Lexer* l)
{
    char c;
    l->start = l->curr;
    Jn_Token t;
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

Jn_Token token_identifier(Jn_Lexer* l)
{
    while (isalnum(peek(l)) || peek(l) == '_') 
        advance(l);
    Jn_Token t = make_token(l, TOK_IDENT);

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


static Jn_Token make_error(Jn_Lexer* l, char* msg, ...)
{
    Jn_Token t;
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

Jn_Token make_token(Jn_Lexer* l, Jn_TokenType type)
{
    Jn_Token t;
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

Jn_Token make_comment(Jn_Lexer* l)
{
    l->curr += 2;
    l->start = l->curr;
    while(peek(l) != '\n' && !at_end(l))
        advance(l);
    return make_token(l, TOK_COMMENT);
}

Jn_Token make_comment_block(Jn_Lexer* l)
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

Jn_Token next_token(Jn_Lexer* l)
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
                return make_token(l, TOK_DCOLON);
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
                return make_token(l, TOK_EQ_PLUS);
            } else if (peek_advance(l, '+'))
                return make_token(l, TOK_PLUS_PLUS);
            return make_token(l, TOK_PLUS);
        case '-':
            if (peek(l) == '=')
            {
                advance(l);
                return make_token(l, TOK_EQ_MINUS);
            }

            if (peek_advance(l, '>'))
                return make_token(l, TOK_ARROW);
            return make_token(l, TOK_MINUS);
        case '*':
            if (peek(l) == '=')
            {
                advance(l);
                return make_token(l, TOK_EQ_MUL);
            } else if (peek_advance(l, '*'))
            {
                if (peek_advance(l, '='))
                    return make_token(l, TOK_EQ_POW);
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
                return make_token(l, TOK_EQ_GT);
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
                    return make_token(l, TOK_EQ_RSHIFT);
                }
                return make_token(l, TOK_RSHIFT);
            }
            return make_token(l, TOK_GT);
        case '^':
            if (peek_advance(l, '='))
                return make_token(l, TOK_EQ_XOR);
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
                    return make_token(l, TOK_EQ_LSHIFT);
                }
                return make_token(l, TOK_LSHIFT);
            }
            return make_token(l, TOK_LT);
        case '%':
            if (peek(l) == '=')
            {
                advance(l);
                return make_token(l, TOK_EQ_PERCENTAGE);
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
                return make_token(l, TOK_EQ_SLASH);
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
                return make_token(l, TOK_EQ_BITAND);
            return make_token(l, TOK_BITAND);
        case '|':
            if (peek(l) == '|')
            {
                advance(l);
                return make_token(l, TOK_OR);
            } else if (peek_advance(l, '='))
                return make_token(l, TOK_EQ_BITOR);
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
