// src/machine_code.c
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

int encode_push_rbp(MachineCode* mc) {
    uint8_t c[] = {0x55};
    return append_bytes(mc, c, 1);
}

int encode_mov_rbp_rsp(MachineCode* mc) {
    uint8_t c[] = {0x48, 0x89, 0xE5};
    return append_bytes(mc, c, 3);
}

int encode_mov_rbp_rdi(MachineCode* mc) {
    uint8_t c[] = {0x48, 0x89, 0x7D, 0xF8};
    return append_bytes(mc, c, 4);
}

int encode_mov_rdi_imm64(MachineCode* mc, uint64_t v) {
    uint8_t c[10] = {0x48, 0xBF};
    memcpy(c + 2, &v, 8);
    return append_bytes(mc, c, 10);
}

int encode_mov_rax_imm32(MachineCode* mc, int32_t v) {
    uint8_t c[7] = {0x48, 0xC7, 0xC0};
    memcpy(c + 3, &v, 4);
    return append_bytes(mc, c, 7);
}

int encode_mov_rbx_imm32(MachineCode* mc, int32_t v) {
    uint8_t c[7] = {0x48, 0xC7, 0xC3};
    memcpy(c + 3, &v, 4);
    return append_bytes(mc, c, 7);
}

int encode_add_rax_rbx(MachineCode* mc) {
    uint8_t c[] = {0x48, 0x01, 0xD8};
    return append_bytes(mc, c, 3);
}

int encode_sub_rax_rbx(MachineCode* mc) {
    uint8_t c[] = {0x48, 0x29, 0xD8};
    return append_bytes(mc, c, 3);
}

int encode_mul_rbx(MachineCode* mc) {
    uint8_t c[] = {0x48, 0x0F, 0xAF, 0xC3};
    return append_bytes(mc, c, 4);
}

int encode_div_rbx(MachineCode* mc) {
    uint8_t a[] = {0x48, 0x31, 0xD2};
    uint8_t b[] = {0x48, 0xF7, 0xF3};
    if (append_bytes(mc, a, 3)) return -1;
    return append_bytes(mc, b, 3);
}

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

int encode_pop_rbp(MachineCode* mc) {
    uint8_t c[] = {0x5D};
    return append_bytes(mc, c, 1);
}

int encode_ret(MachineCode* mc) {
    uint8_t c[] = {0xC3};
    return append_bytes(mc, c, 1);
}

int encode_call_printf(MachineCode* mc) {
    uint8_t c[] = {0xE8, 0x00, 0x00, 0x00, 0x00};
    return append_bytes(mc, c, 5);
}

int encode_call_external(MachineCode* mc) {
    uint8_t c[] = {0xE8, 0x00, 0x00, 0x00, 0x00};
    return append_bytes(mc, c, 5);
}

int add_relocation_entry(MachineCode* mc, uint32_t sym, uint32_t type, int64_t add) {
    if (mc->reloc_count >= mc->reloc_capacity) {
        mc->reloc_capacity *= 2;
        mc->relocations = realloc(mc->relocations, mc->reloc_capacity * sizeof(Elf64_Rela));
    }
    Elf64_Rela* r = &mc->relocations[mc->reloc_count++];
    r->r_offset = mc->size;
    r->r_info = ELF64_R_INFO(sym, type);
    r->r_addend = add;
    return 0;
}

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