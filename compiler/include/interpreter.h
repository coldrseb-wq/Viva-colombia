#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "parser.h"
#include "symbol_table.h"

int interpret_ast(ASTNode* node, SymbolTable* symbol_table);
void print_ast(ASTNode* node, int depth);

#endif