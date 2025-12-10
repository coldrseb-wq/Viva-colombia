#include "interpreter.h"
#include "runtime.h" 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_ast(ASTNode* node, int depth) {
    if (node == NULL) return;

    for (int i = 0; i < depth; i++) {
        printf("  ");
    }

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
        case FOR_NODE:
            printf("FOR\n");
            break;
        case ASSIGN_NODE:
            printf("ASSIGN: %s\n", node->value ? node->value : "(null)");
            break;
        case RETURN_NODE:
            printf("RETURN\n");
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

    if (node->left) {
        print_ast(node->left, depth + 1);
    }
    if (node->right) {
        print_ast(node->right, depth + 1);
    }
    if (node->extra) {
        print_ast(node->extra, depth + 1);
    }
    if (node->next) {
        print_ast(node->next, depth);  // Same depth - siblings
    }
}

static int is_string_literal(const char* str) {
    if (str == NULL) return 0;
    size_t len = strlen(str);
    return (len >= 2 && str[0] == '"' && str[len - 1] == '"');
}

static char* remove_quotes(const char* str) {
    if (str == NULL) return NULL;
    size_t len = strlen(str);
    
    if (len < 2) {
        char* copy = malloc(len + 1);
        if (copy) strcpy(copy, str);
        return copy;
    }
    
    char* clean = malloc(len - 1);
    if (clean == NULL) return NULL;
    
    strncpy(clean, str + 1, len - 2);
    clean[len - 2] = '\0';
    
    return clean;
}

int interpret_ast(ASTNode* node, SymbolTable* symbol_table) {
    if (node == NULL) return 0;

    switch (node->type) {
        case PROGRAM_NODE: {
            ASTNode* current = node->left;
            while (current != NULL) {
                interpret_ast(current, symbol_table);
                // Use 'next' for sibling chaining
                current = current->next;
            }
            break;
        }

        case FN_CALL_NODE: {
            if (node->value == NULL) {
                printf("Error: Function call with NULL name\n");
                break;
            }
            
            if (strcmp(node->value, "println") == 0 || strcmp(node->value, "print") == 0) {
                int ln = (strcmp(node->value, "println") == 0);
                char* arg_value = NULL;
                int found = 0;
                int is_string = 0;

                if (node->left != NULL) {
                    if (node->left->type == IDENTIFIER_NODE) {
                        arg_value = symbol_table_get(symbol_table, node->left->value, &found, &is_string);
                        if (!found) {
                            arg_value = node->left->value;
                            is_string = is_string_literal(arg_value);
                        }
                    } else if (node->left->type == NUMBER_NODE) {
                        arg_value = node->left->value;
                        is_string = 0;
                    } else if (node->left->type == STRING_LITERAL_NODE) {
                        arg_value = node->left->value;
                        is_string = 1;
                    } else if (node->left->type == BINARY_OP_NODE) {
                        int result = interpret_ast(node->left, symbol_table);
                        char buffer[32];
                        sprintf(buffer, "%d", result);
                        ln ? builtin_println(buffer) : builtin_print(buffer);
                        break;
                    }
                }

                if (arg_value) {
                    if (is_string) {
                        if (is_string_literal(arg_value)) {
                            char* clean = remove_quotes(arg_value);
                            if (clean) {
                                ln ? builtin_println(clean) : builtin_print(clean);
                                free(clean);
                            }
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
            else if (strcmp(node->value, "simon_bolivar") == 0 || strcmp(node->value, "bolivar") == 0) {
                builtin_simon_bolivar();
            }
            else if (strcmp(node->value, "francisco_narino") == 0 || strcmp(node->value, "narino") == 0) {
                builtin_francisco_narino();
            }
            else if (strcmp(node->value, "maria_cano") == 0 || strcmp(node->value, "cano") == 0) {
                builtin_maria_cano();
            }
            else if (strcmp(node->value, "jorge_eliecer_gaitan") == 0 || strcmp(node->value, "gaitan") == 0) {
                builtin_jorge_eliecer_gaitan();
            }
            else if (strcmp(node->value, "gabriel_garcia_marquez") == 0 || strcmp(node->value, "garcia") == 0) {
                builtin_gabriel_garcia_marquez();
            }
            else {
                printf("Unknown function: %s\n", node->value);
            }
            break;
        }

        case VAR_DECL_NODE:
        case VAR_DECL_SPANISH_NODE: {
            char* var_name = node->value;
            if (var_name == NULL) {
                printf("Error: Variable declaration with NULL name\n");
                break;
            }
            
            if (node->left != NULL) {
                if (node->left->type == STRING_LITERAL_NODE) {
                    symbol_table_set(symbol_table, var_name, node->left->value, 1);
                } else if (node->left->type == IDENTIFIER_NODE && is_string_literal(node->left->value)) {
                    symbol_table_set(symbol_table, var_name, node->left->value, 1);
                } else {
                    int value = interpret_ast(node->left, symbol_table);
                    char value_str[32];
                    sprintf(value_str, "%d", value);
                    symbol_table_set(symbol_table, var_name, value_str, 0);
                }
            } else {
                symbol_table_set(symbol_table, var_name, "0", 0);
            }
            break;
        }

        case ASSIGN_NODE: {
            char* var_name = node->value;
            if (var_name == NULL) {
                printf("Error: Assignment with NULL variable name\n");
                break;
            }
            
            if (node->left != NULL) {
                if (node->left->type == STRING_LITERAL_NODE) {
                    symbol_table_set(symbol_table, var_name, node->left->value, 1);
                } else if (node->left->type == IDENTIFIER_NODE && is_string_literal(node->left->value)) {
                    symbol_table_set(symbol_table, var_name, node->left->value, 1);
                } else {
                    int value = interpret_ast(node->left, symbol_table);
                    char value_str[32];
                    sprintf(value_str, "%d", value);
                    symbol_table_set(symbol_table, var_name, value_str, 0);
                }
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
            else if (strcmp(node->value, "%") == 0) return b != 0 ? a % b : 0;
            else if (strcmp(node->value, ">") == 0) return a > b;
            else if (strcmp(node->value, "<") == 0) return a < b;
            else if (strcmp(node->value, ">=") == 0) return a >= b;
            else if (strcmp(node->value, "<=") == 0) return a <= b;
            else if (strcmp(node->value, "==") == 0) return a == b;
            else if (strcmp(node->value, "!=") == 0) return a != b;
            else if (strcmp(node->value, "&&") == 0 || strcmp(node->value, "y") == 0) return a && b;
            else if (strcmp(node->value, "||") == 0 || strcmp(node->value, "o") == 0) return a || b;
            return 0;
        }

        case NUMBER_NODE:
            return node->value ? atoi(node->value) : 0;

        case IDENTIFIER_NODE: {
            int found = 0;
            int is_string = 0;
            char* val = symbol_table_get(symbol_table, node->value, &found, &is_string);
            if (!found) {
                printf("Warning: Undefined variable '%s'\n", node->value);
                return 0;
            }
            return val ? atoi(val) : 0;
        }

        case IF_SPANISH_NODE:
        case IF_NODE: {
            int condition = interpret_ast(node->left, symbol_table);
            if (condition) {
                interpret_ast(node->right, symbol_table);
            } else if (node->extra) {
                interpret_ast(node->extra, symbol_table);
            }
            break;
        }

        case WHILE_SPANISH_NODE:
        case WHILE_NODE: {
            while (interpret_ast(node->left, symbol_table)) {
                interpret_ast(node->right, symbol_table);
            }
            break;
        }

        case FOR_SPANISH_NODE:
        case FOR_NODE: {
            if (node->left) {
                interpret_ast(node->left, symbol_table);
            }
            
            if (node->extra) {
                ASTNode* condition_node = node->extra->left;
                ASTNode* increment_node = node->extra->right;
                
                while (condition_node == NULL || interpret_ast(condition_node, symbol_table)) {
                    interpret_ast(node->right, symbol_table);
                    
                    if (increment_node) {
                        interpret_ast(increment_node, symbol_table);
                    }
                    
                    if (condition_node == NULL) break;
                }
            } else {
                interpret_ast(node->right, symbol_table);
            }
            break;
        }

        case RETURN_NODE: {
            if (node->left) {
                return interpret_ast(node->left, symbol_table);
            }
            return 0;
        }

        case FN_DECL_NODE:
        case FN_DECL_SPANISH_NODE: {
            break;
        }

        case UNARY_OP_NODE: {
            if (!node->right && !node->left) return 0;
            ASTNode* operand = node->right ? node->right : node->left;
            int value = interpret_ast(operand, symbol_table);
            
            if (node->value) {
                if (strcmp(node->value, "no") == 0 || strcmp(node->value, "!") == 0) {
                    return !value;
                }
                else if (strcmp(node->value, "-") == 0) {
                    return -value;
                }
            }
            return value;
        }

        case CONDITION_NODE: {
            return interpret_ast(node->left, symbol_table);
        }

        case EXPRESSION_NODE: {
            if (node->left) {
                return interpret_ast(node->left, symbol_table);
            }
            return 0;
        }

        case STRING_LITERAL_NODE: {
            return 0;
        }

        default:
            printf("Warning: Unhandled node type %d\n", node->type);
            break;
    }

    return 0;
}