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
typedef struct joan_parser_t joan_parser_t;
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
    AST_LAMBDA,
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
    AST_IMPORT,
    AST_COMMENT,

    AST_ERROR,
} AST_TYPE;

typedef struct AST{
    AST_TYPE type;
    union {

        const char* identifier;
        JnObject* literal;
        struct {
            AST* left;
            J_TokenType op;
            AST* right;
        } binary;

        struct {
            J_TokenType op;
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
            AST* callie;
            AST* setter;
            char* field;
            J_TokenType tok;
            bool is_setter;
            bool is_call;
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
            AST* callee;

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
            char* name;
            AST* value;
            bool is_const;
            //TODO: char** sub_types; //NULL
        } assign;

        struct {
            char* ident;
            AST* value;
            J_TokenType op; // +=, -=, ...
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
            AST* stop;
            AST* step;
            int op, has_step;
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
            AST *init, *cond, *incr;
            AST* block;
        } for_node;

        struct {
            AST* cond;
            AST* block;
        } while_node;

        struct {
            AST* expr;
            char** args;
            int count;
        } lambda_node;

        struct {
            char* lib;
            char* alias;
        } import_node;

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
    int line, col;
} AST;

AST* ast_create(joan_parser_t* p, AST_TYPE type);

AST* new_block(joan_parser_t* p);

void add_block(AST* ast, AST* node);
// LITERAL: -> true, false, None
AST* ast_literal(joan_parser_t* p, JnObject* object);

AST* ast_binary(joan_parser_t* p, AST* lhs, J_TokenType op, AST* rhs);

AST* ast_unary(joan_parser_t* p, J_TokenType op, AST* right);

AST* ast_println(joan_parser_t* p, AST* out);

//ARRAY: AST functions
AST* ast_array(joan_parser_t* p);
void ast_array_add(AST* arr, AST* element);

AST* ast_identifier(joan_parser_t* p, const char* identifier);

//ASSIGN: v = true; const x = 4
AST* ast_assign(
    joan_parser_t* p,
    char* name,
    bool is_const,
    AST* value
);

//IF STATEMENT
AST* ast_if_node(joan_parser_t* p, AST* cond, AST* then, elseif* elseif, AST* else_node);

//BREAK, CONTINUE
AST* ast_break(joan_parser_t* p);
AST* ast_continue(joan_parser_t* p);

// obj()
AST* ast_call(joan_parser_t* p, AST* callee);

//Match
AST* ast_match(joan_parser_t* p, AST* sub, case_t* cases, AST* def);
//CLASS, STRUCT
AST* ast_struct(joan_parser_t* p, const char* ident, attr_t* attr);

//AST* ast_class(const char* ident, attr_t* attr, klass_t* base);

//ERROR
AST* ast_error(joan_parser_t* p, const char* msg);

#endif