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

// Check if identifier is a keyword and return the appropriate token type
static TokenType check_keyword(const char* word) {
    // Variable declaration
    if (strcmp(word, "let") == 0 || strcmp(word, "decreto") == 0) return DECRETO;

    // Functions
    if (strcmp(word, "fn") == 0 || strcmp(word, "cancion") == 0) return CANCION;
    if (strcmp(word, "return") == 0 || strcmp(word, "retorno") == 0) return RETORNO;

    // Control flow
    if (strcmp(word, "if") == 0 || strcmp(word, "si") == 0) return SI;
    if (strcmp(word, "else") == 0 || strcmp(word, "sino") == 0) return SINO;
    if (strcmp(word, "while") == 0 || strcmp(word, "mientras") == 0) return MIENTRAS;
    if (strcmp(word, "for") == 0 || strcmp(word, "para") == 0) return PARA;
    if (strcmp(word, "until") == 0 || strcmp(word, "hasta") == 0) return HASTA;
    if (strcmp(word, "break") == 0 || strcmp(word, "romper") == 0) return ROMPER;
    if (strcmp(word, "continue") == 0 || strcmp(word, "continuar") == 0) return CONTINUAR;

    // Logical operators as keywords
    if (strcmp(word, "and") == 0 || strcmp(word, "y") == 0) return Y;
    if (strcmp(word, "or") == 0 || strcmp(word, "o") == 0) return O;
    if (strcmp(word, "not") == 0 || strcmp(word, "no") == 0) return NO;

    // Types (critical for bootstrap)
    if (strcmp(word, "int") == 0 || strcmp(word, "entero") == 0) return TIPO_ENTERO;
    if (strcmp(word, "byte") == 0 || strcmp(word, "octeto") == 0) return TIPO_OCTETO;
    if (strcmp(word, "string") == 0 || strcmp(word, "cadena") == 0) return TIPO_CADENA;
    if (strcmp(word, "void") == 0 || strcmp(word, "vacio") == 0) return TIPO_VACIO;
    if (strcmp(word, "bool") == 0 || strcmp(word, "booleano") == 0) return TIPO_BOOL;

    // Structures
    if (strcmp(word, "struct") == 0 || strcmp(word, "estructura") == 0) return ESTRUCTURA;
    if (strcmp(word, "sizeof") == 0 || strcmp(word, "tamano") == 0) return TAMANO;

    // Memory/Pointers
    if (strcmp(word, "null") == 0 || strcmp(word, "nulo") == 0) return NULO;
    if (strcmp(word, "new") == 0 || strcmp(word, "nuevo") == 0) return NUEVO;
    if (strcmp(word, "free") == 0 || strcmp(word, "liberar") == 0) return LIBERAR;

    // Syscalls (Spanish names for pure machine code)
    if (strcmp(word, "escribir_sys") == 0 || strcmp(word, "sys_write") == 0) return ESCRIBIR_SYS;
    if (strcmp(word, "leer_sys") == 0 || strcmp(word, "sys_read") == 0) return LEER_SYS;
    if (strcmp(word, "abrir_sys") == 0 || strcmp(word, "sys_open") == 0) return ABRIR_SYS;
    if (strcmp(word, "cerrar_sys") == 0 || strcmp(word, "sys_close") == 0) return CERRAR_SYS;
    if (strcmp(word, "salir_sys") == 0 || strcmp(word, "sys_exit") == 0) return SALIR_SYS;

    return IDENTIFIER;
}

// Check if character is a hex digit
static int is_hex_digit(char c) {
    return isdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
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
            current += 2;
            while (*current != '\0' && *current != '\n') {
                current++;
            }
            if (*current == '\n') {
                line++;
                current++;
            }
            continue;
        }

        // Skip multi-line comments
        if (*current == '/' && *(current + 1) == '*') {
            current += 2;
            while (*current != '\0' && !(*current == '*' && *(current + 1) == '/')) {
                if (*current == '\n') line++;
                current++;
            }
            if (*current != '\0') current += 2;
            continue;
        }

        if (*current == '\0') break;

        char buffer[MAX_TOKEN_LENGTH];
        int i = 0;

        // Identifiers and keywords
        if (isalpha(*current) || *current == '_') {
            while (isalnum(*current) || *current == '_') {
                if (i < MAX_TOKEN_LENGTH - 1) {
                    buffer[i++] = *current;
                }
                current++;
            }
            buffer[i] = '\0';

            TokenType type = check_keyword(buffer);
            add_token(stream, init_token(type, buffer, line));
        }
        // Numbers (decimal and hexadecimal)
        else if (isdigit(*current)) {
            // Check for hexadecimal (0x or 0X)
            if (*current == '0' && (*(current + 1) == 'x' || *(current + 1) == 'X')) {
                buffer[i++] = *current++;  // '0'
                buffer[i++] = *current++;  // 'x' or 'X'
                while (is_hex_digit(*current)) {
                    if (i < MAX_TOKEN_LENGTH - 1) {
                        buffer[i++] = *current;
                    }
                    current++;
                }
                buffer[i] = '\0';
                add_token(stream, init_token(HEX_NUMBER, buffer, line));
            } else {
                // Decimal number
                while (isdigit(*current)) {
                    if (i < MAX_TOKEN_LENGTH - 1) {
                        buffer[i++] = *current;
                    }
                    current++;
                }
                buffer[i] = '\0';
                add_token(stream, init_token(NUMBER, buffer, line));
            }
        }
        // String literals
        else if (*current == '"') {
            current++; // Skip opening quote
            while (*current != '\0' && *current != '"') {
                // Handle escape sequences - convert to actual characters
                if (*current == '\\' && *(current + 1) != '\0') {
                    current++; // Skip backslash
                    if (i < MAX_TOKEN_LENGTH - 1) {
                        switch (*current) {
                            case 'n': buffer[i++] = '\n'; break;
                            case 't': buffer[i++] = '\t'; break;
                            case 'r': buffer[i++] = '\r'; break;
                            case '0': buffer[i++] = '\0'; break;
                            case '\\': buffer[i++] = '\\'; break;
                            case '"': buffer[i++] = '"'; break;
                            default: buffer[i++] = *current; break;
                        }
                    }
                    current++;
                } else {
                    if (i < MAX_TOKEN_LENGTH - 1) {
                        buffer[i++] = *current;
                    }
                    current++;
                }
            }
            if (*current == '"') current++;
            buffer[i] = '\0';
            add_token(stream, init_token(STRING, buffer, line));
        }
        // Character literals (for byte type)
        else if (*current == '\'') {
            current++; // Skip opening quote
            if (*current == '\\' && *(current + 1) != '\0') {
                buffer[i++] = *current++;
                buffer[i++] = *current++;
            } else if (*current != '\0' && *current != '\'') {
                buffer[i++] = *current++;
            }
            if (*current == '\'') current++;
            buffer[i] = '\0';
            add_token(stream, init_token(NUMBER, buffer, line)); // Treat char as number
        }
        // Operators and delimiters
        else {
            char ch[3] = {*current, '\0', '\0'};
            TokenType type;

            switch (*current) {
                case '+': type = PLUS; break;
                case '-':
                    if (*(current + 1) == '>') {
                        current++;
                        ch[1] = '>';
                        type = ARROW;
                    } else {
                        type = MINUS;
                    }
                    break;
                case '*': type = MULTIPLY; break;  // Context determines MULTIPLY vs dereference
                case '/': type = DIVIDE; break;
                case '%': type = MODULO; break;
                case '=':
                    if (*(current + 1) == '=') {
                        current++;
                        ch[1] = '=';
                        type = EQUALITY;
                    } else {
                        type = ASSIGN;
                    }
                    break;
                case '!':
                    if (*(current + 1) == '=') {
                        current++;
                        ch[1] = '=';
                        type = NOT_EQUAL;
                    } else {
                        type = NO;  // Logical not
                    }
                    break;
                case '<':
                    if (*(current + 1) == '=') {
                        current++;
                        ch[1] = '=';
                        type = LESS_EQUAL;
                    } else if (*(current + 1) == '<') {
                        current++;
                        ch[1] = '<';
                        type = BIT_LSHIFT;
                    } else {
                        type = LESS_THAN;
                    }
                    break;
                case '>':
                    if (*(current + 1) == '=') {
                        current++;
                        ch[1] = '=';
                        type = GREATER_EQUAL;
                    } else if (*(current + 1) == '>') {
                        current++;
                        ch[1] = '>';
                        type = BIT_RSHIFT;
                    } else {
                        type = GREATER_THAN;
                    }
                    break;
                case '&':
                    if (*(current + 1) == '&') {
                        current++;
                        ch[1] = '&';
                        type = Y;  // Logical AND
                    } else {
                        type = BIT_AND;  // Bitwise AND or address-of
                    }
                    break;
                case '|':
                    if (*(current + 1) == '|') {
                        current++;
                        ch[1] = '|';
                        type = O;  // Logical OR
                    } else {
                        type = BIT_OR;  // Bitwise OR
                    }
                    break;
                case '^': type = BIT_XOR; break;
                case '~': type = BIT_NOT; break;
                case '(': type = LPAREN; break;
                case ')': type = RPAREN; break;
                case '{': type = LBRACE; break;
                case '}': type = RBRACE; break;
                case '[': type = LBRACKET; break;
                case ']': type = RBRACKET; break;
                case ';': type = SEMICOLON; break;
                case ',': type = COMMA; break;
                case '.': type = DOT; break;
                case ':': type = COLON; break;
                default: type = UNKNOWN; break;
            }

            add_token(stream, init_token(type, ch, line));
            current++;
        }
    }

    return stream;
}
