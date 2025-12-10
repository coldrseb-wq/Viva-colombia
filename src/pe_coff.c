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

// === STANDALONE PE EXECUTABLE (imports from kernel32.dll) ===
// Windows has no stable syscall ABI, so we import from kernel32.dll:
// - GetStdHandle(-11) to get stdout handle
// - WriteFile(handle, buffer, length, &written, NULL) to write
// - ExitProcess(code) to exit

int write_standalone_pe_executable(const char* filename, MachineCode* code,
                                   uint8_t* data, size_t data_size) {
    if (!filename || !code) return -1;

    FILE* f = fopen(filename, "wb");
    if (!f) return -1;

    // PE executable layout (simplified - no imports for now, uses int 0x2e which is obsolete)
    // For a proper Windows executable we'd need import tables
    // This creates a minimal PE that demonstrates the format but won't actually run
    // without proper kernel32.dll imports

    uint32_t file_align = 0x200;      // 512-byte file alignment
    uint32_t sect_align = 0x1000;     // 4KB section alignment
    uint64_t image_base = 0x140000000;  // Default 64-bit image base

    // DOS header (64 bytes)
    DOSHeader dos = {0};
    dos.e_magic = 0x5A4D;  // "MZ"
    dos.e_lfanew = sizeof(DOSHeader);  // PE header right after DOS header

    // PE signature (4 bytes)
    uint32_t pe_sig = 0x00004550;  // "PE\0\0"

    // COFF header (20 bytes)
    COFFHeader coff = {0};
    coff.Machine = IMAGE_FILE_MACHINE_AMD64;
    coff.NumberOfSections = 2;  // .text and .data
    coff.TimeDateStamp = (uint32_t)time(NULL);
    coff.PointerToSymbolTable = 0;
    coff.NumberOfSymbols = 0;
    coff.SizeOfOptionalHeader = sizeof(PE64OptionalHeader) + IMAGE_NUMBEROF_DIRECTORY_ENTRIES * sizeof(DataDirectory);
    coff.Characteristics = IMAGE_FILE_EXECUTABLE_IMAGE | IMAGE_FILE_LARGE_ADDRESS_AWARE;

    // Calculate section offsets
    uint32_t headers_size = sizeof(DOSHeader) + 4 + sizeof(COFFHeader) +
                            coff.SizeOfOptionalHeader + 2 * sizeof(COFFSectionHeader);
    uint32_t headers_aligned = (headers_size + file_align - 1) & ~(file_align - 1);

    uint32_t text_file_offset = headers_aligned;
    uint32_t text_size_aligned = (code->size + file_align - 1) & ~(file_align - 1);
    uint32_t text_rva = sect_align;  // First section at 0x1000

    uint32_t data_file_offset = text_file_offset + text_size_aligned;
    uint32_t data_size_actual = data ? data_size : 0;
    uint32_t data_size_aligned = (data_size_actual + file_align - 1) & ~(file_align - 1);
    uint32_t data_rva = text_rva + ((code->size + sect_align - 1) & ~(sect_align - 1));

    uint32_t total_image_size = data_rva + ((data_size_actual + sect_align - 1) & ~(sect_align - 1));

    // Optional header
    PE64OptionalHeader opt = {0};
    opt.Magic = 0x20b;  // PE32+
    opt.MajorLinkerVersion = 1;
    opt.MinorLinkerVersion = 0;
    opt.SizeOfCode = code->size;
    opt.SizeOfInitializedData = data_size_actual;
    opt.SizeOfUninitializedData = 0;
    opt.AddressOfEntryPoint = text_rva;  // Entry = start of .text
    opt.BaseOfCode = text_rva;
    opt.ImageBase = image_base;
    opt.SectionAlignment = sect_align;
    opt.FileAlignment = file_align;
    opt.MajorOperatingSystemVersion = 6;  // Windows Vista+
    opt.MinorOperatingSystemVersion = 0;
    opt.MajorImageVersion = 1;
    opt.MinorImageVersion = 0;
    opt.MajorSubsystemVersion = 6;
    opt.MinorSubsystemVersion = 0;
    opt.Win32VersionValue = 0;
    opt.SizeOfImage = total_image_size;
    opt.SizeOfHeaders = headers_aligned;
    opt.CheckSum = 0;
    opt.Subsystem = IMAGE_SUBSYSTEM_WINDOWS_CUI;
    opt.DllCharacteristics = 0;
    opt.SizeOfStackReserve = 0x100000;
    opt.SizeOfStackCommit = 0x1000;
    opt.SizeOfHeapReserve = 0x100000;
    opt.SizeOfHeapCommit = 0x1000;
    opt.LoaderFlags = 0;
    opt.NumberOfRvaAndSizes = IMAGE_NUMBEROF_DIRECTORY_ENTRIES;

    // Data directories (all zero - no imports in this simple version)
    DataDirectory data_dirs[IMAGE_NUMBEROF_DIRECTORY_ENTRIES] = {0};

    // Section headers
    COFFSectionHeader text_sect = {0};
    strncpy(text_sect.Name, ".text", 8);
    text_sect.VirtualSize = code->size;
    text_sect.VirtualAddress = text_rva;
    text_sect.SizeOfRawData = text_size_aligned;
    text_sect.PointerToRawData = text_file_offset;
    text_sect.PointerToRelocations = 0;
    text_sect.PointerToLineNumbers = 0;
    text_sect.NumberOfRelocations = 0;
    text_sect.NumberOfLineNumbers = 0;
    text_sect.Characteristics = IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ;

    COFFSectionHeader data_sect = {0};
    strncpy(data_sect.Name, ".data", 8);
    data_sect.VirtualSize = data_size_actual;
    data_sect.VirtualAddress = data_rva;
    data_sect.SizeOfRawData = data_size_aligned;
    data_sect.PointerToRawData = data_file_offset;
    data_sect.PointerToRelocations = 0;
    data_sect.PointerToLineNumbers = 0;
    data_sect.NumberOfRelocations = 0;
    data_sect.NumberOfLineNumbers = 0;
    data_sect.Characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE;

    // Apply data relocations (RIP-relative)
    for (size_t i = 0; i < code->data_reloc_count; i++) {
        DataReloc* dr = &code->data_relocs[i];
        // Virtual addresses
        uint64_t code_va = image_base + text_rva;
        uint64_t data_va = image_base + data_rva;
        uint64_t rip_after = code_va + dr->code_offset + 4;
        uint64_t target = data_va + dr->data_offset;
        int32_t offset = (int32_t)(target - rip_after);
        memcpy(code->code + dr->code_offset, &offset, 4);
    }

    // Write DOS header
    fwrite(&dos, sizeof(dos), 1, f);

    // Write PE signature
    fwrite(&pe_sig, 4, 1, f);

    // Write COFF header
    fwrite(&coff, sizeof(coff), 1, f);

    // Write optional header
    fwrite(&opt, sizeof(opt), 1, f);

    // Write data directories
    fwrite(data_dirs, sizeof(data_dirs), 1, f);

    // Write section headers
    fwrite(&text_sect, sizeof(text_sect), 1, f);
    fwrite(&data_sect, sizeof(data_sect), 1, f);

    // Pad to text section
    long current = ftell(f);
    while (current < text_file_offset) {
        fputc(0, f);
        current++;
    }

    // Write code
    fwrite(code->code, code->size, 1, f);

    // Pad to file alignment
    current = ftell(f);
    while (current < data_file_offset) {
        fputc(0, f);
        current++;
    }

    // Write data
    if (data && data_size > 0) {
        fwrite(data, data_size, 1, f);
    }

    // Pad to file alignment
    current = ftell(f);
    uint32_t total_file = data_file_offset + data_size_aligned;
    while (current < total_file) {
        fputc(0, f);
        current++;
    }

    fclose(f);
    return 0;
}