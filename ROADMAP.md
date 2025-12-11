# Viva Colombia Compiler - Development Roadmap

## Language Vision
Viva is designed to be:
- **As strict as Rust** - Strong type safety and compile-time guarantees
- **Safer than Rust** - Memory safety with additional runtime protections
- **General purpose** - Able to build any product (systems, applications, games, web)
- **Faster performance** - Direct machine code with aggressive optimizations
- **Readable like Go** - Clean, simple Spanish-based syntax that's easy to understand

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

## Phase 3: Assembly-to-Machine Code Transition (IN PROGRESS)
### Objectives:
- Fix assembly generation structural issues
- Generate syntactically correct x86-64 assembly
- Implement proper string literal handling

### Current Tasks:
- ✅ Assembly structure (data before code sections)
- ✅ String literal label generation and reference
- ✅ Proper x86-64 instruction syntax
- ✅ Cross-platform assembly compatibility

### Status: IN PROGRESS

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

## Phase 5: Cross-Platform Optimization & Advanced Features (IN PROGRESS)
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

### Current Tasks:
- [ ] Advanced machine code optimization passes
- [ ] Memory management in machine code
- [ ] Function definition support in machine code
- [ ] Complete Colombian cultural features

### Status: IN PROGRESS

## Phase 6: Final Implementation & Testing (PLANNED)
### Objectives:
- Full machine code compilation
- Multi-platform testing
- Performance validation

### Planned Tasks:
- [ ] End-to-end compilation without C dependencies
- [ ] Cross-platform compatibility testing
- [ ] Performance benchmarks
- [ ] Complete test suite
- [ ] Documentation and tooling

### Status: PLANNED

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

The compiler now generates proper platform-specific object files (ELF, Mach-O, PE) that can be linked with system libraries, marking a major milestone toward rivaling Go and Rust for cross-platform direct machine code compilation.

## Bootstrapping Path
The progression toward a fully bootstrapped Viva compiler:
1. **Phase 1-4**: Current C-written compiler enhanced to generate direct machine code (steps in this roadmap) - **MAJOR MILESTONES ACHIEVED**
2. **Future Phase**: Use this machine code generating compiler to create a Viva compiler written in Viva itself
3. **Final Goal**: Viva compiler (written in Viva) that generates direct machine code - achieving true bootstrapping

## Self-Hosting Status (IN PROGRESS)

### Current Architecture
```
┌─────────────────────┐     ┌─────────────────────┐     ┌─────────────────────┐
│   C Compiler        │────▶│   Bootstrap v1      │────▶│   Bootstrap v2      │
│   (src/*.c)         │     │   (viva_bootstrap)  │     │   (self-hosted)     │
│   Written in C      │     │   Compiled by C     │     │   Compiled by v1    │
└─────────────────────┘     └─────────────────────┘     └─────────────────────┘
```

### What We're Trying to Achieve
- **Goal**: Make the bootstrap compiler (`bootstrap/viva_bootstrap.viva`) compile itself
- **Success Criteria**: bootstrap_v2 should be byte-identical (or functionally equivalent) to bootstrap_v1
- **Why It Matters**: Self-hosting proves the language is complete enough to write real software

### Completed Self-Hosting Fixes
- ✅ Increased string literal buffers (8KB → 32KB, 64 → 256 entries)
- ✅ Removed recursive `siguiente_token()` calls that caused stack overflow
- ✅ Added bounds checking in `leer_cadena()` for string safety
- ✅ Fixed entry stub generation (moved to end when `main_addr` is known)
- ✅ Added `find_src_len()` for proper source length detection
- ✅ Increased code/data buffer sizes (131KB code, 16KB data)
- ✅ Two-pass compilation for forward references
- ✅ Element size tracking for arrays

### Current Problems
1. **Empty code section in bootstrap_v2**: When bootstrap_v1 compiles the bootstrap source, the resulting binary has:
   - Correct ELF headers
   - String literals in data section (working)
   - Empty code section (`code_pos` appears to be 0)

2. **Suspected issue area**: Something between line 77-98 of the bootstrap source causes code generation to stop. This area contains the `leer_identificador` and `leer_numero` functions.

3. **Debugging complexity**: The bootstrap compiler doesn't have debug output, making it hard to trace where code generation fails.

### Next Steps for Self-Hosting
1. [ ] Add diagnostic output to bootstrap compiler
2. [ ] Trace code_pos value through compilation
3. [ ] Identify specific construct causing code generation to fail
4. [ ] Fix the code generation issue
5. [ ] Verify bootstrap_v2 produces working executables
6. [ ] Compare bootstrap_v1 and bootstrap_v2 output

## Next Critical Steps
1. **Fix self-hosting** - Resolve the empty code section in bootstrap_v2
2. Complete advanced machine code optimization passes
3. Enhance PE/COFF format with full COFF section structure
4. Develop comprehensive runtime system with memory management
5. Add complete expression evaluation and control flow in machine code