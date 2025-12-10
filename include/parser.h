#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

// Type information for variables, parameters, and fields
typedef enum {
    VIVA_TYPE_UNKNOWN,
    VIVA_TYPE_ENTERO,       // int (64-bit)
    VIVA_TYPE_OCTETO,       // byte (8-bit)
    VIVA_TYPE_CADENA,       // string (pointer)
    VIVA_TYPE_BOOLEANO,     // bool
    VIVA_TYPE_VACIO,        // void
    VIVA_TYPE_PUNTERO,      // pointer to another type
    VIVA_TYPE_ARREGLO,      // array of another type
    VIVA_TYPE_ESTRUCTURA    // struct type
} VivaType;

// Type descriptor for complex types
typedef struct TypeDesc {
    VivaType base_type;
    struct TypeDesc* element_type;  // For pointers and arrays
    char* struct_name;              // For struct types
    int array_size;                 // For fixed-size arrays (-1 for dynamic)
    int pointer_depth;              // Number of pointer indirections
} TypeDesc;

typedef enum {
    // Program structure
    PROGRAM_NODE,

    // Expressions
    EXPRESSION_NODE,
    BINARY_OP_NODE,
    UNARY_OP_NODE,
    NUMBER_NODE,
    HEX_NUMBER_NODE,        // For 0x... literals
    IDENTIFIER_NODE,
    STRING_LITERAL_NODE,

    // Variables
    VAR_DECL_NODE,
    VAR_DECL_SPANISH_NODE,
    ASSIGN_NODE,

    // Functions
    FN_DECL_NODE,
    FN_DECL_SPANISH_NODE,
    FN_CALL_NODE,
    RETURN_NODE,
    PARAM_NODE,             // Function parameter with type

    // Control flow
    IF_NODE,
    IF_SPANISH_NODE,
    WHILE_NODE,
    WHILE_SPANISH_NODE,
    FOR_NODE,
    FOR_SPANISH_NODE,
    BREAK_NODE,
    CONTINUE_NODE,
    CONDITION_NODE,

    // Arrays (critical for bootstrap)
    ARRAY_DECL_NODE,
    ARRAY_ACCESS_NODE,
    ARRAY_LITERAL_NODE,

    // Structures (critical for bootstrap)
    STRUCT_DECL_NODE,       // estructura NombreStruct { ... }
    STRUCT_FIELD_NODE,      // campo: tipo;
    STRUCT_INIT_NODE,       // NombreStruct { campo = valor, ... }
    FIELD_ACCESS_NODE,      // objeto.campo
    ARROW_ACCESS_NODE,      // puntero->campo

    // Pointers (critical for bootstrap)
    ADDRESS_OF_NODE,        // &variable
    DEREFERENCE_NODE,       // *puntero
    POINTER_TYPE_NODE,      // *tipo

    // Memory operations
    SIZEOF_NODE,            // tamano(tipo) or tamano(expr)
    NEW_NODE,               // nuevo tipo or nuevo tipo[n]
    FREE_NODE,              // liberar puntero

    // Syscalls (critical for pure machine code)
    SYSCALL_WRITE_NODE,     // escribir_sys(fd, buf, len)
    SYSCALL_READ_NODE,      // leer_sys(fd, buf, len)
    SYSCALL_OPEN_NODE,      // abrir_sys(ruta, flags, modo)
    SYSCALL_CLOSE_NODE,     // cerrar_sys(fd)
    SYSCALL_EXIT_NODE,      // salir_sys(codigo)

    // Type annotations
    TYPE_ANNOTATION_NODE,   // : tipo

    // Null literal
    NULL_LITERAL_NODE       // nulo
} NodeType;

// Extended AST node with type information
typedef struct ASTNode {
    NodeType type;
    char* value;
    struct ASTNode* left;       // Left child / first operand
    struct ASTNode* right;      // Right child / next statement in list
    struct ASTNode* extra;      // Extra child (else clause, third operand, etc.)
    struct ASTNode* params;     // Parameter list for functions
    TypeDesc* type_info;        // Type information
    int line;                   // Source line number
} ASTNode;

// Node creation and destruction
ASTNode* init_ast_node(NodeType type);
ASTNode* init_ast_node_with_value(NodeType type, const char* value);
void free_ast_node(ASTNode* node);
void free_type_desc(TypeDesc* td);

// Type descriptor creation
TypeDesc* create_type_desc(VivaType base_type);
TypeDesc* create_pointer_type(TypeDesc* element_type);
TypeDesc* create_array_type(TypeDesc* element_type, int size);
TypeDesc* create_struct_type(const char* name);

// Parsing functions
ASTNode* parse_expression(TokenStream* tokens, int* pos);
ASTNode* parse_statement(TokenStream* tokens, int* pos);
ASTNode* parse_program(TokenStream* tokens);
ASTNode* parse_type(TokenStream* tokens, int* pos);
ASTNode* parse_struct_declaration(TokenStream* tokens, int* pos);
ASTNode* parse_function_params(TokenStream* tokens, int* pos);

#endif
