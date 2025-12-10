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
    node->body = NULL;
    node->extra = NULL;
    node->value = NULL;
    return node;
}

void free_ast_node(ASTNode* node) {
    if (node != NULL) {
        // Validate the node type to catch corruption early
        NodeType type = node->type;

        // Only proceed with freeing if the type looks valid
        // This prevents freeing corrupted memory structures
        if (type >= PROGRAM_NODE && type <= ELSE_NODE) {
            // To handle potential circular references, set pointers to NULL before freeing
            // to avoid accessing memory after it's freed
            ASTNode* left = node->left;
            ASTNode* right = node->right;
            ASTNode* body = node->body;
            ASTNode* extra = node->extra;

            // Set to NULL before recursively freeing to avoid issues with circular references
            node->left = NULL;
            node->right = NULL;
            node->body = NULL;
            node->extra = NULL;

            // Free the subtrees
            if (left != NULL) {
                free_ast_node(left);
            }
            if (right != NULL) {
                free_ast_node(right);
            }
            if (body != NULL) {
                free_ast_node(body);
            }
            if (extra != NULL) {
                free_ast_node(extra);
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
           type == EQUALITY || type == NOT_EQUAL || type == LESS_THAN || type == GREATER_THAN ||
           type == LESS_EQUAL || type == GREATER_EQUAL;
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

            // Parse arguments as comma-separated list
            ASTNode* first_arg = NULL;
            ASTNode* current_arg = NULL;

            while (*pos < tokens->count && tokens->tokens[*pos]->type != RPAREN) {
                ASTNode* arg = parse_expression(tokens, pos);
                if (arg) {
                    if (first_arg == NULL) {
                        first_arg = arg;
                        current_arg = arg;
                    } else {
                        current_arg->right = arg;
                        current_arg = arg;
                    }
                }

                // Check for comma (more arguments) or end
                if (*pos < tokens->count && tokens->tokens[*pos]->type == COMMA) {
                    (*pos)++; // skip comma
                } else {
                    break; // end of arguments
                }
            }
            node->left = first_arg;

            // Skip closing parenthesis
            if (*pos < tokens->count && tokens->tokens[*pos]->type == RPAREN) {
                (*pos)++;
            }

            // Check and consume semicolon if present after function call
            if (*pos < tokens->count && tokens->tokens[*pos]->type == SEMICOLON) {
                (*pos)++; // skip semicolon
            }
        } else {
            node = init_ast_node(IDENTIFIER_NODE);
            node->value = strdup(current_token->value);
            (*pos)++;

            // Check for array access: identifier[expression]
            if (*pos < tokens->count && tokens->tokens[*pos]->type == LBRACKET) {
                // Transform from IDENTIFIER_NODE to ARRAY_ACCESS_NODE
                char* array_name = strdup(node->value);
                free_ast_node(node); // Free the temporary identifier node
                node = init_ast_node(ARRAY_ACCESS_NODE);
                node->value = array_name;

                (*pos)++; // skip '['

                // Parse the index expression
                node->left = parse_expression(tokens, pos);

                // Expect closing ']'
                if (*pos < tokens->count && tokens->tokens[*pos]->type == RBRACKET) {
                    (*pos)++; // skip ']'
                } else {
                    // Error: expected closing bracket
                    fprintf(stderr, "Error: Expected ']' at line %d\n", current_token->line);
                }
            }

            // Check and consume semicolon if present after identifier/array access (in case of variable usage without assignment)
            if (*pos < tokens->count && tokens->tokens[*pos]->type == SEMICOLON) {
                (*pos)++; // skip semicolon
            }
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
        case LESS_EQUAL:
        case GREATER_EQUAL:
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

        // If this is not an operator (PREC_NONE), stop expression parsing
        if (current_prec == PREC_NONE) {
            break;
        }

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

                // Check and consume semicolon if present
                if (*pos < tokens->count && tokens->tokens[*pos]->type == SEMICOLON) {
                    (*pos)++; // skip semicolon
                }
            } else {
                // Handle case where there's no assignment (just declaration)
                if (current_token->type == DECRETO) {
                    node = init_ast_node(VAR_DECL_SPANISH_NODE);
                } else {
                    node = init_ast_node(VAR_DECL_NODE);
                }
                node->value = var_name;
                free(var_name);

                // Check and consume semicolon if present
                if (*pos < tokens->count && tokens->tokens[*pos]->type == SEMICOLON) {
                    (*pos)++; // skip semicolon
                }
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
                        int original_pos = *pos;  // Track original position
                        ASTNode* stmt = parse_statement(tokens, pos);
                        if (stmt != NULL) {
                            if (then_body == NULL) {
                                then_body = stmt;
                                current_stmt = stmt;
                            } else {
                                current_stmt->right = stmt;
                                current_stmt = stmt;
                            }
                        } else if (*pos == original_pos) {
                            // If no statement was parsed and position didn't advance,
                            // we need to skip the current token to avoid infinite loop
                            (*pos)++;
                        }
                    }

                    // Skip closing brace
                    if (*pos < tokens->count && tokens->tokens[*pos]->type == RBRACE) {
                        (*pos)++; // skip '}'
                    }

                    // Check and consume semicolon if present after the if block
                    if (*pos < tokens->count && tokens->tokens[*pos]->type == SEMICOLON) {
                        (*pos)++; // skip semicolon
                    }

                    // Create if node
                    node = init_ast_node(IF_SPANISH_NODE);
                    node->left = condition;   // condition
                    node->body = then_body;   // then block body (use body field, not right)

                    // Check for 'sino' (else) clause after the if block
                    if (*pos < tokens->count && tokens->tokens[*pos]->type == SINO) {
                        (*pos)++; // skip 'sino'

                        // Expect opening brace for the else block
                        if (*pos < tokens->count && tokens->tokens[*pos]->type == LBRACE) {
                            (*pos)++; // skip '{'

                            // Parse the else block body
                            ASTNode* else_body = NULL;
                            ASTNode* current_stmt = NULL;

                            // Parse statements in the else block until closing brace
                            while (*pos < tokens->count && tokens->tokens[*pos]->type != RBRACE) {
                                int original_pos = *pos;  // Track original position
                                ASTNode* stmt = parse_statement(tokens, pos);
                                if (stmt != NULL) {
                                    if (else_body == NULL) {
                                        else_body = stmt;
                                        current_stmt = stmt;
                                    } else {
                                        current_stmt->right = stmt;
                                        current_stmt = stmt;
                                    }
                                } else if (*pos == original_pos) {
                                    // If no statement was parsed and position didn't advance,
                                    // we need to skip the current token to avoid infinite loop
                                    (*pos)++;
                                }
                            }

                            // Skip closing brace
                            if (*pos < tokens->count && tokens->tokens[*pos]->type == RBRACE) {
                                (*pos)++; // skip '}'
                            }

                            // Store else body in extra field
                            node->extra = else_body;
                        }
                    }
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
                        int original_pos = *pos;  // Track original position
                        ASTNode* stmt = parse_statement(tokens, pos);
                        if (stmt != NULL) {
                            if (loop_body == NULL) {
                                loop_body = stmt;
                                current_stmt = stmt;
                            } else {
                                current_stmt->right = stmt;
                                current_stmt = stmt;
                            }
                        } else if (*pos == original_pos) {
                            // If no statement was parsed and position didn't advance,
                            // we need to skip the current token to avoid infinite loop
                            (*pos)++;
                        }
                    }

                    // Skip closing brace
                    if (*pos < tokens->count && tokens->tokens[*pos]->type == RBRACE) {
                        (*pos)++; // skip '}'
                    }

                    // Check and consume semicolon if present after the while block
                    if (*pos < tokens->count && tokens->tokens[*pos]->type == SEMICOLON) {
                        (*pos)++; // skip semicolon
                    }

                    // Create while node
                    node = init_ast_node(WHILE_SPANISH_NODE);
                    node->left = condition;  // condition
                    node->body = loop_body;  // loop body (use body field, not right)
                } else {
                    // Create while node with just condition if no block found
                    node = init_ast_node(WHILE_SPANISH_NODE);
                    node->left = condition;  // condition
                    // loop body will be NULL
                }
            }
        }
    } else if (current_token->type == PARA) {
        // Handle "para" (for) loops: para (init; condition; increment) { body }
        // For now, we'll support a simplified version: para identifier = start, end { body }
        (*pos)++; // skip 'para'

        // Expect opening parenthesis
        if (*pos < tokens->count && tokens->tokens[*pos]->type == LPAREN) {
            (*pos)++; // skip '('

            // Parse the for loop components: init, condition, increment
            // Simple form: for var = start to end
            ASTNode* init_expr = NULL;
            ASTNode* condition_expr = NULL;
            ASTNode* increment_expr = NULL;

            // For now, parse a simplified form like: para i = 0; i < 10; i = i + 1)
            // Parse initialization expression
            init_expr = parse_expression(tokens, pos);

            // Expect semicolon
            if (*pos < tokens->count && tokens->tokens[*pos]->type == SEMICOLON) {
                (*pos)++; // skip ';'
            }

            // Parse condition expression
            condition_expr = parse_expression(tokens, pos);

            // Expect semicolon
            if (*pos < tokens->count && tokens->tokens[*pos]->type == SEMICOLON) {
                (*pos)++; // skip ';'
            }

            // Parse increment expression
            increment_expr = parse_expression(tokens, pos);

            // Expect closing parenthesis
            if (*pos < tokens->count && tokens->tokens[*pos]->type == RPAREN) {
                (*pos)++; // skip ')'

                // Expect opening brace for the block
                if (*pos < tokens->count && tokens->tokens[*pos]->type == LBRACE) {
                    (*pos)++; // skip '{'

                    // Parse the loop body
                    ASTNode* loop_body = NULL;
                    ASTNode* current_stmt = NULL;

                    // Parse statements in the block until closing brace
                    while (*pos < tokens->count && tokens->tokens[*pos]->type != RBRACE) {
                        int original_pos = *pos;  // Track original position
                        ASTNode* stmt = parse_statement(tokens, pos);
                        if (stmt != NULL) {
                            if (loop_body == NULL) {
                                loop_body = stmt;
                                current_stmt = stmt;
                            } else {
                                current_stmt->right = stmt;
                                current_stmt = stmt;
                            }
                        } else if (*pos == original_pos) {
                            // If no statement was parsed and position didn't advance,
                            // we need to skip the current token to avoid infinite loop
                            (*pos)++;
                        }
                    }

                    // Skip closing brace
                    if (*pos < tokens->count && tokens->tokens[*pos]->type == RBRACE) {
                        (*pos)++; // skip '}'
                    }

                    // Create a special structure for the for loop:
                    // node->left = init_condition_increment (linked list of 3 expressions)
                    // node->right = loop_body
                    ASTNode* for_node = init_ast_node(FOR_SPANISH_NODE);

                    // Create a chain of the three expressions: init -> condition -> increment
                    ASTNode* init_chain = init_expr;
                    if (init_expr) {
                        init_expr->right = condition_expr;
                        if (condition_expr) {
                            condition_expr->right = increment_expr;
                        }
                    }

                    for_node->left = init_chain;  // init; condition; increment
                    for_node->right = loop_body;  // loop body

                    return for_node;
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

            // Create function node early to store parameters
            ASTNode* func_node = init_ast_node(is_spanish ? FN_DECL_SPANISH_NODE : FN_DECL_NODE);
            func_node->value = func_name;  // function name

            // Expect opening parenthesis for parameters
            if (*pos < tokens->count && tokens->tokens[*pos]->type == LPAREN) {
                (*pos)++; // skip '('

                // Parse the parameter list - for now, just track them as identifiers
                // We'll create a linked list of parameter names in the right->left (parameters in the left of the right node)
                int paren_count = 1;
                ASTNode* param_list = NULL;
                ASTNode* current_param = NULL;

                // Check if there are actually parameters (not just empty parentheses)
                if (*pos < tokens->count && tokens->tokens[*pos]->type != RPAREN) {
                    while (*pos < tokens->count && paren_count > 0) {
                        if (tokens->tokens[*pos]->type == IDENTIFIER) {
                            // Found a parameter identifier
                            ASTNode* param_node = init_ast_node(IDENTIFIER_NODE);
                            param_node->value = strdup(tokens->tokens[*pos]->value);

                            if (param_list == NULL) {
                                param_list = param_node;
                                current_param = param_node;
                            } else {
                                current_param->right = param_node;  // Link parameters together
                                current_param = param_node;
                            }

                            (*pos)++;
                        } else if (tokens->tokens[*pos]->type == COMMA) {
                            // Skip comma between parameters
                            (*pos)++;
                        } else if (tokens->tokens[*pos]->type == LPAREN) {
                            paren_count++;
                            (*pos)++;
                        } else if (tokens->tokens[*pos]->type == RPAREN) {
                            paren_count--;
                            if (paren_count > 0) {
                                (*pos)++;
                            }
                        } else {
                            // Unexpected token, just advance to avoid infinite loop
                            (*pos)++;
                        }
                    }
                }

                // If we're still at RPAREN, advance
                if (*pos < tokens->count && tokens->tokens[*pos]->type == RPAREN) {
                    (*pos)++; // skip final RPAREN
                }

                // Store the parameter list in the function node as the left child of the right node
                // This allows us to have: func_node->left = parameters, func_node->right = body
                // Actually, let's store parameters in func_node->left to keep it clean
                func_node->left = param_list; // Store function parameters
            }

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

                // Store the function body in func_node->body (consistent with control flow nodes)
                func_node->body = func_body;  // function body

                return func_node;
            } else {
                // No body, still return the function node with parameters
                func_node->body = NULL;  // no body
                return func_node;
            }
        } else {
            return NULL;
        }
    } else if (current_token->type == ROMPER) {
        // Handle "romper" (break) statements: romper;
        (*pos)++; // skip 'romper'

        // Create break node (no expression needed)
        node = init_ast_node(BREAK_NODE);
        // No left child for break statement

        // Check and consume semicolon if present
        if (*pos < tokens->count && tokens->tokens[*pos]->type == SEMICOLON) {
            (*pos)++; // skip semicolon
        }
        return node;
    } else if (current_token->type == CONTINUAR) {
        // Handle "continuar" (continue) statements: continuar;
        (*pos)++; // skip 'continuar'

        // Create continue node (no expression needed)
        node = init_ast_node(CONTINUE_NODE);
        // No left child for continue statement

        // Check and consume semicolon if present
        if (*pos < tokens->count && tokens->tokens[*pos]->type == SEMICOLON) {
            (*pos)++; // skip semicolon
        }
        return node;
    } else if (current_token->type == SEMICOLON) {
        // Skip semicolon tokens - they are statement terminators
        (*pos)++;
        // Return NULL to indicate this is just a semicolon, not a statement
        return NULL;
    } else if (current_token->type == RETORNO) {
        // Handle "retorno" (return) statements: retorno expression;
        (*pos)++; // skip 'retorno'

        // Parse the return expression (if there is one)
        ASTNode* return_expr = NULL;
        if (*pos < tokens->count && tokens->tokens[*pos]->type != SEMICOLON &&
            tokens->tokens[*pos]->type != RBRACE) {
            return_expr = parse_expression(tokens, pos);
        }

        // Create return node
        node = init_ast_node(RETURN_NODE);
        node->left = return_expr; // return value expression (or NULL if no value)

        // Check and consume semicolon if present
        if (*pos < tokens->count && tokens->tokens[*pos]->type == SEMICOLON) {
            (*pos)++; // skip semicolon
        }
        return node;
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

            // Check and consume semicolon if present
            if (*pos < tokens->count && tokens->tokens[*pos]->type == SEMICOLON) {
                (*pos)++; // skip semicolon
            }
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
        int original_pos = pos;  // Track original position
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
        } else if (pos == original_pos) {
            // If no statement was parsed and position didn't advance,
            // we need to skip the current token to avoid infinite loop
            pos++;
        }
    }

    return root;
}