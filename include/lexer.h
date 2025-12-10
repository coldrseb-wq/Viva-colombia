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
    FUNCION,        // Spanish for "function" - function declaration
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
    // File I/O tokens (Phase 6)
    ABRIR,          // Spanish for "open" - file open
    CERRAR,         // Spanish for "close" - file close
    LEER,           // Spanish for "read" - file read
    ESCRIBIR,       // Spanish for "write" - file write
    ARCHIVO,        // Spanish for "file" - file type
    // Standard library tokens
    LONGITUD,       // Spanish for "length" - string length
    CONCATENAR,     // Spanish for "concatenate" - string concat
    ABSOLUTO,       // Spanish for "absolute" - abs value
    POTENCIA,       // Spanish for "power" - exponentiation
    RAIZ,           // Spanish for "root" - square root
    MODULO,         // Modulo operation
    // Bootstrap support tokens
    ORD,            // ord(c) - character to ASCII code
    CHR,            // chr(n) - ASCII code to character
    ITOA,           // itoa(n) - integer to string
    ESCRIBIR_BYTE,  // escribir_byte(f, byte) - write single byte to file
    // Phase 7: Arrays, Structs, Dynamic Memory
    ESTRUCTURA,     // Spanish for "struct" - structure declaration
    RESERVAR,       // Spanish for "reserve/malloc" - memory allocation
    LIBERAR,        // Spanish for "free" - memory deallocation
    NUEVO,          // Spanish for "new" - create new instance
    DOT,            // . for struct member access
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