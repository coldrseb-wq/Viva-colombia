#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/lexer.h"
#include "../include/parser.h"
#include "../include/interpreter.h"
#include "../include/symbol_table.h"
#include "../include/compiler.h"

// Function to read file content
char* read_file(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Could not open file: %s\n", filename);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buffer = malloc(file_size + 1);
    if (buffer == NULL) {
        fprintf(stderr, "Not enough memory to read file: %s\n", filename);
        fclose(file);
        return NULL;
    }

    fread(buffer, 1, file_size, file);
    buffer[file_size] = '\0';

    fclose(file);
    return buffer;
}

// Main entry point for your language
int main(int argc, char *argv[]) {
    printf("Welcome to Viva - Your new programming language!\n");

    if (argc > 1) {
        char* source = read_file(argv[1]);
        if (source == NULL) {
            return 1;
        }

        // Check compilation mode and optional platform target
        int compile_mode = 0;
        int asm_mode = 0;
        int elf_mode = 0;
        const char* output_file = NULL;
        PlatformTarget platform = PLATFORM_LINUX; // Default

        if (argc >= 3 && strcmp(argv[2], "-c") == 0 && argc == 4) {
            compile_mode = 1;
            output_file = argv[3];
        } else if (argc >= 3 && strcmp(argv[2], "-s") == 0 && argc == 4) {
            asm_mode = 1;
            output_file = argv[3];
        } else if (argc >= 3 && strcmp(argv[2], "-e") == 0) {
            elf_mode = 1;
            // Handle different argument patterns for ELF mode:
            // 4 args: viva file.viva -e output.o (default to Linux)
            // 6 args: viva file.viva -e -p macos output.o or viva file.viva -e --platform windows output.o
            if (argc == 4) {
                // Default ELF mode (Linux)
                output_file = argv[3];
                platform = PLATFORM_LINUX;
            } else if (argc == 6 &&
                      (strcmp(argv[3], "--platform") == 0 || strcmp(argv[3], "-p") == 0)) {
                // With platform specification
                if (strcmp(argv[4], "linux") == 0) {
                    platform = PLATFORM_LINUX;
                } else if (strcmp(argv[4], "macos") == 0) {
                    platform = PLATFORM_MACOS;
                } else if (strcmp(argv[4], "windows") == 0) {
                    platform = PLATFORM_WINDOWS;
                } else {
                    fprintf(stderr, "Invalid platform: %s. Use linux, macos, or windows\n", argv[4]);
                    free(source);
                    return 1;
                }
                output_file = argv[5];
            } else {
                fprintf(stderr, "Invalid arguments. Use: %s [file.viva] -e [-p|--platform linux|macos|windows] [output.o]\n", argv[0]);
                free(source);
                return 1;
            }
        }

        if (compile_mode) {
            printf("Compiling %s to C file %s\n", argv[1], output_file);
            int result = compile_viva_to_c(source, output_file);
            if (result != 0) {
                fprintf(stderr, "C compilation failed\n");
                free(source);
                return 1;
            }
            printf("C compilation successful!\n");
        } else if (asm_mode) {
            printf("Compiling %s to assembly file %s\n", argv[1], output_file);
            int result = compile_viva_to_asm(source, output_file);
            if (result != 0) {
                fprintf(stderr, "Assembly compilation failed\n");
                free(source);
                return 1;
            }
            printf("Assembly compilation successful!\n");
        } else if (elf_mode) {
            if (platform == PLATFORM_LINUX) {
                printf("Compiling %s to ELF object file %s (Linux)\n", argv[1], output_file);
                int result = compile_viva_to_elf(source, output_file);
                if (result != 0) {
                    fprintf(stderr, "Linux ELF compilation failed\n");
                    free(source);
                    return 1;
                }
                printf("Linux ELF compilation successful!\n");
            } else {
                printf("Compiling %s to object file %s (Platform: ", argv[1], output_file);
                switch(platform) {
                    case PLATFORM_MACOS:
                        printf("macOS)\n");
                        break;
                    case PLATFORM_WINDOWS:
                        printf("Windows)\n");
                        break;
                    default:
                        printf("Unknown)\n");
                        break;
                }
                int result = compile_viva_to_platform(source, output_file, platform);
                if (result != 0) {
                    fprintf(stderr, "Platform-specific compilation failed\n");
                    free(source);
                    return 1;
                }
                printf("Platform-specific compilation successful!\n");
            }
        } else {
            printf("Running file: %s\n", argv[1]);

            // Tokenize the source
            TokenStream* tokens = tokenize(source);
            printf("Tokenized source - found %d tokens\n", tokens->count);

            // Parse the tokens
            ASTNode* ast = parse_program(tokens);
            printf("Parsed source into AST\n");

            // Print AST for debugging
            printf("AST structure:\n");
            print_ast(ast, 0);

            // Create and initialize the symbol table
            SymbolTable* symbol_table = init_symbol_table();

            // Interpret the AST
            printf("Executing program:\n");
            interpret_ast(ast, symbol_table);

            // Clean up
            free(source);
            free_token_stream(tokens);
            free_ast_node(ast);
            free_symbol_table(symbol_table);
        }
    } else {
        printf("Usage: %s [file.viva]                    (run in interpreter mode)\n", argv[0]);
        printf("       %s [file.viva] -c [output.c]      (compile to C code)\n", argv[0]);
        printf("       %s [file.viva] -s [output.s]      (compile to assembly code)\n", argv[0]);
        printf("       %s [file.viva] -e [output.o]      (compile to Linux ELF object code)\n", argv[0]);
        printf("       %s [file.viva] -e -p [platform] [output.o]  (compile to platform object code)\n", argv[0]);
        printf("                                          where platform is linux, macos, or windows\n");
        return 1;
    }

    return 0;
}