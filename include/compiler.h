#ifndef COMPILER_H
#define COMPILER_H

#include "parser.h"

// Platform target enum (for cross-compilation)
typedef enum {
    PLATFORM_LINUX,
    PLATFORM_MACOS,
    PLATFORM_WINDOWS
} PlatformTarget;

// Function to compile Viva source code to C code
int compile_viva_to_c(const char* viva_code, const char* c_output_file);

// Function to compile Viva source code to assembly code
int compile_viva_to_asm(const char* viva_code, const char* asm_output_file);

// Function to compile Viva source code to ELF object code
int compile_viva_to_elf(const char* viva_code, const char* elf_output_file);

// Function to compile Viva source code to platform-specific object code
int compile_viva_to_platform(const char* viva_code, const char* output_file, PlatformTarget platform);

// Phase 8: Compile to standalone ELF (no libc, direct syscalls)
int compile_viva_to_standalone_elf(const char* viva_code, const char* output_file);

#endif