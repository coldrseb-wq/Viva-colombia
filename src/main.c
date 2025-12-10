#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/lexer.h"
#include "../include/parser.h"
#include "../include/interpreter.h"
#include "../include/symbol_table.h"
#include "../include/compiler.h"

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

    size_t read_size = fread(buffer, 1, file_size, file);
    buffer[read_size] = '\0';

    fclose(file);
    return buffer;
}

int main(int argc, char *argv[]) {
    printf("Welcome to Viva - Your new programming language!\n");

    if (argc < 2) {
        printf("Usage: %s [file.viva]                              (run interpreter)\n", argv[0]);
        printf("       %s [file.viva] -c [output.c]                (compile to C)\n", argv[0]);
        printf("       %s [file.viva] -s [output.s]                (compile to assembly)\n", argv[0]);
        printf("       %s [file.viva] -e [output.o]                (compile to ELF object)\n", argv[0]);
        printf("       %s [file.viva] -x [output]                  (standalone executable - NO libc!)\n", argv[0]);
        printf("       %s [file.viva] -e -p [platform] [output.o]  (compile to platform object)\n", argv[0]);
        printf("       %s [file.viva] -x -p [platform] [output]    (standalone for platform)\n", argv[0]);
        printf("         platforms: linux, freebsd, macos, windows\n");
        return 1;
    }

    char* source = read_file(argv[1]);
    if (source == NULL) {
        return 1;
    }

    int compile_mode = 0;
    int asm_mode = 0;
    int elf_mode = 0;
    int standalone_mode = 0;
    const char* output_file = NULL;
    PlatformTarget platform = PLATFORM_LINUX;

    // Parse arguments
    if (argc >= 4 && strcmp(argv[2], "-c") == 0) {
        compile_mode = 1;
        output_file = argv[3];
    } else if (argc >= 4 && strcmp(argv[2], "-s") == 0) {
        asm_mode = 1;
        output_file = argv[3];
    } else if (argc >= 4 && strcmp(argv[2], "-x") == 0) {
        standalone_mode = 1;
        if (argc == 4) {
            output_file = argv[3];
            platform = PLATFORM_LINUX;  // Default to Linux
        } else if (argc == 6 && (strcmp(argv[3], "-p") == 0 || strcmp(argv[3], "--platform") == 0)) {
            if (strcmp(argv[4], "linux") == 0) {
                platform = PLATFORM_LINUX;
            } else if (strcmp(argv[4], "freebsd") == 0) {
                platform = PLATFORM_FREEBSD;
            } else if (strcmp(argv[4], "macos") == 0) {
                platform = PLATFORM_MACOS;
            } else if (strcmp(argv[4], "windows") == 0) {
                platform = PLATFORM_WINDOWS;
            } else {
                fprintf(stderr, "Invalid platform: %s (use linux, freebsd, macos, windows)\n", argv[4]);
                free(source);
                return 1;
            }
            output_file = argv[5];
        } else {
            fprintf(stderr, "Invalid standalone arguments\n");
            free(source);
            return 1;
        }
    } else if (argc >= 4 && strcmp(argv[2], "-e") == 0) {
        elf_mode = 1;
        if (argc == 4) {
            output_file = argv[3];
            platform = PLATFORM_LINUX;
        } else if (argc == 6 && (strcmp(argv[3], "-p") == 0 || strcmp(argv[3], "--platform") == 0)) {
            if (strcmp(argv[4], "linux") == 0) {
                platform = PLATFORM_LINUX;
            } else if (strcmp(argv[4], "freebsd") == 0) {
                platform = PLATFORM_FREEBSD;
            } else if (strcmp(argv[4], "macos") == 0) {
                platform = PLATFORM_MACOS;
            } else if (strcmp(argv[4], "windows") == 0) {
                platform = PLATFORM_WINDOWS;
            } else {
                fprintf(stderr, "Invalid platform: %s (use linux, freebsd, macos, windows)\n", argv[4]);
                free(source);
                return 1;
            }
            output_file = argv[5];
        } else {
            fprintf(stderr, "Invalid ELF arguments\n");
            free(source);
            return 1;
        }
    }

    // Execute based on mode
    if (compile_mode) {
        printf("Compiling %s to C: %s\n", argv[1], output_file);
        int result = compile_viva_to_c(source, output_file);
        free(source);
        if (result != 0) {
            fprintf(stderr, "C compilation failed\n");
            return 1;
        }
        printf("C compilation successful!\n");
    }
    else if (asm_mode) {
        printf("Compiling %s to ASM: %s\n", argv[1], output_file);
        int result = compile_viva_to_asm(source, output_file);
        free(source);
        if (result != 0) {
            fprintf(stderr, "Assembly compilation failed\n");
            return 1;
        }
        printf("Assembly compilation successful!\n");
    }
    else if (elf_mode) {
        const char* plat_name = "Linux";
        if (platform == PLATFORM_FREEBSD) plat_name = "FreeBSD";
        else if (platform == PLATFORM_MACOS) plat_name = "macOS";
        else if (platform == PLATFORM_WINDOWS) plat_name = "Windows";

        printf("Compiling %s to %s object: %s\n", argv[1], plat_name, output_file);

        int result;
        if (platform == PLATFORM_LINUX || platform == PLATFORM_FREEBSD) {
            result = compile_viva_to_elf(source, output_file);
        } else {
            result = compile_viva_to_platform(source, output_file, platform);
        }
        
        free(source);
        if (result != 0) {
            fprintf(stderr, "%s compilation failed\n", plat_name);
            return 1;
        }
        printf("%s compilation successful!\n", plat_name);
    }
    else if (standalone_mode) {
        const char* plat_name = "Linux";
        if (platform == PLATFORM_FREEBSD) plat_name = "FreeBSD";
        else if (platform == PLATFORM_MACOS) plat_name = "macOS";
        else if (platform == PLATFORM_WINDOWS) plat_name = "Windows";

        printf("Compiling %s to %s standalone executable: %s\n", argv[1], plat_name, output_file);
        int result = compile_viva_to_standalone_platform(source, output_file, platform);
        free(source);
        if (result != 0) {
            fprintf(stderr, "%s standalone compilation failed\n", plat_name);
            return 1;
        }
        printf("%s standalone compilation successful! (NO libc required)\n", plat_name);
        if (platform != PLATFORM_WINDOWS) {
            printf("Run with: ./%s\n", output_file);
        } else {
            printf("Run with: %s\n", output_file);
        }
    }
    else {
        // Interpreter mode
        printf("Running: %s\n", argv[1]);

        TokenStream* tokens = tokenize(source);
        printf("Tokenized: %d tokens\n", tokens->count);

        ASTNode* ast = parse_program(tokens);
        printf("Parsed AST\n");

        printf("\nAST structure:\n");
        print_ast(ast, 0);

        SymbolTable* symbol_table = init_symbol_table();

        printf("\n--- Output ---\n");
        interpret_ast(ast, symbol_table);

        free(source);
        free_token_stream(tokens);
        free_ast_node(ast);
        free_symbol_table(symbol_table);
    }

    return 0;
}