#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

typedef enum {
    PROGRAM_NODE,
    EXPRESSION_NODE,
    BINARY_OP_NODE,         // For binary operations like +, -, *, /
    NUMBER_NODE,
    IDENTIFIER_NODE,
    STRING_LITERAL_NODE,    // For string literals
    VAR_DECL_NODE,
    FN_DECL_NODE,
    FN_CALL_NODE,
    IF_NODE,
    WHILE_NODE,
    ASSIGN_NODE,
    VAR_DECL_SPANISH_NODE,  // For "decreto" variable declarations
    FN_DECL_SPANISH_NODE,   // For "cancion" function declarations
    IF_SPANISH_NODE,        // For "si" conditionals
    WHILE_SPANISH_NODE,     // For "mientras" loops
    FOR_SPANISH_NODE,       // For "para" loops
    RETURN_NODE,            // For "retorno" return statements
    BREAK_NODE,             // For "romper" break statements
    CONTINUE_NODE,          // For "continuar" continue statements
    ARRAY_ACCESS_NODE,      // For array access operations (arr[index])
    ARRAY_DECL_NODE,        // For array declarations and assignments
    UNARY_OP_NODE,          // For unary operations like ! (not)
    CONDITION_NODE          // For condition expressions
} NodeType;

typedef struct ASTNode {
    NodeType type;
    char* value;
    struct ASTNode* left;   // Left child
    struct ASTNode* right;  // Right child or next statement
} ASTNode;

ASTNode* init_ast_node(NodeType type);
void free_ast_node(ASTNode* node);
ASTNode* parse_expression(TokenStream* tokens, int* pos);
ASTNode* parse_statement(TokenStream* tokens, int* pos);
ASTNode* parse_program(TokenStream* tokens);

#endif