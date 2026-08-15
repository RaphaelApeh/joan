#ifndef HELPER_H

#define HELPER_H
#include <stdint.h>
#include "arena.h"

#if defined(JN_WINDOWS)
    #define getcwd _getcwd
#endif

typedef struct Jn_Node Jn_Node;
typedef uint64_t u64;
typedef struct JnObject JnObject;
typedef struct J_DArray_Obj J_DArray_Obj;

#define RESIZE_DOBJ(arr) (arr)->items = realloc((arr)->items, sizeof(*(arr)->items) * (arr)->capacity)


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
    Jn_Node* pattern;
    Jn_Node* block;
} case_o;

typedef struct case_t{
    case_o* cases;
    u64 capacity;
    u64 count;
} case_t;

typedef struct{
    Jn_Node* cond;
    Jn_Node* stmt;
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

void print_source_lines(char* source, int line, int column, int context);
void print_source_line(char* source, int line, int column);
void runtime_error(char* msg, ...);
void call_add_pos(Jn_Node* call, Jn_Node* arg);
case_t* init_case(Jn_Arena* arena);
void push_case(case_t* caseObj, Jn_Node* sub, Jn_Node* block);

elseif* elseif_init(void);
void elseif_add(elseif* elif, Jn_Node* block, Jn_Node* cond);

#endif