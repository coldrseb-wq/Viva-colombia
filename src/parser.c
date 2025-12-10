#define _GNU_SOURCE
#include "parser.h"
#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// === TYPE DESCRIPTOR FUNCTIONS ===

TypeDesc* create_type_desc(VivaType base_type) {
    TypeDesc* td = calloc(1, sizeof(TypeDesc));
    td->base_type = base_type;
    td->array_size = -1;
    return td;
}

TypeDesc* create_pointer_type(TypeDesc* element_type) {
    TypeDesc* td = calloc(1, sizeof(TypeDesc));
    td->base_type = VIVA_TYPE_PUNTERO;
    td->element_type = element_type;
    td->pointer_depth = 1;
    if (element_type && element_type->base_type == VIVA_TYPE_PUNTERO) {
        td->pointer_depth = element_type->pointer_depth + 1;
    }
    return td;
}

TypeDesc* create_array_type(TypeDesc* element_type, int size) {
    TypeDesc* td = calloc(1, sizeof(TypeDesc));
    td->base_type = VIVA_TYPE_ARREGLO;
    td->element_type = element_type;
    td->array_size = size;
    return td;
}

TypeDesc* create_struct_type(const char* name) {
    TypeDesc* td = calloc(1, sizeof(TypeDesc));
    td->base_type = VIVA_TYPE_ESTRUCTURA;
    td->struct_name = strdup(name);
    return td;
}

void free_type_desc(TypeDesc* td) {
    if (td) {
        if (td->element_type) free_type_desc(td->element_type);
        if (td->struct_name) free(td->struct_name);
        free(td);
    }
}

// === AST NODE FUNCTIONS ===

ASTNode* init_ast_node(NodeType type) {
    ASTNode* node = calloc(1, sizeof(ASTNode));
    node->type = type;
    return node;
}

ASTNode* init_ast_node_with_value(NodeType type, const char* value) {
    ASTNode* node = init_ast_node(type);
    if (value) node->value = strdup(value);
    return node;
}

void free_ast_node(ASTNode* node) {
    if (node == NULL) return;

    // Validate the node type
    if (node->type < PROGRAM_NODE || node->type > NULL_LITERAL_NODE) {
        if (node->value) free(node->value);
        free(node);
        return;
    }

    // Free children
    if (node->left) free_ast_node(node->left);
    if (node->right) free_ast_node(node->right);
    if (node->extra) free_ast_node(node->extra);
    if (node->params) free_ast_node(node->params);
    if (node->type_info) free_type_desc(node->type_info);
    if (node->value) free(node->value);
    free(node);
}

// === HELPER FUNCTIONS ===

static int is_operator(TokenType type) {
    return type == PLUS || type == MINUS || type == MULTIPLY || type == DIVIDE ||
           type == MODULO || type == EQUALITY || type == NOT_EQUAL ||
           type == LESS_THAN || type == GREATER_THAN || type == LESS_EQUAL ||
           type == GREATER_EQUAL || type == Y || type == O ||
           type == BIT_AND || type == BIT_OR || type == BIT_XOR ||
           type == BIT_LSHIFT || type == BIT_RSHIFT;
}

static int is_type_keyword(TokenType type) {
    return type == TIPO_ENTERO || type == TIPO_OCTETO || type == TIPO_CADENA ||
           type == TIPO_VACIO || type == TIPO_BOOL || type == ESTRUCTURA;
}

// Forward declarations
static ASTNode* parse_primary(TokenStream* tokens, int* pos);
static ASTNode* parse_unary(TokenStream* tokens, int* pos);
static ASTNode* parse_expression_helper(TokenStream* tokens, int* pos, int min_prec);

// === OPERATOR PRECEDENCE ===

typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,    // =
    PREC_OR,            // || o
    PREC_AND,           // && y
    PREC_BIT_OR,        // |
    PREC_BIT_XOR,       // ^
    PREC_BIT_AND,       // &
    PREC_EQUALITY,      // == !=
    PREC_COMPARISON,    // < > <= >=
    PREC_SHIFT,         // << >>
    PREC_TERM,          // + -
    PREC_FACTOR,        // * / %
    PREC_UNARY,         // ! ~ - * &
    PREC_POSTFIX,       // () [] . ->
    PREC_PRIMARY
} Precedence;

static Precedence get_precedence(TokenType type) {
    switch (type) {
        case O: return PREC_OR;
        case Y: return PREC_AND;
        case BIT_OR: return PREC_BIT_OR;
        case BIT_XOR: return PREC_BIT_XOR;
        case BIT_AND: return PREC_BIT_AND;
        case EQUALITY:
        case NOT_EQUAL: return PREC_EQUALITY;
        case LESS_THAN:
        case GREATER_THAN:
        case LESS_EQUAL:
        case GREATER_EQUAL: return PREC_COMPARISON;
        case BIT_LSHIFT:
        case BIT_RSHIFT: return PREC_SHIFT;
        case PLUS:
        case MINUS: return PREC_TERM;
        case MULTIPLY:
        case DIVIDE:
        case MODULO: return PREC_FACTOR;
        default: return PREC_NONE;
    }
}

// === TYPE PARSING ===

// Parse a type annotation: entero, octeto, *entero, [10]octeto, etc.
ASTNode* parse_type(TokenStream* tokens, int* pos) {
    if (*pos >= tokens->count) return NULL;

    Token* current = tokens->tokens[*pos];
    ASTNode* type_node = NULL;

    // Handle pointer types: *tipo
    if (current->type == MULTIPLY) {
        (*pos)++;
        ASTNode* inner_type = parse_type(tokens, pos);
        type_node = init_ast_node(POINTER_TYPE_NODE);
        type_node->left = inner_type;
        if (inner_type && inner_type->type_info) {
            type_node->type_info = create_pointer_type(inner_type->type_info);
        }
        return type_node;
    }

    // Handle array types: [size]tipo
    if (current->type == LBRACKET) {
        (*pos)++;
        int size = -1;
        if (*pos < tokens->count && tokens->tokens[*pos]->type == NUMBER) {
            size = atoi(tokens->tokens[*pos]->value);
            (*pos)++;
        }
        if (*pos < tokens->count && tokens->tokens[*pos]->type == RBRACKET) {
            (*pos)++;
        }
        ASTNode* inner_type = parse_type(tokens, pos);
        type_node = init_ast_node(TYPE_ANNOTATION_NODE);
        type_node->value = strdup("arreglo");
        type_node->left = inner_type;
        if (inner_type && inner_type->type_info) {
            type_node->type_info = create_array_type(inner_type->type_info, size);
        }
        return type_node;
    }

    // Handle basic types
    type_node = init_ast_node(TYPE_ANNOTATION_NODE);

    switch (current->type) {
        case TIPO_ENTERO:
            type_node->value = strdup("entero");
            type_node->type_info = create_type_desc(VIVA_TYPE_ENTERO);
            (*pos)++;
            break;
        case TIPO_OCTETO:
            type_node->value = strdup("octeto");
            type_node->type_info = create_type_desc(VIVA_TYPE_OCTETO);
            (*pos)++;
            break;
        case TIPO_CADENA:
            type_node->value = strdup("cadena");
            type_node->type_info = create_type_desc(VIVA_TYPE_CADENA);
            (*pos)++;
            break;
        case TIPO_VACIO:
            type_node->value = strdup("vacio");
            type_node->type_info = create_type_desc(VIVA_TYPE_VACIO);
            (*pos)++;
            break;
        case TIPO_BOOL:
            type_node->value = strdup("booleano");
            type_node->type_info = create_type_desc(VIVA_TYPE_BOOLEANO);
            (*pos)++;
            break;
        case IDENTIFIER:
            // Struct type by name
            type_node->value = strdup(current->value);
            type_node->type_info = create_struct_type(current->value);
            (*pos)++;
            break;
        default:
            free_ast_node(type_node);
            return NULL;
    }

    return type_node;
}

// === STRUCT PARSING ===

// Parse struct declaration: estructura NombreStruct { campo: tipo; ... }
ASTNode* parse_struct_declaration(TokenStream* tokens, int* pos) {
    if (*pos >= tokens->count || tokens->tokens[*pos]->type != ESTRUCTURA) {
        return NULL;
    }
    (*pos)++;  // Skip 'estructura'

    // Get struct name
    if (*pos >= tokens->count || tokens->tokens[*pos]->type != IDENTIFIER) {
        return NULL;
    }
    char* struct_name = strdup(tokens->tokens[*pos]->value);
    (*pos)++;

    ASTNode* struct_node = init_ast_node(STRUCT_DECL_NODE);
    struct_node->value = struct_name;

    // Expect opening brace
    if (*pos >= tokens->count || tokens->tokens[*pos]->type != LBRACE) {
        return struct_node;
    }
    (*pos)++;

    // Parse fields
    ASTNode* field_list = NULL;
    ASTNode* current_field = NULL;

    while (*pos < tokens->count && tokens->tokens[*pos]->type != RBRACE) {
        // Parse field: nombre: tipo;
        if (tokens->tokens[*pos]->type == IDENTIFIER) {
            ASTNode* field = init_ast_node(STRUCT_FIELD_NODE);
            field->value = strdup(tokens->tokens[*pos]->value);
            (*pos)++;

            // Expect colon
            if (*pos < tokens->count && tokens->tokens[*pos]->type == COLON) {
                (*pos)++;
                field->left = parse_type(tokens, pos);
            }

            // Skip semicolon
            if (*pos < tokens->count && tokens->tokens[*pos]->type == SEMICOLON) {
                (*pos)++;
            }

            if (field_list == NULL) {
                field_list = field;
                current_field = field;
            } else {
                current_field->right = field;
                current_field = field;
            }
        } else {
            (*pos)++;  // Skip unexpected token
        }
    }

    // Skip closing brace
    if (*pos < tokens->count && tokens->tokens[*pos]->type == RBRACE) {
        (*pos)++;
    }

    struct_node->left = field_list;
    return struct_node;
}

// === FUNCTION PARAMETER PARSING ===

// Parse function parameters: (nombre: tipo, nombre2: tipo2)
ASTNode* parse_function_params(TokenStream* tokens, int* pos) {
    if (*pos >= tokens->count || tokens->tokens[*pos]->type != LPAREN) {
        return NULL;
    }
    (*pos)++;  // Skip '('

    ASTNode* param_list = NULL;
    ASTNode* current_param = NULL;

    while (*pos < tokens->count && tokens->tokens[*pos]->type != RPAREN) {
        if (tokens->tokens[*pos]->type == IDENTIFIER) {
            ASTNode* param = init_ast_node(PARAM_NODE);
            param->value = strdup(tokens->tokens[*pos]->value);
            (*pos)++;

            // Check for type annotation
            if (*pos < tokens->count && tokens->tokens[*pos]->type == COLON) {
                (*pos)++;
                param->left = parse_type(tokens, pos);
            }

            if (param_list == NULL) {
                param_list = param;
                current_param = param;
            } else {
                current_param->right = param;
                current_param = param;
            }
        }

        // Skip comma
        if (*pos < tokens->count && tokens->tokens[*pos]->type == COMMA) {
            (*pos)++;
        } else if (tokens->tokens[*pos]->type != RPAREN) {
            break;
        }
    }

    // Skip ')'
    if (*pos < tokens->count && tokens->tokens[*pos]->type == RPAREN) {
        (*pos)++;
    }

    return param_list;
}

// === PRIMARY EXPRESSION PARSING ===

static ASTNode* parse_primary(TokenStream* tokens, int* pos) {
    if (*pos >= tokens->count) return NULL;

    Token* current = tokens->tokens[*pos];
    ASTNode* node = NULL;

    // Null literal
    if (current->type == NULO) {
        node = init_ast_node(NULL_LITERAL_NODE);
        node->value = strdup("nulo");
        (*pos)++;
        return node;
    }

    // Number literal
    if (current->type == NUMBER) {
        node = init_ast_node(NUMBER_NODE);
        node->value = strdup(current->value);
        node->type_info = create_type_desc(VIVA_TYPE_ENTERO);
        (*pos)++;
        return node;
    }

    // Hex number literal
    if (current->type == HEX_NUMBER) {
        node = init_ast_node(HEX_NUMBER_NODE);
        node->value = strdup(current->value);
        node->type_info = create_type_desc(VIVA_TYPE_ENTERO);
        (*pos)++;
        return node;
    }

    // String literal
    if (current->type == STRING) {
        node = init_ast_node(STRING_LITERAL_NODE);
        node->value = malloc(strlen(current->value) + 3);
        sprintf(node->value, "\"%s\"", current->value);
        node->type_info = create_type_desc(VIVA_TYPE_CADENA);
        (*pos)++;
        return node;
    }

    // sizeof / tamano
    if (current->type == TAMANO) {
        (*pos)++;
        node = init_ast_node(SIZEOF_NODE);
        if (*pos < tokens->count && tokens->tokens[*pos]->type == LPAREN) {
            (*pos)++;
            node->left = parse_expression(tokens, pos);
            if (*pos < tokens->count && tokens->tokens[*pos]->type == RPAREN) {
                (*pos)++;
            }
        }
        return node;
    }

    // new / nuevo
    if (current->type == NUEVO) {
        (*pos)++;
        node = init_ast_node(NEW_NODE);
        node->left = parse_type(tokens, pos);
        // Check for array allocation: nuevo tipo[size]
        if (*pos < tokens->count && tokens->tokens[*pos]->type == LBRACKET) {
            (*pos)++;
            node->extra = parse_expression(tokens, pos);
            if (*pos < tokens->count && tokens->tokens[*pos]->type == RBRACKET) {
                (*pos)++;
            }
        }
        return node;
    }

    // Syscalls - use left/params/extra to avoid conflict with statement chaining via right
    if (current->type == ESCRIBIR_SYS) {
        (*pos)++;
        node = init_ast_node(SYSCALL_WRITE_NODE);
        if (*pos < tokens->count && tokens->tokens[*pos]->type == LPAREN) {
            (*pos)++;
            node->left = parse_expression(tokens, pos);  // fd
            if (*pos < tokens->count && tokens->tokens[*pos]->type == COMMA) (*pos)++;
            node->params = parse_expression(tokens, pos);  // buf (use params instead of right)
            if (*pos < tokens->count && tokens->tokens[*pos]->type == COMMA) (*pos)++;
            node->extra = parse_expression(tokens, pos);  // len
            if (*pos < tokens->count && tokens->tokens[*pos]->type == RPAREN) (*pos)++;
        }
        return node;
    }

    if (current->type == LEER_SYS) {
        (*pos)++;
        node = init_ast_node(SYSCALL_READ_NODE);
        if (*pos < tokens->count && tokens->tokens[*pos]->type == LPAREN) {
            (*pos)++;
            node->left = parse_expression(tokens, pos);  // fd
            if (*pos < tokens->count && tokens->tokens[*pos]->type == COMMA) (*pos)++;
            node->params = parse_expression(tokens, pos);  // buf
            if (*pos < tokens->count && tokens->tokens[*pos]->type == COMMA) (*pos)++;
            node->extra = parse_expression(tokens, pos);  // len
            if (*pos < tokens->count && tokens->tokens[*pos]->type == RPAREN) (*pos)++;
        }
        return node;
    }

    if (current->type == ABRIR_SYS) {
        (*pos)++;
        node = init_ast_node(SYSCALL_OPEN_NODE);
        if (*pos < tokens->count && tokens->tokens[*pos]->type == LPAREN) {
            (*pos)++;
            node->left = parse_expression(tokens, pos);  // path
            if (*pos < tokens->count && tokens->tokens[*pos]->type == COMMA) (*pos)++;
            node->params = parse_expression(tokens, pos);  // flags
            if (*pos < tokens->count && tokens->tokens[*pos]->type == COMMA) (*pos)++;
            node->extra = parse_expression(tokens, pos);  // mode
            if (*pos < tokens->count && tokens->tokens[*pos]->type == RPAREN) (*pos)++;
        }
        return node;
    }

    if (current->type == CERRAR_SYS) {
        (*pos)++;
        node = init_ast_node(SYSCALL_CLOSE_NODE);
        if (*pos < tokens->count && tokens->tokens[*pos]->type == LPAREN) {
            (*pos)++;
            node->left = parse_expression(tokens, pos);
            if (*pos < tokens->count && tokens->tokens[*pos]->type == RPAREN) (*pos)++;
        }
        return node;
    }

    if (current->type == SALIR_SYS) {
        (*pos)++;
        node = init_ast_node(SYSCALL_EXIT_NODE);
        if (*pos < tokens->count && tokens->tokens[*pos]->type == LPAREN) {
            (*pos)++;
            node->left = parse_expression(tokens, pos);
            if (*pos < tokens->count && tokens->tokens[*pos]->type == RPAREN) (*pos)++;
        }
        return node;
    }

    // Identifier, function call, or array access
    if (current->type == IDENTIFIER) {
        int next_pos = *pos + 1;

        // Function call
        if (next_pos < tokens->count && tokens->tokens[next_pos]->type == LPAREN) {
            node = init_ast_node(FN_CALL_NODE);
            node->value = strdup(current->value);
            (*pos) += 2;  // Skip identifier and '('

            // Parse arguments
            ASTNode* arg_list = NULL;
            ASTNode* current_arg = NULL;

            while (*pos < tokens->count && tokens->tokens[*pos]->type != RPAREN) {
                ASTNode* arg = parse_expression(tokens, pos);
                if (arg) {
                    if (arg_list == NULL) {
                        arg_list = arg;
                        current_arg = arg;
                    } else {
                        current_arg->right = arg;
                        current_arg = arg;
                    }
                }
                if (*pos < tokens->count && tokens->tokens[*pos]->type == COMMA) {
                    (*pos)++;
                } else {
                    break;
                }
            }
            node->left = arg_list;

            if (*pos < tokens->count && tokens->tokens[*pos]->type == RPAREN) {
                (*pos)++;
            }
            return node;
        }

        // Struct initialization: NombreStruct { campo = valor }
        if (next_pos < tokens->count && tokens->tokens[next_pos]->type == LBRACE) {
            node = init_ast_node(STRUCT_INIT_NODE);
            node->value = strdup(current->value);
            (*pos) += 2;  // Skip identifier and '{'

            ASTNode* field_list = NULL;
            ASTNode* current_field = NULL;

            while (*pos < tokens->count && tokens->tokens[*pos]->type != RBRACE) {
                if (tokens->tokens[*pos]->type == IDENTIFIER) {
                    ASTNode* field_init = init_ast_node(ASSIGN_NODE);
                    field_init->value = strdup(tokens->tokens[*pos]->value);
                    (*pos)++;

                    if (*pos < tokens->count && tokens->tokens[*pos]->type == ASSIGN) {
                        (*pos)++;
                        field_init->left = parse_expression(tokens, pos);
                    }

                    if (field_list == NULL) {
                        field_list = field_init;
                        current_field = field_init;
                    } else {
                        current_field->right = field_init;
                        current_field = field_init;
                    }
                }

                if (*pos < tokens->count && tokens->tokens[*pos]->type == COMMA) {
                    (*pos)++;
                } else if (tokens->tokens[*pos]->type != RBRACE) {
                    (*pos)++;
                }
            }
            node->left = field_list;

            if (*pos < tokens->count && tokens->tokens[*pos]->type == RBRACE) {
                (*pos)++;
            }
            return node;
        }

        // Simple identifier
        node = init_ast_node(IDENTIFIER_NODE);
        node->value = strdup(current->value);
        (*pos)++;

        // Check for postfix operations
        while (*pos < tokens->count) {
            Token* postfix = tokens->tokens[*pos];

            // Array access: identifier[index]
            if (postfix->type == LBRACKET) {
                (*pos)++;
                ASTNode* access = init_ast_node(ARRAY_ACCESS_NODE);
                access->value = node->value ? strdup(node->value) : NULL;
                access->left = node;
                access->right = parse_expression(tokens, pos);
                if (*pos < tokens->count && tokens->tokens[*pos]->type == RBRACKET) {
                    (*pos)++;
                }
                node = access;
            }
            // Field access: object.field
            else if (postfix->type == DOT) {
                (*pos)++;
                ASTNode* access = init_ast_node(FIELD_ACCESS_NODE);
                access->left = node;
                if (*pos < tokens->count && tokens->tokens[*pos]->type == IDENTIFIER) {
                    access->value = strdup(tokens->tokens[*pos]->value);
                    (*pos)++;
                }
                node = access;
            }
            // Arrow access: pointer->field
            else if (postfix->type == ARROW) {
                (*pos)++;
                ASTNode* access = init_ast_node(ARROW_ACCESS_NODE);
                access->left = node;
                if (*pos < tokens->count && tokens->tokens[*pos]->type == IDENTIFIER) {
                    access->value = strdup(tokens->tokens[*pos]->value);
                    (*pos)++;
                }
                node = access;
            }
            else {
                break;
            }
        }

        return node;
    }

    // Parenthesized expression
    if (current->type == LPAREN) {
        (*pos)++;
        node = parse_expression(tokens, pos);
        if (*pos < tokens->count && tokens->tokens[*pos]->type == RPAREN) {
            (*pos)++;
        }
        return node;
    }

    // Array literal: [expr1, expr2, ...]
    if (current->type == LBRACKET) {
        (*pos)++;
        node = init_ast_node(ARRAY_LITERAL_NODE);

        ASTNode* elem_list = NULL;
        ASTNode* current_elem = NULL;

        while (*pos < tokens->count && tokens->tokens[*pos]->type != RBRACKET) {
            ASTNode* elem = parse_expression(tokens, pos);
            if (elem) {
                if (elem_list == NULL) {
                    elem_list = elem;
                    current_elem = elem;
                } else {
                    current_elem->right = elem;
                    current_elem = elem;
                }
            }
            if (*pos < tokens->count && tokens->tokens[*pos]->type == COMMA) {
                (*pos)++;
            } else {
                break;
            }
        }
        node->left = elem_list;

        if (*pos < tokens->count && tokens->tokens[*pos]->type == RBRACKET) {
            (*pos)++;
        }
        return node;
    }

    return NULL;
}

// === UNARY EXPRESSION PARSING ===

static ASTNode* parse_unary(TokenStream* tokens, int* pos) {
    if (*pos >= tokens->count) return NULL;

    Token* current = tokens->tokens[*pos];

    // Unary NOT: ! or no
    if (current->type == NO) {
        (*pos)++;
        ASTNode* node = init_ast_node(UNARY_OP_NODE);
        node->value = strdup("no");
        node->right = parse_unary(tokens, pos);
        return node;
    }

    // Bitwise NOT: ~
    if (current->type == BIT_NOT) {
        (*pos)++;
        ASTNode* node = init_ast_node(UNARY_OP_NODE);
        node->value = strdup("~");
        node->right = parse_unary(tokens, pos);
        return node;
    }

    // Unary minus: -
    if (current->type == MINUS) {
        (*pos)++;
        ASTNode* node = init_ast_node(UNARY_OP_NODE);
        node->value = strdup("-");
        node->right = parse_unary(tokens, pos);
        return node;
    }

    // Address-of: &
    if (current->type == BIT_AND) {
        (*pos)++;
        ASTNode* node = init_ast_node(ADDRESS_OF_NODE);
        node->right = parse_unary(tokens, pos);
        return node;
    }

    // Dereference: *
    if (current->type == MULTIPLY) {
        (*pos)++;
        ASTNode* node = init_ast_node(DEREFERENCE_NODE);
        node->right = parse_unary(tokens, pos);
        return node;
    }

    return parse_primary(tokens, pos);
}

// === EXPRESSION PARSING ===

static ASTNode* parse_expression_helper(TokenStream* tokens, int* pos, int min_prec) {
    ASTNode* left = parse_unary(tokens, pos);
    if (!left) return NULL;

    while (*pos < tokens->count) {
        Token* op_token = tokens->tokens[*pos];
        Precedence prec = get_precedence(op_token->type);

        if (prec == PREC_NONE || prec < min_prec) break;

        (*pos)++;  // Consume operator

        ASTNode* op_node = init_ast_node(BINARY_OP_NODE);
        op_node->value = strdup(op_token->value);
        op_node->left = left;
        op_node->right = parse_expression_helper(tokens, pos, prec + 1);

        if (!op_node->right) {
            free(op_node->value);
            free(op_node);
            return left;
        }

        left = op_node;
    }

    return left;
}

ASTNode* parse_expression(TokenStream* tokens, int* pos) {
    return parse_expression_helper(tokens, pos, PREC_NONE + 1);
}

// === STATEMENT PARSING ===

ASTNode* parse_statement(TokenStream* tokens, int* pos) {
    if (*pos >= tokens->count) return NULL;

    Token* current = tokens->tokens[*pos];
    ASTNode* node = NULL;

    // Skip semicolons
    if (current->type == SEMICOLON) {
        (*pos)++;
        return NULL;
    }

    // Struct declaration: estructura NombreStruct { ... }
    if (current->type == ESTRUCTURA) {
        return parse_struct_declaration(tokens, pos);
    }

    // Variable declaration: decreto nombre: tipo = valor;
    if (current->type == LET || current->type == DECRETO) {
        int is_spanish = (current->type == DECRETO);
        (*pos)++;

        if (*pos >= tokens->count || tokens->tokens[*pos]->type != IDENTIFIER) {
            return NULL;
        }

        node = init_ast_node(is_spanish ? VAR_DECL_SPANISH_NODE : VAR_DECL_NODE);
        node->value = strdup(tokens->tokens[*pos]->value);
        (*pos)++;

        // Type annotation: : tipo
        if (*pos < tokens->count && tokens->tokens[*pos]->type == COLON) {
            (*pos)++;
            node->extra = parse_type(tokens, pos);
        }

        // Assignment: = value
        if (*pos < tokens->count && tokens->tokens[*pos]->type == ASSIGN) {
            (*pos)++;
            node->left = parse_expression(tokens, pos);
        }

        if (*pos < tokens->count && tokens->tokens[*pos]->type == SEMICOLON) {
            (*pos)++;
        }
        return node;
    }

    // Function declaration: cancion nombre(params): tipo { ... }
    if (current->type == FN || current->type == CANCION) {
        int is_spanish = (current->type == CANCION);
        (*pos)++;

        if (*pos >= tokens->count || tokens->tokens[*pos]->type != IDENTIFIER) {
            return NULL;
        }

        node = init_ast_node(is_spanish ? FN_DECL_SPANISH_NODE : FN_DECL_NODE);
        node->value = strdup(tokens->tokens[*pos]->value);
        (*pos)++;

        // Parse parameters
        node->params = parse_function_params(tokens, pos);

        // Return type: : tipo
        if (*pos < tokens->count && tokens->tokens[*pos]->type == COLON) {
            (*pos)++;
            node->extra = parse_type(tokens, pos);
        }

        // Function body
        if (*pos < tokens->count && tokens->tokens[*pos]->type == LBRACE) {
            (*pos)++;

            ASTNode* body = NULL;
            ASTNode* current_stmt = NULL;

            while (*pos < tokens->count && tokens->tokens[*pos]->type != RBRACE) {
                int start_pos = *pos;
                ASTNode* stmt = parse_statement(tokens, pos);
                if (stmt) {
                    if (body == NULL) {
                        body = stmt;
                        current_stmt = stmt;
                    } else {
                        current_stmt->right = stmt;
                        current_stmt = stmt;
                    }
                } else if (*pos == start_pos) {
                    (*pos)++;
                }
            }
            node->left = body;

            if (*pos < tokens->count && tokens->tokens[*pos]->type == RBRACE) {
                (*pos)++;
            }
        }
        return node;
    }

    // If statement: si (cond) { ... } sino { ... }
    if (current->type == SI) {
        (*pos)++;
        node = init_ast_node(IF_SPANISH_NODE);

        if (*pos < tokens->count && tokens->tokens[*pos]->type == LPAREN) {
            (*pos)++;
            node->left = parse_expression(tokens, pos);
            if (*pos < tokens->count && tokens->tokens[*pos]->type == RPAREN) {
                (*pos)++;
            }
        }

        // Then block
        if (*pos < tokens->count && tokens->tokens[*pos]->type == LBRACE) {
            (*pos)++;
            ASTNode* then_body = NULL;
            ASTNode* current_stmt = NULL;

            while (*pos < tokens->count && tokens->tokens[*pos]->type != RBRACE) {
                int start_pos = *pos;
                ASTNode* stmt = parse_statement(tokens, pos);
                if (stmt) {
                    if (then_body == NULL) {
                        then_body = stmt;
                        current_stmt = stmt;
                    } else {
                        current_stmt->right = stmt;
                        current_stmt = stmt;
                    }
                } else if (*pos == start_pos) {
                    (*pos)++;
                }
            }
            node->right = then_body;

            if (*pos < tokens->count && tokens->tokens[*pos]->type == RBRACE) {
                (*pos)++;
            }
        }

        // Else block: sino { ... }
        if (*pos < tokens->count && tokens->tokens[*pos]->type == SINO) {
            (*pos)++;

            if (*pos < tokens->count && tokens->tokens[*pos]->type == LBRACE) {
                (*pos)++;
                ASTNode* else_body = NULL;
                ASTNode* current_stmt = NULL;

                while (*pos < tokens->count && tokens->tokens[*pos]->type != RBRACE) {
                    int start_pos = *pos;
                    ASTNode* stmt = parse_statement(tokens, pos);
                    if (stmt) {
                        if (else_body == NULL) {
                            else_body = stmt;
                            current_stmt = stmt;
                        } else {
                            current_stmt->right = stmt;
                            current_stmt = stmt;
                        }
                    } else if (*pos == start_pos) {
                        (*pos)++;
                    }
                }
                node->extra = else_body;

                if (*pos < tokens->count && tokens->tokens[*pos]->type == RBRACE) {
                    (*pos)++;
                }
            }
        }
        return node;
    }

    // While loop: mientras (cond) { ... }
    if (current->type == MIENTRAS) {
        (*pos)++;
        node = init_ast_node(WHILE_SPANISH_NODE);

        if (*pos < tokens->count && tokens->tokens[*pos]->type == LPAREN) {
            (*pos)++;
            node->left = parse_expression(tokens, pos);
            if (*pos < tokens->count && tokens->tokens[*pos]->type == RPAREN) {
                (*pos)++;
            }
        }

        if (*pos < tokens->count && tokens->tokens[*pos]->type == LBRACE) {
            (*pos)++;
            ASTNode* body = NULL;
            ASTNode* current_stmt = NULL;

            while (*pos < tokens->count && tokens->tokens[*pos]->type != RBRACE) {
                int start_pos = *pos;
                ASTNode* stmt = parse_statement(tokens, pos);
                if (stmt) {
                    if (body == NULL) {
                        body = stmt;
                        current_stmt = stmt;
                    } else {
                        current_stmt->right = stmt;
                        current_stmt = stmt;
                    }
                } else if (*pos == start_pos) {
                    (*pos)++;
                }
            }
            node->right = body;

            if (*pos < tokens->count && tokens->tokens[*pos]->type == RBRACE) {
                (*pos)++;
            }
        }
        return node;
    }

    // For loop: para (init; cond; incr) { ... }
    if (current->type == PARA) {
        (*pos)++;
        node = init_ast_node(FOR_SPANISH_NODE);

        if (*pos < tokens->count && tokens->tokens[*pos]->type == LPAREN) {
            (*pos)++;

            // Init expression
            ASTNode* init_expr = parse_expression(tokens, pos);
            if (*pos < tokens->count && tokens->tokens[*pos]->type == SEMICOLON) (*pos)++;

            // Condition
            ASTNode* cond_expr = parse_expression(tokens, pos);
            if (*pos < tokens->count && tokens->tokens[*pos]->type == SEMICOLON) (*pos)++;

            // Increment
            ASTNode* incr_expr = parse_expression(tokens, pos);

            node->left = init_expr;
            node->params = cond_expr;  // Use params for condition
            node->extra = incr_expr;

            if (*pos < tokens->count && tokens->tokens[*pos]->type == RPAREN) {
                (*pos)++;
            }
        }

        if (*pos < tokens->count && tokens->tokens[*pos]->type == LBRACE) {
            (*pos)++;
            ASTNode* body = NULL;
            ASTNode* current_stmt = NULL;

            while (*pos < tokens->count && tokens->tokens[*pos]->type != RBRACE) {
                int start_pos = *pos;
                ASTNode* stmt = parse_statement(tokens, pos);
                if (stmt) {
                    if (body == NULL) {
                        body = stmt;
                        current_stmt = stmt;
                    } else {
                        current_stmt->right = stmt;
                        current_stmt = stmt;
                    }
                } else if (*pos == start_pos) {
                    (*pos)++;
                }
            }
            node->right = body;

            if (*pos < tokens->count && tokens->tokens[*pos]->type == RBRACE) {
                (*pos)++;
            }
        }
        return node;
    }

    // Return: retorno expr;
    if (current->type == RETORNO) {
        (*pos)++;
        node = init_ast_node(RETURN_NODE);
        if (*pos < tokens->count && tokens->tokens[*pos]->type != SEMICOLON &&
            tokens->tokens[*pos]->type != RBRACE) {
            node->left = parse_expression(tokens, pos);
        }
        if (*pos < tokens->count && tokens->tokens[*pos]->type == SEMICOLON) {
            (*pos)++;
        }
        return node;
    }

    // Break: romper;
    if (current->type == ROMPER) {
        (*pos)++;
        node = init_ast_node(BREAK_NODE);
        if (*pos < tokens->count && tokens->tokens[*pos]->type == SEMICOLON) {
            (*pos)++;
        }
        return node;
    }

    // Continue: continuar;
    if (current->type == CONTINUAR) {
        (*pos)++;
        node = init_ast_node(CONTINUE_NODE);
        if (*pos < tokens->count && tokens->tokens[*pos]->type == SEMICOLON) {
            (*pos)++;
        }
        return node;
    }

    // Free: liberar expr;
    if (current->type == LIBERAR) {
        (*pos)++;
        node = init_ast_node(FREE_NODE);
        node->left = parse_expression(tokens, pos);
        if (*pos < tokens->count && tokens->tokens[*pos]->type == SEMICOLON) {
            (*pos)++;
        }
        return node;
    }

    // Assignment: identifier = expr; or expression statement
    if (current->type == IDENTIFIER) {
        int next_pos = *pos + 1;

        // Assignment
        if (next_pos < tokens->count && tokens->tokens[next_pos]->type == ASSIGN) {
            node = init_ast_node(ASSIGN_NODE);
            node->value = strdup(current->value);
            (*pos) += 2;
            node->left = parse_expression(tokens, pos);
            if (*pos < tokens->count && tokens->tokens[*pos]->type == SEMICOLON) {
                (*pos)++;
            }
            return node;
        }
    }

    // Expression statement
    node = parse_expression(tokens, pos);
    if (*pos < tokens->count && tokens->tokens[*pos]->type == SEMICOLON) {
        (*pos)++;
    }
    return node;
}

// === PROGRAM PARSING ===

ASTNode* parse_program(TokenStream* tokens) {
    ASTNode* root = init_ast_node(PROGRAM_NODE);
    ASTNode* current = NULL;

    int pos = 0;
    while (pos < tokens->count) {
        int start_pos = pos;
        ASTNode* stmt = parse_statement(tokens, &pos);
        if (stmt) {
            if (root->left == NULL) {
                root->left = stmt;
                current = stmt;
            } else {
                current->right = stmt;
                current = stmt;
            }
        } else if (pos == start_pos) {
            pos++;
        }
    }

    return root;
}
