#ifndef JOAN_AST_H
#define JOAN_AST_H
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "token.h"
#include "object.h"
#include "arena.h"

typedef uint64_t u64;
typedef struct case_t case_t;
typedef struct case_o case_o;
typedef struct elseif elseif;
typedef struct JnParser JnParser;
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
    AST_MODULE,
    AST_DEFINE,
    AST_INSTANCE,
    AST_ENUM,
    AST_IF,
    AST_INLINE_IF,
    AST_MULTI_VAR,
    AST_BLOCK,
    AST_FOR,
    AST_LAMBDA,
    AST_FUNCTION,
    AST_CALL,
    AST_TUPLE,
    AST_ARRAY_INDEX,
    AST_STRUCT,
    AST_RETURN,
    AST_BREAK,
    AST_PRINTLN,
    AST_CONTINUE,
    AST_CLASS,
    AST_FOR_EACH,
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
            char* ident;
            char** fields;
            int count;
        } enum_stmt;

        struct {
            AST* callie;
            AST* setter;
            AST* field;
            J_TokenType tok;
            bool is_setter;
            bool is_call;
        } member;

        struct {
            AST** elements;
            size_t count;
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
            char* ident;
            AST* call_node;
        } c_define_node;
        
        struct {
            AST* callee;

            AST** pos_args;
            int pos_count;
        } call;

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
            AST* value;
            AST* type;
            char* name;
            bool is_const;
        } assign;

        struct {
            char** idents;
            AST* value;
            int count;
            int op;
        } assign_multiple;

        struct {
            AST* expr;
            AST* value;
            J_TokenType op; // +=, -=, ...
        } reassign;

        struct {
            AST* block;
            char* name;
            // param_t* params;
            char** params;
            int count;
            bool is_defined;
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
            char** fields;
            char* lib;
            int count;
        } import_node;

        struct{
            char** fields;
            char* ident;
            int count;
        } struct_node;

        struct {
            AST** values;
            AST* object;
            char** fields;
            int count;
        } instance_node;
        
        struct {
            const char* index, *ident;
            AST* iter;
            AST* block;
        } foreach_node;

        struct{
            AST* subject;
            case_t* cases;
            AST* def; // defualt
        }match_node;
        //variable var;
        char* comment;
        const char* error_msg;
    };
    J_State* state;
    char* filename;
    int line, col;
} AST;

AST* ast_create(JnParser* p, AST_TYPE type);

AST* new_block(JnParser* p);

void add_block(AST* ast, AST* node);
// LITERAL: -> true, false, None
AST* ast_literal(JnParser* p, JnObject* object);

AST* ast_binary(JnParser* p, AST* lhs, J_TokenType op, AST* rhs);

AST* ast_unary(JnParser* p, J_TokenType op, AST* right);

AST* ast_println(JnParser* p, AST* out);

//ARRAY: AST functions
AST* ast_array(JnParser* p);
void ast_array_add(AST* arr, AST* element);

AST* ast_identifier(JnParser* p, const char* identifier);

//ASSIGN: v = true; const x = 4
AST* ast_assign(
    JnParser* p,
    char* name,
    bool is_const,
    AST* value
);


// Function
AST* ast_function(JnParser* p, char* ident, AST* block, int count, char** params);

//IF STATEMENT
AST* ast_if_node(JnParser* p, AST* cond, AST* then, elseif* elseif, AST* else_node);

//BREAK, CONTINUE
AST* ast_break(JnParser* p);
AST* ast_continue(JnParser* p);
AST* ast_return(JnParser* p, AST* value);
// Call
AST* ast_call(JnParser* p, AST* callee, AST** args, size_t count);

AST* ast_while(JnParser* p, AST* cond, AST* block);
//Match
AST* ast_match(JnParser* p, AST* sub, case_t* cases, AST* def);

//ERROR
AST* ast_error(JnParser* p, const char* msg);

#endif // JOAN_AST_H