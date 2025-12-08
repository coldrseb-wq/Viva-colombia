#include "interpreter.h"
#include "runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int interpret_ast(ASTNode* node, SymbolTable* symbol_table) {
    if (node == NULL) {
        return 0;
    }

    switch (node->type) {
        case PROGRAM_NODE:
            // Process all sequential statements
            // Sequential statements appear to be linked via right pointers starting from node->left
            ASTNode* current = node->left;  // Start with the first statement
            while (current != NULL) {
                interpret_ast(current, symbol_table);
                // Move to the next statement in the sequence (linked via right pointer)
                current = current->right;
            }
            break;

        case FN_CALL_NODE:
            if (strcmp(node->value, "println") == 0) {
                // Get the argument value (now supports variable lookup)
                char* arg_value = NULL;
                int found, is_string;

                if (node->left != NULL) {
                    if (node->left->type == IDENTIFIER_NODE) {
                        // Check if this is a variable lookup
                        arg_value = symbol_table_get(symbol_table, node->left->value, &found, &is_string);
                        if (!found) {
                            // If not found in symbol table, use as-is (might be a string literal)
                            arg_value = node->left->value;
                            // Check if it's a string literal
                            if (arg_value[0] == '"' && arg_value[strlen(arg_value)-1] == '"') {
                                is_string = 1;
                            } else {
                                is_string = 0;
                            }
                        }
                    } else if (node->left->type == NUMBER_NODE) {
                        arg_value = node->left->value;
                        is_string = 0;
                    }
                }

                if (arg_value != NULL) {
                    // Remove quotes from string literals when printing (if it's a string literal stored in a variable)
                    if (arg_value[0] == '"' && arg_value[strlen(arg_value)-1] == '"') {
                        // It's a string literal, print without quotes
                        char* str_without_quotes = malloc(strlen(arg_value) - 1);  // -1 to remove first quote
                        strcpy(str_without_quotes, arg_value + 1);  // Skip first quote
                        str_without_quotes[strlen(str_without_quotes)-1] = '\0';  // Remove last quote
                        builtin_println(str_without_quotes);
                        free(str_without_quotes);
                    } else {
                        // This could be a variable value (e.g., "42" from a number variable)
                        builtin_println(arg_value);
                    }
                } else {
                    builtin_println("");
                }
            }
            else if (strcmp(node->value, "print") == 0) {
                // Similar handling for print function
                char* arg_value = NULL;
                int found, is_string;

                if (node->left != NULL) {
                    if (node->left->type == IDENTIFIER_NODE) {
                        // Check if this is a variable lookup
                        arg_value = symbol_table_get(symbol_table, node->left->value, &found, &is_string);
                        if (!found) {
                            // If not found in symbol table, use as-is
                            arg_value = node->left->value;
                            // Check if it's a string literal
                            if (arg_value[0] == '"' && arg_value[strlen(arg_value)-1] == '"') {
                                is_string = 1;
                            } else {
                                is_string = 0;
                            }
                        }
                    } else if (node->left->type == NUMBER_NODE) {
                        arg_value = node->left->value;
                        is_string = 0;
                    }
                }

                if (arg_value != NULL) {
                    // Remove quotes from string literals when printing
                    if (arg_value[0] == '"' && arg_value[strlen(arg_value)-1] == '"') {
                        // It's a string literal, print without quotes
                        char* str_without_quotes = malloc(strlen(arg_value) - 1);  // -1 to remove first quote
                        strcpy(str_without_quotes, arg_value + 1);  // Skip first quote
                        str_without_quotes[strlen(str_without_quotes)-1] = '\0';  // Remove last quote
                        builtin_print(str_without_quotes);
                        free(str_without_quotes);
                    } else {
                        builtin_print(arg_value);
                    }
                }
            }
            // Colombian hero functions
            else if (strcmp(node->value, "simon_bolivar") == 0) {
                builtin_simon_bolivar();
            }
            else if (strcmp(node->value, "francisco_narino") == 0) {
                builtin_francisco_narino();
            }
            else if (strcmp(node->value, "maria_cano") == 0) {
                builtin_maria_cano();
            }
            else if (strcmp(node->value, "jorge_eliecer_gaitan") == 0) {
                builtin_jorge_eliecer_gaitan();
            }
            else if (strcmp(node->value, "gabriel_garcia_marquez") == 0) {
                builtin_gabriel_garcia_marquez();
            }
            else {
                printf("Unknown function: %s\n", node->value);
            }
            break;

        case BINARY_OP_NODE:
            // Handle binary operations like +, -, *, / and comparisons
            if (node->left != NULL && node->right != NULL) {
                int left_val = interpret_ast(node->left, symbol_table);
                int right_val = interpret_ast(node->right, symbol_table);

                if (node->value != NULL) {
                    if (strcmp(node->value, "+") == 0) {
                        return left_val + right_val;
                    } else if (strcmp(node->value, "-") == 0) {
                        return left_val - right_val;
                    } else if (strcmp(node->value, "*") == 0) {
                        return left_val * right_val;
                    } else if (strcmp(node->value, "/") == 0) {
                        return right_val != 0 ? left_val / right_val : 0;  // avoid division by zero
                    } else if (strcmp(node->value, ">") == 0) {
                        return left_val > right_val ? 1 : 0;  // true if left > right
                    } else if (strcmp(node->value, "<") == 0) {
                        return left_val < right_val ? 1 : 0;  // true if left < right
                    } else if (strcmp(node->value, ">=") == 0) {
                        return left_val >= right_val ? 1 : 0;  // true if left >= right
                    } else if (strcmp(node->value, "<=") == 0) {
                        return left_val <= right_val ? 1 : 0;  // true if left <= right
                    } else if (strcmp(node->value, "==") == 0) {
                        return left_val == right_val ? 1 : 0;  // true if left == right
                    } else if (strcmp(node->value, "!=") == 0) {
                        return left_val != right_val ? 1 : 0;  // true if left != right
                    }
                }
            }
            return 0;

        case NUMBER_NODE:
            // For now, just return the value
            return node->value ? atoi(node->value) : 0;

        case IDENTIFIER_NODE:
            // Look up variable value in symbol table
            {
                int found, is_string;
                char* var_value = symbol_table_get(symbol_table, node->value, &found, &is_string);
                if (found) {
                    return var_value ? atoi(var_value) : 0;
                } else {
                    // If not found, return 0 (or handle as needed)
                    return 0;
                }
            }

        case VAR_DECL_NODE:
        case VAR_DECL_SPANISH_NODE:
            // Handle variable declarations: store the variable in the symbol table
            if (node->value != NULL && node->left != NULL) {
                char* value_to_store = NULL;
                int is_string_type = 0;

                // Check if the expression is a string literal (has quotes)
                if (node->left->type == IDENTIFIER_NODE && node->left->value != NULL) {
                    char* expr_value = node->left->value;
                    if (expr_value[0] == '"' && expr_value[strlen(expr_value)-1] == '"') {
                        // This is a string literal, store directly
                        value_to_store = expr_value;
                        is_string_type = 1;
                    }
                }

                if (value_to_store != NULL) {
                    // Remove quotes from string literal before storing
                    char* str_without_quotes = malloc(strlen(value_to_store) - 1);
                    strcpy(str_without_quotes, value_to_store + 1);  // Skip first quote
                    str_without_quotes[strlen(str_without_quotes)-1] = '\0';  // Remove last quote
                    symbol_table_set(symbol_table, node->value, str_without_quotes, is_string_type);
                    free(str_without_quotes);
                } else {
                    // Evaluate as expression (number, variable reference, etc.)
                    int expr_result = interpret_ast(node->left, symbol_table);
                    char result_str[32];
                    sprintf(result_str, "%d", expr_result);
                    symbol_table_set(symbol_table, node->value, result_str, 0); // 0 indicates not a string
                }
            }
            break;

        case EXPRESSION_NODE:
            if (node->left != NULL) {
                interpret_ast(node->left, symbol_table);
            }
            break;

        case IF_SPANISH_NODE:
            // Handle "si" (if) statements
            // For now, just evaluate the condition (full implementation would include blocks)
            if (node->left != NULL) {
                interpret_ast(node->left, symbol_table);
            }
            break;

        case WHILE_SPANISH_NODE:
            // Handle "mientras" (while) statements
            if (node->left != NULL) {  // node->left is the condition
                // Loop until condition becomes false
                while (interpret_ast(node->left, symbol_table) != 0) {
                    // Execute loop body (node->right) - execute all statements in the sequence
                    ASTNode* current_stmt = node->right;
                    while (current_stmt != NULL) {
                        interpret_ast(current_stmt, symbol_table);
                        current_stmt = current_stmt->right;
                    }
                }
            }
            break;

        case ASSIGN_NODE:
            // Handle assignment: update the variable in the symbol table
            if (node->value != NULL && node->left != NULL) {
                // Evaluate the expression to assign
                int expr_result = interpret_ast(node->left, symbol_table);

                // Convert result to string for storage
                char result_str[32];
                sprintf(result_str, "%d", expr_result);

                // Store in symbol table
                symbol_table_set(symbol_table, node->value, result_str, 0); // 0 indicates not a string
            }
            break;

        case UNARY_OP_NODE:
            // Handle unary operations like "no" (not)
            if (node->value != NULL && node->right != NULL) {
                if (strcmp(node->value, "no") == 0) {
                    return !interpret_ast(node->right, symbol_table);
                }
                // Add additional unary operators as needed
            }
            return 0;

        default:
            break;
    }

    return 0;
}

void print_ast(ASTNode* node, int depth) {
    if (node == NULL) return;

    // Add a depth limit to prevent infinite recursion in case of cycles
    if (depth > 100) {  // reasonable limit to catch infinite loops
        for (int i = 0; i < depth; i++) {
            printf("  ");
        }
        printf("[DEPTH LIMIT REACHED - POSSIBLE CYCLE]\n");
        return;
    }

    for (int i = 0; i < depth; i++) {
        printf("  ");
    }

    switch (node->type) {
        case PROGRAM_NODE:
            printf("PROGRAM_NODE\n");
            break;
        case FN_CALL_NODE:
            printf("FN_CALL_NODE: %s\n", node->value ? node->value : "NULL");
            break;
        case NUMBER_NODE:
            printf("NUMBER_NODE: %s\n", node->value ? node->value : "NULL");
            break;
        case IDENTIFIER_NODE:
            printf("IDENTIFIER_NODE: %s\n", node->value ? node->value : "NULL");
            break;
        case VAR_DECL_NODE:
            printf("VAR_DECL_NODE: %s\n", node->value ? node->value : "NULL");
            break;
        default:
            printf("OTHER_NODE: %d\n", node->type);
            break;
    }

    if (node->left != NULL) {
        print_ast(node->left, depth + 1);
    }
    if (node->right != NULL) {
        print_ast(node->right, depth + 1);
    }
}