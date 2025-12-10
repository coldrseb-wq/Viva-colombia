// src/macho.c - macOS Mach-O object file generation
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
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

// === STANDALONE MACHO EXECUTABLE (no libc needed) ===
// Uses LC_UNIXTHREAD to set entry point directly - no dyld needed

#define LC_UNIXTHREAD   0x5
#define x86_THREAD_STATE64      4
#define x86_THREAD_STATE64_COUNT 42

// Thread state for x86_64 (only need RIP for entry)
typedef struct {
    uint64_t rax, rbx, rcx, rdx;
    uint64_t rdi, rsi, rbp, rsp;
    uint64_t r8, r9, r10, r11;
    uint64_t r12, r13, r14, r15;
    uint64_t rip, rflags;
    uint64_t cs, fs, gs;
} x86_thread_state64_t;

typedef struct {
    uint32_t cmd;
    uint32_t cmdsize;
    uint32_t flavor;
    uint32_t count;
    x86_thread_state64_t state;
} UnixThreadCommand;

int write_standalone_macho_executable(const char* filename, MachineCode* code,
                                      uint8_t* data, size_t data_size) {
    if (!filename || !code) return -1;

    FILE* f = fopen(filename, "wb");
    if (!f) return -1;

    // macOS x86_64 executable layout:
    // [Mach-O Header]
    // [LC_SEGMENT_64 __PAGEZERO]
    // [LC_SEGMENT_64 __TEXT with __text section]
    // [LC_SEGMENT_64 __DATA with __data section] (if data exists)
    // [LC_UNIXTHREAD]
    // [padding to page boundary]
    // [__text section data]
    // [__data section data]

    uint64_t page_size = 0x1000;
    uint64_t vm_base = 0x100000000;  // Standard macOS 64-bit base address

    // Calculate header sizes
    uint32_t header_size = sizeof(MachHeader64);

    // __PAGEZERO segment (no sections)
    uint32_t pagezero_cmd_size = sizeof(SegmentCommand64);

    // __TEXT segment + 1 section
    uint32_t text_cmd_size = sizeof(SegmentCommand64) + sizeof(Section64);

    // __DATA segment + 1 section (only if we have data)
    uint32_t data_cmd_size = (data && data_size > 0) ?
                             sizeof(SegmentCommand64) + sizeof(Section64) : 0;

    // LC_UNIXTHREAD
    uint32_t thread_cmd_size = sizeof(UnixThreadCommand);

    uint32_t ncmds = 3;  // PAGEZERO + TEXT + UNIXTHREAD
    if (data && data_size > 0) ncmds++;  // + DATA

    uint32_t total_cmd_size = pagezero_cmd_size + text_cmd_size + data_cmd_size + thread_cmd_size;
    uint32_t header_total = header_size + total_cmd_size;

    // Align code start to page boundary
    uint64_t code_file_offset = (header_total + page_size - 1) & ~(page_size - 1);
    uint64_t code_vm_addr = vm_base + code_file_offset;

    // Data follows code
    uint64_t data_file_offset = code_file_offset + code->size;
    data_file_offset = (data_file_offset + 15) & ~15;  // 16-byte align
    uint64_t data_vm_addr = vm_base + data_file_offset;

    // Total file size and segment size
    (void)(data_file_offset + (data ? data_size : 0));  // total_file_size - calculated but not needed
    uint64_t text_segment_size = (data_file_offset - code_file_offset + page_size - 1) & ~(page_size - 1);

    // Apply data relocations (RIP-relative addressing)
    for (size_t i = 0; i < code->data_reloc_count; i++) {
        DataReloc* dr = &code->data_relocs[i];
        uint64_t rip_after = code_vm_addr + dr->code_offset + 4;
        uint64_t target = data_vm_addr + dr->data_offset;
        int32_t offset = (int32_t)(target - rip_after);
        memcpy(code->code + dr->code_offset, &offset, 4);
    }

    // Build Mach-O header
    MachHeader64 mh = {0};
    mh.magic = MH_MAGIC_64;
    mh.cputype = CPU_TYPE_X86_64;
    mh.cpusubtype = CPU_SUBTYPE_X86_64;
    mh.filetype = MH_EXECUTE;
    mh.ncmds = ncmds;
    mh.sizeofcmds = total_cmd_size;
    mh.flags = 0;
    mh.reserved = 0;

    // __PAGEZERO segment (null pointer trap)
    SegmentCommand64 pagezero = {0};
    pagezero.cmd = LC_SEGMENT_64;
    pagezero.cmdsize = pagezero_cmd_size;
    strncpy(pagezero.segname, "__PAGEZERO", 16);
    pagezero.vmaddr = 0;
    pagezero.vmsize = vm_base;  // All memory below base is __PAGEZERO
    pagezero.fileoff = 0;
    pagezero.filesize = 0;
    pagezero.maxprot = 0;
    pagezero.initprot = 0;
    pagezero.nsects = 0;
    pagezero.flags = 0;

    // __TEXT segment
    SegmentCommand64 text_seg = {0};
    text_seg.cmd = LC_SEGMENT_64;
    text_seg.cmdsize = text_cmd_size;
    strncpy(text_seg.segname, "__TEXT", 16);
    text_seg.vmaddr = vm_base;
    text_seg.vmsize = code_file_offset + text_segment_size;
    text_seg.fileoff = 0;
    text_seg.filesize = code_file_offset + code->size;
    text_seg.maxprot = 7;   // rwx
    text_seg.initprot = 5;  // r-x
    text_seg.nsects = 1;
    text_seg.flags = 0;

    // __text section
    Section64 text_sect = {0};
    strncpy(text_sect.sectname, "__text", 16);
    strncpy(text_sect.segname, "__TEXT", 16);
    text_sect.addr = code_vm_addr;
    text_sect.size = code->size;
    text_sect.offset = code_file_offset;
    text_sect.align = 4;  // 2^4 = 16 byte alignment
    text_sect.reloff = 0;
    text_sect.nreloc = 0;
    text_sect.flags = S_ATTR_PURE_INSTRUCTIONS | S_ATTR_SOME_INSTRUCTIONS;

    // __DATA segment (if needed)
    SegmentCommand64 data_seg = {0};
    Section64 data_sect = {0};
    if (data && data_size > 0) {
        data_seg.cmd = LC_SEGMENT_64;
        data_seg.cmdsize = data_cmd_size;
        strncpy(data_seg.segname, "__DATA", 16);
        data_seg.vmaddr = data_vm_addr;
        data_seg.vmsize = (data_size + page_size - 1) & ~(page_size - 1);
        data_seg.fileoff = data_file_offset;
        data_seg.filesize = data_size;
        data_seg.maxprot = 7;   // rwx
        data_seg.initprot = 3;  // rw-
        data_seg.nsects = 1;
        data_seg.flags = 0;

        strncpy(data_sect.sectname, "__data", 16);
        strncpy(data_sect.segname, "__DATA", 16);
        data_sect.addr = data_vm_addr;
        data_sect.size = data_size;
        data_sect.offset = data_file_offset;
        data_sect.align = 3;  // 2^3 = 8 byte alignment
        data_sect.reloff = 0;
        data_sect.nreloc = 0;
        data_sect.flags = S_REGULAR;
    }

    // LC_UNIXTHREAD - sets initial thread state (entry point in RIP)
    UnixThreadCommand thread = {0};
    thread.cmd = LC_UNIXTHREAD;
    thread.cmdsize = thread_cmd_size;
    thread.flavor = x86_THREAD_STATE64;
    thread.count = x86_THREAD_STATE64_COUNT;
    memset(&thread.state, 0, sizeof(thread.state));
    thread.state.rip = code_vm_addr;  // Entry point

    // Write header
    fwrite(&mh, sizeof(mh), 1, f);

    // Write load commands
    fwrite(&pagezero, sizeof(pagezero), 1, f);
    fwrite(&text_seg, sizeof(text_seg), 1, f);
    fwrite(&text_sect, sizeof(text_sect), 1, f);

    if (data && data_size > 0) {
        fwrite(&data_seg, sizeof(data_seg), 1, f);
        fwrite(&data_sect, sizeof(data_sect), 1, f);
    }

    fwrite(&thread, sizeof(thread), 1, f);

    // Pad to code offset
    long current = ftell(f);
    while (current < (long)code_file_offset) {
        fputc(0, f);
        current++;
    }

    // Write code
    fwrite(code->code, code->size, 1, f);

    // Pad to data offset
    if (data && data_size > 0) {
        current = ftell(f);
        while (current < (long)data_file_offset) {
            fputc(0, f);
            current++;
        }
        fwrite(data, data_size, 1, f);
    }

    fclose(f);

    // Make executable (chmod +x)
    #ifndef _WIN32
    chmod(filename, 0755);
    #endif

    return 0;
}