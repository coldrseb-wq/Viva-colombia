// src/machine_code.c - EXPANDED for Phase 4
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
    return mc;
}

void free_machine_code(MachineCode* mc) {
    if (mc) {
        if (mc->code) free(mc->code);
        if (mc->relocations) free(mc->relocations);
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

int encode_mov_rdx_rax(MachineCode* mc) {
    uint8_t c[] = {0x48, 0x89, 0xC2};  // mov rdx, rax
    return append_bytes(mc, c, 3);
}

int encode_mov_rax_rdi(MachineCode* mc) {
    uint8_t c[] = {0x48, 0x89, 0xF8};  // mov rax, rdi
    return append_bytes(mc, c, 3);
}

int encode_mov_rax_rsi(MachineCode* mc) {
    uint8_t c[] = {0x48, 0x89, 0xF0};  // mov rax, rsi
    return append_bytes(mc, c, 3);
}

int encode_mov_rax_rdx(MachineCode* mc) {
    uint8_t c[] = {0x48, 0x89, 0xD0};  // mov rax, rdx
    return append_bytes(mc, c, 3);
}

int encode_mov_rax_rcx(MachineCode* mc) {
    uint8_t c[] = {0x48, 0x89, 0xC8};  // mov rax, rcx
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

// Store parameter registers to stack (for function parameters)
int encode_mov_memory_from_rdi(MachineCode* mc, int off) {
    uint8_t c[] = {0x48, 0x89, 0xBD};  // mov [rbp+off], rdi
    if (append_bytes(mc, c, 3)) return -1;
    return append_bytes(mc, (uint8_t*)&off, 4);
}

int encode_mov_memory_from_rsi(MachineCode* mc, int off) {
    uint8_t c[] = {0x48, 0x89, 0xB5};  // mov [rbp+off], rsi
    if (append_bytes(mc, c, 3)) return -1;
    return append_bytes(mc, (uint8_t*)&off, 4);
}

int encode_mov_memory_from_rdx(MachineCode* mc, int off) {
    uint8_t c[] = {0x48, 0x89, 0x95};  // mov [rbp+off], rdx
    if (append_bytes(mc, c, 3)) return -1;
    return append_bytes(mc, (uint8_t*)&off, 4);
}

int encode_mov_memory_from_rcx(MachineCode* mc, int off) {
    uint8_t c[] = {0x48, 0x89, 0x8D};  // mov [rbp+off], rcx
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

// === LOGICAL/BITWISE ===
int encode_and_rax_rbx(MachineCode* mc) {
    uint8_t c[] = {0x48, 0x21, 0xD8};  // and rax, rbx
    return append_bytes(mc, c, 3);
}

int encode_or_rax_rbx(MachineCode* mc) {
    uint8_t c[] = {0x48, 0x09, 0xD8};  // or rax, rbx
    return append_bytes(mc, c, 3);
}

int encode_xor_rax_rbx(MachineCode* mc) {
    uint8_t c[] = {0x48, 0x31, 0xD8};  // xor rax, rbx
    return append_bytes(mc, c, 3);
}

int encode_xor_rax_rax(MachineCode* mc) {
    uint8_t c[] = {0x48, 0x31, 0xC0};  // xor rax, rax (zero rax)
    return append_bytes(mc, c, 3);
}

int encode_xor_rdx_rdx(MachineCode* mc) {
    uint8_t c[] = {0x48, 0x31, 0xD2};  // xor rdx, rdx (zero rdx)
    return append_bytes(mc, c, 3);
}

// === SHIFT OPERATIONS ===
int encode_shl_rax_cl(MachineCode* mc) {
    uint8_t c[] = {0x48, 0xD3, 0xE0};  // shl rax, cl
    return append_bytes(mc, c, 3);
}

int encode_shr_rax_cl(MachineCode* mc) {
    uint8_t c[] = {0x48, 0xD3, 0xE8};  // shr rax, cl (unsigned)
    return append_bytes(mc, c, 3);
}

int encode_sar_rax_cl(MachineCode* mc) {
    uint8_t c[] = {0x48, 0xD3, 0xF8};  // sar rax, cl (signed)
    return append_bytes(mc, c, 3);
}

int encode_mov_cl_bl(MachineCode* mc) {
    uint8_t c[] = {0x88, 0xD9};  // mov cl, bl
    return append_bytes(mc, c, 2);
}

// === MODULO ===
int encode_mod_rbx(MachineCode* mc) {
    // Modulo: rax = rax % rbx
    // Uses idiv which puts quotient in rax, remainder in rdx
    uint8_t a[] = {0x48, 0x31, 0xD2};  // xor rdx, rdx (clear for division)
    uint8_t b[] = {0x48, 0xF7, 0xF3};  // idiv rbx
    uint8_t c[] = {0x48, 0x89, 0xD0};  // mov rax, rdx (move remainder to rax)
    if (append_bytes(mc, a, 3)) return -1;
    if (append_bytes(mc, b, 3)) return -1;
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
    add_elf_section(e, ".strtab", SHT_STRTAB, 0);
    int idx = add_elf_section(e, ".symtab", SHT_SYMTAB, 0);
    if (idx >= 0) e->sections[idx]->entsize = sizeof(Elf64_Sym);
    add_elf_section(e, ".shstrtab", SHT_STRTAB, 0);
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
    
    e->header.e_shnum = e->num_sections + 1;
    e->header.e_shoff = off;
    
    fwrite(&e->header, sizeof(Elf64_Ehdr), 1, f);
    
    for (int i = 0; i < e->num_sections; i++) {
        if (e->sections[i]->data && e->sections[i]->size > 0) {
            fseek(f, e->sections[i]->offset, SEEK_SET);
            fwrite(e->sections[i]->data, 1, e->sections[i]->size, f);
        }
    }
    
    fseek(f, off, SEEK_SET);
    Elf64_Shdr null_sh = {0};
    fwrite(&null_sh, sizeof(Elf64_Shdr), 1, f);
    
    for (int i = 0; i < e->num_sections; i++) {
        Elf64_Shdr sh = {0};
        sh.sh_type = e->sections[i]->type;
        sh.sh_flags = e->sections[i]->flags;
        sh.sh_offset = e->sections[i]->offset;
        sh.sh_size = e->sections[i]->size;
        sh.sh_addralign = e->sections[i]->addralign;
        sh.sh_entsize = e->sections[i]->entsize;
        fwrite(&sh, sizeof(Elf64_Shdr), 1, f);
    }
    
    fclose(f);
    return 0;
}