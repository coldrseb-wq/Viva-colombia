#ifndef MACHINE_CODE_H
#define MACHINE_CODE_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// ELF (Executable and Linkable Format) definitions
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

// Section header
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

// Symbol table entry
typedef struct {
    uint32_t st_name;
    uint8_t st_info;
    uint8_t st_other;
    uint16_t st_shndx;
    uint64_t st_value;
    uint64_t st_size;
} Elf64_Sym;

// Mach-O (macOS) definitions
#define MH_MAGIC_64 0xfeedfacf
#define MH_EXECUTE 0x2
#define MH_OBJECT 0x1

typedef struct {
    uint32_t magic;
    int32_t cputype;
    int32_t cpusubtype;
    uint32_t filetype;
    uint32_t ncmds;
    uint32_t sizeofcmds;
    uint32_t flags;
    uint32_t reserved;
} mach_header_64;

typedef struct {
    uint32_t cmd;
    uint32_t cmdsize;
} load_command;

#define LC_SEGMENT_64 0x19

typedef struct {
    uint32_t cmd;
    uint32_t cmdsize;
    char segname[16];
    uint64_t vmaddr;
    uint64_t vmsize;
    uint64_t fileoff;
    uint64_t filesize;
    uint32_t maxprot;
    uint32_t initprot;
    uint32_t nsects;
    uint32_t flags;
} segment_command_64;

#define CPU_TYPE_X86_64 0x01000007

// PE/COFF (Windows) definitions
#define IMAGE_NT_SIGNATURE 0x00004550  // 'PE\0\0'

typedef struct {
    uint16_t Machine;
    uint16_t NumberOfSections;
    uint32_t TimeDateStamp;
    uint32_t PointerToSymbolTable;
    uint32_t NumberOfSymbols;
    uint16_t SizeOfOptionalHeader;
    uint16_t Characteristics;
} IMAGE_FILE_HEADER;

typedef struct {
    uint32_t VirtualSize;
    uint32_t VirtualAddress;
    uint32_t SizeOfRawData;
    uint32_t PointerToRawData;
    uint32_t PointerToRelocations;
    uint32_t PointerToLinenumbers;
    uint16_t NumberOfRelocations;
    uint16_t NumberOfLinenumbers;
    uint32_t Characteristics;
} IMAGE_SECTION_HEADER;

// ELF constants
#define ELFCLASS64 2
#define ELFDATA2LSB 1
#define EV_CURRENT 1
#define ET_REL 1  // Relocatable file
#define EM_X86_64 62
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

// Symbol information
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

// Relocation entry for x86-64
typedef struct {
    uint64_t r_offset;    // Address of the relocation
    uint64_t r_info;      // Symbol index and type
} Elf64_Rel;

// Relocation entry with addend for x86-64
typedef struct {
    uint64_t r_offset;    // Address of the relocation
    uint64_t r_info;      // Symbol index and type
    int64_t  r_addend;    // Addend for relocation
} Elf64_Rela;

// Relocation types for x86-64
#define R_X86_64_64 1     // Direct 64-bit reference
#define R_X86_64_PC32 2   // PC-relative 32-bit reference
#define R_X86_64_PLT32 4  // PLT 32-bit reference

// ELF64 relocation info macros
#define ELF64_R_SYM(i) ((i)>>32)
#define ELF64_R_TYPE(i) ((i)&0xffffffffL)
#define ELF64_R_INFO(s,t) ((((uint64_t)(s))<<32)+((t)&0xffffffffL))

// Machine code generation for ELF output
typedef struct {
    uint8_t* code;
    size_t size;
    size_t capacity;
    // Track relocation information
    Elf64_Rela* relocations;
    size_t reloc_count;
    size_t reloc_capacity;
} MachineCode;

// Section management structure
typedef struct {
    char* name;           // Section name
    uint32_t type;        // Section type
    uint64_t flags;       // Section flags
    uint64_t addr;        // Virtual address
    uint64_t offset;      // File offset
    uint64_t size;        // Section size
    uint32_t link;        // Link to another section
    uint32_t info;        // Additional section info
    uint64_t addralign;   // Address alignment
    uint64_t entsize;     // Entry size for table sections
    uint8_t* data;        // Section data
} ElfSection;

// ELF file structure
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

MachineCode* init_machine_code();
void free_machine_code(MachineCode* mc);
int append_bytes(MachineCode* mc, const uint8_t* bytes, size_t len);
int encode_push_rbp(MachineCode* mc);
int encode_mov_rbp_rsp(MachineCode* mc);
int encode_mov_rbp_rdi(MachineCode* mc);  // For function parameters
int encode_mov_rdi_imm64(MachineCode* mc, uint64_t value);
int encode_mov_rax_imm32(MachineCode* mc, int32_t value);
int encode_mov_rbx_imm32(MachineCode* mc, int32_t value);
int encode_add_rax_rbx(MachineCode* mc);
int encode_sub_rax_rbx(MachineCode* mc);
int encode_mul_rbx(MachineCode* mc);
int encode_div_rbx(MachineCode* mc);
int encode_mov_rax_from_memory(MachineCode* mc, int offset_from_rbp);
int encode_mov_memory_from_rax(MachineCode* mc, int offset_from_rbp);
int encode_mov_rbx_from_memory(MachineCode* mc, int offset_from_rbp);
int encode_mov_rcx_from_memory(MachineCode* mc, int offset_from_rbp);
int encode_mov_rbx_from_rax(MachineCode* mc);
int encode_mov_rcx_from_rax(MachineCode* mc);
int encode_mov_rbx_from_rcx(MachineCode* mc);
int encode_mov_rdi_from_rax(MachineCode* mc);
int encode_mov_rsi_imm32(MachineCode* mc, int32_t value);
int encode_mov_rax_from_rbx(MachineCode* mc);
int encode_pop_rbp(MachineCode* mc);
int encode_ret(MachineCode* mc);
int encode_call_printf(MachineCode* mc);
int encode_call_external(MachineCode* mc);
int encode_mov_rax_from_rdi(MachineCode* mc);
int encode_mov_rax_from_rsi(MachineCode* mc);
int encode_mov_rax_from_rdx(MachineCode* mc);
int encode_mov_rax_from_rcx(MachineCode* mc);
int encode_mov_rax_from_r8(MachineCode* mc);
int encode_mov_rax_from_r9(MachineCode* mc);
int add_relocation_entry(MachineCode* mc, uint32_t sym_index, uint32_t type, int64_t addend);

// ELF-related functions
ELFFile* init_elf_file();
void free_elf_file(ELFFile* elf);
int add_elf_section(ELFFile* elf, const char* name, uint32_t type, uint64_t flags);
int write_complete_elf_file(ELFFile* elf, const char* filename);
void create_text_section(ELFFile* elf, MachineCode* code);
void create_symbol_table(ELFFile* elf);

#endif