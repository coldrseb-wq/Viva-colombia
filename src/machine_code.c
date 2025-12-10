// src/machine_code.c - EXPANDED for Phase 4
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "../include/machine_code.h"

MachineCode* init_machine_code() {
    MachineCode* mc = malloc(sizeof(MachineCode));
    if (!mc) return NULL;
    mc->capacity = 4096;
    mc->size = 0;
    mc->code = malloc(mc->capacity);
    mc->reloc_count = 0;
    mc->reloc_capacity = 16;
    mc->relocations = malloc(mc->reloc_capacity * sizeof(Elf64_Rela));
    mc->data_reloc_count = 0;
    mc->data_reloc_capacity = 16;
    mc->data_relocs = malloc(mc->data_reloc_capacity * sizeof(DataReloc));
    return mc;
}

void free_machine_code(MachineCode* mc) {
    if (mc) {
        if (mc->code) free(mc->code);
        if (mc->relocations) free(mc->relocations);
        if (mc->data_relocs) free(mc->data_relocs);
        free(mc);
    }
}

int append_bytes(MachineCode* mc, const uint8_t* b, size_t len) {
    if (!mc || !b) return -1;
    while (mc->size + len > mc->capacity) {
        mc->capacity *= 2;
        mc->code = realloc(mc->code, mc->capacity);
        if (!mc->code) return -1;
    }
    memcpy(mc->code + mc->size, b, len);
    mc->size += len;
    return 0;
}

// === STACK OPERATIONS ===
int encode_push_rbp(MachineCode* mc) {
    uint8_t c[] = {0x55};
    return append_bytes(mc, c, 1);
}

int encode_pop_rbp(MachineCode* mc) {
    uint8_t c[] = {0x5D};
    return append_bytes(mc, c, 1);
}

int encode_push_rax(MachineCode* mc) {
    uint8_t c[] = {0x50};
    return append_bytes(mc, c, 1);
}

int encode_pop_rax(MachineCode* mc) {
    uint8_t c[] = {0x58};
    return append_bytes(mc, c, 1);
}

int encode_push_rbx(MachineCode* mc) {
    uint8_t c[] = {0x53};
    return append_bytes(mc, c, 1);
}

int encode_pop_rbx(MachineCode* mc) {
    uint8_t c[] = {0x5B};
    return append_bytes(mc, c, 1);
}

int encode_push_rcx(MachineCode* mc) {
    uint8_t c[] = {0x51};
    return append_bytes(mc, c, 1);
}

int encode_pop_rcx(MachineCode* mc) {
    uint8_t c[] = {0x59};
    return append_bytes(mc, c, 1);
}

int encode_push_rdx(MachineCode* mc) {
    uint8_t c[] = {0x52};
    return append_bytes(mc, c, 1);
}

int encode_pop_rdx(MachineCode* mc) {
    uint8_t c[] = {0x5A};
    return append_bytes(mc, c, 1);
}

// === MOV OPERATIONS ===
int encode_mov_rbp_rsp(MachineCode* mc) {
    uint8_t c[] = {0x48, 0x89, 0xE5};
    return append_bytes(mc, c, 3);
}

int encode_mov_rsp_rbp(MachineCode* mc) {
    uint8_t c[] = {0x48, 0x89, 0xEC};
    return append_bytes(mc, c, 3);
}

int encode_mov_rbp_rdi(MachineCode* mc) {
    uint8_t c[] = {0x48, 0x89, 0x7D, 0xF8};
    return append_bytes(mc, c, 4);
}

int encode_mov_rax_imm32(MachineCode* mc, int32_t v) {
    uint8_t c[7] = {0x48, 0xC7, 0xC0};
    memcpy(c + 3, &v, 4);
    return append_bytes(mc, c, 7);
}

int encode_mov_rax_imm64(MachineCode* mc, int64_t v) {
    uint8_t c[10] = {0x48, 0xB8};
    memcpy(c + 2, &v, 8);
    return append_bytes(mc, c, 10);
}

int encode_mov_rbx_imm32(MachineCode* mc, int32_t v) {
    uint8_t c[7] = {0x48, 0xC7, 0xC3};
    memcpy(c + 3, &v, 4);
    return append_bytes(mc, c, 7);
}

int encode_mov_rcx_imm32(MachineCode* mc, int32_t v) {
    uint8_t c[7] = {0x48, 0xC7, 0xC1};
    memcpy(c + 3, &v, 4);
    return append_bytes(mc, c, 7);
}

int encode_mov_rdx_imm32(MachineCode* mc, int32_t v) {
    uint8_t c[7] = {0x48, 0xC7, 0xC2};
    memcpy(c + 3, &v, 4);
    return append_bytes(mc, c, 7);
}

int encode_mov_rdi_imm64(MachineCode* mc, uint64_t v) {
    uint8_t c[10] = {0x48, 0xBF};
    memcpy(c + 2, &v, 8);
    return append_bytes(mc, c, 10);
}

int encode_mov_rsi_imm64(MachineCode* mc, uint64_t v) {
    uint8_t c[10] = {0x48, 0xBE};
    memcpy(c + 2, &v, 8);
    return append_bytes(mc, c, 10);
}

// Register to register
int encode_mov_rax_rbx(MachineCode* mc) {
    uint8_t c[] = {0x48, 0x89, 0xD8};
    return append_bytes(mc, c, 3);
}

int encode_mov_rbx_rax(MachineCode* mc) {
    uint8_t c[] = {0x48, 0x89, 0xC3};
    return append_bytes(mc, c, 3);
}

int encode_mov_rcx_rax(MachineCode* mc) {
    uint8_t c[] = {0x48, 0x89, 0xC1};
    return append_bytes(mc, c, 3);
}

int encode_mov_rdi_rax(MachineCode* mc) {
    uint8_t c[] = {0x48, 0x89, 0xC7};
    return append_bytes(mc, c, 3);
}

int encode_mov_rsi_rax(MachineCode* mc) {
    uint8_t c[] = {0x48, 0x89, 0xC6};
    return append_bytes(mc, c, 3);
}

// Memory operations
int encode_mov_rax_from_memory(MachineCode* mc, int off) {
    uint8_t c[] = {0x48, 0x8B, 0x85};
    if (append_bytes(mc, c, 3)) return -1;
    return append_bytes(mc, (uint8_t*)&off, 4);
}

int encode_mov_memory_from_rax(MachineCode* mc, int off) {
    uint8_t c[] = {0x48, 0x89, 0x85};
    if (append_bytes(mc, c, 3)) return -1;
    return append_bytes(mc, (uint8_t*)&off, 4);
}

int encode_mov_rbx_from_memory(MachineCode* mc, int off) {
    uint8_t c[] = {0x48, 0x8B, 0x9D};
    if (append_bytes(mc, c, 3)) return -1;
    return append_bytes(mc, (uint8_t*)&off, 4);
}

int encode_mov_memory_from_rbx(MachineCode* mc, int off) {
    uint8_t c[] = {0x48, 0x89, 0x9D};
    if (append_bytes(mc, c, 3)) return -1;
    return append_bytes(mc, (uint8_t*)&off, 4);
}

// === ARITHMETIC ===
int encode_add_rax_rbx(MachineCode* mc) {
    uint8_t c[] = {0x48, 0x01, 0xD8};
    return append_bytes(mc, c, 3);
}

int encode_add_rax_imm32(MachineCode* mc, int32_t v) {
    uint8_t c[6] = {0x48, 0x05};
    memcpy(c + 2, &v, 4);
    return append_bytes(mc, c, 6);
}

int encode_sub_rax_rbx(MachineCode* mc) {
    uint8_t c[] = {0x48, 0x29, 0xD8};
    return append_bytes(mc, c, 3);
}

int encode_sub_rax_imm32(MachineCode* mc, int32_t v) {
    uint8_t c[6] = {0x48, 0x2D};
    memcpy(c + 2, &v, 4);
    return append_bytes(mc, c, 6);
}

int encode_sub_rsp_imm8(MachineCode* mc, int8_t v) {
    uint8_t c[] = {0x48, 0x83, 0xEC, (uint8_t)v};
    return append_bytes(mc, c, 4);
}

int encode_add_rsp_imm8(MachineCode* mc, int8_t v) {
    uint8_t c[] = {0x48, 0x83, 0xC4, (uint8_t)v};
    return append_bytes(mc, c, 4);
}

int encode_mul_rbx(MachineCode* mc) {
    uint8_t c[] = {0x48, 0x0F, 0xAF, 0xC3};
    return append_bytes(mc, c, 4);
}

int encode_div_rbx(MachineCode* mc) {
    uint8_t a[] = {0x48, 0x31, 0xD2};  // xor rdx, rdx
    uint8_t b[] = {0x48, 0xF7, 0xF3};  // div rbx
    if (append_bytes(mc, a, 3)) return -1;
    return append_bytes(mc, b, 3);
}

int encode_neg_rax(MachineCode* mc) {
    uint8_t c[] = {0x48, 0xF7, 0xD8};
    return append_bytes(mc, c, 3);
}

int encode_not_rax(MachineCode* mc) {
    uint8_t c[] = {0x48, 0xF7, 0xD0};
    return append_bytes(mc, c, 3);
}

// === COMPARISONS ===
int encode_cmp_rax_rbx(MachineCode* mc) {
    uint8_t c[] = {0x48, 0x39, 0xD8};
    return append_bytes(mc, c, 3);
}

int encode_cmp_rax_imm32(MachineCode* mc, int32_t v) {
    uint8_t c[6] = {0x48, 0x3D};
    memcpy(c + 2, &v, 4);
    return append_bytes(mc, c, 6);
}

int encode_cmp_rax_zero(MachineCode* mc) {
    uint8_t c[] = {0x48, 0x83, 0xF8, 0x00};
    return append_bytes(mc, c, 4);
}

int encode_test_rax_rax(MachineCode* mc) {
    uint8_t c[] = {0x48, 0x85, 0xC0};
    return append_bytes(mc, c, 3);
}

// Set byte based on flags
int encode_sete_al(MachineCode* mc) {
    uint8_t c[] = {0x0F, 0x94, 0xC0};
    return append_bytes(mc, c, 3);
}

int encode_setne_al(MachineCode* mc) {
    uint8_t c[] = {0x0F, 0x95, 0xC0};
    return append_bytes(mc, c, 3);
}

int encode_setl_al(MachineCode* mc) {
    uint8_t c[] = {0x0F, 0x9C, 0xC0};
    return append_bytes(mc, c, 3);
}

int encode_setg_al(MachineCode* mc) {
    uint8_t c[] = {0x0F, 0x9F, 0xC0};
    return append_bytes(mc, c, 3);
}

int encode_setle_al(MachineCode* mc) {
    uint8_t c[] = {0x0F, 0x9E, 0xC0};
    return append_bytes(mc, c, 3);
}

int encode_setge_al(MachineCode* mc) {
    uint8_t c[] = {0x0F, 0x9D, 0xC0};
    return append_bytes(mc, c, 3);
}

int encode_movzx_rax_al(MachineCode* mc) {
    uint8_t c[] = {0x48, 0x0F, 0xB6, 0xC0};
    return append_bytes(mc, c, 4);
}

// === JUMPS ===
int encode_jmp_rel32(MachineCode* mc, int32_t off) {
    uint8_t c[5] = {0xE9};
    memcpy(c + 1, &off, 4);
    return append_bytes(mc, c, 5);
}

int encode_je_rel32(MachineCode* mc, int32_t off) {
    uint8_t c[6] = {0x0F, 0x84};
    memcpy(c + 2, &off, 4);
    return append_bytes(mc, c, 6);
}

int encode_jne_rel32(MachineCode* mc, int32_t off) {
    uint8_t c[6] = {0x0F, 0x85};
    memcpy(c + 2, &off, 4);
    return append_bytes(mc, c, 6);
}

int encode_jl_rel32(MachineCode* mc, int32_t off) {
    uint8_t c[6] = {0x0F, 0x8C};
    memcpy(c + 2, &off, 4);
    return append_bytes(mc, c, 6);
}

int encode_jg_rel32(MachineCode* mc, int32_t off) {
    uint8_t c[6] = {0x0F, 0x8F};
    memcpy(c + 2, &off, 4);
    return append_bytes(mc, c, 6);
}

int encode_jle_rel32(MachineCode* mc, int32_t off) {
    uint8_t c[6] = {0x0F, 0x8E};
    memcpy(c + 2, &off, 4);
    return append_bytes(mc, c, 6);
}

int encode_jge_rel32(MachineCode* mc, int32_t off) {
    uint8_t c[6] = {0x0F, 0x8D};
    memcpy(c + 2, &off, 4);
    return append_bytes(mc, c, 6);
}

// Short jumps (8-bit offset)
int encode_jmp_rel8(MachineCode* mc, int8_t off) {
    uint8_t c[] = {0xEB, (uint8_t)off};
    return append_bytes(mc, c, 2);
}

int encode_je_rel8(MachineCode* mc, int8_t off) {
    uint8_t c[] = {0x74, (uint8_t)off};
    return append_bytes(mc, c, 2);
}

int encode_jne_rel8(MachineCode* mc, int8_t off) {
    uint8_t c[] = {0x75, (uint8_t)off};
    return append_bytes(mc, c, 2);
}

// === CALL/RET ===
int encode_call_rel32(MachineCode* mc, int32_t off) {
    uint8_t c[5] = {0xE8};
    memcpy(c + 1, &off, 4);
    return append_bytes(mc, c, 5);
}

int encode_call_printf(MachineCode* mc) {
    uint8_t c[] = {0xE8, 0x00, 0x00, 0x00, 0x00};
    return append_bytes(mc, c, 5);
}

int encode_call_external(MachineCode* mc) {
    uint8_t c[] = {0xE8, 0x00, 0x00, 0x00, 0x00};
    return append_bytes(mc, c, 5);
}

int encode_ret(MachineCode* mc) {
    uint8_t c[] = {0xC3};
    return append_bytes(mc, c, 1);
}

int encode_leave(MachineCode* mc) {
    uint8_t c[] = {0xC9};
    return append_bytes(mc, c, 1);
}

// === LOGICAL ===
int encode_and_rax_rbx(MachineCode* mc) {
    uint8_t c[] = {0x48, 0x21, 0xD8};
    return append_bytes(mc, c, 3);
}

int encode_or_rax_rbx(MachineCode* mc) {
    uint8_t c[] = {0x48, 0x09, 0xD8};
    return append_bytes(mc, c, 3);
}

int encode_xor_rax_rax(MachineCode* mc) {
    uint8_t c[] = {0x48, 0x31, 0xC0};
    return append_bytes(mc, c, 3);
}

int encode_xor_rdx_rdx(MachineCode* mc) {
    uint8_t c[] = {0x48, 0x31, 0xD2};
    return append_bytes(mc, c, 3);
}

// === NOP ===
int encode_nop(MachineCode* mc) {
    uint8_t c[] = {0x90};
    return append_bytes(mc, c, 1);
}

// === SYSCALL (Linux) ===
int encode_syscall(MachineCode* mc) {
    uint8_t c[] = {0x0F, 0x05};
    return append_bytes(mc, c, 2);
}

// === LINUX SYSCALL HELPERS ===
// sys_write(fd=RDI, buf=RSI, count=RDX) - syscall #1
int encode_sys_write(MachineCode* mc) {
    encode_mov_rax_imm32(mc, 1);  // syscall number for write
    return encode_syscall(mc);
}

// sys_exit(status=RDI) - syscall #60
int encode_sys_exit(MachineCode* mc) {
    encode_mov_rax_imm32(mc, 60);  // syscall number for exit
    return encode_syscall(mc);
}

// Helper: print string at RIP-relative offset, length in RDX
// Sets up: RDI=1 (stdout), RSI=string address, RDX=length (must be set before)
int encode_print_string_setup(MachineCode* mc, int32_t str_offset) {
    encode_mov_rax_imm32(mc, 1);           // RDI = 1 (stdout)
    encode_mov_rdi_rax(mc);
    encode_lea_rsi_rip_rel(mc, str_offset); // RSI = string address
    return 0;
}

// Helper: exit with status code
int encode_exit_with_code(MachineCode* mc, int code) {
    encode_mov_rax_imm32(mc, code);
    encode_mov_rdi_rax(mc);
    encode_mov_rax_imm32(mc, 60);
    return encode_syscall(mc);
}

// === LEA ===
int encode_lea_rax_rip_rel(MachineCode* mc, int32_t off) {
    uint8_t c[7] = {0x48, 0x8D, 0x05};
    memcpy(c + 3, &off, 4);
    return append_bytes(mc, c, 7);
}

int encode_lea_rdi_rip_rel(MachineCode* mc, int32_t off) {
    uint8_t c[7] = {0x48, 0x8D, 0x3D};
    memcpy(c + 3, &off, 4);
    return append_bytes(mc, c, 7);
}

int encode_lea_rsi_rip_rel(MachineCode* mc, int32_t off) {
    uint8_t c[7] = {0x48, 0x8D, 0x35};
    memcpy(c + 3, &off, 4);
    return append_bytes(mc, c, 7);
}

// === RELOCATIONS ===
int add_relocation_entry(MachineCode* mc, uint32_t sym, uint32_t type, int64_t add) {
    if (mc->reloc_count >= mc->reloc_capacity) {
        mc->reloc_capacity *= 2;
        mc->relocations = realloc(mc->relocations, mc->reloc_capacity * sizeof(Elf64_Rela));
        if (!mc->relocations) return -1;
    }
    Elf64_Rela* r = &mc->relocations[mc->reloc_count++];
    r->r_offset = mc->size;
    r->r_info = ELF64_R_INFO(sym, type);
    r->r_addend = add;
    return 0;
}

// Add a data relocation for standalone mode
// Records that the 4 bytes at (current_position - 4) need to be patched
// with the RIP-relative offset to data_offset in the data section
int add_data_relocation(MachineCode* mc, int32_t data_offset) {
    if (!mc) return -1;
    if (mc->data_reloc_count >= mc->data_reloc_capacity) {
        mc->data_reloc_capacity *= 2;
        mc->data_relocs = realloc(mc->data_relocs, mc->data_reloc_capacity * sizeof(DataReloc));
        if (!mc->data_relocs) return -1;
    }
    DataReloc* dr = &mc->data_relocs[mc->data_reloc_count++];
    dr->code_offset = mc->size - 4;  // The 4-byte displacement was just written
    dr->data_offset = data_offset;
    return 0;
}

// === LABEL MANAGEMENT ===
int get_current_offset(MachineCode* mc) {
    return mc ? mc->size : 0;
}

void patch_jump_offset(MachineCode* mc, int jump_pos, int target_pos) {
    if (!mc || jump_pos < 0 || jump_pos + 4 > (int)mc->size) return;
    int32_t offset = target_pos - (jump_pos + 4);
    memcpy(mc->code + jump_pos, &offset, 4);
}

// === ELF FILE FUNCTIONS ===
ELFFile* init_elf_file() {
    ELFFile* e = calloc(1, sizeof(ELFFile));
    if (!e) return NULL;
    e->header.e_ident[0] = 0x7f;
    e->header.e_ident[1] = 'E';
    e->header.e_ident[2] = 'L';
    e->header.e_ident[3] = 'F';
    e->header.e_ident[4] = ELFCLASS64;
    e->header.e_ident[5] = ELFDATA2LSB;
    e->header.e_ident[6] = EV_CURRENT;
    e->header.e_type = ET_REL;
    e->header.e_machine = EM_X86_64;
    e->header.e_version = EV_CURRENT;
    e->header.e_ehsize = sizeof(Elf64_Ehdr);
    e->header.e_shentsize = sizeof(Elf64_Shdr);
    e->sections = malloc(sizeof(ElfSection*) * 16);
    e->num_sections = 0;
    e->text_section_idx = -1;
    e->data_section_idx = -1;
    e->symtab_section_idx = -1;
    e->strtab_section_idx = -1;
    e->shstrtab_section_idx = -1;
    return e;
}

void free_elf_file(ELFFile* e) {
    if (!e) return;
    for (int i = 0; i < e->num_sections; i++) {
        if (e->sections[i]) {
            if (e->sections[i]->data) free(e->sections[i]->data);
            if (e->sections[i]->name) free(e->sections[i]->name);
            free(e->sections[i]);
        }
    }
    if (e->sections) free(e->sections);
    free(e);
}

int add_elf_section(ELFFile* e, const char* name, uint32_t type, uint64_t flags) {
    if (e->num_sections >= 16) return -1;
    ElfSection* s = calloc(1, sizeof(ElfSection));
    if (!s) return -1;
    s->name = strdup(name);
    s->type = type;
    s->flags = flags;
    s->addralign = 1;
    e->sections[e->num_sections] = s;
    if (strcmp(name, ".text") == 0) e->text_section_idx = e->num_sections;
    else if (strcmp(name, ".data") == 0) e->data_section_idx = e->num_sections;
    else if (strcmp(name, ".symtab") == 0) e->symtab_section_idx = e->num_sections;
    else if (strcmp(name, ".strtab") == 0) e->strtab_section_idx = e->num_sections;
    else if (strcmp(name, ".shstrtab") == 0) e->shstrtab_section_idx = e->num_sections;
    return e->num_sections++;
}

void create_text_section(ELFFile* e, MachineCode* mc) {
    int idx = add_elf_section(e, ".text", SHT_PROGBITS, SHF_ALLOC | SHF_EXECINSTR);
    if (idx < 0) return;
    ElfSection* s = e->sections[idx];
    s->size = mc->size;
    s->addralign = 16;
    s->data = malloc(mc->size);
    if (s->data) memcpy(s->data, mc->code, mc->size);
}

void create_data_section(ELFFile* e, uint8_t* data, size_t size) {
    int idx = add_elf_section(e, ".data", SHT_PROGBITS, SHF_ALLOC | SHF_WRITE);
    if (idx < 0) return;
    ElfSection* s = e->sections[idx];
    s->size = size;
    s->addralign = 8;
    s->data = malloc(size);
    if (s->data) memcpy(s->data, data, size);
}

void create_symbol_table(ELFFile* e) {
    // Create string table with symbol names: "\0main\0printf\0"
    int strtab_idx = add_elf_section(e, ".strtab", SHT_STRTAB, 0);
    if (strtab_idx >= 0) {
        const char* strtab_data = "\0main\0printf\0";
        size_t strtab_size = 14;  // 1 + 5 + 7 + 1 = null + "main\0" + "printf\0"
        e->sections[strtab_idx]->data = malloc(strtab_size);
        memcpy(e->sections[strtab_idx]->data, strtab_data, strtab_size);
        e->sections[strtab_idx]->size = strtab_size;
        e->strtab_section_idx = strtab_idx;
    }

    // Create symbol table with: null symbol, main symbol, printf symbol
    int symtab_idx = add_elf_section(e, ".symtab", SHT_SYMTAB, 0);
    if (symtab_idx >= 0) {
        e->sections[symtab_idx]->entsize = sizeof(Elf64_Sym);
        e->sections[symtab_idx]->link = strtab_idx + 1;  // Index of .strtab (1-based in section table)
        e->sections[symtab_idx]->info = 2;  // First global symbol index

        // Allocate space for 3 symbols: null, main, printf
        size_t symtab_size = 3 * sizeof(Elf64_Sym);
        Elf64_Sym* syms = calloc(3, sizeof(Elf64_Sym));

        // Symbol 0: NULL symbol (required)
        syms[0].st_name = 0;
        syms[0].st_info = 0;
        syms[0].st_other = 0;
        syms[0].st_shndx = 0;
        syms[0].st_value = 0;
        syms[0].st_size = 0;

        // Symbol 1: main (global function in .text)
        syms[1].st_name = 1;  // Offset in strtab: "main"
        syms[1].st_info = (1 << 4) | 2;  // STB_GLOBAL | STT_FUNC
        syms[1].st_other = 0;
        syms[1].st_shndx = e->text_section_idx + 1;  // .text section index (1-based)
        syms[1].st_value = 0;  // Offset within section
        syms[1].st_size = 0;

        // Symbol 2: printf (undefined external)
        syms[2].st_name = 6;  // Offset in strtab: "printf"
        syms[2].st_info = (1 << 4) | 0;  // STB_GLOBAL | STT_NOTYPE
        syms[2].st_other = 0;
        syms[2].st_shndx = 0;  // SHN_UNDEF
        syms[2].st_value = 0;
        syms[2].st_size = 0;

        e->sections[symtab_idx]->data = (uint8_t*)syms;
        e->sections[symtab_idx]->size = symtab_size;
        e->symtab_section_idx = symtab_idx;
    }

    // Create section header string table
    int shstrtab_idx = add_elf_section(e, ".shstrtab", SHT_STRTAB, 0);
    if (shstrtab_idx >= 0) {
        // Build shstrtab: "\0.text\0.data\0.strtab\0.symtab\0.shstrtab\0"
        // Offsets: 0=null, 1=.text, 7=.data, 13=.strtab, 21=.symtab, 29=.shstrtab
        static const char shstrtab_data[] = "\0.text\0.data\0.strtab\0.symtab\0.shstrtab";
        size_t shstrtab_size = sizeof(shstrtab_data);
        e->sections[shstrtab_idx]->data = malloc(shstrtab_size);
        memcpy(e->sections[shstrtab_idx]->data, shstrtab_data, shstrtab_size);
        e->sections[shstrtab_idx]->size = shstrtab_size;
        e->shstrtab_section_idx = shstrtab_idx;

        // Set section name offsets
        for (int i = 0; i < e->num_sections; i++) {
            if (strcmp(e->sections[i]->name, ".text") == 0)
                e->sections[i]->name_offset = 1;
            else if (strcmp(e->sections[i]->name, ".data") == 0)
                e->sections[i]->name_offset = 7;
            else if (strcmp(e->sections[i]->name, ".strtab") == 0)
                e->sections[i]->name_offset = 13;
            else if (strcmp(e->sections[i]->name, ".symtab") == 0)
                e->sections[i]->name_offset = 21;
            else if (strcmp(e->sections[i]->name, ".shstrtab") == 0)
                e->sections[i]->name_offset = 29;
        }
    }
}

int write_complete_elf_file(ELFFile* e, const char* filename) {
    if (!e || !filename) return -1;
    FILE* f = fopen(filename, "wb");
    if (!f) return -1;

    uint64_t off = sizeof(Elf64_Ehdr);
    for (int i = 0; i < e->num_sections; i++) {
        if (e->sections[i]->size > 0) {
            off = (off + 7) & ~7;
            e->sections[i]->offset = off;
            off += e->sections[i]->size;
        }
    }

    // Set up ELF header
    e->header.e_shnum = e->num_sections + 1;  // +1 for null section
    e->header.e_shoff = off;
    e->header.e_shstrndx = e->shstrtab_section_idx + 1;  // Index of .shstrtab (1-based)

    fwrite(&e->header, sizeof(Elf64_Ehdr), 1, f);

    // Write section data
    for (int i = 0; i < e->num_sections; i++) {
        if (e->sections[i]->data && e->sections[i]->size > 0) {
            fseek(f, e->sections[i]->offset, SEEK_SET);
            fwrite(e->sections[i]->data, 1, e->sections[i]->size, f);
        }
    }

    // Write section headers starting at e_shoff
    fseek(f, off, SEEK_SET);

    // Section 0: NULL section header (required)
    Elf64_Shdr null_sh = {0};
    fwrite(&null_sh, sizeof(Elf64_Shdr), 1, f);

    // Write section headers for each section
    for (int i = 0; i < e->num_sections; i++) {
        Elf64_Shdr sh = {0};
        sh.sh_name = e->sections[i]->name_offset;  // Offset in .shstrtab
        sh.sh_type = e->sections[i]->type;
        sh.sh_flags = e->sections[i]->flags;
        sh.sh_offset = e->sections[i]->offset;
        sh.sh_size = e->sections[i]->size;
        sh.sh_addralign = e->sections[i]->addralign;
        sh.sh_entsize = e->sections[i]->entsize;
        sh.sh_link = e->sections[i]->link;
        sh.sh_info = e->sections[i]->info;
        fwrite(&sh, sizeof(Elf64_Shdr), 1, f);
    }

    fclose(f);
    return 0;
}

// === STANDALONE ELF EXECUTABLE WRITER ===
// Creates a directly executable ELF binary - NO libc, NO linker needed!
// Uses Linux syscalls directly for I/O.

#define VADDR_BASE 0x400000  // Standard Linux executable base address

int write_standalone_elf_executable(const char* filename, MachineCode* code,
                                    uint8_t* data, size_t data_size) {
    if (!filename || !code) return -1;

    FILE* f = fopen(filename, "wb");
    if (!f) return -1;

    // Simple layout: single LOAD segment containing everything
    // [ELF Header (64 bytes)]
    // [Program Header (56 bytes)]
    // [.text section (code->size bytes)]
    // [.data section (data_size bytes)]
    //
    // All loaded at virtual address VADDR_BASE, mapped from file offset 0

    size_t ehdr_size = sizeof(Elf64_Ehdr);      // 64 bytes
    size_t phdr_size = sizeof(Elf64_Phdr);      // 56 bytes
    size_t headers_size = ehdr_size + phdr_size;  // 120 bytes

    size_t code_offset = headers_size;
    size_t data_offset = code_offset + code->size;
    size_t total_size = data_offset + (data ? data_size : 0);

    // Virtual addresses
    size_t code_vaddr = VADDR_BASE + code_offset;
    size_t data_vaddr = VADDR_BASE + data_offset;

    // Build ELF header
    Elf64_Ehdr ehdr = {0};
    ehdr.e_ident[0] = 0x7f;
    ehdr.e_ident[1] = 'E';
    ehdr.e_ident[2] = 'L';
    ehdr.e_ident[3] = 'F';
    ehdr.e_ident[4] = ELFCLASS64;
    ehdr.e_ident[5] = ELFDATA2LSB;
    ehdr.e_ident[6] = EV_CURRENT;
    ehdr.e_type = ET_EXEC;
    ehdr.e_machine = EM_X86_64;
    ehdr.e_version = EV_CURRENT;
    ehdr.e_entry = code_vaddr;  // Entry point = start of code
    ehdr.e_phoff = ehdr_size;   // Program headers right after ELF header
    ehdr.e_shoff = 0;           // No section headers needed for execution
    ehdr.e_flags = 0;
    ehdr.e_ehsize = ehdr_size;
    ehdr.e_phentsize = phdr_size;
    ehdr.e_phnum = 1;           // Single LOAD segment
    ehdr.e_shentsize = 0;
    ehdr.e_shnum = 0;
    ehdr.e_shstrndx = 0;

    // Single program header: load entire file from offset 0
    Elf64_Phdr phdr = {0};
    phdr.p_type = PT_LOAD;
    phdr.p_flags = PF_R | PF_W | PF_X;  // Read + Write + Execute
    phdr.p_offset = 0;                  // Load from start of file
    phdr.p_vaddr = VADDR_BASE;          // Map to base address
    phdr.p_paddr = VADDR_BASE;
    phdr.p_filesz = total_size;         // Entire file
    phdr.p_memsz = total_size;
    phdr.p_align = 0x1000;              // Page alignment

    // Apply data relocations: patch RIP-relative offsets to data section
    for (size_t i = 0; i < code->data_reloc_count; i++) {
        DataReloc* dr = &code->data_relocs[i];
        // Calculate RIP-relative offset:
        // instruction_addr = code_vaddr + dr->code_offset (points to the 4-byte displacement)
        // RIP after instruction = instruction_addr + 4 (displacement is 4 bytes)
        // target_addr = data_vaddr + dr->data_offset
        // offset = target_addr - RIP_after
        size_t rip_after = code_vaddr + dr->code_offset + 4;
        size_t target = data_vaddr + dr->data_offset;
        int32_t offset = (int32_t)(target - rip_after);
        memcpy(code->code + dr->code_offset, &offset, 4);
    }

    // Write everything
    fwrite(&ehdr, ehdr_size, 1, f);
    fwrite(&phdr, phdr_size, 1, f);
    fwrite(code->code, code->size, 1, f);
    if (data && data_size > 0) {
        fwrite(data, data_size, 1, f);
    }

    fclose(f);

    // Make executable (chmod +x)
    #ifndef _WIN32
    chmod(filename, 0755);
    #endif

    return 0;
}