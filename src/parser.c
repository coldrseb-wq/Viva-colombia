#define _GNU_SOURCE
#include "parser.h"
#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Placeholder AST node implementation
ASTNode* init_ast_node(NodeType type) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = type;
    node->left = NULL;
    node->right = NULL;
    node->value = NULL;
    return node;
}

void free_ast_node(ASTNode* node) {
    if (node != NULL) {
        // Validate the node type to catch corruption early
        NodeType type = node->type;

        // Only proceed with freeing if the type looks valid
        // This prevents freeing corrupted memory structures
        if (type >= PROGRAM_NODE && type <= CONDITION_NODE) {
            // To handle potential circular references, set pointers to NULL before freeing
            // to avoid accessing memory after it's freed
            ASTNode* left = node->left;
            ASTNode* right = node->right;

            // Set to NULL before recursively freeing to avoid issues with circular references
            node->left = NULL;
            node->right = NULL;

            // Free the subtrees
            if (left != NULL) {
                free_ast_node(left);
            }
            if (right != NULL) {
                free_ast_node(right);
            }

            // Free value if exists
            if (node->value != NULL) {
                free(node->value);
            }

            // Finally free the node itself
            free(node);
        } else {
            // Node is corrupted, just free it directly without recursion to avoid segfault
            free(node->value);  // Free value if it exists
            free(node);         // Free the corrupted node
        }
    }
}

// Helper function to check if a token is an operator
int is_operator(TokenType type) {
    return type == PLUS || type == MINUS || type == MULTIPLY || type == DIVIDE ||
           type == EQUALITY || type == NOT_EQUAL || type == LESS_THAN || type == GREATER_THAN;
}

// Parse the primary part of an expression (numbers, identifiers, function calls, parenthesized expressions)
ASTNode* parse_primary(TokenStream* tokens, int* pos) {
    if (*pos >= tokens->count) {
        return NULL;
    }

    Token* current_token = tokens->tokens[*pos];
    ASTNode* node = NULL;

    // Handle unary operators (like "no" for NOT)
    if (current_token->type == NO) {
        (*pos)++;  // skip 'no' token
        // Parse the operand for the unary operator
        ASTNode* operand = parse_primary(tokens, pos);

        if (operand != NULL) {
            node = init_ast_node(UNARY_OP_NODE);
            node->value = strdup("no");  // operator
            node->right = operand;  // operand
        }
        return node;
    }

    if (current_token->type == NUMBER) {
        node = init_ast_node(NUMBER_NODE);
        node->value = strdup(current_token->value);
        (*pos)++;
    }
    else if (current_token->type == STRING) {
        node = init_ast_node(IDENTIFIER_NODE); // Using IDENTIFIER_NODE for strings too, for simplicity
        node->value = malloc(strlen(current_token->value) + 3); // +3 for quotes and null terminator
        sprintf(node->value, "\"%s\"", current_token->value);
        (*pos)++;
    }
    else if (current_token->type == IDENTIFIER) {
        // Check if this is a function call
        int next_pos = *pos + 1;
        if (next_pos < tokens->count && tokens->tokens[next_pos]->type == LPAREN) {
            node = init_ast_node(FN_CALL_NODE);
            node->value = strdup(current_token->value);  // function name
            (*pos) += 2;  // skip identifier and left parenthesis

            // Parse the first argument if it exists (for now, just handle single argument)
            if (*pos < tokens->count && tokens->tokens[*pos]->type != RPAREN) {
                node->left = parse_expression(tokens, pos);
            }

            // Skip to the closing parenthesis
            int paren_count = 1;
            while (*pos < tokens->count && paren_count > 0) {
                if (*pos < tokens->count && tokens->tokens[*pos]->type == LPAREN) {
                    paren_count++;
                } else if (*pos < tokens->count && tokens->tokens[*pos]->type == RPAREN) {
                    paren_count--;
                }
                if (paren_count > 0) {
                    (*pos)++;
                }
            }
            if (*pos < tokens->count) (*pos)++; // skip final RPAREN
        } else {
            node = init_ast_node(IDENTIFIER_NODE);
            node->value = strdup(current_token->value);
            (*pos)++;
        }
    }
    else if (current_token->type == LPAREN) {
        // Handle parenthesized expressions
        (*pos)++; // skip '('
        node = parse_expression(tokens, pos);
        // Expect closing parenthesis
        if (*pos < tokens->count && tokens->tokens[*pos]->type == RPAREN) {
            (*pos)++; // skip ')'
        }
    }

    return node;
}

// Operator precedence levels
typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,  // =
    PREC_OR,          // or, ||
    PREC_AND,         // and, &&
    PREC_EQUALITY,    // ==, !=
    PREC_COMPARISON,  // <, >, <=, >=
    PREC_TERM,        // +, -
    PREC_FACTOR,      // *, /
    PREC_UNARY,       // !, - (unary)
    PREC_PRIMARY      // numbers, identifiers, grouping
} Precedence;

Precedence get_precedence(TokenType type) {
    switch (type) {
        case PLUS:
        case MINUS:
            return PREC_TERM;
        case MULTIPLY:
        case DIVIDE:
            return PREC_FACTOR;
        case EQUALITY:
        case NOT_EQUAL:
            return PREC_EQUALITY;
        case LESS_THAN:
        case GREATER_THAN:
            return PREC_COMPARISON;
        default:
            return PREC_NONE;
    }
}

// Parse a simple expression with operators (with precedence)
ASTNode* parse_expression_helper(TokenStream* tokens, int* pos, Precedence precedence) {
    ASTNode* left = parse_primary(tokens, pos);
    if (left == NULL) {
        return NULL;
    }

    while (*pos < tokens->count) {
        Token* current_token = tokens->tokens[*pos];
        Precedence current_prec = get_precedence(current_token->type);

        if (current_prec < precedence) {
            break;
        }

        // Consume the operator token
        (*pos)++;

        // Create a binary operation node
        ASTNode* op_node = init_ast_node(BINARY_OP_NODE);
        op_node->value = strdup(current_token->value);  // operator symbol
        op_node->left = left;  // left operand

        // Parse the right operand with higher precedence to handle left associativity correctly
        Precedence next_precedence = current_prec + 1;
        op_node->right = parse_expression_helper(tokens, pos, next_precedence);

        if (op_node->right == NULL) {
            // If we can't parse the right operand, return the left part
            // Important: Only free the operator node we just created, not its left child
            // which might be a valid node in the AST
            free(op_node->value);  // Free the operator value
            free(op_node);         // Free the operator node itself
            // Don't free op_node->left or op_node->right as they might be valid AST parts
            return left;
        }

        left = op_node;  // The result becomes the new left operand
    }

    return left;
}

// Main expression parsing function that starts at lowest precedence
ASTNode* parse_expression(TokenStream* tokens, int* pos) {
    return parse_expression_helper(tokens, pos, PREC_NONE);
}

ASTNode* parse_statement(TokenStream* tokens, int* pos) {
    if (*pos >= tokens->count) {
        return NULL;
    }

    Token* current_token = tokens->tokens[*pos];
    ASTNode* node = NULL;

    if (current_token->type == LET || current_token->type == DECRETO) {
        // Handle variable declaration with assignment: decreto var = value;
        (*pos)++; // skip 'let' or 'decreto'

        // Expect an identifier (variable name)
        if (*pos < tokens->count && tokens->tokens[*pos]->type == IDENTIFIER) {
            char* var_name = strdup(tokens->tokens[*pos]->value);
            (*pos)++; // skip identifier

            // Expect assignment operator
            if (*pos < tokens->count && tokens->tokens[*pos]->type == ASSIGN) {
                (*pos)++; // skip assignment operator

                // Parse the expression to be assigned
                ASTNode* value_expr = parse_expression(tokens, pos);

                // Create assignment node
                if (current_token->type == DECRETO) {
                    node = init_ast_node(VAR_DECL_SPANISH_NODE);  // Spanish node type for variable declaration
                } else {
                    node = init_ast_node(VAR_DECL_NODE);  // English node type
                }
                node->value = var_name;  // variable name
                node->left = value_expr; // value to assign
            } else {
                // Handle case where there's no assignment (just declaration)
                if (current_token->type == DECRETO) {
                    node = init_ast_node(VAR_DECL_SPANISH_NODE);
                } else {
                    node = init_ast_node(VAR_DECL_NODE);
                }
                node->value = var_name;
                free(var_name);
            }
        }
    } else if (current_token->type == SI) {
        // Handle "si" (if) statements: si (condition) { then_block }
        (*pos)++; // skip 'si'

        // Expect opening parenthesis
        if (*pos < tokens->count && tokens->tokens[*pos]->type == LPAREN) {
            (*pos)++; // skip '('

            // Parse the condition
            ASTNode* condition = parse_expression(tokens, pos);

            // Expect closing parenthesis
            if (*pos < tokens->count && tokens->tokens[*pos]->type == RPAREN) {
                (*pos)++; // skip ')'

                // Expect opening brace for the block
                if (*pos < tokens->count && tokens->tokens[*pos]->type == LBRACE) {
                    (*pos)++; // skip '{'

                    // Parse the then block body
                    ASTNode* then_body = NULL;
                    ASTNode* current_stmt = NULL;

                    // Parse statements in the block until closing brace
                    while (*pos < tokens->count && tokens->tokens[*pos]->type != RBRACE) {
                        ASTNode* stmt = parse_statement(tokens, pos);
                        if (stmt != NULL) {
                            if (then_body == NULL) {
                                then_body = stmt;
                                current_stmt = stmt;
                            } else {
                                current_stmt->right = stmt;
                                current_stmt = stmt;
                            }
                        }
                    }

                    // Skip closing brace
                    if (*pos < tokens->count && tokens->tokens[*pos]->type == RBRACE) {
                        (*pos)++; // skip '}'
                    }

                    // Create if node
                    node = init_ast_node(IF_SPANISH_NODE);
                    node->left = condition;   // condition
                    node->right = then_body;  // then block body
                } else {
                    // Create if node with just condition if no block found
                    node = init_ast_node(IF_SPANISH_NODE);
                    node->left = condition;   // condition
                    // then block will be NULL
                }
            }
        }
    } else if (current_token->type == MIENTRAS) {
        // Handle "mientras" (while) statements: mientras (condition) { block }
        (*pos)++; // skip 'mientras'

        // Expect opening parenthesis
        if (*pos < tokens->count && tokens->tokens[*pos]->type == LPAREN) {
            (*pos)++; // skip '('

            // Parse the condition
            ASTNode* condition = parse_expression(tokens, pos);

            // Expect closing parenthesis
            if (*pos < tokens->count && tokens->tokens[*pos]->type == RPAREN) {
                (*pos)++; // skip ')'

                // Expect opening brace for the block
                if (*pos < tokens->count && tokens->tokens[*pos]->type == LBRACE) {
                    (*pos)++; // skip '{'

                    // Parse the block/loop body
                    ASTNode* loop_body = NULL;
                    ASTNode* current_stmt = NULL;

                    // Parse statements in the block until closing brace
                    while (*pos < tokens->count && tokens->tokens[*pos]->type != RBRACE) {
                        ASTNode* stmt = parse_statement(tokens, pos);
                        if (stmt != NULL) {
                            if (loop_body == NULL) {
                                loop_body = stmt;
                                current_stmt = stmt;
                            } else {
                                current_stmt->right = stmt;
                                current_stmt = stmt;
                            }
                        }
                    }

                    // Skip closing brace
                    if (*pos < tokens->count && tokens->tokens[*pos]->type == RBRACE) {
                        (*pos)++; // skip '}'
                    }

                    // Create while node
                    node = init_ast_node(WHILE_SPANISH_NODE);
                    node->left = condition;  // condition
                    node->right = loop_body; // loop body
                } else {
                    // Create while node with just condition if no block found
                    node = init_ast_node(WHILE_SPANISH_NODE);
                    node->left = condition;  // condition
                    // loop body will be NULL
                }
            }
        }
    } else if (current_token->type == FN || current_token->type == CANCION) {
        // Handle function declarations: fn name(params) { body } or cancion name(params) { body }
        int is_spanish = (current_token->type == CANCION);
        (*pos)++; // skip 'fn' or 'cancion'

        // Expect function name (identifier)
        if (*pos < tokens->count && tokens->tokens[*pos]->type == IDENTIFIER) {
            char* func_name = strdup(tokens->tokens[*pos]->value);
            (*pos)++; // skip function name

            // Expect opening parenthesis for parameters
            if (*pos < tokens->count && tokens->tokens[*pos]->type == LPAREN) {
                (*pos)++; // skip '('

                // For now, we'll parse the parameter list but ignore it
                // In a full implementation we'd build the parameter list
                int paren_count = 1;
                while (*pos < tokens->count && paren_count > 0) {
                    if (tokens->tokens[*pos]->type == LPAREN) {
                        paren_count++;
                    } else if (tokens->tokens[*pos]->type == RPAREN) {
                        paren_count--;
                    }
                    if (paren_count > 0) {
                        (*pos)++;
                    }
                }
                if (*pos < tokens->count) (*pos)++; // skip final RPAREN

                // Expect opening brace for function body
                if (*pos < tokens->count && tokens->tokens[*pos]->type == LBRACE) {
                    (*pos)++; // skip '{'

                    // Parse the function body
                    ASTNode* func_body = NULL;
                    ASTNode* current_stmt = NULL;

                    // Parse statements in the function body until closing brace
                    while (*pos < tokens->count && tokens->tokens[*pos]->type != RBRACE) {
                        ASTNode* stmt = parse_statement(tokens, pos);
                        if (stmt != NULL) {
                            if (func_body == NULL) {
                                func_body = stmt;
                                current_stmt = stmt;
                            } else {
                                current_stmt->right = stmt;
                                current_stmt = stmt;
                            }
                        }
                    }

                    // Skip closing brace
                    if (*pos < tokens->count && tokens->tokens[*pos]->type == RBRACE) {
                        (*pos)++; // skip '}'
                    }

                    // Create function declaration node
                    ASTNode* func_node = init_ast_node(is_spanish ? FN_DECL_SPANISH_NODE : FN_DECL_NODE);
                    func_node->value = func_name;  // function name
                    func_node->right = func_body;  // function body

                    return func_node;
                } else {
                    free(func_name);
                    return NULL;
                }
            } else {
                free(func_name);
                return NULL;
            }
        } else {
            return NULL;
        }
    } else if (current_token->type == SEMICOLON) {
        // Skip semicolon tokens - they are statement terminators
        (*pos)++;
        // Return NULL to indicate this is just a semicolon, not a statement
        return NULL;
    } else if (current_token->type == IDENTIFIER) {
        // Handle assignment statements: identifier = expression;
        int next_pos = *pos + 1;
        if (next_pos < tokens->count && tokens->tokens[next_pos]->type == ASSIGN) {
            char* var_name = strdup(tokens->tokens[*pos]->value);
            (*pos) += 2; // skip identifier and assignment operator

            ASTNode* value_expr = parse_expression(tokens, pos);

            node = init_ast_node(ASSIGN_NODE);
            node->value = var_name;
            node->left = value_expr; // value to assign
            return node;
        } else {
            // If it's just an identifier without assignment, continue to expression parsing
            node = parse_expression(tokens, pos);
        }
    } else {
        // Try to parse as expression
        node = parse_expression(tokens, pos);
    }

    return node;
}

ASTNode* parse_program(TokenStream* tokens) {
    ASTNode* root = init_ast_node(PROGRAM_NODE);
    ASTNode* current = NULL;
    
    int pos = 0;
    while (pos < tokens->count) {
        ASTNode* stmt = parse_statement(tokens, &pos);
        if (stmt != NULL) {
            if (root->left == NULL) {
                root->left = stmt; // First statement
                current = stmt;
            } else {
                // Link statements together
                current->right = stmt;
                current = stmt;
            }
        }
    }

    return root;
}