#include <sys/stat.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include "object.h"
#include "helper.h"
#include "ast.h"

#define CHAR_EQUAL(a, b) (tolower((a)) == tolower((b)))

static int levenshtein(const char* str1, const char* str2)
{
    int str1_len = strlen(str1), str2_len = strlen(str2);

    int matrix[str1_len + 1][str2_len + 1];

    for (int i = 0; i <= str1_len; ++i)
        matrix[i][0] = i;
    
    for (int i = 0; i <= str2_len; ++i)
        matrix[0][i] = i;

    for (int i = 1; i <= str1_len; ++i)
    {
        for (int j = 1; j <= str2_len; ++j)
        {
            int cost = CHAR_EQUAL(str1[i - 1], str2[j - 1]) ? 0 : 1;
            int del = matrix[i - 1][j] + 1;
            int ins = matrix[i][j - 1] + 1;
            int sub = matrix[i - 1][j - 1] + cost;
            int min = del;
            if (ins < min)
                min = ins;
            if (sub < min)
                min = sub;

            matrix[i][j] = min;
        }
    }
    return matrix[str1_len][str2_len];
}

static float prefix_bonus(const char* a, const char* b)
{
    int n = 0;
    while(*a && *b)
    {
        if (tolower(*a) != tolower(*b))
            break;
        n++; a++; b++;
    }
    return n * 0.08f;
}

static float length_score(const char* a, const char* b)
{
    int len_a = strlen(a), len_b = strlen(b);

    int diff = abs(len_a - len_b);
    int max = len_a > len_b ? len_a: len_b;
    return 1.0f - ((float) diff / max);
}


static float similarity_score(const char* a, const char* b)
{
    int dist = levenshtein(a, b);

    int max_len = strlen(a);
    if (strlen(b) > max_len)
        max_len = strlen(b);

    float e_score = 1.0f - ((float)dist/max_len);
    float prefix = prefix_bonus(a, b);

    float len_score = length_score(a, b);
    float final = (e_score * 0.7f) + (len_score * 0.2f) * (prefix);

    if (final > 1)
        final = 1;
    return final;
}

static int cmp_match(const void* a, const void* b)
{
    struct FuzzMatch *m1 = (struct FuzzMatch *)a;
    struct FuzzMatch *m2 = (struct FuzzMatch *)b;
    if (m2->score > m1->score)
        return 1;
    if (m2->score < m1->score)
        return -1;
    return 0;
}

unsigned long djb2_hash(unsigned char* str)
{
    int c;
    unsigned long hash = 5281;
    while (c = *str++)
        hash = ((hash << 5) + hash) + c;
    return hash;
}

unsigned long fnv_hash(const void* key, uint32_t h)
{
    h ^= 2166136261UL;
    const uint8_t* d = (const uint8_t*)key;
    for (int i = 0; d[i] != '\0'; ++i)
    {
        h ^= d[i];
        h *= 16777619;
    }
    return h;
}

bool isnumber(JnObject* obj)
{
    if (NULL == obj) return false;
    if (obj->type == INT_TYPE || obj->type == FLOAT_TYPE || _JN_CHECK_TYPE(obj, CHAR_TYPE))
        return true;
    return false;
}

double tonumber(JnObject* obj)
{
    if (obj->type == INT_TYPE)
        return (double)obj->int32;
    if (obj->type == CHAR_TYPE)
        return (double)JN_AS_CHAR(obj);
    return obj->float32;
}

void runtime_error(char* msg, ...)
{
    va_list arg;
    va_start(arg, msg);
    vfprintf(stderr, msg, arg);
    va_end(arg);
    exit(72);
}


case_t* init_case(Arena* arena)
{
    case_t* caseObj = arena_alloc(arena, sizeof(case_t));
    caseObj->count = 0;
    caseObj->capacity = 100;
    caseObj->cases = arena_alloc(
        arena, sizeof(case_o) * caseObj->capacity
    );
    return caseObj;
}

void push_case(case_t* caseObj, AST* sub, AST* block)
{
    if (NULL == caseObj) return;
    if (caseObj->count >= caseObj->capacity)
    {
        caseObj->capacity *= 2;
        caseObj->cases = realloc(caseObj->cases, sizeof(case_o) * caseObj->capacity);
    }
    caseObj->cases[caseObj->count++] = (case_o){.pattern = sub, .block = block};
}

void call_add_pos(AST* call, AST* arg)
{
    call->call.pos_args[call->call.pos_count++] = arg;
}

param_t* param_init(void)
{
    param_t* param = malloc(sizeof(param));
    param->count = 0;
    param->capacity = 10;
    param->params = malloc(sizeof(param_o) * 10);
    return param;
}

void param_add(param_t* param, const char* ident, AST* value)
{
    if (NULL == param)
        perror("param is NULL");
    if (param->count >= param->capacity)
    {
        param->capacity *= 2;
        param->params = realloc(param->params, sizeof(param_o) * param->capacity);
    }
    param->params[param->count++] = (param_o){
        .ident = (char *)ident, .ast = value
    };
}

elseif* elseif_init(void)
{
    elseif* elif = malloc(sizeof(elseif));
    if (elif == NULL)
        perror("Memory allocation failed.");
    elif->count = 0;
    elif->capacity = 10;
    elif->children = malloc(
        sizeof(elif_node) * elif->capacity
    );
    return elif;
}

void elseif_add(elseif* elif, AST* block, AST* cond)
{
    if (elif->count >= elif->capacity)
    {
        elif->capacity *= 2;
        elif->children = realloc(
            elif->children,
            sizeof(elif_node) * elif->capacity
        );
    }
    elif->children[elif->count++] = (elif_node){
        .cond = cond,
        .stmt = block,
    };
}

void print_source_lines(char* source, int line, int column, int context)
{
    if (line < 1)   return;
    char* p = source;
    int cur_line = 1;

    while (*p)
    {
        char* line_s = p;
        while (*p && *p != '\n') p++;
        char* line_end = p;
        if (cur_line >= line - context && cur_line <= line + context)
        {
            printf("%4d| ", cur_line);
            fwrite(line_s, 1, line_end - line_s, stdout);
            putchar('\n');
            if (cur_line == line)
            {
                printf("   | ");
                for (int i = 1; i < column; ++i)    putchar(' ');
                printf("^^\n");
            }

        }
        if (*p == '\n') p++;
        cur_line++;
        if (cur_line > line + context)
            break;
    }
}

void print_source_line(char* source, int line, int column)
{
    
    char* p = source;
    int current = 1;

    while (*p && current < line)
    {
        if (*p == '\n')
            current++;
        p++;
    }
    char* s = p;
    while (*p && *p != '\n')
        p++;

    printf("%4d| ", line);
    fwrite(s, 1, p - s, stdout);
    putchar('\n');
    printf("   | ");
    for (int i = 1; i < column; ++i)
        putchar(' ');
    printf("^\n");
}

bool file_exists(const char* filename)
{
    struct stat st;
    return stat(filename, &st) == 0;
}


int fuzzy_match(const char* word, char** list_words, int size, struct FuzzMatch* matches)
{
    int found = 0;
    for (int i = 0; i < size; ++i)
    {
        float score = similarity_score(word, list_words[i]);
        if (score < MIN_SCORE)
            continue;
        matches[found].word = list_words[i];
        matches[found].score = score;
        found++;
    }
    qsort(matches, found, sizeof(struct FuzzMatch), cmp_match);
    if (found > MAX_MATCHES)
        found = MAX_MATCHES;
    return found;
}