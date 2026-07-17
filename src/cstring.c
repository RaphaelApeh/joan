#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>


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
    // TODO: add Python-type of string escape
    if (NULL == str) return NULL;

    size_t esc_len = 0;
    char* s = (char *)str;
    while (*s)
    {
        switch (*s)
        {
        case '\n':
        case '\'':
        case '\t':
        case '\r':
        case '\f':
        case '\v':
        case '\0':
        case '\\':
            esc_len += 2; break;
        default:
            esc_len += 1; break;
        }
        s++;
    }
    char* esc = malloc(esc_len + 1);
    if (NULL == esc) return NULL;
    char* d = esc;
    s = (char *)str;

    while (*s)
    {
        switch (*s)
        {
            case '\\':  *d++ = '\\'; break;
            case '\n':  *d++ = 'n'; break;
            case '\r':  *d++ = 'r'; break;
            case '\t':  *d++ = 't'; break;
            case '\b':  *d++ = 'b'; break;
            case '\f':  *d++ = 'f'; break;
            case '\v':  *d++ = 'v'; break;
            case '\0':  *d++ = '0'; break;
            default:
                *d++ = *s; break;
        }
        s++;
    }
    *d = '\0';
    return esc;
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