#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>

#ifdef _WIN32
    #define PATH_SEP '\\'
    #define PATH_LIST_SEP ';'
#else
    #define PATH_SEP '/'
    #define PATH_LIST_SEP ':'
#endif

static int hex_value(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';

    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;

    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;

    return -1;
}

static int hex_n(const char *s, int n, uint32_t *value)
{
    uint32_t v = 0;

    for (int i = 0; i < n; i++)
    {
        int x = hex_value(s[i]);

        if (x < 0)
            return 0;

        v = (v << 4) | (uint32_t)x;
    }

    *value = v;
    return 1;
}

bool strends(const char* str, const char* suf)
{
    int str_len = strlen(str), suf_len = strlen(suf);
    return (
        suf_len <= str_len && 
        strcmp(str + str_len - suf_len, suf) == 0
    );
}

char* str_esc(const char* str)
{
    if (NULL == str) return NULL;

    size_t len = 0;
    for (const unsigned char* s = (const unsigned char *)str; *s; s++)
    {
        switch (*s)
        {
            case '\\':
            case '\"':
            case '\n':
            case '\r':
            case '\t':
            case '\b':
            case '\f':
            case '\v':
                len += 2;
                break;
            default:
                if (!isprint(*s)) len += 4;
                else len += 1;
                break;
        }
    }
    char *esc = malloc(len + 1);
    if (esc == NULL) return NULL;
    char *d = esc;
    for (const unsigned char *s = (const unsigned char *)str;
         *s;
         s++)
    {
        switch (*s)
        {
            case '\\':
                *d++ = '\\';
                *d++ = '\\';
                break;

            case '\"':
                *d++ = '\\';
                *d++ = '"';
                break;

            case '\n':
                *d++ = '\\';
                *d++ = 'n';
                break;

            case '\r':
                *d++ = '\\';
                *d++ = 'r';
                break;

            case '\t':
                *d++ = '\\';
                *d++ = 't';
                break;

            case '\b':
                *d++ = '\\';
                *d++ = 'b';
                break;

            case '\f':
                *d++ = '\\';
                *d++ = 'f';
                break;

            case '\v':
                *d++ = '\\';
                *d++ = 'v';
                break;

            default:
                if (!isprint(*s))
                {
                    static const char hex[] = "0123456789abcdef";

                    *d++ = '\\';
                    *d++ = 'x';
                    *d++ = hex[(*s >> 4) & 0x0f];
                    *d++ = hex[*s & 0x0f];
                }
                else
                {
                    *d++ = (char)*s;
                }
                break;
        }
    }
    *d = '\0';
    return esc;
}

char* str_unesc(const char *str)
{
    if (str == NULL)
        return NULL;

    size_t input_len = strlen(str);

    char *out = malloc(input_len + 1);

    if (out == NULL)
        return NULL;

    const unsigned char *s = (const unsigned char *)str;
    unsigned char *d = (unsigned char *)out;

    while (*s)
    {
        if (*s != '\\')
        {
            *d++ = *s++;
            continue;
        }
        if (s[1] == '\0')
        {
            *d++ = '\\';
            break;
        }

        s++; // skip '\'

        switch (*s)
        {
            case '\\':
                *d++ = '\\';
                s++;
                break;

            case '\'':
                *d++ = '\'';
                s++;
                break;

            case '"':
                *d++ = '"';
                s++;
                break;

            case 'a':
                *d++ = '\a';
                s++;
                break;

            case 'b':
                *d++ = '\b';
                s++;
                break;

            case 'f':
                *d++ = '\f';
                s++;
                break;

            case 'n':
                *d++ = '\n';
                s++;
                break;

            case 'r':
                *d++ = '\r';
                s++;
                break;

            case 't':
                *d++ = '\t';
                s++;
                break;

            case 'v':
                *d++ = '\v';
                s++;
                break;

            case '0':
                *d++ = '\0';
                s++;
                break;

            case 'x':
                {
                uint32_t value;

                if (s[1] && s[2] && hex_n((const char *)s + 1, 2, &value))
                {
                    *d++ = (unsigned char)value;
                    s += 3;
                }
                else
                {
                    *d++ = '\\';
                    *d++ = 'x';
                    s++;
                }

                break;
            }

            default:
                *d++ = '\\';
                *d++ = *s++;
                break;
        }
    }

    *d = '\0';

    return out;
}

bool strstarts(const char* str, const char* pre)
{
    int str_len = strlen(str), pre_len = strlen(pre);
    return (
        pre_len <= str_len && 
        strncmp(str, pre, pre_len) == 0
    );
}

char* strrpl(const char* str, const char* old, const char* new)
{
    int count = 0, old_len = strlen(old), new_len = strlen(new);
    for (int i = 0; str[i] != '\0'; ++i)
    {
        if (strstr(&str[i], old) == &str[i])
        {
            count++;
            i += old_len - 1;
        }
    }

    char* buff = malloc(strlen(str) + count + (new_len - old_len + 1));
    if (NULL == buff) return NULL;

    char* ptr = buff;
    for (int i = 0; str[i];) 
    {
        if (strstr(&str[i], old) == &str[i])
        {
            memcpy(ptr, new, new_len);
            ptr += new_len;
            i += old_len;
        } else
            *ptr++ = str[i++];
    }
    *ptr = '\0';
    return buff;
}


char** strsplt(const char* str, char c, int* size)
{
    *size = 0;
    int len = strlen(str);
    for (int i = 0; i < len; ++i)
    {
        if (str[i] == c) (*size)++;
    }
    (*size)++;

    char **buff = malloc((*size + 1) * sizeof(char *) + len + 1);
    if (NULL == buff) return NULL;
    
    char* des = (char *)(buff + *size + 1);

    int idx = 0;
    const char* start = str;
    for (int i = 0; i <= len; ++i)
    {
        if (str[i] == c || str[i] == '\0')
        {
            int seg_len = &str[i] - start;
            buff[idx] = des;
            memcpy(des, start, seg_len);
            des[seg_len] = '\0';
            des += seg_len + 1;
            idx++;
            start = &str[i] + 1;
        }
    }
    buff[idx] = NULL;
    return buff;
}

char* strstrp(const char* str)
{
    if (NULL == str) return NULL;
    const char* s = str;
    char* buff;
    while (*s && isspace(*s)) s++;

    if (*s == '\0')
    {
        buff = malloc(sizeof(char)); // or 1
        if (NULL == buff) buff[0] = '\0';
        return buff;
    }
    const char* end = str + strlen(str) - 1;
    while (end > s && isspace(*end)) end--;

    int new_len = end - s + 1;
    buff = malloc(new_len + 1);
    if (buff == NULL) return NULL;
    memcpy(buff, s, new_len);
    buff[new_len] = 0;
    return buff;
}

bool strstrcmp(const char** src, const char* src2)
{
    for (int i = 0; src[i] != NULL; ++i)
        if (strcmp(src[i], src2) == 0)
            return true;
    return false;
}


int strpart(const char* str, char delim, char** left, char** right)
{
    if (!str || !right || !left) return -1;
    const char* delim_pos = strchr(str, delim);

    if (!delim_pos)
    {
        *left = strdup(str);
        *right = NULL;
        return 0;
    }
    size_t left_len = delim_pos - str;
    size_t right_len = strlen(delim_pos + 1);

    *left = malloc(left_len + 1);
    if (!*left) return -1;

    strncpy(*left, str, left_len);
    (*left)[left_len] = 0;
    
    if (right_len > 0)
    {
        *right = malloc(right_len + 1);
        if (!*right)
        {
            free(*left);
            *left = NULL;
            return -1;
        }
        strcpy(*right, delim_pos + 1);
    } else {
        *right = NULL;
    }
    return 0;
}

size_t strlen_utf8(const char* str)
{
    //https://stackoverflow.com/questions/32936646/getting-the-string-length-on-utf-8-in-c
    size_t len = 0;
    char c;
    while ((c = *str++) != '\0')
        len += (c & 0xC0) != 0x80;
    return len;
}

char* strpjoin(const char* str, const char* str2)
{
size_t str_len = strlen(str);
size_t str2_len = strlen(str2);
bool slash = str_len > 0 && str[str_len - 1] != '/' && str[str_len - 1] != '\\'; 
size_t size = str_len + str2_len + (slash ? 1 : 0) + 1; 
char* result = malloc(size); 
if (!result) return NULL; 
if (slash) snprintf(result, size, "%s%c%s", str, PATH_SEP, str2); 
else snprintf(result, size, "%s%s", str, str2); 
return result; 
}
