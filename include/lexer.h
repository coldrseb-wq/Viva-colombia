#ifndef LEXER_H
#define LEXER_H

#define MAX_TOKEN_LENGTH 256
#define INITIAL_TOKEN_CAPACITY 16

typedef enum {
    IDENTIFIER,
    NUMBER,
    STRING,
    PLUS,
    MINUS,
    MULTIPLY,
    DIVIDE,
    ASSIGN,         // For = in variable assignment
    EQUALITY,       // For == operator
    NOT_EQUAL,      // For != operator
    LESS_THAN,      // For < operator
    GREATER_THAN,   // For > operator
    LESS_EQUAL,     // For <= operator
    GREATER_EQUAL,  // For >= operator
    LPAREN,
    RPAREN,
    LBRACE,
    RBRACE,
    SEMICOLON,
    COMMA,
    LET,
    DECRETO,        // Spanish for "decree" - Colombian historical reference
    FN,
    CANCION,        // Spanish for "song" - cultural reference
    SI,             // Spanish for "if"
    SINO,           // Spanish for "else"
    MIENTRAS,       // Spanish for "while"
    PARA,           // Spanish for "for"
    HASTA,          // Spanish for "until"
    RETORNO,        // Spanish for "return"
    ROMPER,         // Spanish for "break"
    CONTINUAR,      // Spanish for "continue"
    Y,              // Spanish for "and" (&&)
    O,              // Spanish for "or" (||)
    NO,             // Spanish for "not" (!)
    LBRACKET,       // Left bracket [
    RBRACKET,       // Right bracket ]
    UNKNOWN
} TokenType;

typedef struct {
    TokenType type;
    char* value;
    int line;
} Token;

typedef struct {
    Token** tokens;
    int count;
    int capacity;
} TokenStream;

Token* init_token(TokenType type, char* value, int line);
void free_token(Token* token);
TokenStream* init_token_stream();
void add_token(TokenStream* stream, Token* token);
void free_token_stream(TokenStream* stream);
TokenStream* tokenize(const char* source);

#endif