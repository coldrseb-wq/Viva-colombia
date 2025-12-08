#include "interpreter.h"
#include "runtime.h" 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_ast(ASTNode* node, int depth) {
    if (node == NULL) return;

    // Print indentation
    for (int i = 0; i < depth; i++) {
        printf("  ");
    }

    // Print node info based on type
    switch (node->type) {
        case PROGRAM_NODE:
            printf("PROGRAM\n");
            break;
        case EXPRESSION_NODE:
            printf("EXPRESSION: %s\n", node->value ? node->value : "(null)");
            break;
        case BINARY_OP_NODE:
            printf("BINARY_OP: %s\n", node->value ? node->value : "(null)");
            break;
        case NUMBER_NODE:
            printf("NUMBER: %s\n", node->value ? node->value : "(null)");
            break;
        case IDENTIFIER_NODE:
            printf("IDENTIFIER: %s\n", node->value ? node->value : "(null)");
            break;
        case STRING_LITERAL_NODE:
            printf("STRING: %s\n", node->value ? node->value : "(null)");
            break;
        case VAR_DECL_NODE:
            printf("VAR_DECL: %s\n", node->value ? node->value : "(null)");
            break;
        case FN_DECL_NODE:
            printf("FN_DECL: %s\n", node->value ? node->value : "(null)");
            break;
        case FN_CALL_NODE:
            printf("FN_CALL: %s\n", node->value ? node->value : "(null)");
            break;
        case IF_NODE:
            printf("IF\n");
            break;
        case WHILE_NODE:
            printf("WHILE\n");
            break;
        case ASSIGN_NODE:
            printf("ASSIGN: %s\n", node->value ? node->value : "(null)");
            break;
        case VAR_DECL_SPANISH_NODE:
            printf("VAR_DECL_SPANISH: %s\n", node->value ? node->value : "(null)");
            break;
        case FN_DECL_SPANISH_NODE:
            printf("FN_DECL_SPANISH: %s\n", node->value ? node->value : "(null)");
            break;
        case IF_SPANISH_NODE:
            printf("IF_SPANISH\n");
            break;
        case WHILE_SPANISH_NODE:
            printf("WHILE_SPANISH\n");
            break;
        case FOR_SPANISH_NODE:
            printf("FOR_SPANISH: %s\n", node->value ? node->value : "(null)");
            break;
        case UNARY_OP_NODE:
            printf("UNARY_OP: %s\n", node->value ? node->value : "(null)");
            break;
        case CONDITION_NODE:
            printf("CONDITION\n");
            break;
        default:
            printf("UNKNOWN NODE TYPE: %d\n", node->type);
            break;
    }

    // Recursively print children
    if (node->left) {
        print_ast(node->left, depth + 1);
    }
    if (node->right) {
        print_ast(node->right, depth + 1);
    }
}

int interpret_ast(ASTNode* node, SymbolTable* symbol_table) {
    if (node == NULL) return 0;

    switch (node->type) {
        case PROGRAM_NODE: {
            ASTNode* current = node->left;
            while (current != NULL) {
                interpret_ast(current, symbol_table);
                current = current->right;
            }
            break;
        }

        case FN_CALL_NODE: {
            if (strcmp(node->value, "println") == 0 || strcmp(node->value, "print") == 0) {
                int ln = (strcmp(node->value, "println") == 0);
                char* arg_value = NULL;
                int found, is_string;

                if (node->left != NULL) {
                    if (node->left->type == IDENTIFIER_NODE) {
                        arg_value = symbol_table_get(symbol_table, node->left->value, &found, &is_string);
                        if (!found) {
                            arg_value = node->left->value;
                            // Check if it's a string literal
                            if (arg_value[0] == '"' && arg_value[strlen(arg_value)-1] == '"') {
                                is_string = 1;
                            }
                        }
                    } else if (node->left->type == NUMBER_NODE) {
                        arg_value = node->left->value;
                        is_string = 0;
                    } else if (node->left->type == STRING_LITERAL_NODE) {
                        arg_value = node->left->value;
                        is_string = 1;
                    }
                }

                if (arg_value) {
                    if (is_string) {
                        // Remove quotes for printing
                        if (arg_value[0] == '"' && arg_value[strlen(arg_value)-1] == '"') {
                            char* clean = malloc(strlen(arg_value) - 1);
                            strcpy(clean, arg_value + 1);
                            clean[strlen(clean)-1] = '\0';
                            ln ? builtin_println(clean) : builtin_print(clean);
                            free(clean);
                        } else {
                            ln ? builtin_println(arg_value) : builtin_print(arg_value);
                        }
                    } else {
                        int num = atoi(arg_value);
                        char buffer[32];
                        sprintf(buffer, "%d", num);
                        ln ? builtin_println(buffer) : builtin_print(buffer);
                    }
                } else {
                    ln ? builtin_println("") : builtin_print("");
                }
            }
            // Handle Colombian historical figures
            else if (strcmp(node->value, "simon_bolivar") == 0 || strcmp(node->value, "bolivar") == 0) builtin_simon_bolivar();
            else if (strcmp(node->value, "francisco_narino") == 0 || strcmp(node->value, "narino") == 0) builtin_francisco_narino();
            else if (strcmp(node->value, "maria_cano") == 0 || strcmp(node->value, "cano") == 0) builtin_maria_cano();
            else if (strcmp(node->value, "jorge_eliecer_gaitan") == 0 || strcmp(node->value, "gaitan") == 0) builtin_jorge_eliecer_gaitan();
            else if (strcmp(node->value, "gabriel_garcia_marquez") == 0 || strcmp(node->value, "garcia") == 0) builtin_gabriel_garcia_marquez();
            else {
                printf("Unknown function: %s\n", node->value);
            }
            break;
        }

        case VAR_DECL_NODE:
        case VAR_DECL_SPANISH_NODE: {
            // Handle variable declaration: let x = expression;
            char* var_name = node->value;
            if (node->left != NULL) {  // There is an expression to assign
                int value = interpret_ast(node->left, symbol_table);
                char value_str[32];
                sprintf(value_str, "%d", value);
                symbol_table_set(symbol_table, var_name, value_str, 0); // 0 = not string
            } else {
                // Just declare variable with default value
                symbol_table_set(symbol_table, var_name, "0", 0); // 0 = not string
            }
            break;
        }

        case ASSIGN_NODE: {
            // Handle assignment: x = expression;
            char* var_name = node->value;
            if (node->left != NULL) {
                int value = interpret_ast(node->left, symbol_table);
                char value_str[32];
                sprintf(value_str, "%d", value);
                symbol_table_set(symbol_table, var_name, value_str, 0); // 0 = not string
            }
            break;
        }

        case BINARY_OP_NODE: {
            if (!node->left || !node->right) return 0;
            int a = interpret_ast(node->left, symbol_table);
            int b = interpret_ast(node->right, symbol_table);
            
            if (!node->value) return 0;
            
            if (strcmp(node->value, "+") == 0) return a + b;
            else if (strcmp(node->value, "-") == 0) return a - b;
            else if (strcmp(node->value, "*") == 0) return a * b;
            else if (strcmp(node->value, "/") == 0) return b != 0 ? a / b : 0;
            else if (strcmp(node->value, ">") == 0) return a > b;
            else if (strcmp(node->value, "<") == 0) return a < b;
            else if (strcmp(node->value, ">=") == 0) return a >= b;
            else if (strcmp(node->value, "<=") == 0) return a <= b;
            else if (strcmp(node->value, "==") == 0) return a == b;
            else if (strcmp(node->value, "!=") == 0) return a != b;
            return 0;
        }

        case NUMBER_NODE:
            return node->value ? atoi(node->value) : 0;

        case IDENTIFIER_NODE: {
            int found, is_string;
            char* val = symbol_table_get(symbol_table, node->value, &found, &is_string);
            return found && val ? atoi(val) : 0;
        }

        case IF_SPANISH_NODE:  // Handle "si" (if) statements
        case IF_NODE: {
            int condition = interpret_ast(node->left, symbol_table);  // condition
            if (condition) {
                interpret_ast(node->right, symbol_table);  // then block
            }
            break;
        }

        case WHILE_SPANISH_NODE:  // Handle "mientras" (while) loops
        case WHILE_NODE: {
            int condition = interpret_ast(node->left, symbol_table);  // condition
            while (condition) {
                interpret_ast(node->right, symbol_table);  // body
                condition = interpret_ast(node->left, symbol_table);  // re-evaluate condition
            }
            break;
        }

        case FN_DECL_NODE:
        case FN_DECL_SPANISH_NODE: {
            // Function declarations don't execute during interpretation, just store for future calls
            break;
        }

        case UNARY_OP_NODE: {
            if (!node->right) return 0;
            int value = interpret_ast(node->right, symbol_table);
            if (node->value && strcmp(node->value, "no") == 0) {  // Spanish "not"
                return !value;
            }
            return value;
        }

        case CONDITION_NODE: {
            return interpret_ast(node->left, symbol_table);
        }

        default:
            break;
    }

    return 0;
}