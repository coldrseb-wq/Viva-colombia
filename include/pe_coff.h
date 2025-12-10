#ifndef PE_COFF_H
#define PE_COFF_H

#include <stdint.h>
#include <stdlib.h>
#include "machine_code.h"

// COFF Machine types
#define IMAGE_FILE_MACHINE_AMD64    0x8664
#define IMAGE_FILE_MACHINE_I386     0x14c

// COFF Characteristics
#define IMAGE_FILE_RELOCS_STRIPPED          0x0001
#define IMAGE_FILE_EXECUTABLE_IMAGE         0x0002
#define IMAGE_FILE_LINE_NUMS_STRIPPED       0x0004
#define IMAGE_FILE_LOCAL_SYMS_STRIPPED      0x0008
#define IMAGE_FILE_LARGE_ADDRESS_AWARE      0x0020

// Section Characteristics
#define IMAGE_SCN_CNT_CODE                  0x00000020
#define IMAGE_SCN_CNT_INITIALIZED_DATA      0x00000040
#define IMAGE_SCN_CNT_UNINITIALIZED_DATA    0x00000080
#define IMAGE_SCN_MEM_EXECUTE               0x20000000
#define IMAGE_SCN_MEM_READ                  0x40000000
#define IMAGE_SCN_MEM_WRITE                 0x80000000
#define IMAGE_SCN_ALIGN_16BYTES             0x00500000

// Symbol storage classes
#define IMAGE_SYM_CLASS_EXTERNAL            2
#define IMAGE_SYM_CLASS_STATIC              3
#define IMAGE_SYM_CLASS_LABEL               6

// Symbol type
#define IMAGE_SYM_TYPE_NULL                 0
#define IMAGE_SYM_DTYPE_FUNCTION            0x20

// COFF File Header
typedef struct {
    uint16_t Machine;
    uint16_t NumberOfSections;
    uint32_t TimeDateStamp;
    uint32_t PointerToSymbolTable;
    uint32_t NumberOfSymbols;
    uint16_t SizeOfOptionalHeader;
    uint16_t Characteristics;
} COFFHeader;

// COFF Section Header
typedef struct {
    char Name[8];
    uint32_t VirtualSize;
    uint32_t VirtualAddress;
    uint32_t SizeOfRawData;
    uint32_t PointerToRawData;
    uint32_t PointerToRelocations;
    uint32_t PointerToLineNumbers;
    uint16_t NumberOfRelocations;
    uint16_t NumberOfLineNumbers;
    uint32_t Characteristics;
} COFFSectionHeader;

// COFF Symbol (18 bytes)
typedef struct {
    union {
        char ShortName[8];
        struct {
            uint32_t Zeros;
            uint32_t Offset;
        } LongName;
    } Name;
    uint32_t Value;
    int16_t SectionNumber;
    uint16_t Type;
    uint8_t StorageClass;
    uint8_t NumberOfAuxSymbols;
} COFFSymbol;

// COFF Relocation
typedef struct {
    uint32_t VirtualAddress;
    uint32_t SymbolTableIndex;
    uint16_t Type;
} COFFRelocation;

// Relocation types for AMD64
#define IMAGE_REL_AMD64_ADDR64      0x0001
#define IMAGE_REL_AMD64_ADDR32      0x0002
#define IMAGE_REL_AMD64_ADDR32NB    0x0003
#define IMAGE_REL_AMD64_REL32       0x0004

// PE/COFF file structure
typedef struct {
    COFFHeader header;
    COFFSectionHeader* text_section;
    COFFSectionHeader* data_section;
    uint8_t* code;
    size_t code_size;
    uint8_t* data;
    size_t data_size;
    COFFSymbol* symbols;
    size_t num_symbols;
    char* string_table;
    size_t string_table_size;
} PECOFFFile;

// Functions
PECOFFFile* init_pecoff_file();
void free_pecoff_file(PECOFFFile* p);
void pecoff_set_code(PECOFFFile* p, uint8_t* code, size_t size);
void pecoff_set_data(PECOFFFile* p, uint8_t* data, size_t size);
int pecoff_add_symbol(PECOFFFile* p, const char* name, uint32_t value, int16_t section, uint8_t storage_class);
int write_pecoff_file(PECOFFFile* p, const char* filename);
int compile_to_pecoff(MachineCode* mc, const char* filename);

#endif