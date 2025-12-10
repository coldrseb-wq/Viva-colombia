#define _GNU_SOURCE
#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

Token* init_token(TokenType type, char* value, int line) {
    Token* token = malloc(sizeof(Token));
    token->type = type;
    token->value = strdup(value);
    token->line = line;
    return token;
}

void free_token(Token* token) {
    if (token != NULL) {
        free(token->value);
        free(token);
    }
}

TokenStream* init_token_stream() {
    TokenStream* stream = malloc(sizeof(TokenStream));
    stream->tokens = malloc(INITIAL_TOKEN_CAPACITY * sizeof(Token*));
    stream->capacity = INITIAL_TOKEN_CAPACITY;
    stream->count = 0;
    return stream;
}

void add_token(TokenStream* stream, Token* token) {
    if (stream->count >= stream->capacity) {
        stream->capacity *= 2;
        stream->tokens = realloc(stream->tokens, stream->capacity * sizeof(Token*));
    }
    stream->tokens[stream->count++] = token;
}

void free_token_stream(TokenStream* stream) {
    if (stream != NULL) {
        for (int i = 0; i < stream->count; i++) {
            free_token(stream->tokens[i]);
        }
        free(stream->tokens);
        free(stream);
    }
}

TokenStream* tokenize(const char* source) {
    TokenStream* stream = init_token_stream();
    int line = 1;
    const char* current = source;

    while (*current != '\0') {
        // Skip whitespace
        while (isspace(*current)) {
            if (*current == '\n') line++;
            current++;
        }

        // Skip single-line comments
        if (*current == '/' && *(current + 1) == '/') {
            current += 2; // Skip '//'
            while (*current != '\0' && *current != '\n') {
                current++;
            }
            if (*current == '\n') {
                line++;
                current++;
            }
            continue; // Skip to next iteration to handle whitespace
        }

        if (*current == '\0') break;

        // Simple token recognition
        char buffer[MAX_TOKEN_LENGTH];
        int i = 0;

        if (isalpha(*current) || *current == '_') {
            // Identifier or keyword
            while (isalnum(*current) || *current == '_') {
                if (i < MAX_TOKEN_LENGTH - 1) {
                    buffer[i++] = *current;
                }
                current++;
            }
            buffer[i] = '\0';
            
            TokenType type = IDENTIFIER; // Default to identifier
            
            // Check if it's a keyword
            if (strcmp(buffer, "let") == 0 || strcmp(buffer, "decreto") == 0) type = DECRETO;
            else if (strcmp(buffer, "fn") == 0 || strcmp(buffer, "cancion") == 0) type = CANCION;
            else if (strcmp(buffer, "funcion") == 0) type = FUNCION;
            else if (strcmp(buffer, "if") == 0 || strcmp(buffer, "si") == 0) type = SI;
            else if (strcmp(buffer, "else") == 0 || strcmp(buffer, "sino") == 0) type = SINO;
            else if (strcmp(buffer, "while") == 0 || strcmp(buffer, "mientras") == 0) type = MIENTRAS;
            else if (strcmp(buffer, "not") == 0 || strcmp(buffer, "no") == 0) type = NO; // Spanish "not"
            else if (strcmp(buffer, "return") == 0 || strcmp(buffer, "retorno") == 0) type = RETORNO;
            // File I/O keywords (Phase 6)
            else if (strcmp(buffer, "open") == 0 || strcmp(buffer, "abrir") == 0) type = ABRIR;
            else if (strcmp(buffer, "close") == 0 || strcmp(buffer, "cerrar") == 0) type = CERRAR;
            else if (strcmp(buffer, "read") == 0 || strcmp(buffer, "leer") == 0) type = LEER;
            else if (strcmp(buffer, "write") == 0 || strcmp(buffer, "escribir") == 0) type = ESCRIBIR;
            else if (strcmp(buffer, "file") == 0 || strcmp(buffer, "archivo") == 0) type = ARCHIVO;
            // Standard library keywords
            else if (strcmp(buffer, "strlen") == 0 || strcmp(buffer, "longitud") == 0) type = LONGITUD;
            else if (strcmp(buffer, "concat") == 0 || strcmp(buffer, "concatenar") == 0) type = CONCATENAR;
            else if (strcmp(buffer, "abs") == 0 || strcmp(buffer, "absoluto") == 0) type = ABSOLUTO;
            else if (strcmp(buffer, "pow") == 0 || strcmp(buffer, "potencia") == 0) type = POTENCIA;
            else if (strcmp(buffer, "sqrt") == 0 || strcmp(buffer, "raiz") == 0) type = RAIZ;
            // Phase 7: Arrays, Structs, Dynamic Memory
            else if (strcmp(buffer, "struct") == 0 || strcmp(buffer, "estructura") == 0) type = ESTRUCTURA;
            else if (strcmp(buffer, "malloc") == 0 || strcmp(buffer, "reservar") == 0) type = RESERVAR;
            else if (strcmp(buffer, "free") == 0 || strcmp(buffer, "liberar") == 0) type = LIBERAR;
            else if (strcmp(buffer, "new") == 0 || strcmp(buffer, "nuevo") == 0) type = NUEVO;
            // Bootstrap support keywords
            else if (strcmp(buffer, "ord") == 0) type = ORD;
            else if (strcmp(buffer, "chr") == 0) type = CHR;
            else if (strcmp(buffer, "itoa") == 0 || strcmp(buffer, "a_cadena") == 0) type = ITOA;
            else if (strcmp(buffer, "escribir_byte") == 0 || strcmp(buffer, "write_byte") == 0) type = ESCRIBIR_BYTE;

            add_token(stream, init_token(type, buffer, line));
        }
        else if (isdigit(*current)) {
            // Number
            while (isdigit(*current)) {
                if (i < MAX_TOKEN_LENGTH - 1) {
                    buffer[i++] = *current;
                }
                current++;
            }
            buffer[i] = '\0';
            add_token(stream, init_token(NUMBER, buffer, line));
        }
        else if (*current == '"') {
            // String literal
            current++; // Skip opening quote

            // Copy characters until we find the closing quote or end of string
            while (*current != '\0' && *current != '"') {
                if (i < MAX_TOKEN_LENGTH - 1) {
                    buffer[i++] = *current;
                }
                current++;
            }

            if (*current == '"') {
                current++; // Skip closing quote
            }

            buffer[i] = '\0';
            add_token(stream, init_token(STRING, buffer, line));
        }
        else {
            // Single character tokens (may become two-character)
            char ch[3] = {*current, '\0', '\0'};

            TokenType type;
            switch (*current) {
                case '+': type = PLUS; break;
                case '-': type = MINUS; break;
                case '*': type = MULTIPLY; break;
                case '/': type = DIVIDE; break;
                case '=':
                    if (*(current + 1) == '=') {
                        ch[1] = '=';
                        current++;
                        type = EQUALITY;
                    } else {
                        type = ASSIGN;
                    }
                    break;
                case '!':
                    if (*(current + 1) == '=') {
                        ch[1] = '=';
                        current++;
                        type = NOT_EQUAL;
                    } else {
                        type = UNKNOWN;
                    }
                    break;
                case '<':
                    if (*(current + 1) == '=') {
                        ch[1] = '=';
                        current++;
                        type = LESS_EQUAL;
                    } else {
                        type = LESS_THAN;
                    }
                    break;
                case '>':
                    if (*(current + 1) == '=') {
                        ch[1] = '=';
                        current++;
                        type = GREATER_EQUAL;
                    } else {
                        type = GREATER_THAN;
                    }
                    break;
                case '&':
                    if (*(current + 1) == '&') {
                        ch[1] = '&';
                        current++;
                        type = Y; // Spanish "and"
                    } else {
                        type = UNKNOWN;
                    }
                    break;
                case '|':
                    if (*(current + 1) == '|') {
                        ch[1] = '|';
                        current++;
                        type = O; // Spanish "or"
                    } else {
                        type = UNKNOWN;
                    }
                    break;
                case '(': type = LPAREN; break;
                case ')': type = RPAREN; break;
                case '{': type = LBRACE; break;
                case '}': type = RBRACE; break;
                case ';': type = SEMICOLON; break;
                case ',': type = COMMA; break;
                case '[': type = LBRACKET; break;
                case ']': type = RBRACKET; break;
                case '.': type = DOT; break;
                default: type = UNKNOWN; break;
            }

            add_token(stream, init_token(type, ch, line));
            current++;
        }
    }

    return stream;
}