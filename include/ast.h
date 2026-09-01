#ifndef JOAN_AST_H
#define JOAN_AST_H
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "token.h"
#include "object.h"
#include "arena.h"

typedef struct case_t case_t;
typedef struct case_o case_o;
typedef struct elseif elseif;
typedef struct Jn_Parser Jn_Parser;
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
    AST_CONTINUE,
    AST_CLASS,
    AST_FOR_EACH,
    AST_IMPORT,
    AST_YIELD,
    AST_COMMENT,

    AST_PROGRAM,

    AST_ERROR,
} Jn_NodeType;

typedef struct {
   const char* name;
   Jn_Node* default_value;
} Jn_ArgParam;

typedef struct {
const char* name;
Jn_Node* value;
} Jn_Kwarg;

typedef struct Jn_Node{
    Jn_NodeType type;
    union {

        const char* identifier;
        
        JnObject* literal;
        
        struct {
            Jn_Node** items;
            size_t count, capacity;
        } program_node;
        struct {
            Jn_Node* left;
            Jn_TokenType op;
            Jn_Node* right;
        } binary;

        struct {
            Jn_TokenType op;
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
            Jn_TokenType tok;
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
            Jn_TokenType op; // +=, -=, ...
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
            Jn_Node* value;
        } yield_node;

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

        struct {
            Jn_Node* value;
        } raise_node;

        struct{
            Jn_Node* subject;
            case_t* cases;
            Jn_Node* def; // defualt
        }match_node;
        //variable var;
        char* comment;
        const char* error_msg;
    };
    Jn_State* state;
    char* filename;
    int line, col;
} Jn_Node;

Jn_Node* ast_create(Jn_Parser* p, Jn_NodeType type);

Jn_Node* new_block(Jn_Parser* p);

void add_block(Jn_Node* ast, Jn_Node* node);
// LITERAL: -> true, false, None
Jn_Node* ast_literal(Jn_Parser* p, JnObject* object);

Jn_Node* ast_binary(Jn_Parser* p, Jn_Node* lhs, Jn_TokenType op, Jn_Node* rhs);

Jn_Node* ast_unary(Jn_Parser* p, Jn_TokenType op, Jn_Node* right);

Jn_Node* ast_program(Jn_Parser* p);
void ast_program_add(Jn_Parser* p, Jn_Node* prog, Jn_Node* node);
//ARRAY: Jn_Node functions
Jn_Node* ast_array(Jn_Parser* p);
void ast_array_add(Jn_Node* arr, Jn_Node* element);

// Tuple
Jn_Node* ast_tuple(Jn_Parser* p);
void ast_tuple_add(Jn_Node* tpl, Jn_Node* node);
Jn_Node* ast_empty_tuple(Jn_Parser* p);

Jn_Node* ast_identifier(Jn_Parser* p, const char* identifier);

Jn_Node* ast_raise(Jn_Parser* p, Jn_Node* node);

Jn_Node* ast_yield(Jn_Parser* p, Jn_Node* node);

//ASSIGN: v = true; const x = 4
Jn_Node* ast_assign(
    Jn_Parser* p,
    char* name,
    bool is_const,
    Jn_Node* value
);


// Function
Jn_Node* ast_function(Jn_Parser* p, char* ident, Jn_Node* block, int count, char** params);

//IF STATEMENT
Jn_Node* ast_if_node(Jn_Parser* p, Jn_Node* cond, Jn_Node* then, elseif* elseif, Jn_Node* else_node);

//BREAK, CONTINUE
Jn_Node* ast_break(Jn_Parser* p);
Jn_Node* ast_continue(Jn_Parser* p);
Jn_Node* ast_return(Jn_Parser* p, Jn_Node* value);
// Call
Jn_Node* ast_call(Jn_Parser* p, Jn_Node* callee, Jn_Node** args, size_t count);

Jn_Node* ast_range(Jn_Parser* p, Jn_Node* start, Jn_Node* stop, Jn_Node* step int op);

Jn_Node* ast_while(Jn_Parser* p, Jn_Node* cond, Jn_Node* block);
//Match
Jn_Node* ast_match(Jn_Parser* p, Jn_Node* sub, case_t* cases, Jn_Node* def);

//ERROR
Jn_Node* ast_error(Jn_Parser* p, const char* msg);

#endif // JOAN_AST_H
