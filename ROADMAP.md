# Viva Colombia Compiler - Development Roadmap

## Project Overview
Viva Colombia is a Spanish-based programming language focused on Colombian culture and heroes, transitioning from C-code generation to direct machine code generation. The ultimate goal is to achieve bootstrapping - creating a Viva compiler written entirely in Viva that generates direct machine code for Windows, macOS, and Linux.

## Phase 1: Foundation & C-Code Generation (COMPLETED)
### Objectives:
- Create basic compiler infrastructure
- Implement C-code generation capability
- Support core language features

### Completed Tasks:
- ✅ Basic compiler architecture (C-based implementation)
- ✅ Lexer, parser, and interpreter functionality
- ✅ C-code generation backend
- ✅ Support for Colombian-themed functions (Simón Bolívar, etc.)
- ✅ Variable declarations and assignments
- ✅ Function calls and println operations
- ✅ Expression evaluation (arithmetic operations)

### Status: COMPLETED

## Phase 2: Assembly Generation Infrastructure (COMPLETED)
### Objectives:
- Transition from C-code to assembly code generation
- Implement proper assembly syntax handling
- Establish cross-platform foundation

### Completed Tasks:
- ✅ Dual output mode (C and Assembly)
- ✅ x86-64 assembly generation capability
- ✅ NASM assembly format support
- ✅ Assembly compilation flag (`-s` option)
- ✅ Basic instruction generation (mov, call, etc.)
- ✅ Buffered output system for proper assembly structure
- ✅ String literal label management
- ✅ Data section and code section handling

### Status: COMPLETED

## Phase 3: Assembly-to-Machine Code Transition (COMPLETED)
### Objectives:
- Fix assembly generation structural issues
- Generate syntactically correct x86-64 assembly
- Implement proper string literal handling

### Completed Tasks:
- ✅ Assembly structure (data before code sections)
- ✅ String literal label generation and reference
- ✅ Proper x86-64 instruction syntax
- ✅ Cross-platform assembly compatibility

### Status: COMPLETED

## Phase 4: Object File Generation (COMPLETED)
### Objectives:
- Generate native object files (ELF, Mach-O, PE)
- Direct machine code generation
- OS-specific executable format support

### Completed Tasks:
- ✅ ELF object file generation (Linux) - Complete ELF structure with sections (.text, .data, .symtab, .strtab)
- ✅ Mach-O object file generation (macOS) - Complete Mach-O structure for x86-64
- ✅ PE/COFF object file generation (Windows) - Basic PE header structure implemented
- ✅ Cross-platform executable format detection and generation
- ✅ Proper section headers with correct structure for all platforms
- ✅ Symbol table generation with function symbols (main, etc.)
- ✅ Basic x86-64 machine code instruction generation
- ✅ Relocation support infrastructure for external function calls
- ✅ Machine code generation for basic operations (push, mov, pop, ret)
- ✅ Command-line interface for cross-platform targeting

### Additional Features:
- ✅ Cross-compilation: Generate code for Linux, macOS, Windows from any platform
- ✅ Platform-specific binary formats with proper headers and structure
- ✅ Backward compatibility: Existing `-e` flag still defaults to Linux ELF

### Status: COMPLETED

## Phase 5: Cross-Platform Optimization & Advanced Features (COMPLETED)
### Objectives:
- Advanced cross-compilation capability with full platform feature parity
- Performance optimization
- Advanced language features

### Completed Tasks:
- ✅ Machine code optimization passes - Basic instruction generation
- ✅ Basic expression evaluation in machine code
- ✅ Loop and conditional support in AST
- ✅ Function call implementation in machine code
- ✅ Symbol table and relocation infrastructure
- ✅ Cross-compilation targeting all platforms (Linux, macOS, Windows)
- ✅ Complete variable storage management with proper stack frame tracking
- ✅ Fixed variable lookup for all output modes (C, Assembly, ELF)
- ✅ Proper register allocation for complex operations
- ✅ Complete machine code implementation for conditionals (if/else statements) with both C and assembly backends
- ✅ Added support for 'sino' (else) clauses in Spanish if statements
- ✅ Complete expression evaluation in machine code including comparison operations
- ✅ Proper conditional jump handling with offset calculation in ELF mode
- ✅ Assignment operation fixes for all output modes
- ✅ User-defined function parameters with System V AMD64 ABI (rdi, rsi, rdx, rcx, r8, r9)
- ✅ User-defined function calls in machine code
- ✅ Multiple function call arguments support
- ✅ Proper AST node structure with `next` pointer for statement chaining
- ✅ Fixed block body statement traversal for while/if/for loops
- ✅ While loop (mientras) execution with proper body interpretation
- ✅ Colombian cultural features (heroes functions)

### Status: COMPLETED

## Phase 6: File I/O & Standard Library (COMPLETED)
### Objectives:
- File input/output operations as Viva language features
- Standard library functions
- Runtime system enhancements

### Completed Tasks:
- ✅ File open/close/read/write operations in Viva (abrir, cerrar, leer, escribir)
- ✅ File declaration with `archivo` keyword
- ✅ Standard library functions: strlen (longitud), abs (absoluto), pow (potencia), sqrt (raiz)
- ✅ Spanish and English keyword support for all new features
- ✅ C code generation for all file I/O and stdlib functions
- ✅ math.h integration for mathematical operations
- ✅ Test suite for Phase 6 features (file_test.viva, stdlib_test.viva)

### Status: COMPLETED

## Phase 7: Arrays, Structs & Dynamic Memory (COMPLETED)
### Objectives:
- Add array support for data structure representation
- Implement struct/record types for complex data
- Add dynamic memory allocation for flexibility

### Completed Tasks:
- ✅ Array declaration with size: `decreto arr[10];`
- ✅ Array initialization with values: `decreto arr = [1, 2, 3];`
- ✅ Array element access: `arr[index]`
- ✅ Array element assignment: `arr[0] = value;`
- ✅ Struct declaration: `estructura Persona { edad; altura; }`
- ✅ Struct instance creation: `nuevo Persona juan;`
- ✅ Struct member access: `obj.field`
- ✅ Struct member assignment: `obj.field = value;`
- ✅ Dynamic memory allocation: `reservar(size)` / `malloc(size)`
- ✅ Memory deallocation: `liberar(ptr)` / `free(ptr)`
- ✅ C code generation for all Phase 7 features
- ✅ Test suite for Phase 7 features (phase7_test.viva)

### Status: COMPLETED

## Phase 8: Standalone Executables & Syscall Interface (COMPLETED)
### Objectives:
- Enable fully standalone executables without libc dependency
- Direct Linux syscall interface for I/O and process control
- Inline string embedding for minimal binary size

### Completed Tasks:
- ✅ Linux x86-64 syscall interface (syscall instruction encoding)
- ✅ sys_write syscall for stdout output
- ✅ sys_exit syscall for program termination
- ✅ Standalone println using syscalls (no printf/libc)
- ✅ Inline string embedding in .text section (no cross-section relocations)
- ✅ _start entry point for standalone binaries (instead of main)
- ✅ Command-line `-S` flag for standalone compilation
- ✅ Proper ELF symbol table with _start symbol
- ✅ Minimal binary size (~4.7KB for Hello World)

### Usage:
```bash
./viva program.viva -S output.o      # Compile to standalone object
ld -o program output.o               # Link without libc
./program                            # Run standalone binary
```

### Status: COMPLETED

## Current State Summary
The Viva compiler has made tremendous progress toward direct machine code compilation rivaling Go and Rust:
- ✅ Successfully transitioned from C-code to assembly generation
- ✅ Implemented proper ELF object file generation with complete section headers
- ✅ Implemented proper Mach-O object file generation for macOS
- ✅ Implemented basic PE/COFF object file generation for Windows
- ✅ Created symbol table and relocation infrastructure for external function calls
- ✅ Generated basic x86-64 machine code with proper instruction encoding
- ✅ Established foundation for optimization passes and advanced features
- ✅ Cross-compilation capability: Generate code for Linux, macOS, and Windows from any platform
- ✅ Command-line interface supporting cross-platform targeting with `-e -p [platform]` flags
- ✅ User-defined functions with parameters following System V AMD64 ABI
- ✅ Multiple function call arguments support
- ✅ Proper while/if/for loop execution with correct block body traversal
- ✅ Complete Colombian cultural features with themed hero functions
- ✅ File I/O operations (abrir, cerrar, leer, escribir) with archivo keyword
- ✅ Standard library: strlen (longitud), abs (absoluto), pow (potencia), sqrt (raiz)
- ✅ Arrays with size declarations and initializers
- ✅ Structs (estructura) with member access
- ✅ Dynamic memory allocation (reservar/liberar)
- ✅ **Standalone executables without libc** (Phase 8)
- ✅ Direct Linux syscall interface (sys_write, sys_exit)
- ✅ Inline string embedding for minimal binary size

**Phases 1-8 are now COMPLETED!** The compiler can now generate truly standalone executables that require no external runtime libraries. This is a major milestone toward self-sufficiency - the generated binaries use direct syscalls and have no libc dependency.

## Bootstrapping Path
The progression toward a fully bootstrapped Viva compiler:
1. **Phase 1-7**: Current C-written compiler with foundational features - **COMPLETED**
2. **Phase 8**: Standalone executables with direct syscalls - **COMPLETED**
3. **Phase 9 (Next)**: Begin writing Viva lexer in Viva itself
4. **Phase 10**: Write Viva parser in Viva
5. **Phase 11**: Write Viva compiler in Viva
6. **Final Goal**: Viva compiler (written in Viva) that generates direct machine code - achieving true bootstrapping

## Phase 9: Bootstrap Foundation (IN PROGRESS)
### Objectives:
- Add language features required for self-hosting compiler
- Create project structure for bootstrap development
- Prove bootstrap capability with working lexer in Viva

### Completed Tasks:
- ✅ `funcion` keyword for Spanish function declarations
- ✅ `ord()` function - character to ASCII code (via array access str[i])
- ✅ `chr(n)` function - ASCII code to character
- ✅ `itoa(n)` function - integer to string conversion
- ✅ `escribir_byte(f, byte)` - byte-level file I/O
- ✅ Proper type inference for chr() (char) and itoa() (char*)
- ✅ Project reorganization:
  - `compiler/` - C-based Viva compiler
  - `bootstrap/` - Viva source for self-hosting compiler
  - `stdlib/` - Standard library (future)
- ✅ Mini lexer proof-of-concept (bootstrap/lexer.viva)
  - Tokenizes simple Viva code
  - Uses character access, functions, loops
  - Compiles to working C code

### Known Issues to Fix:
- ⚠️ `&&` in while conditions doesn't work correctly
- ⚠️ `sino si` (else if) not chaining properly
- ⚠️ String concatenation not yet implemented

### Status: IN PROGRESS

## Future Phases

### Phase 10: Complete Bootstrap Lexer
- Full tokenization of all Viva keywords and operators
- Token structure using `estructura`
- Error reporting with line numbers
- String literal handling

### Phase 11: Bootstrap Parser
- AST node structures in Viva
- Recursive descent parser
- Expression parsing with precedence
- Statement and declaration parsing

### Phase 12: Bootstrap Code Generator
- C code generation from Viva AST
- Self-compilation: compile bootstrap compiler with itself
- Verify output matches original compiler

### Phase 13: Native Code Bootstrap
- Machine code generation in Viva
- ELF file generation in Viva
- Full self-hosting without C dependency

## Brainstorm: Future Directions

### Language Enhancements
- String concatenation (`+` or `concatenar`)
- String comparison (`==` for strings)
- Multi-line strings
- Character literals (`'a'`)
- Escape sequences in strings (`\n`, `\t`)
- Constants (`constante` keyword)
- Enums (`enumeracion`)

### Standard Library (stdlib/)
- String manipulation (substr, find, replace)
- Math functions (sin, cos, tan, log)
- File path utilities
- Command-line argument parsing
- Environment variables

### Tooling
- REPL (interactive mode)
- Debugger support
- Syntax highlighting definitions
- Language server (LSP)
- Package manager

### Platform Support
- ARM64 machine code (Apple Silicon, Raspberry Pi)
- WebAssembly target
- Embedded systems support

### Colombian Cultural Features
- More historical figures
- Regional dialects support
- Educational mode with Spanish explanations