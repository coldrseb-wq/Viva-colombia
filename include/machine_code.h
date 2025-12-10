#ifndef MACHINE_CODE_H
#define MACHINE_CODE_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// === ELF DEFINITIONS ===
#define EI_NIDENT 16

typedef struct {
    unsigned char e_ident[EI_NIDENT];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} Elf64_Ehdr;

typedef struct {
    uint32_t sh_name;
    uint32_t sh_type;
    uint64_t sh_flags;
    uint64_t sh_addr;
    uint64_t sh_offset;
    uint64_t sh_size;
    uint32_t sh_link;
    uint32_t sh_info;
    uint64_t sh_addralign;
    uint64_t sh_entsize;
} Elf64_Shdr;

typedef struct {
    uint32_t st_name;
    uint8_t st_info;
    uint8_t st_other;
    uint16_t st_shndx;
    uint64_t st_value;
    uint64_t st_size;
} Elf64_Sym;

typedef struct {
    uint64_t r_offset;
    uint64_t r_info;
} Elf64_Rel;

typedef struct {
    uint64_t r_offset;
    uint64_t r_info;
    int64_t r_addend;
} Elf64_Rela;

// ELF constants
#define ELFCLASS64 2
#define ELFDATA2LSB 1
#define EV_CURRENT 1
#define ET_REL 1
#define ET_EXEC 2
#define EM_X86_64 62

// Program header types
#define PT_NULL 0
#define PT_LOAD 1

// Program header flags
#define PF_X 1  // Execute
#define PF_W 2  // Write
#define PF_R 4  // Read

// Program header structure
typedef struct {
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
} Elf64_Phdr;
#define SHT_NULL 0
#define SHT_PROGBITS 1
#define SHT_SYMTAB 2
#define SHT_STRTAB 3
#define SHT_RELA 4
#define SHT_NOBITS 8
#define SHT_REL 9
#define SHF_WRITE 1
#define SHF_ALLOC 2
#define SHF_EXECINSTR 4

// Symbol info
#define ELF64_ST_BIND(i) ((i)>>4)
#define ELF64_ST_TYPE(i) ((i)&0xf)
#define ELF64_ST_INFO(b,t) (((b)<<4)+((t)&0xf))
#define STB_LOCAL 0
#define STB_GLOBAL 1
#define STT_NOTYPE 0
#define STT_OBJECT 1
#define STT_FUNC 2
#define STT_SECTION 3
#define STT_FILE 4

// Relocation
#define R_X86_64_64 1
#define R_X86_64_PC32 2
#define R_X86_64_PLT32 4
#define ELF64_R_SYM(i) ((i)>>32)
#define ELF64_R_TYPE(i) ((i)&0xffffffffL)
#define ELF64_R_INFO(s,t) ((((uint64_t)(s))<<32)+((t)&0xffffffffL))

// === DATA RELOCATION (for standalone executables) ===
typedef struct {
    size_t code_offset;    // Position in code where to patch (offset of the 32-bit displacement)
    int32_t data_offset;   // Offset within data section
} DataReloc;

// === MACHINE CODE STRUCT ===
typedef struct {
    uint8_t* code;
    size_t size;
    size_t capacity;
    Elf64_Rela* relocations;
    size_t reloc_count;
    size_t reloc_capacity;
    DataReloc* data_relocs;  // For standalone mode: tracks LEA instructions needing fixup
    size_t data_reloc_count;
    size_t data_reloc_capacity;
} MachineCode;

// === ELF SECTION ===
typedef struct {
    char* name;
    uint32_t name_offset;  // Offset in .shstrtab
    uint32_t type;
    uint64_t flags;
    uint64_t addr;
    uint64_t offset;
    uint64_t size;
    uint32_t link;
    uint32_t info;
    uint64_t addralign;
    uint64_t entsize;
    uint8_t* data;
} ElfSection;

// === ELF FILE ===
typedef struct {
    Elf64_Ehdr header;
    ElfSection** sections;
    int num_sections;
    int text_section_idx;
    int data_section_idx;
    int symtab_section_idx;
    int strtab_section_idx;
    int shstrtab_section_idx;
} ELFFile;

// === CORE FUNCTIONS ===
MachineCode* init_machine_code();
void free_machine_code(MachineCode* mc);
int append_bytes(MachineCode* mc, const uint8_t* bytes, size_t len);

// === STACK OPERATIONS ===
int encode_push_rbp(MachineCode* mc);
int encode_pop_rbp(MachineCode* mc);
int encode_push_rax(MachineCode* mc);
int encode_pop_rax(MachineCode* mc);
int encode_push_rbx(MachineCode* mc);
int encode_pop_rbx(MachineCode* mc);
int encode_push_rcx(MachineCode* mc);
int encode_pop_rcx(MachineCode* mc);
int encode_push_rdx(MachineCode* mc);
int encode_pop_rdx(MachineCode* mc);

// === MOV OPERATIONS ===
int encode_mov_rbp_rsp(MachineCode* mc);
int encode_mov_rsp_rbp(MachineCode* mc);
int encode_mov_rbp_rdi(MachineCode* mc);
int encode_mov_rax_imm32(MachineCode* mc, int32_t v);
int encode_mov_rax_imm64(MachineCode* mc, int64_t v);
int encode_mov_rbx_imm32(MachineCode* mc, int32_t v);
int encode_mov_rcx_imm32(MachineCode* mc, int32_t v);
int encode_mov_rdx_imm32(MachineCode* mc, int32_t v);
int encode_mov_rdi_imm64(MachineCode* mc, uint64_t v);
int encode_mov_rsi_imm64(MachineCode* mc, uint64_t v);
int encode_mov_rax_rbx(MachineCode* mc);
int encode_mov_rbx_rax(MachineCode* mc);
int encode_mov_rcx_rax(MachineCode* mc);
int encode_mov_rdi_rax(MachineCode* mc);
int encode_mov_rsi_rax(MachineCode* mc);
int encode_mov_rax_from_memory(MachineCode* mc, int off);
int encode_mov_memory_from_rax(MachineCode* mc, int off);
int encode_mov_rbx_from_memory(MachineCode* mc, int off);
int encode_mov_memory_from_rbx(MachineCode* mc, int off);

// === ARITHMETIC ===
int encode_add_rax_rbx(MachineCode* mc);
int encode_add_rax_imm32(MachineCode* mc, int32_t v);
int encode_sub_rax_rbx(MachineCode* mc);
int encode_sub_rax_imm32(MachineCode* mc, int32_t v);
int encode_sub_rsp_imm8(MachineCode* mc, int8_t v);
int encode_add_rsp_imm8(MachineCode* mc, int8_t v);
int encode_mul_rbx(MachineCode* mc);
int encode_div_rbx(MachineCode* mc);
int encode_neg_rax(MachineCode* mc);
int encode_not_rax(MachineCode* mc);

// === COMPARISONS ===
int encode_cmp_rax_rbx(MachineCode* mc);
int encode_cmp_rax_imm32(MachineCode* mc, int32_t v);
int encode_cmp_rax_zero(MachineCode* mc);
int encode_test_rax_rax(MachineCode* mc);
int encode_sete_al(MachineCode* mc);
int encode_setne_al(MachineCode* mc);
int encode_setl_al(MachineCode* mc);
int encode_setg_al(MachineCode* mc);
int encode_setle_al(MachineCode* mc);
int encode_setge_al(MachineCode* mc);
int encode_movzx_rax_al(MachineCode* mc);

// === JUMPS ===
int encode_jmp_rel32(MachineCode* mc, int32_t off);
int encode_je_rel32(MachineCode* mc, int32_t off);
int encode_jne_rel32(MachineCode* mc, int32_t off);
int encode_jl_rel32(MachineCode* mc, int32_t off);
int encode_jg_rel32(MachineCode* mc, int32_t off);
int encode_jle_rel32(MachineCode* mc, int32_t off);
int encode_jge_rel32(MachineCode* mc, int32_t off);
int encode_jmp_rel8(MachineCode* mc, int8_t off);
int encode_je_rel8(MachineCode* mc, int8_t off);
int encode_jne_rel8(MachineCode* mc, int8_t off);

// === CALL/RET ===
int encode_call_rel32(MachineCode* mc, int32_t off);
int encode_call_printf(MachineCode* mc);
int encode_call_external(MachineCode* mc);
int encode_ret(MachineCode* mc);
int encode_leave(MachineCode* mc);

// === LOGICAL ===
int encode_and_rax_rbx(MachineCode* mc);
int encode_or_rax_rbx(MachineCode* mc);
int encode_xor_rax_rax(MachineCode* mc);
int encode_xor_rdx_rdx(MachineCode* mc);

// === MISC ===
int encode_nop(MachineCode* mc);
int encode_syscall(MachineCode* mc);
int encode_lea_rax_rip_rel(MachineCode* mc, int32_t off);
int encode_lea_rdi_rip_rel(MachineCode* mc, int32_t off);
int encode_lea_rsi_rip_rel(MachineCode* mc, int32_t off);

// === LINUX SYSCALL HELPERS (C-free I/O) ===
int encode_sys_write(MachineCode* mc);
int encode_sys_exit(MachineCode* mc);
int encode_print_string_setup(MachineCode* mc, int32_t str_offset);
int encode_exit_with_code(MachineCode* mc, int code);

// === RELOCATIONS ===
int add_relocation_entry(MachineCode* mc, uint32_t sym, uint32_t type, int64_t add);
int add_data_relocation(MachineCode* mc, int32_t data_offset);  // For standalone mode

// === LABEL MANAGEMENT ===
int get_current_offset(MachineCode* mc);
void patch_jump_offset(MachineCode* mc, int jump_pos, int target_pos);

// === ELF FUNCTIONS ===
ELFFile* init_elf_file();
void free_elf_file(ELFFile* elf);
int add_elf_section(ELFFile* elf, const char* name, uint32_t type, uint64_t flags);
void create_text_section(ELFFile* elf, MachineCode* code);
void create_data_section(ELFFile* elf, uint8_t* data, size_t size);
void create_symbol_table(ELFFile* elf);
int write_complete_elf_file(ELFFile* elf, const char* filename);

// === STANDALONE ELF EXECUTABLE (no libc needed) ===
int write_standalone_elf_executable(const char* filename, MachineCode* code,
                                    uint8_t* data, size_t data_size);

#endif