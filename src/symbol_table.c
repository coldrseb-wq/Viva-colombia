#define _GNU_SOURCE
#include "symbol_table.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

SymbolTable* init_symbol_table() {
    SymbolTable* table = malloc(sizeof(SymbolTable));
    table->count = 0;
    return table;
}

void symbol_table_set(SymbolTable* table, char* name, char* value, int is_string) {
    // Check if variable already exists and update it
    for (int i = 0; i < table->count; i++) {
        if (strcmp(table->variables[i].name, name) == 0) {
            free(table->variables[i].value);
            table->variables[i].value = strdup(value);
            table->variables[i].is_string = is_string;
            return;
        }
    }
    
    // Add new variable if not found
    if (table->count < MAX_VARIABLES) {
        table->variables[table->count].name = strdup(name);
        table->variables[table->count].value = strdup(value);
        table->variables[table->count].is_string = is_string;
        table->count++;
    }
}

char* symbol_table_get(SymbolTable* table, char* name, int* found, int* is_string) {
    *found = 0;
    *is_string = 0;
    
    for (int i = 0; i < table->count; i++) {
        if (strcmp(table->variables[i].name, name) == 0) {
            *found = 1;
            *is_string = table->variables[i].is_string;
            return table->variables[i].value;
        }
    }
    
    return NULL;
}

void free_symbol_table(SymbolTable* table) {
    if (table != NULL) {
        for (int i = 0; i < table->count; i++) {
            free(table->variables[i].name);
            free(table->variables[i].value);
        }
        free(table);
    }
}