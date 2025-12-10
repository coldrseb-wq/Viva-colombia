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
    FOR_NODE,               // For English "for" loops
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
    CONDITION_NODE,         // For condition expressions
    // File I/O nodes (Phase 6)
    FILE_OPEN_NODE,         // For file open operations
    FILE_CLOSE_NODE,        // For file close operations
    FILE_READ_NODE,         // For file read operations
    FILE_WRITE_NODE,        // For file write operations
    FILE_DECL_NODE,         // For file variable declarations
    // Standard library nodes
    STRLEN_NODE,            // String length
    CONCAT_NODE,            // String concatenation
    ABS_NODE,               // Absolute value
    POW_NODE,               // Power/exponentiation
    SQRT_NODE,              // Square root
    // Phase 7: Arrays, Structs, Dynamic Memory
    ARRAY_INIT_NODE,        // Array initialization with values
    STRUCT_DECL_NODE,       // Struct type declaration
    STRUCT_INST_NODE,       // Struct instance declaration
    MEMBER_ACCESS_NODE,     // Struct member access (obj.field)
    MALLOC_NODE,            // Memory allocation
    FREE_NODE,              // Memory deallocation
    // Bootstrap support nodes
    ORD_NODE,               // ord(c) - character to ASCII code
    CHR_NODE,               // chr(n) - ASCII code to character
    ITOA_NODE,              // itoa(n) - integer to string
    WRITE_BYTE_NODE         // escribir_byte(f, byte) - write single byte
} NodeType;

typedef struct ASTNode {
    NodeType type;
    char* value;
    struct ASTNode* left;   // Left child (condition for if/while, params for fn, init for for)
    struct ASTNode* right;  // Right child (body for if/while/for/fn)
    struct ASTNode* extra;  // Extra node (else block for if, condition/increment for for)
    struct ASTNode* next;   // Next statement in sequence (for statement chaining)
} ASTNode;

ASTNode* init_ast_node(NodeType type);
void free_ast_node(ASTNode* node);
ASTNode* parse_expression(TokenStream* tokens, int* pos);
ASTNode* parse_statement(TokenStream* tokens, int* pos);
ASTNode* parse_program(TokenStream* tokens);

#endif