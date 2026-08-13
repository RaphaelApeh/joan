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
typedef struct Jn_Node Jn_Node;

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

typedef struct Jn_Node{
    AST_TYPE type;
    union {

        const char* identifier;
        JnObject* literal;
        struct {
            Jn_Node* left;
            J_TokenType op;
            Jn_Node* right;
        } binary;

        struct {
            J_TokenType op;
            Jn_Node* right;
        } unary;
        // let x, y, z = 0
        struct {
            char** idents;
            Jn_Node* expr;
            size_t ident_count;
        } var_decl_stmt;

        struct {
            Jn_Node** keys;
            Jn_Node** values;
            size_t count;
        } hmp_node;

        struct {
            Jn_Node* block;
        } loop_stmt;

        struct {
            char* ident;
            char** fields;
            int count;
        } enum_stmt;

        struct {
            Jn_Node* callie;
            Jn_Node* setter;
            Jn_Node* field;
            J_TokenType tok;
            bool is_setter;
            bool is_call;
        } member;

        struct {
            Jn_Node** elements;
            size_t count;
        } tuple;

        struct {
            Jn_Node* then;
            Jn_Node* cond;
            Jn_Node* otherwise;
        } inline_if_stmt;

        struct {
            Jn_Node* out;
        } println;

        struct {
            char* ident;
            Jn_Node* call_node;
        } c_define_node;
        
        struct {
            Jn_Node* callee;

            Jn_Node** pos_args;
            int pos_count;
        } call;

        struct {
            Jn_Node** elements;
            u64 count;
            u64 capacity;
        } array;

        struct {
            Jn_Node* array;
            Jn_Node* pos;
            Jn_Node* value;
            bool is_set; // a[0] = 4
        } index;

        struct {
            Jn_Node* value;
            Jn_Node* type;
            char* name;
            bool is_const;
        } assign;

        struct {
            char** idents;
            Jn_Node* value;
            int count;
            int op;
        } assign_multiple;

        struct {
            Jn_Node* expr;
            Jn_Node* value;
            J_TokenType op; // +=, -=, ...
        } reassign;

        struct {
            Jn_Node* block;
            char* name;
            // param_t* params;
            char** params;
            int count;
            bool is_defined;
            bool is_yield;
            bool is_async;
        } fn_node;

        struct {
            Jn_Node* value;
        } return_stmt;

        struct {
            Jn_Node* start;
            Jn_Node* stop;
            Jn_Node* step;
            int op, has_step;
        } range_node;

        struct {
            Jn_Node** statements;
            u64 count;
            u64 capacity;
        } block;

        struct {
            Jn_Node* condition;
            Jn_Node* then;
            elseif* elseif;
            Jn_Node* else_node;
        } if_node;

        struct {
            Jn_Node *init, *cond, *incr;
            Jn_Node* block;
        } for_node;

        struct {
            Jn_Node* cond;
            Jn_Node* block;
        } while_node;

        struct {
            Jn_Node* expr;
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
            Jn_Node** values;
            Jn_Node* object;
            char** fields;
            int count;
        } instance_node;
        
        struct {
            const char* index, *ident;
            Jn_Node* iter;
            Jn_Node* block;
        } foreach_node;

        struct{
            Jn_Node* subject;
            case_t* cases;
            Jn_Node* def; // defualt
        }match_node;
        //variable var;
        char* comment;
        const char* error_msg;
    };
    J_State* state;
    char* filename;
    int line, col;
} Jn_Node;

Jn_Node* ast_create(JnParser* p, AST_TYPE type);

Jn_Node* new_block(JnParser* p);

void add_block(Jn_Node* ast, Jn_Node* node);
// LITERAL: -> true, false, None
Jn_Node* ast_literal(JnParser* p, JnObject* object);

Jn_Node* ast_binary(JnParser* p, Jn_Node* lhs, J_TokenType op, Jn_Node* rhs);

Jn_Node* ast_unary(JnParser* p, J_TokenType op, Jn_Node* right);

Jn_Node* ast_println(JnParser* p, Jn_Node* out);

//ARRAY: Jn_Node functions
Jn_Node* ast_array(JnParser* p);
void ast_array_add(Jn_Node* arr, Jn_Node* element);

Jn_Node* ast_identifier(JnParser* p, const char* identifier);

//ASSIGN: v = true; const x = 4
Jn_Node* ast_assign(
    JnParser* p,
    char* name,
    bool is_const,
    Jn_Node* value
);


// Function
Jn_Node* ast_function(JnParser* p, char* ident, Jn_Node* block, int count, char** params);

//IF STATEMENT
Jn_Node* ast_if_node(JnParser* p, Jn_Node* cond, Jn_Node* then, elseif* elseif, Jn_Node* else_node);

//BREAK, CONTINUE
Jn_Node* ast_break(JnParser* p);
Jn_Node* ast_continue(JnParser* p);
Jn_Node* ast_return(JnParser* p, Jn_Node* value);
// Call
Jn_Node* ast_call(JnParser* p, Jn_Node* callee, Jn_Node** args, size_t count);

Jn_Node* ast_while(JnParser* p, Jn_Node* cond, Jn_Node* block);
//Match
Jn_Node* ast_match(JnParser* p, Jn_Node* sub, case_t* cases, Jn_Node* def);

//ERROR
Jn_Node* ast_error(JnParser* p, const char* msg);

#endif // JOAN_AST_H