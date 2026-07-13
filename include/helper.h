#ifndef HELPER_H

#define HELPER_H
#include <stdint.h>
#include "arena.h"

#ifdef _WIN32
    #include <direct.h>

    #define getcwd _getcwd
#else
    #include <unistd.h>
#endif

typedef struct AST AST;
typedef uint64_t u64;
typedef struct JnObject JnObject;
typedef struct J_DArray_Obj J_DArray_Obj;

#define RESIZE_DOBJ(arr) (arr)->items = realloc((arr)->items, sizeof(*(arr)->items) * (arr)->capacity)

/* COLORS */
#define RESET "\x1b[0m"
#define RED "\x1b[31m"
#define GREEN "\x1b[31m"


#define JOAN_PATH getenv("JOAN_PATH")
#define JN_STD_PATH getenv("JN_STD_PATH")

#define PUSH_ITEM(arr, obj) do {\
    if ((arr)->size >= (arr)->capacity)\
    {                                   \
        (arr)->capacity *= 2;             \
        RESIZE_DOBJ(arr);                   \
    }                                        \
    (arr)->items[(arr)->size++] = (obj);    \
    } while(false)

#define MAX_MATCHES 5
#define MIN_SCORE 0.45f

struct FuzzMatch {
    char* word;
    float score;
};


int fuzzy_match(const char* word, char** list_words, int size, struct FuzzMatch* matches);


typedef struct case_o{
    AST* pattern;
    AST* block;
} case_o;

typedef struct case_t{
    case_o* cases;
    u64 capacity;
    u64 count;
} case_t;

typedef struct{
    AST* cond;
    AST* stmt;
} elif_node;

typedef struct elseif{
    elif_node* children;
    size_t capacity;
    size_t count;
}elseif;


struct J_DArray_Obj {
    void** items;
    size_t size;
    size_t capacity;
};

// Hash functions
unsigned long fnv_hash(const void* key, uint32_t h);
unsigned long djb2_hash(unsigned const char* str);

bool isnumber(JnObject* obj);
double tonumber(JnObject* obj);

void print_source_lines(char* source, int line, int column, int context);
void print_source_line(char* source, int line, int column);
bool file_exists(const char* filename);
void runtime_error(char* msg, ...);
void call_add_pos(AST* call, AST* arg);
case_t* init_case(Arena* arena);
void push_case(case_t* caseObj, AST* sub, AST* block);

elseif* elseif_init(void);
void elseif_add(elseif* elif, AST* block, AST* cond);

#endif