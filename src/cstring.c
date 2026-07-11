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


char* str_sp(const char* str)
{
    char* s = str;
    while (*s)
    {
        if (*s == '\\' && *s)
            s += 2;
        s++;
    }
    return strdup(s);
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