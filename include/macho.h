#ifndef MACHO_H
#define MACHO_H

#include <stdint.h>
#include <stdlib.h>
#include "machine_code.h"

// Mach-O constants
#define MH_MAGIC_64     0xfeedfacf
#define MH_OBJECT       0x1
#define MH_EXECUTE      0x2

#define CPU_TYPE_X86_64     0x01000007
#define CPU_SUBTYPE_X86_64  0x3

#define LC_SEGMENT_64   0x19
#define LC_SYMTAB       0x2
#define LC_DYSYMTAB     0xB

#define S_REGULAR       0x0
#define S_ATTR_PURE_INSTRUCTIONS    0x80000000
#define S_ATTR_SOME_INSTRUCTIONS    0x00000400

#define N_EXT   0x01
#define N_SECT  0x0e

// Mach-O header (64-bit)
typedef struct {
    uint32_t magic;
    int32_t cputype;
    int32_t cpusubtype;
    uint32_t filetype;
    uint32_t ncmds;
    uint32_t sizeofcmds;
    uint32_t flags;
    uint32_t reserved;
} MachHeader64;

// Segment command (64-bit)
typedef struct {
    uint32_t cmd;
    uint32_t cmdsize;
    char segname[16];
    uint64_t vmaddr;
    uint64_t vmsize;
    uint64_t fileoff;
    uint64_t filesize;
    int32_t maxprot;
    int32_t initprot;
    uint32_t nsects;
    uint32_t flags;
} SegmentCommand64;

// Section (64-bit)
typedef struct {
    char sectname[16];
    char segname[16];
    uint64_t addr;
    uint64_t size;
    uint32_t offset;
    uint32_t align;
    uint32_t reloff;
    uint32_t nreloc;
    uint32_t flags;
    uint32_t reserved1;
    uint32_t reserved2;
    uint32_t reserved3;
} Section64;

// Symbol table command
typedef struct {
    uint32_t cmd;
    uint32_t cmdsize;
    uint32_t symoff;
    uint32_t nsyms;
    uint32_t stroff;
    uint32_t strsize;
} SymtabCommand;

// Nlist (symbol entry)
typedef struct {
    uint32_t n_strx;
    uint8_t n_type;
    uint8_t n_sect;
    uint16_t n_desc;
    uint64_t n_value;
} Nlist64;

// Mach-O file structure
typedef struct {
    MachHeader64 header;
    SegmentCommand64* segment;
    Section64* text_section;
    Section64* data_section;
    SymtabCommand* symtab;
    uint8_t* code;
    size_t code_size;
    uint8_t* data;
    size_t data_size;
    int num_sections;
} MachOFile;

// Functions
MachOFile* init_macho_file();
void free_macho_file(MachOFile* m);
void macho_set_code(MachOFile* m, uint8_t* code, size_t size);
void macho_set_data(MachOFile* m, uint8_t* data, size_t size);
int write_macho_file(MachOFile* m, const char* filename);
int compile_to_macho(MachineCode* mc, const char* filename);

#endif