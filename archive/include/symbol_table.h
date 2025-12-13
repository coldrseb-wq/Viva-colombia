#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_VARIABLES 100

typedef struct {
    char* name;
    char* value;  // Storing as string for simplicity, can represent numbers or strings
    int is_string; // 1 if it's a string literal, 0 if it's a number or identifier
} Variable;

typedef struct {
    Variable variables[MAX_VARIABLES];
    int count;
} SymbolTable;

SymbolTable* init_symbol_table();
void symbol_table_set(SymbolTable* table, char* name, char* value, int is_string);
char* symbol_table_get(SymbolTable* table, char* name, int* found, int* is_string);
void free_symbol_table(SymbolTable* table);

#endif