// src/pe_coff.c - Windows PE/COFF object file generation
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../include/pe_coff.h"

PECOFFFile* init_pecoff_file() {
    PECOFFFile* p = calloc(1, sizeof(PECOFFFile));
    if (!p) return NULL;
    
    // Initialize header
    p->header.Machine = IMAGE_FILE_MACHINE_AMD64;
    p->header.NumberOfSections = 0;
    p->header.TimeDateStamp = (uint32_t)time(NULL);
    p->header.PointerToSymbolTable = 0;
    p->header.NumberOfSymbols = 0;
    p->header.SizeOfOptionalHeader = 0;  // No optional header for .obj
    p->header.Characteristics = 0;
    
    // Allocate text section
    p->text_section = calloc(1, sizeof(COFFSectionHeader));
    if (!p->text_section) { free(p); return NULL; }
    
    strncpy(p->text_section->Name, ".text", 8);
    p->text_section->Characteristics = IMAGE_SCN_CNT_CODE | 
                                        IMAGE_SCN_MEM_EXECUTE | 
                                        IMAGE_SCN_MEM_READ |
                                        IMAGE_SCN_ALIGN_16BYTES;
    
    // Allocate data section
    p->data_section = calloc(1, sizeof(COFFSectionHeader));
    if (!p->data_section) { free(p->text_section); free(p); return NULL; }
    
    strncpy(p->data_section->Name, ".data", 8);
    p->data_section->Characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA |
                                        IMAGE_SCN_MEM_READ |
                                        IMAGE_SCN_MEM_WRITE;
    
    p->code = NULL;
    p->code_size = 0;
    p->data = NULL;
    p->data_size = 0;
    p->symbols = NULL;
    p->num_symbols = 0;
    
    // Initialize string table (starts with 4-byte size)
    p->string_table_size = 4;
    p->string_table = calloc(1, p->string_table_size);
    
    return p;
}

void free_pecoff_file(PECOFFFile* p) {
    if (!p) return;
    if (p->text_section) free(p->text_section);
    if (p->data_section) free(p->data_section);
    if (p->code) free(p->code);
    if (p->data) free(p->data);
    if (p->symbols) free(p->symbols);
    if (p->string_table) free(p->string_table);
    free(p);
}

void pecoff_set_code(PECOFFFile* p, uint8_t* code, size_t size) {
    if (!p || !code || size == 0) return;
    p->code = malloc(size);
    if (p->code) {
        memcpy(p->code, code, size);
        p->code_size = size;
    }
}

void pecoff_set_data(PECOFFFile* p, uint8_t* data, size_t size) {
    if (!p || !data || size == 0) return;
    p->data = malloc(size);
    if (p->data) {
        memcpy(p->data, data, size);
        p->data_size = size;
    }
}

int pecoff_add_symbol(PECOFFFile* p, const char* name, uint32_t value, int16_t section, uint8_t storage_class) {
    if (!p || !name) return -1;
    
    // Reallocate symbol array
    size_t new_count = p->num_symbols + 1;
    COFFSymbol* new_syms = realloc(p->symbols, new_count * sizeof(COFFSymbol));
    if (!new_syms) return -1;
    p->symbols = new_syms;
    
    COFFSymbol* sym = &p->symbols[p->num_symbols];
    memset(sym, 0, sizeof(COFFSymbol));
    
    size_t name_len = strlen(name);
    
    if (name_len <= 8) {
        // Short name - fits in symbol
        strncpy(sym->Name.ShortName, name, 8);
    } else {
        // Long name - add to string table
        sym->Name.LongName.Zeros = 0;
        sym->Name.LongName.Offset = p->string_table_size;
        
        // Expand string table
        size_t new_size = p->string_table_size + name_len + 1;
        char* new_table = realloc(p->string_table, new_size);
        if (!new_table) return -1;
        p->string_table = new_table;
        
        strcpy(p->string_table + p->string_table_size, name);
        p->string_table_size = new_size;
    }
    
    sym->Value = value;
    sym->SectionNumber = section;
    sym->Type = IMAGE_SYM_DTYPE_FUNCTION;
    sym->StorageClass = storage_class;
    sym->NumberOfAuxSymbols = 0;
    
    p->num_symbols++;
    return 0;
}

int write_pecoff_file(PECOFFFile* p, const char* filename) {
    if (!p || !filename) return -1;
    
    FILE* f = fopen(filename, "wb");
    if (!f) return -1;
    
    // Calculate layout
    uint32_t num_sections = 0;
    if (p->code_size > 0) num_sections++;
    if (p->data_size > 0) num_sections++;
    
    uint32_t header_size = sizeof(COFFHeader);
    uint32_t section_headers_size = num_sections * sizeof(COFFSectionHeader);
    uint32_t section_data_offset = header_size + section_headers_size;
    
    // Align to 4 bytes
    section_data_offset = (section_data_offset + 3) & ~3;
    
    // Update text section
    uint32_t text_offset = section_data_offset;
    p->text_section->PointerToRawData = text_offset;
    p->text_section->SizeOfRawData = p->code_size;
    p->text_section->VirtualSize = p->code_size;
    p->text_section->VirtualAddress = 0;
    
    // Update data section
    uint32_t data_offset = text_offset + p->code_size;
    data_offset = (data_offset + 3) & ~3;  // Align
    p->data_section->PointerToRawData = data_offset;
    p->data_section->SizeOfRawData = p->data_size;
    p->data_section->VirtualSize = p->data_size;
    p->data_section->VirtualAddress = p->code_size;
    
    // Symbol table offset
    uint32_t symtab_offset = data_offset + p->data_size;
    symtab_offset = (symtab_offset + 3) & ~3;  // Align
    
    // Update header
    p->header.NumberOfSections = num_sections;
    p->header.PointerToSymbolTable = symtab_offset;
    p->header.NumberOfSymbols = p->num_symbols;
    
    // Update string table size field
    if (p->string_table && p->string_table_size >= 4) {
        uint32_t size = p->string_table_size;
        memcpy(p->string_table, &size, 4);
    }
    
    // Write header
    fwrite(&p->header, sizeof(COFFHeader), 1, f);
    
    // Write section headers
    if (p->code_size > 0) {
        fwrite(p->text_section, sizeof(COFFSectionHeader), 1, f);
    }
    if (p->data_size > 0) {
        fwrite(p->data_section, sizeof(COFFSectionHeader), 1, f);
    }
    
    // Pad to section data offset
    long current = ftell(f);
    while (current < section_data_offset) {
        fputc(0, f);
        current++;
    }
    
    // Write code
    if (p->code && p->code_size > 0) {
        fwrite(p->code, 1, p->code_size, f);
    }
    
    // Pad to data offset
    current = ftell(f);
    while (current < data_offset) {
        fputc(0, f);
        current++;
    }
    
    // Write data
    if (p->data && p->data_size > 0) {
        fwrite(p->data, 1, p->data_size, f);
    }
    
    // Pad to symbol table
    current = ftell(f);
    while (current < symtab_offset) {
        fputc(0, f);
        current++;
    }
    
    // Write symbols
    if (p->symbols && p->num_symbols > 0) {
        fwrite(p->symbols, sizeof(COFFSymbol), p->num_symbols, f);
    }
    
    // Write string table
    if (p->string_table && p->string_table_size > 0) {
        fwrite(p->string_table, 1, p->string_table_size, f);
    }
    
    fclose(f);
    return 0;
}

int compile_to_pecoff(MachineCode* mc, const char* filename) {
    if (!mc || !filename) return -1;
    
    PECOFFFile* p = init_pecoff_file();
    if (!p) return -1;
    
    pecoff_set_code(p, mc->code, mc->size);
    
    // Add main symbol
    pecoff_add_symbol(p, "main", 0, 1, IMAGE_SYM_CLASS_EXTERNAL);
    
    int result = write_pecoff_file(p, filename);
    free_pecoff_file(p);
    
    return result;
}