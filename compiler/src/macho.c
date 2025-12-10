// src/macho.c - macOS Mach-O object file generation
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/macho.h"

MachOFile* init_macho_file() {
    MachOFile* m = calloc(1, sizeof(MachOFile));
    if (!m) return NULL;
    
    // Initialize header
    m->header.magic = MH_MAGIC_64;
    m->header.cputype = CPU_TYPE_X86_64;
    m->header.cpusubtype = CPU_SUBTYPE_X86_64;
    m->header.filetype = MH_OBJECT;
    m->header.ncmds = 0;
    m->header.sizeofcmds = 0;
    m->header.flags = 0;
    m->header.reserved = 0;
    
    // Allocate segment
    m->segment = calloc(1, sizeof(SegmentCommand64));
    if (!m->segment) { free(m); return NULL; }
    
    m->segment->cmd = LC_SEGMENT_64;
    m->segment->cmdsize = sizeof(SegmentCommand64);
    strncpy(m->segment->segname, "", 16);
    m->segment->vmaddr = 0;
    m->segment->vmsize = 0;
    m->segment->fileoff = 0;
    m->segment->filesize = 0;
    m->segment->maxprot = 7;  // rwx
    m->segment->initprot = 7;
    m->segment->nsects = 0;
    m->segment->flags = 0;
    
    // Allocate text section
    m->text_section = calloc(1, sizeof(Section64));
    if (!m->text_section) { free(m->segment); free(m); return NULL; }
    
    strncpy(m->text_section->sectname, "__text", 16);
    strncpy(m->text_section->segname, "__TEXT", 16);
    m->text_section->flags = S_REGULAR | S_ATTR_PURE_INSTRUCTIONS | S_ATTR_SOME_INSTRUCTIONS;
    m->text_section->align = 4;  // 16-byte aligned (2^4)
    
    // Allocate data section
    m->data_section = calloc(1, sizeof(Section64));
    if (!m->data_section) { free(m->text_section); free(m->segment); free(m); return NULL; }
    
    strncpy(m->data_section->sectname, "__data", 16);
    strncpy(m->data_section->segname, "__DATA", 16);
    m->data_section->flags = S_REGULAR;
    m->data_section->align = 3;  // 8-byte aligned (2^3)
    
    // Allocate symtab
    m->symtab = calloc(1, sizeof(SymtabCommand));
    if (!m->symtab) { free(m->data_section); free(m->text_section); free(m->segment); free(m); return NULL; }
    
    m->symtab->cmd = LC_SYMTAB;
    m->symtab->cmdsize = sizeof(SymtabCommand);
    
    m->num_sections = 0;
    m->code = NULL;
    m->code_size = 0;
    m->data = NULL;
    m->data_size = 0;
    
    return m;
}

void free_macho_file(MachOFile* m) {
    if (!m) return;
    if (m->segment) free(m->segment);
    if (m->text_section) free(m->text_section);
    if (m->data_section) free(m->data_section);
    if (m->symtab) free(m->symtab);
    if (m->code) free(m->code);
    if (m->data) free(m->data);
    free(m);
}

void macho_set_code(MachOFile* m, uint8_t* code, size_t size) {
    if (!m || !code || size == 0) return;
    m->code = malloc(size);
    if (m->code) {
        memcpy(m->code, code, size);
        m->code_size = size;
    }
}

void macho_set_data(MachOFile* m, uint8_t* data, size_t size) {
    if (!m || !data || size == 0) return;
    m->data = malloc(size);
    if (m->data) {
        memcpy(m->data, data, size);
        m->data_size = size;
    }
}

int write_macho_file(MachOFile* m, const char* filename) {
    if (!m || !filename) return -1;
    
    FILE* f = fopen(filename, "wb");
    if (!f) return -1;
    
    // Calculate offsets
    uint32_t header_size = sizeof(MachHeader64);
    uint32_t num_sections = 0;
    
    if (m->code_size > 0) num_sections++;
    if (m->data_size > 0) num_sections++;
    
    m->segment->nsects = num_sections;
    m->segment->cmdsize = sizeof(SegmentCommand64) + num_sections * sizeof(Section64);
    
    uint32_t load_cmds_size = m->segment->cmdsize + sizeof(SymtabCommand);
    uint32_t section_data_offset = header_size + load_cmds_size;
    
    // Align to 16 bytes
    section_data_offset = (section_data_offset + 15) & ~15;
    
    // Update header
    m->header.ncmds = 2;  // segment + symtab
    m->header.sizeofcmds = load_cmds_size;
    
    // Update segment
    m->segment->fileoff = section_data_offset;
    m->segment->filesize = m->code_size + m->data_size;
    m->segment->vmsize = m->segment->filesize;
    
    // Update text section
    uint32_t text_offset = section_data_offset;
    m->text_section->offset = text_offset;
    m->text_section->size = m->code_size;
    m->text_section->addr = 0;
    
    // Update data section
    uint32_t data_offset = text_offset + m->code_size;
    m->data_section->offset = data_offset;
    m->data_section->size = m->data_size;
    m->data_section->addr = m->code_size;
    
    // Symbol table at end
    uint32_t symtab_offset = section_data_offset + m->code_size + m->data_size;
    symtab_offset = (symtab_offset + 7) & ~7;  // 8-byte align
    
    // Create symbol for _main
    char* strtab = "\0_main";
    size_t strtab_size = 7;
    
    Nlist64 main_sym = {0};
    main_sym.n_strx = 1;  // offset of "_main" in strtab
    main_sym.n_type = N_SECT | N_EXT;
    main_sym.n_sect = 1;  // __text section
    main_sym.n_desc = 0;
    main_sym.n_value = 0;  // start of text
    
    m->symtab->symoff = symtab_offset;
    m->symtab->nsyms = 1;
    m->symtab->stroff = symtab_offset + sizeof(Nlist64);
    m->symtab->strsize = strtab_size;
    
    // Write header
    fwrite(&m->header, sizeof(MachHeader64), 1, f);
    
    // Write segment command
    fwrite(m->segment, sizeof(SegmentCommand64), 1, f);
    
    // Write sections
    if (m->code_size > 0) {
        fwrite(m->text_section, sizeof(Section64), 1, f);
    }
    if (m->data_size > 0) {
        fwrite(m->data_section, sizeof(Section64), 1, f);
    }
    
    // Write symtab command
    fwrite(m->symtab, sizeof(SymtabCommand), 1, f);
    
    // Pad to section data offset
    long current = ftell(f);
    while (current < section_data_offset) {
        fputc(0, f);
        current++;
    }
    
    // Write code
    if (m->code && m->code_size > 0) {
        fwrite(m->code, 1, m->code_size, f);
    }
    
    // Write data
    if (m->data && m->data_size > 0) {
        fwrite(m->data, 1, m->data_size, f);
    }
    
    // Pad to symtab offset
    current = ftell(f);
    while (current < symtab_offset) {
        fputc(0, f);
        current++;
    }
    
    // Write symbol table
    fwrite(&main_sym, sizeof(Nlist64), 1, f);
    
    // Write string table
    fwrite(strtab, 1, strtab_size, f);
    
    fclose(f);
    return 0;
}

int compile_to_macho(MachineCode* mc, const char* filename) {
    if (!mc || !filename) return -1;
    
    MachOFile* m = init_macho_file();
    if (!m) return -1;
    
    macho_set_code(m, mc->code, mc->size);
    
    int result = write_macho_file(m, filename);
    free_macho_file(m);
    
    return result;
}