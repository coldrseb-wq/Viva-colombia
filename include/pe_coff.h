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

// === PE EXECUTABLE STRUCTURES (full EXE, not just COFF .obj) ===

// DOS Header (64 bytes)
typedef struct {
    uint16_t e_magic;      // "MZ"
    uint16_t e_cblp;
    uint16_t e_cp;
    uint16_t e_crlc;
    uint16_t e_cparhdr;
    uint16_t e_minalloc;
    uint16_t e_maxalloc;
    uint16_t e_ss;
    uint16_t e_sp;
    uint16_t e_csum;
    uint16_t e_ip;
    uint16_t e_cs;
    uint16_t e_lfarlc;
    uint16_t e_ovno;
    uint16_t e_res[4];
    uint16_t e_oemid;
    uint16_t e_oeminfo;
    uint16_t e_res2[10];
    uint32_t e_lfanew;     // Offset to PE header
} DOSHeader;

// PE Optional Header (64-bit)
typedef struct {
    uint16_t Magic;                  // 0x20b for PE32+
    uint8_t  MajorLinkerVersion;
    uint8_t  MinorLinkerVersion;
    uint32_t SizeOfCode;
    uint32_t SizeOfInitializedData;
    uint32_t SizeOfUninitializedData;
    uint32_t AddressOfEntryPoint;
    uint32_t BaseOfCode;
    uint64_t ImageBase;
    uint32_t SectionAlignment;
    uint32_t FileAlignment;
    uint16_t MajorOperatingSystemVersion;
    uint16_t MinorOperatingSystemVersion;
    uint16_t MajorImageVersion;
    uint16_t MinorImageVersion;
    uint16_t MajorSubsystemVersion;
    uint16_t MinorSubsystemVersion;
    uint32_t Win32VersionValue;
    uint32_t SizeOfImage;
    uint32_t SizeOfHeaders;
    uint32_t CheckSum;
    uint16_t Subsystem;
    uint16_t DllCharacteristics;
    uint64_t SizeOfStackReserve;
    uint64_t SizeOfStackCommit;
    uint64_t SizeOfHeapReserve;
    uint64_t SizeOfHeapCommit;
    uint32_t LoaderFlags;
    uint32_t NumberOfRvaAndSizes;
} PE64OptionalHeader;

// Data Directory
typedef struct {
    uint32_t VirtualAddress;
    uint32_t Size;
} DataDirectory;

#define IMAGE_NUMBEROF_DIRECTORY_ENTRIES 16
#define IMAGE_DIRECTORY_ENTRY_IMPORT     1

// PE subsystem values
#define IMAGE_SUBSYSTEM_WINDOWS_CUI      3   // Console application

// === STANDALONE PE EXECUTABLE (no libc needed, uses kernel32.dll) ===
int write_standalone_pe_executable(const char* filename, MachineCode* code,
                                   uint8_t* data, size_t data_size);

#endif