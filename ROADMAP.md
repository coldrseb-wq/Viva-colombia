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
- ✅ Comments in source code (// style)
- ✅ User-defined function declarations (parsing)
- ✅ Return statements with values

### Status: COMPLETED

---

## Phase 6: Bootstrap Language Features (IN PROGRESS)
### Objectives:
- Implement all language features required to write a Viva compiler in Viva
- Remove libc dependency by using raw syscalls
- Enable self-hosting compilation

### Priority 1: Core Language Features
#### Function System (Critical)
- [x] **Function parameters in codegen** - Parameters stored to stack from rdi, rsi, rdx, rcx
- [x] **User function calls with arguments** - Evaluates args and passes in registers
- [x] **Local variable scoping** - Function-local variable tables with save/restore
- [ ] **Recursive function support** - Call stack management (basic support exists)

#### Data Structures (Critical)
- [x] **Arrays with indexing** - Full codegen implemented for all backends
  - Array declaration: `decreto arr[10];` ✅
  - Array access: `arr[i]` ✅
  - Array assignment: `arr[i] = value;` ✅
- [ ] **Structs/Records** - New feature needed
  - Struct definition: `estructura Token { tipo; valor; }`
  - Field access: `token.tipo`
  - Struct allocation

#### String Operations
- [ ] **String length** - `longitud(str)`
- [ ] **String comparison** - `comparar(str1, str2)`
- [ ] **String concatenation** - `concatenar(str1, str2)`
- [ ] **Character access** - `str[i]`

### Priority 2: Low-Level System Access
#### Raw Syscalls (Remove libc dependency)
- [x] **Wire up syscall instruction** - `encode_syscall()` now used in compiler!
- [x] **write() syscall** - `syscall_write()` builtin (simplified version)
- [ ] **read() syscall** - File input `syscall(0, fd, buf, len)`
- [ ] **open() syscall** - File handling `syscall(2, path, flags, mode)`
- [ ] **close() syscall** - `syscall(3, fd)`
- [x] **exit() syscall** - `syscall_exit(code)` builtin working

#### Memory Management
- [ ] **mmap() syscall** - Heap allocation `syscall(9, addr, len, prot, flags, fd, off)`
- [ ] **munmap() syscall** - Free memory
- [ ] **Simple allocator** - Basic malloc/free in Viva

### Priority 3: Additional Operators
#### Bitwise Operators
- [x] **Bitwise AND** - `&` (all backends)
- [x] **Bitwise OR** - `|` (all backends)
- [x] **Bitwise XOR** - `^` (all backends)
- [x] **Bitwise NOT** - `~` (all backends)

#### Shift Operators
- [x] **Left shift** - `<<` (all backends)
- [x] **Right shift** - `>>` (all backends)

#### Additional
- [x] **Modulo** - `%` (all backends)
- [x] **Less-equal** - `<=` (lexer, codegen)
- [x] **Greater-equal** - `>=` (lexer, codegen)

#### Pointer Operations
- [ ] **Address-of** - `&variable`
- [ ] **Dereference** - `*pointer`
- [ ] **Pointer arithmetic** - `ptr + offset`

### Machine Code Implementation Status:
| Feature | Parsed | C Output | ASM Output | Machine Code |
|---------|--------|----------|------------|--------------|
| Function params | ✅ | ✅ | ✅ | ✅ |
| User fn calls | ✅ | ✅ | ✅ | ✅ |
| Arrays | ✅ | ✅ | ✅ | ✅ |
| Structs | ❌ | ❌ | ❌ | ❌ |
| Bitwise ops | ✅ | ✅ | ✅ | ✅ |
| Shifts | ✅ | ✅ | ✅ | ✅ |
| Modulo | ✅ | ✅ | ✅ | ✅ |
| Syscalls | N/A | N/A | N/A | ✅ (exit, write) |

### Status: IN PROGRESS

---

## Phase 7: Self-Hosting Compiler (PLANNED)
### Objectives:
- Write Viva compiler in Viva language
- Achieve true bootstrap (no C dependency)
- Viva compiles itself to machine code

### Planned Tasks:
- [ ] Implement lexer in Viva (`lexer.viva`)
- [ ] Implement parser in Viva (`parser.viva`)
- [ ] Implement code generator in Viva (`codegen.viva`)
- [ ] Implement ELF/Mach-O/PE writer in Viva
- [ ] Self-compilation test (compile Viva compiler with itself)
- [ ] Bootstrap verification (3-stage bootstrap)

### Bootstrap Process:
```
Stage 1: C compiler compiles viva_compiler.viva → viva_compiler_v1
Stage 2: viva_compiler_v1 compiles viva_compiler.viva → viva_compiler_v2
Stage 3: viva_compiler_v2 compiles viva_compiler.viva → viva_compiler_v3
Verify: viva_compiler_v2 == viva_compiler_v3 (byte-identical)
```

### Status: PLANNED

---

## Phase 8: Final Implementation & Testing (PLANNED)
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
The Viva compiler has made tremendous progress toward direct machine code compilation:

### Completed (Phases 1-5):
- ✅ Successfully transitioned from C-code to assembly generation
- ✅ Implemented proper ELF object file generation with complete section headers
- ✅ Implemented proper Mach-O object file generation for macOS
- ✅ Implemented basic PE/COFF object file generation for Windows
- ✅ Created symbol table and relocation infrastructure for external function calls
- ✅ Generated basic x86-64 machine code with proper instruction encoding
- ✅ Cross-compilation capability: Generate code for Linux, macOS, and Windows from any platform
- ✅ Control flow (if/else, while, for) in machine code
- ✅ Expression evaluation with all arithmetic and comparison operators

### Current Blocker (Phase 6):
The compiler is written in **~3,500 lines of C**. To achieve bootstrap, we must first implement:
- ❌ Function parameters (codegen ignores them)
- ❌ Arrays (parsed but no codegen)
- ❌ Structs (not implemented)
- ❌ Raw syscalls (encoder exists but unused)
- ❌ Memory management

### Key Discovery:
`encode_syscall()` exists in `machine_code.c:426` but is **never called** from `compiler.c`. Wiring this up enables libc-free I/O.

## Bootstrapping Path
```
Current State:
┌─────────────────────────────┐
│  C Compiler (3,500 LOC)     │  ← We are here
│  Generates: ELF/Mach-O/PE   │
│  Missing: params, arrays,   │
│           structs, syscalls │
└─────────────────────────────┘
            │
            ▼ Phase 6: Add missing features
┌─────────────────────────────┐
│  C Compiler (enhanced)      │
│  Can compile complex Viva   │
│  programs with all features │
└─────────────────────────────┘
            │
            ▼ Phase 7: Write compiler in Viva
┌─────────────────────────────┐
│  viva_compiler.viva         │
│  Lexer + Parser + Codegen   │
│  Written entirely in Viva   │
└─────────────────────────────┘
            │
            ▼ Bootstrap!
┌─────────────────────────────┐
│  Self-Hosting Compiler      │
│  Viva compiles itself       │
│  No C dependency            │
└─────────────────────────────┘
```

## Next Critical Steps
1. **Function parameters** - Wire up `n->left` in `compile_func()`
2. **User function calls** - Generate call instructions with argument passing
3. **Array codegen** - Implement array operations in `compile_node()`
4. **Syscall integration** - Connect `encode_syscall()` to compiler
5. **Struct support** - Add parsing and codegen for structs
6. **Write viva_compiler.viva** - Full compiler in Viva language
7. **Bootstrap test** - Compile Viva compiler with itself