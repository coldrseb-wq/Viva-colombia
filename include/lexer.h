#ifndef LEXER_H
#define LEXER_H

#define MAX_TOKEN_LENGTH 256
#define INITIAL_TOKEN_CAPACITY 16

typedef enum {
    // Basic tokens
    IDENTIFIER,
    NUMBER,
    HEX_NUMBER,         // 0x... hexadecimal literals
    STRING,

    // Arithmetic operators
    PLUS,
    MINUS,
    MULTIPLY,
    DIVIDE,
    MODULO,             // %

    // Assignment and comparison
    ASSIGN,             // =
    EQUALITY,           // ==
    NOT_EQUAL,          // !=
    LESS_THAN,          // <
    GREATER_THAN,       // >
    LESS_EQUAL,         // <=
    GREATER_EQUAL,      // >=

    // Bitwise operators (critical for bootstrap)
    BIT_AND,            // &
    BIT_OR,             // |
    BIT_XOR,            // ^
    BIT_NOT,            // ~
    BIT_LSHIFT,         // <<
    BIT_RSHIFT,         // >>

    // Logical operators
    Y,                  // && (Spanish "y" = and)
    O,                  // || (Spanish "o" = or)
    NO,                 // !  (Spanish "no" = not)

    // Delimiters
    LPAREN,
    RPAREN,
    LBRACE,
    RBRACE,
    LBRACKET,
    RBRACKET,
    SEMICOLON,
    COMMA,
    DOT,                // . for struct field access
    ARROW,              // -> for pointer field access
    COLON,              // : for type annotations
    AMPERSAND,          // & for address-of
    ASTERISK,           // * for dereference (context-dependent with MULTIPLY)

    // Keywords - Variable declaration
    LET,
    DECRETO,            // Spanish "decreto" = decree/let

    // Keywords - Functions
    FN,
    CANCION,            // Spanish "cancion" = song/function
    RETORNO,            // Spanish "retorno" = return

    // Keywords - Control flow
    SI,                 // Spanish "si" = if
    SINO,               // Spanish "sino" = else
    MIENTRAS,           // Spanish "mientras" = while
    PARA,               // Spanish "para" = for
    HASTA,              // Spanish "hasta" = until
    ROMPER,             // Spanish "romper" = break
    CONTINUAR,          // Spanish "continuar" = continue

    // Keywords - Types (critical for bootstrap)
    TIPO_ENTERO,        // "entero" = int
    TIPO_OCTETO,        // "octeto" = byte/u8
    TIPO_CADENA,        // "cadena" = string
    TIPO_VACIO,         // "vacio" = void
    TIPO_BOOL,          // "booleano" = bool

    // Keywords - Structures (critical for bootstrap)
    ESTRUCTURA,         // Spanish "estructura" = struct
    TAMANO,             // Spanish "tamano" = sizeof

    // Keywords - Memory/Pointers
    NULO,               // Spanish "nulo" = null
    NUEVO,              // Spanish "nuevo" = new (allocation)
    LIBERAR,            // Spanish "liberar" = free

    // Keywords - Syscalls (for pure machine code, no libc)
    ESCRIBIR_SYS,       // sys_write
    LEER_SYS,           // sys_read
    ABRIR_SYS,          // sys_open
    CERRAR_SYS,         // sys_close
    SALIR_SYS,          // sys_exit

    // Special
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
