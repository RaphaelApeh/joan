#ifndef AST_H
#define AST_H
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "token.h"
#include "object.h"
#include "arena.h"

typedef uint64_t u64;
typedef struct param_o param_o;
typedef struct param_t param_t;
typedef struct klass_o klass_o;
typedef struct klass_t klass_t;
typedef struct case_t case_t;
typedef struct case_o case_o;
typedef struct elseif elseif;
typedef struct parser parser;
typedef struct attr_t attr_t;
typedef struct AST AST;

typedef enum{
    GETTER_CALL,
    GETTER_NORMAL,
} GetterType;

typedef enum{
    AST_LITERAL,
    AST_BINARY,
    AST_IDENTIFIER,
    AST_UNARY,
    AST_ASSIGN,
    AST_REASSIGN,
    AST_WHILE,
    AST_MATCH,
    AST_ARRAY,
    AST_RANGE,
    AST_HASHMAP,
    AST_LOOP,
    AST_MEMBER,
    AST_MEMBER_SETTER,
    AST_MODULE,
    AST_GETTER,
    AST_SETTER,
    AST_INSTANCE,
    AST_ASSERT,
    AST_ENUM,
    AST_IF,
    AST_INLINE_IF,
    AST_BLOCK,
    AST_FOR,
    AST_FUNCTION,
    AST_CALL,
    AST_TUPLE,
    AST_PROPERTY,
    AST_ARRAY_INDEX,
    AST_STRUCT,
    AST_RETURN,
    AST_BREAK,
    AST_PRINTLN,
    AST_CONTINUE,
    AST_CLASS,
    AST_DO,
    AST_COMMENT,

    AST_ERROR,
} AST_TYPE;

typedef struct AST{
    AST_TYPE type;
    union {

        const char* identifier;
        Object* literal;
        struct {
            AST* left;
            TokenType op;
            AST* right;
        } binary;

        struct {
            TokenType op;
            AST* right;
        } unary;
        // let x, y, z = 0
        struct {
            char** idents;
            AST* expr;
            size_t ident_count;
        } var_decl_stmt;

        struct {
            AST** keys;
            AST** values;
            size_t count;
        } hmp_node;

        struct {
            AST* block;
        } loop_stmt;

        struct {
            AST* cond;
            char* msg;
        } assert_stmt;

        struct {
            char* ident;
            char** fields;
            int count;
        } enum_stmt;

        struct {
            AST* obj;
            const char* field;
            bool is_setter;
            bool is_call;
            bool is_getter;
            AST* setter;
            AST* callie;
        } member;

        struct {
            AST** elements;
            size_t count;
            size_t capacity;
        } tuple;

        struct {
            AST* then;
            AST* cond;
            AST* otherwise;
        } inline_if_stmt;

        struct {
            AST* out;
        } println;

        struct {
            const char* callee;

            AST** pos_args;
            int pos_count;

            param_t* params;
        } call;

        struct {
            AST* obj;
            GetterType type;
            param_t* params;
            const char* ident;
        } getter;

        struct {
            AST* obj;
            AST* value;
            const char* ident;
        } setter;

        struct {
            const char* ident;
            param_t* params;
        } instance_T;

        struct {
            AST* object;
            const char* attr;
        } property;

        struct {
            AST** elements;
            u64 count;
            u64 capacity;
        } array;

        struct {
            AST* array;
            AST* pos;
            AST* value;
            bool is_set; // a[0] = 4
        } index;

        struct {
            AST* array;
            AST* start;
            AST* end;
        } slice;

        struct {
            char* name;
            AST* value;
            bool is_const;
            //TODO: char** sub_types; //NULL
        } assign;

        struct {
            char* ident;
            AST* value;
            TokenType op; // +=, -=, ...
        } reassign;

        struct {
            AST* block;
            char* name;
            // param_t* params;
            char** params;
            int count;
            bool is_yield;
            bool is_async;
        } fn_node;

        struct {
            AST* value;
        } return_stmt;

        struct {
            AST* start;
            AST* end;
        } range_node;

        struct {
            AST** statements;
            u64 count;
            u64 capacity;
        } block;

        struct {
            AST* condition;
            AST* then;
            elseif* elseif;
            AST* else_node;
        } if_node;

        struct {
            AST* iter;
            AST* block;
            const char* ident;
        } for_node;

        struct {
            AST* cond;
            AST* block;
        } while_node;

        struct {
            AST* lib;
            AST* alias;
        } using_stmt;

        struct {
            AST* condition;
            AST* body;
        } do_node;

        struct{
            const char* ident;
            attr_t* attrs;
        } struct_node;
        struct{
            const char* ident;
            attr_t* attrs;
            klass_t* base; // inheritance
        }class_node;
        struct{
            AST* subject;
            case_t* cases;
            AST* def; // defualt
        }match_node;
        //variable var;
        char* comment;
        const char* error_msg;
    };
} AST;

AST* ast_create(Arena* arena, AST_TYPE type);

AST* new_block(Arena* arena);

void add_block(AST* ast, AST* node);
// LITERAL: -> true, false, None
AST* ast_literal(Arena* arena, Object* object);

AST* ast_binary(Arena* arena, AST* lhs, TokenType op, AST* rhs);

AST* ast_unary(Arena* arena, TokenType op, AST* right);

AST* ast_println(Arena* arena, AST* out);

//ARRAY: AST functions
AST* ast_array(Arena* arena);
void ast_array_add(AST* arr, AST* element);

AST* ast_identifier(Arena* arena, const char* identifier);
//LOOP
AST* ast_for(Arena* arena, const char* ident, AST* iter, AST* block);


//ASSIGN: v = true; const x = 4
AST* ast_assign(
    Arena* arena,
    char* name,
    bool is_const,
    AST* value
);

AST* ast_member(Arena* arena, AST* obj, const char* field);

//IF STATEMENT
AST* ast_if_node(Arena* arena, AST* cond, AST* then, elseif* elseif, AST* else_node);

//BREAK, CONTINUE
AST* ast_break(Arena* arena);
AST* ast_continue(Arena* arena);

// obj()
AST* ast_call(Arena* arena, const char* callee);

//Match
AST* ast_match(Arena* arena, AST* sub, case_t* cases, AST* def);
//CLASS, STRUCT
AST* ast_struct(Arena* arena, const char* ident, attr_t* attr);

//AST* ast_class(const char* ident, attr_t* attr, klass_t* base);

//ERROR
AST* ast_error(Arena* arena, parser* p, const char* msg);

#endif