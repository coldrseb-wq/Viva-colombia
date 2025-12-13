// src/compiler.c - Viva Colombia Bootstrap Compiler
// Supports raw syscalls for standalone executables (no libc)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include "../include/lexer.h"
#include "../include/parser.h"
#include "../include/compiler.h"
#include "../include/machine_code.h"

#define MAX_VARS 256
#define MAX_STRS 256
#define MAX_BUF 65536
#define MAX_FUNCS 256
#define MAX_PARAMS 6
#define MAX_GLOBALS 128
#define MAX_CALL_PATCHES 1024

typedef enum { OUT_C, OUT_ASM, OUT_ELF, OUT_STANDALONE } OutMode;

typedef struct {
    char name[64];
    int is_str;
    int offset;
    VivaType type;
    VivaType elem_type;  // Element type for arrays
    int size;
} Var;

typedef struct {
    char val[512];
    char lbl[32];
    int offset;
} Str;

typedef struct {
    char name[64];
    int offset;         // Code offset
    int param_count;
    char params[MAX_PARAMS][64];
    VivaType param_types[MAX_PARAMS];
} Func;

typedef struct {
    char name[64];
    int data_offset;    // Offset in data section
    VivaType type;
    VivaType elem_type;  // Element type for arrays
    int size;
    int64_t init_value; // Initial value (for constant initializers)
} GlobalVar;

typedef struct {
    char func_name[64];  // Name of function being called
    int patch_offset;    // Offset in code where rel32 needs to be patched
} CallPatch;

typedef struct {
    FILE* f;
    OutMode mode;
    PlatformTarget plat;
    int indent;
    Var vars[MAX_VARS];
    int nvar;
    Str strs[MAX_STRS];
    int nstr;
    Func funcs[MAX_FUNCS];
    int nfunc;
    char buf[MAX_BUF];
    int bufpos;
    char outname[256];
    MachineCode* mc;
    ELFFile* elf;
    int stack_off;
    int data_size;
    int use_syscalls;   // 1 = raw syscalls, 0 = libc
    GlobalVar globals[MAX_GLOBALS];
    int nglobal;
    int in_function;    // 1 = inside function, 0 = global scope
} Compiler;

// Forward declarations
static void compile_node(Compiler* c, ASTNode* n);
static void compile_expr(Compiler* c, ASTNode* n);
static void compile_call(Compiler* c, ASTNode* n);

// === EMIT HELPERS ===
static void emit(Compiler* c, const char* fmt, ...) {
    va_list a;
    va_start(a, fmt);
    if (c->mode == OUT_ASM) {
        int r = MAX_BUF - c->bufpos;
        int n = vsnprintf(c->buf + c->bufpos, r, fmt, a);
        if (n > 0 && n < r) c->bufpos += n;
    } else if (c->mode == OUT_C) {
        vfprintf(c->f, fmt, a);
    }
    va_end(a);
}

static void ind(Compiler* c) {
    for (int i = 0; i < c->indent; i++) emit(c, "    ");
}

// === VARIABLE MANAGEMENT ===
static int find_var(Compiler* c, const char* name) {
    for (int i = 0; i < c->nvar; i++)
        if (strcmp(c->vars[i].name, name) == 0) return i;
    return -1;
}

static int add_var(Compiler* c, const char* name, int is_str, VivaType type, VivaType elem_type, int size) {
    if (c->nvar >= MAX_VARS) return -1;
    strncpy(c->vars[c->nvar].name, name, 63);
    c->vars[c->nvar].is_str = is_str;
    c->vars[c->nvar].type = type;
    c->vars[c->nvar].elem_type = elem_type;
    c->vars[c->nvar].size = size;
    c->stack_off -= size;
    c->vars[c->nvar].offset = c->stack_off;
    return c->nvar++;
}

static int get_var_off(Compiler* c, const char* name) {
    int i = find_var(c, name);
    return (i >= 0) ? c->vars[i].offset : -8;
}

static int is_var_str(Compiler* c, const char* name) {
    int i = find_var(c, name);
    return (i >= 0) ? c->vars[i].is_str : 0;
}

static VivaType get_var_type(Compiler* c, const char* name) {
    int i = find_var(c, name);
    return (i >= 0) ? c->vars[i].type : VIVA_TYPE_ENTERO;
}

static VivaType get_var_elem_type(Compiler* c, const char* name) {
    int i = find_var(c, name);
    return (i >= 0) ? c->vars[i].elem_type : VIVA_TYPE_ENTERO;
}

// === GLOBAL VARIABLE MANAGEMENT ===
static int find_global(Compiler* c, const char* name) {
    for (int i = 0; i < c->nglobal; i++)
        if (strcmp(c->globals[i].name, name) == 0) return i;
    return -1;
}

static int add_global(Compiler* c, const char* name, VivaType type, VivaType elem_type, int size, int64_t init_val) {
    if (c->nglobal >= MAX_GLOBALS) return -1;
    strncpy(c->globals[c->nglobal].name, name, 63);
    c->globals[c->nglobal].type = type;
    c->globals[c->nglobal].elem_type = elem_type;
    c->globals[c->nglobal].size = size;
    c->globals[c->nglobal].data_offset = c->data_size;
    c->globals[c->nglobal].init_value = init_val;
    c->data_size += size;
    // Align to 8 bytes for next variable
    if (c->data_size % 8 != 0) c->data_size += 8 - (c->data_size % 8);
    return c->nglobal++;
}

static int get_global_offset(Compiler* c, const char* name) {
    int i = find_global(c, name);
    return (i >= 0) ? c->globals[i].data_offset : -1;
}

static VivaType get_global_type(Compiler* c, const char* name) {
    int i = find_global(c, name);
    return (i >= 0) ? c->globals[i].type : VIVA_TYPE_ENTERO;
}

static VivaType get_global_elem_type(Compiler* c, const char* name) {
    int i = find_global(c, name);
    return (i >= 0) ? c->globals[i].elem_type : VIVA_TYPE_ENTERO;
}

static int is_global_var(Compiler* c, const char* name) {
    return find_global(c, name) >= 0;
}

// === FUNCTION MANAGEMENT ===
static int find_func(Compiler* c, const char* name) {
    for (int i = 0; i < c->nfunc; i++)
        if (strcmp(c->funcs[i].name, name) == 0) return i;
    return -1;
}

static int add_func(Compiler* c, const char* name, int offset, int param_count) {
    // Check if function already exists (forward declaration)
    int existing = find_func(c, name);
    if (existing >= 0) {
        // Update offset if it was a placeholder (-1)
        if (c->funcs[existing].offset < 0 && offset >= 0) {
            c->funcs[existing].offset = offset;
        }
        return existing;
    }
    if (c->nfunc >= MAX_FUNCS) return -1;
    strncpy(c->funcs[c->nfunc].name, name, 63);
    c->funcs[c->nfunc].offset = offset;
    c->funcs[c->nfunc].param_count = param_count;
    return c->nfunc++;
}

// === STRING MANAGEMENT ===
static char* add_str(Compiler* c, const char* s) {
    if (c->nstr >= MAX_STRS) return "str_err";
    static int cnt = 0;
    snprintf(c->strs[c->nstr].lbl, 32, "str_%d", cnt++);

    const char* p = (s[0] == '"') ? s + 1 : s;
    strncpy(c->strs[c->nstr].val, p, 511);
    size_t len = strlen(c->strs[c->nstr].val);
    if (len > 0 && c->strs[c->nstr].val[len-1] == '"')
        c->strs[c->nstr].val[len-1] = '\0';

    c->strs[c->nstr].offset = c->data_size;
    c->data_size += strlen(c->strs[c->nstr].val) + 1;

    return c->strs[c->nstr++].lbl;
}

static int get_str_offset(Compiler* c, const char* lbl) {
    for (int i = 0; i < c->nstr; i++)
        if (strcmp(c->strs[i].lbl, lbl) == 0) return c->strs[i].offset;
    return 0;
}

// === COMPILER INIT/FINISH ===
static Compiler* init_compiler_internal(const char* outfile, OutMode mode, int use_syscalls) {
    Compiler* c = calloc(1, sizeof(Compiler));
    if (!c) return NULL;
    c->mode = mode;
    c->plat = PLATFORM_LINUX;
    c->use_syscalls = use_syscalls;
    strncpy(c->outname, outfile, 255);

    if (mode == OUT_ELF || mode == OUT_STANDALONE) {
        c->mc = init_machine_code();
        c->elf = init_elf_file();
    }

    c->f = fopen(outfile, "wb");
    if (!c->f) { free(c); return NULL; }

    if (mode == OUT_C) {
        fprintf(c->f,
            "// Generated by Viva Colombia compiler\n"
            "#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\n#include <stdint.h>\n\n"
            "void prt(char* s){printf(\"%%s\",s);}\n"
            "void prtln(char* s){printf(\"%%s\\n\",s);}\n"
            "void prtnum(int64_t n){printf(\"%%ld\",n);}\n"
            "void prtlnnum(int64_t n){printf(\"%%ld\\n\",n);}\n\n");
    } else if (mode == OUT_ASM) {
        fprintf(c->f,
            "; Generated by Viva Colombia\n"
            "section .data\n"
            "    fmt_s db \"%%s\",10,0\n"
            "    fmt_d db \"%%ld\",10,0\n");
    }
    return c;
}

static void write_elf_data_section(Compiler* c) {
    // Write data section if there are strings OR global variables
    if ((c->mode != OUT_ELF && c->mode != OUT_STANDALONE) ||
        (c->nstr == 0 && c->nglobal == 0)) return;

    uint8_t* data = calloc(1, c->data_size + 1);  // calloc zeros = globals init to 0
    if (!data) return;

    // Write strings at their stored offsets
    for (int i = 0; i < c->nstr; i++) {
        int soff = c->strs[i].offset;
        size_t len = strlen(c->strs[i].val);
        memcpy(data + soff, c->strs[i].val, len);
        data[soff + len] = 0;
    }

    // Write global variable initial values at their stored offsets
    for (int i = 0; i < c->nglobal; i++) {
        int goff = c->globals[i].data_offset;
        int64_t val = c->globals[i].init_value;
        int sz = c->globals[i].size;
        // Write value in little-endian
        for (int b = 0; b < sz && b < 8; b++) {
            data[goff + b] = (val >> (b * 8)) & 0xFF;
        }
    }

    if (c->mode == OUT_STANDALONE) {
        // Write standalone executable
        fclose(c->f);
        c->f = NULL;
        write_standalone_elf(c->elf, c->mc, data, c->data_size, c->outname);
    } else {
        create_data_section(c->elf, data, c->data_size);
    }
    free(data);
}

static void finish_compiler(Compiler* c) {
    if (!c) return;

    if (c->mode == OUT_ASM) {
        // Write strings to data section
        for (int i = 0; i < c->nstr; i++) {
            fprintf(c->f, "    %s db \"%s\",0\n", c->strs[i].lbl, c->strs[i].val);
        }
        fprintf(c->f,
            "section .text\n"
            "    global main\n"
            "    extern printf\n\n");
        if (c->bufpos > 0)
            fwrite(c->buf, 1, c->bufpos, c->f);
    }

    if ((c->mode == OUT_ELF || c->mode == OUT_STANDALONE) && c->elf && c->mc) {
        write_elf_data_section(c);
        if (c->mode == OUT_ELF) {
            create_text_section(c->elf, c->mc);
            create_symbol_table(c->elf);
            fclose(c->f);
            c->f = NULL;
            write_complete_elf_file(c->elf, c->outname);
        } else if (c->mode == OUT_STANDALONE && c->nstr == 0 && c->nglobal == 0) {
            // Write standalone ELF executable without libc (no data section)
            if (c->f) { fclose(c->f); c->f = NULL; }
            write_standalone_elf(c->elf, c->mc, NULL, 0, c->outname);
        }
    }

    if (c->f) fclose(c->f);
    if (c->mc) free_machine_code(c->mc);
    if (c->elf) free_elf_file(c->elf);
    free(c);
}

// === HEX NUMBER PARSING ===
static int64_t parse_hex_num(const char* s) {
    return strtoll(s, NULL, 16);
}

// === EXPRESSION COMPILATION ===
static void compile_expr(Compiler* c, ASTNode* n) {
    if (!n) return;

    switch (n->type) {
        case NUMBER_NODE: {
            int64_t val = atoll(n->value);
            if (c->mode == OUT_C) emit(c, "%s", n->value);
            else if (c->mode == OUT_ASM) emit(c, "    mov rax, %s\n", n->value);
            else if (c->mc) encode_mov_rax_imm64(c->mc, val);
            break;
        }

        case HEX_NUMBER_NODE: {
            int64_t val = parse_hex_num(n->value);
            if (c->mode == OUT_C) emit(c, "%s", n->value);
            else if (c->mode == OUT_ASM) emit(c, "    mov rax, %s\n", n->value);
            else if (c->mc) encode_mov_rax_imm64(c->mc, val);
            break;
        }

        case NULL_LITERAL_NODE: {
            if (c->mode == OUT_C) emit(c, "NULL");
            else if (c->mode == OUT_ASM) emit(c, "    xor rax, rax\n");
            else if (c->mc) encode_xor_rax_rax(c->mc);
            break;
        }

        case STRING_LITERAL_NODE: {
            if (c->mode == OUT_C) emit(c, "%s", n->value);
            else if (c->mode == OUT_ASM) {
                char* l = add_str(c, n->value);
                emit(c, "    lea rax, [%s]\n", l);
            }
            else if (c->mc) {
                char* l = add_str(c, n->value);
                int str_off = get_str_offset(c, l);
                // For standalone ELF: data section is at fixed 64KB (0x10000) offset from code
                // RIP-relative offset = 0x10000 + str_off - current_pos - 7
                int cur_pos = c->mc->size;
                int rip_rel = 0x10000 + str_off - cur_pos - 7;
                encode_lea_rax_rip_rel(c->mc, rip_rel);
            }
            break;
        }

        case IDENTIFIER_NODE: {
            if (n->value && n->value[0] == '"') {
                // String literal disguised as identifier
                if (c->mode == OUT_C) emit(c, "%s", n->value);
                else if (c->mode == OUT_ASM) {
                    char* l = add_str(c, n->value);
                    emit(c, "    lea rax, [%s]\n", l);
                }
                else if (c->mc) {
                    char* l = add_str(c, n->value);
                    int str_off = get_str_offset(c, l);
                    int cur_pos = c->mc->size;
                    int rip_rel = 0x10000 + str_off - cur_pos - 7;
                    encode_lea_rax_rip_rel(c->mc, rip_rel);
                }
            } else {
                if (c->mode == OUT_C) emit(c, "%s", n->value);
                else if (c->mc && is_global_var(c, n->value)) {
                    // Global variable: use RIP-relative addressing
                    int goff = get_global_offset(c, n->value);
                    VivaType type = get_global_type(c, n->value);
                    int cur_pos = c->mc->size;
                    // Global vars are after strings in data section
                    // strings_size is tracked by data_size before globals are added
                    int base_off = 0x10000 + goff;
                    if (type == VIVA_TYPE_ARREGLO) {
                        // LEA rax, [rip + offset]
                        int rip_rel = base_off - cur_pos - 7;
                        encode_lea_rax_rip_rel(c->mc, rip_rel);
                    } else {
                        // MOV rax, [rip + offset]
                        int rip_rel = base_off - cur_pos - 7;
                        encode_mov_rax_rip_rel(c->mc, rip_rel);
                    }
                }
                else if (c->mode == OUT_ASM) {
                    int off = get_var_off(c, n->value);
                    VivaType type = get_var_type(c, n->value);
                    if (type == VIVA_TYPE_ARREGLO) {
                        emit(c, "    lea rax, [rbp%+d]\n", off);
                    } else {
                        emit(c, "    mov rax, [rbp%+d]\n", off);
                    }
                }
                else if (c->mc) {
                    int off = get_var_off(c, n->value);
                    VivaType type = get_var_type(c, n->value);
                    if (type == VIVA_TYPE_ARREGLO) {
                        // Arrays: load address, not value
                        encode_lea_rax_rbp_off(c->mc, off);
                    } else if (type == VIVA_TYPE_OCTETO) {
                        encode_movzx_rax_byte_memory(c->mc, off);
                    } else {
                        encode_mov_rax_from_memory(c->mc, off);
                    }
                }
            }
            break;
        }

        case FN_CALL_NODE: {
            // Function call as expression - compile the call, result in rax
            compile_call(c, n);
            break;
        }

        case BINARY_OP_NODE: {
            if (c->mode == OUT_C) {
                emit(c, "(");
                compile_expr(c, n->left);
                emit(c, " %s ", n->value ? n->value : "+");
                compile_expr(c, n->right);
                emit(c, ")");
            }
            else if (c->mode == OUT_ASM || c->mc) {
                compile_expr(c, n->left);
                if (c->mode == OUT_ASM) emit(c, "    push rax\n");
                else encode_push_rax(c->mc);

                compile_expr(c, n->right);

                if (c->mode == OUT_ASM) emit(c, "    mov rbx, rax\n    pop rax\n");
                else {
                    encode_mov_rbx_rax(c->mc);
                    encode_pop_rax(c->mc);
                }

                if (!n->value) break;
                const char* op = n->value;

                // Arithmetic
                if (strcmp(op, "+") == 0) {
                    if (c->mode == OUT_ASM) emit(c, "    add rax, rbx\n");
                    else encode_add_rax_rbx(c->mc);
                }
                else if (strcmp(op, "-") == 0) {
                    if (c->mode == OUT_ASM) emit(c, "    sub rax, rbx\n");
                    else encode_sub_rax_rbx(c->mc);
                }
                else if (strcmp(op, "*") == 0) {
                    if (c->mode == OUT_ASM) emit(c, "    imul rax, rbx\n");
                    else encode_mul_rbx(c->mc);
                }
                else if (strcmp(op, "/") == 0) {
                    if (c->mode == OUT_ASM) emit(c, "    xor rdx,rdx\n    idiv rbx\n");
                    else encode_div_rbx(c->mc);
                }
                else if (strcmp(op, "%") == 0) {
                    if (c->mode == OUT_ASM) emit(c, "    xor rdx,rdx\n    idiv rbx\n    mov rax,rdx\n");
                    else {
                        encode_xor_rdx_rdx(c->mc);
                        uint8_t div_code[] = {0x48, 0xF7, 0xF3};
                        append_bytes(c->mc, div_code, 3);
                        encode_mov_rax_rdx(c->mc);
                    }
                }
                // Bitwise operations
                else if (strcmp(op, "&") == 0) {
                    if (c->mode == OUT_ASM) emit(c, "    and rax, rbx\n");
                    else encode_and_rax_rbx(c->mc);
                }
                else if (strcmp(op, "|") == 0) {
                    if (c->mode == OUT_ASM) emit(c, "    or rax, rbx\n");
                    else encode_or_rax_rbx(c->mc);
                }
                else if (strcmp(op, "^") == 0) {
                    if (c->mode == OUT_ASM) emit(c, "    xor rax, rbx\n");
                    else encode_xor_rax_rbx(c->mc);
                }
                else if (strcmp(op, "<<") == 0) {
                    if (c->mode == OUT_ASM) emit(c, "    mov rcx, rbx\n    shl rax, cl\n");
                    else {
                        encode_mov_rcx_rax(c->mc);  // Save rax
                        encode_mov_rax_rbx(c->mc);  // rbx = shift count
                        uint8_t xchg[] = {0x48, 0x87, 0xC1};  // xchg rax, rcx
                        append_bytes(c->mc, xchg, 3);
                        encode_shl_rax_cl(c->mc);
                    }
                }
                else if (strcmp(op, ">>") == 0) {
                    if (c->mode == OUT_ASM) emit(c, "    mov rcx, rbx\n    shr rax, cl\n");
                    else {
                        encode_mov_rcx_rax(c->mc);
                        encode_mov_rax_rbx(c->mc);
                        uint8_t xchg[] = {0x48, 0x87, 0xC1};
                        append_bytes(c->mc, xchg, 3);
                        encode_shr_rax_cl(c->mc);
                    }
                }
                // Comparisons
                else if (strcmp(op, ">") == 0) {
                    if (c->mode == OUT_ASM) emit(c, "    cmp rax,rbx\n    setg al\n    movzx rax,al\n");
                    else { encode_cmp_rax_rbx(c->mc); encode_setg_al(c->mc); encode_movzx_rax_al(c->mc); }
                }
                else if (strcmp(op, "<") == 0) {
                    if (c->mode == OUT_ASM) emit(c, "    cmp rax,rbx\n    setl al\n    movzx rax,al\n");
                    else { encode_cmp_rax_rbx(c->mc); encode_setl_al(c->mc); encode_movzx_rax_al(c->mc); }
                }
                else if (strcmp(op, ">=") == 0) {
                    if (c->mode == OUT_ASM) emit(c, "    cmp rax,rbx\n    setge al\n    movzx rax,al\n");
                    else { encode_cmp_rax_rbx(c->mc); encode_setge_al(c->mc); encode_movzx_rax_al(c->mc); }
                }
                else if (strcmp(op, "<=") == 0) {
                    if (c->mode == OUT_ASM) emit(c, "    cmp rax,rbx\n    setle al\n    movzx rax,al\n");
                    else { encode_cmp_rax_rbx(c->mc); encode_setle_al(c->mc); encode_movzx_rax_al(c->mc); }
                }
                else if (strcmp(op, "==") == 0) {
                    if (c->mode == OUT_ASM) emit(c, "    cmp rax,rbx\n    sete al\n    movzx rax,al\n");
                    else { encode_cmp_rax_rbx(c->mc); encode_sete_al(c->mc); encode_movzx_rax_al(c->mc); }
                }
                else if (strcmp(op, "!=") == 0) {
                    if (c->mode == OUT_ASM) emit(c, "    cmp rax,rbx\n    setne al\n    movzx rax,al\n");
                    else { encode_cmp_rax_rbx(c->mc); encode_setne_al(c->mc); encode_movzx_rax_al(c->mc); }
                }
                // Logical
                else if (strcmp(op, "&&") == 0 || strcmp(op, "y") == 0) {
                    if (c->mode == OUT_ASM) emit(c, "    test rax,rax\n    setne al\n    test rbx,rbx\n    setne bl\n    and al,bl\n    movzx rax,al\n");
                    else {
                        encode_test_rax_rax(c->mc);
                        encode_setne_al(c->mc);
                        encode_movzx_rax_al(c->mc);
                        encode_push_rax(c->mc);
                        encode_mov_rax_rbx(c->mc);
                        encode_test_rax_rax(c->mc);
                        encode_setne_al(c->mc);
                        encode_movzx_rax_al(c->mc);
                        encode_pop_rbx(c->mc);
                        encode_and_rax_rbx(c->mc);
                    }
                }
                else if (strcmp(op, "||") == 0 || strcmp(op, "o") == 0) {
                    if (c->mode == OUT_ASM) emit(c, "    or rax,rbx\n    test rax,rax\n    setne al\n    movzx rax,al\n");
                    else {
                        encode_or_rax_rbx(c->mc);
                        encode_test_rax_rax(c->mc);
                        encode_setne_al(c->mc);
                        encode_movzx_rax_al(c->mc);
                    }
                }
            }
            break;
        }

        case UNARY_OP_NODE: {
            ASTNode* operand = n->right ? n->right : n->left;
            if (c->mode == OUT_C) {
                if (n->value && (strcmp(n->value,"!")==0 || strcmp(n->value,"no")==0)) emit(c, "!");
                else if (n->value && strcmp(n->value,"-")==0) emit(c, "-");
                else if (n->value && strcmp(n->value,"~")==0) emit(c, "~");
                compile_expr(c, operand);
            }
            else {
                compile_expr(c, operand);
                if (n->value && (strcmp(n->value,"!")==0 || strcmp(n->value,"no")==0)) {
                    if (c->mode == OUT_ASM) emit(c, "    cmp rax,0\n    sete al\n    movzx rax,al\n");
                    else { encode_cmp_rax_zero(c->mc); encode_sete_al(c->mc); encode_movzx_rax_al(c->mc); }
                }
                else if (n->value && strcmp(n->value,"-")==0) {
                    if (c->mode == OUT_ASM) emit(c, "    neg rax\n");
                    else encode_neg_rax(c->mc);
                }
                else if (n->value && strcmp(n->value,"~")==0) {
                    if (c->mode == OUT_ASM) emit(c, "    not rax\n");
                    else encode_not_rax(c->mc);
                }
            }
            break;
        }

        case ADDRESS_OF_NODE: {
            if (n->right && n->right->type == IDENTIFIER_NODE) {
                int off = get_var_off(c, n->right->value);
                if (c->mode == OUT_C) emit(c, "&%s", n->right->value);
                else if (c->mode == OUT_ASM) emit(c, "    lea rax, [rbp%+d]\n", off);
                else if (c->mc) encode_lea_rax_rbp_off(c->mc, off);
            }
            break;
        }

        case DEREFERENCE_NODE: {
            if (c->mode == OUT_C) { emit(c, "*"); compile_expr(c, n->right); }
            else {
                compile_expr(c, n->right);
                if (c->mode == OUT_ASM) emit(c, "    mov rax, [rax]\n");
                else encode_mov_rax_from_rax_ptr(c->mc);
            }
            break;
        }

        case ARRAY_ACCESS_NODE: {
            if (c->mode == OUT_C) {
                compile_expr(c, n->left);
                emit(c, "[");
                compile_expr(c, n->right);
                emit(c, "]");
            }
            else if (n->left && n->left->type == IDENTIFIER_NODE) {
                const char* arr_name = n->left->value;
                int is_global = is_global_var(c, arr_name);
                VivaType elem_type = is_global ? get_global_elem_type(c, arr_name) : get_var_elem_type(c, arr_name);

                // Determine element size based on element type
                int elem_size = (elem_type == VIVA_TYPE_OCTETO) ? 1 : 8;

                // Load index
                compile_expr(c, n->right);

                if (c->mode == OUT_ASM) {
                    if (elem_size > 1) {
                        emit(c, "    imul rax, %d\n", elem_size);
                    }
                    int off = get_var_off(c, arr_name);
                    emit(c, "    lea rbx, [rbp%+d]\n", off);
                    emit(c, "    add rax, rbx\n");
                    if (elem_size == 1) {
                        emit(c, "    movzx rax, byte [rax]\n");
                    } else {
                        emit(c, "    mov rax, [rax]\n");
                    }
                }
                else if (c->mc) {
                    if (elem_size > 1) {
                        encode_mov_rbx_imm32(c->mc, elem_size);
                        encode_mul_rbx(c->mc);
                    }
                    // rax now has index (possibly scaled)
                    encode_push_rax(c->mc);  // Save index

                    if (is_global) {
                        // Global array: load address using RIP-relative
                        int goff = get_global_offset(c, arr_name);
                        int cur_pos = c->mc->size;
                        int rip_rel = 0x10000 + goff - cur_pos - 7;
                        encode_lea_rax_rip_rel(c->mc, rip_rel);
                        encode_mov_rbx_rax(c->mc);  // rbx = &arr[0]
                    } else {
                        // Local array: use RBP-relative
                        int off = get_var_off(c, arr_name);
                        encode_lea_rbx_rbp_off(c->mc, off);
                    }

                    encode_pop_rax(c->mc);  // Restore index
                    encode_add_rax_rbx(c->mc);  // rax = &arr[index]

                    if (elem_size == 1) {
                        // Move address to rbx for byte load
                        encode_mov_rbx_rax(c->mc);
                        encode_movzx_rax_byte_rbx_ptr(c->mc);  // movzx rax, byte [rbx]
                    } else {
                        encode_mov_rax_from_rax_ptr(c->mc);
                    }
                }
            }
            break;
        }

        case SIZEOF_NODE: {
            if (c->mode == OUT_C) { emit(c, "sizeof("); compile_expr(c, n->left); emit(c, ")"); }
            else {
                // Return 8 for most types
                if (c->mode == OUT_ASM) emit(c, "    mov rax, 8\n");
                else encode_mov_rax_imm32(c->mc, 8);
            }
            break;
        }

        case SYSCALL_WRITE_NODE: {
            // sys_write(fd, buf, len) - rax=1, rdi=fd, rsi=buf, rdx=len
            // Uses left=fd, params=buf, extra=len (not right, to avoid statement chaining conflict)
            if (c->mc) {
                compile_expr(c, n->extra);  // len
                encode_push_rax(c->mc);
                compile_expr(c, n->params);  // buf (was n->right)
                encode_push_rax(c->mc);
                compile_expr(c, n->left);   // fd
                encode_mov_rdi_rax(c->mc);
                encode_pop_rax(c->mc);
                encode_mov_rsi_rax(c->mc);
                encode_pop_rax(c->mc);
                encode_mov_rdx_rax(c->mc);
                encode_mov_rax_imm32(c->mc, SYS_WRITE);
                encode_syscall(c->mc);
            }
            break;
        }

        case SYSCALL_READ_NODE: {
            // Uses left=fd, params=buf, extra=len
            if (c->mc) {
                compile_expr(c, n->extra);
                encode_push_rax(c->mc);
                compile_expr(c, n->params);  // buf (was n->right)
                encode_push_rax(c->mc);
                compile_expr(c, n->left);
                encode_mov_rdi_rax(c->mc);
                encode_pop_rax(c->mc);
                encode_mov_rsi_rax(c->mc);
                encode_pop_rax(c->mc);
                encode_mov_rdx_rax(c->mc);
                encode_mov_rax_imm32(c->mc, SYS_READ);
                encode_syscall(c->mc);
            }
            break;
        }

        case SYSCALL_OPEN_NODE: {
            // Uses left=path, params=flags, extra=mode
            if (c->mc) {
                compile_expr(c, n->extra);
                encode_push_rax(c->mc);
                compile_expr(c, n->params);  // flags (was n->right)
                encode_push_rax(c->mc);
                compile_expr(c, n->left);
                encode_mov_rdi_rax(c->mc);
                encode_pop_rax(c->mc);
                encode_mov_rsi_rax(c->mc);
                encode_pop_rax(c->mc);
                encode_mov_rdx_rax(c->mc);
                encode_mov_rax_imm32(c->mc, SYS_OPEN);
                encode_syscall(c->mc);
            }
            break;
        }

        case SYSCALL_CLOSE_NODE: {
            if (c->mc) {
                compile_expr(c, n->left);
                encode_mov_rdi_rax(c->mc);
                encode_mov_rax_imm32(c->mc, SYS_CLOSE);
                encode_syscall(c->mc);
            }
            break;
        }

        case SYSCALL_EXIT_NODE: {
            if (c->mc) {
                compile_expr(c, n->left);
                encode_mov_rdi_rax(c->mc);
                encode_mov_rax_imm32(c->mc, SYS_EXIT);
                encode_syscall(c->mc);
            }
            break;
        }

        default:
            break;
    }
}

// === FUNCTION CALL ===
static void compile_call(Compiler* c, ASTNode* n) {
    if (!n || !n->value) return;
    const char* fn = n->value;

    if (c->mode == OUT_C) {
        ind(c);
        if (strcmp(fn,"println")==0 || strcmp(fn,"print")==0) {
            int ln = (strcmp(fn,"println")==0);
            if (n->left && n->left->type == NUMBER_NODE)
                emit(c, "prt%snum(%s);\n", ln?"ln":"", n->left->value);
            else if (n->left && (n->left->type == STRING_LITERAL_NODE ||
                     (n->left->type == IDENTIFIER_NODE && n->left->value[0]=='"')))
                emit(c, "prt%s(%s);\n", ln?"ln":"", n->left->value);
            else if (n->left && n->left->type == IDENTIFIER_NODE) {
                if (is_var_str(c, n->left->value))
                    emit(c, "prt%s(%s);\n", ln?"ln":"", n->left->value);
                else
                    emit(c, "prt%snum(%s);\n", ln?"ln":"", n->left->value);
            }
            else if (n->left) {
                emit(c, "prt%snum(", ln?"ln":"");
                compile_expr(c, n->left);
                emit(c, ");\n");
            }
            else emit(c, "prt%s(\"\");\n", ln?"ln":"");
        }
        else {
            emit(c, "%s(", fn);
            ASTNode* arg = n->left;
            while (arg) {
                compile_expr(c, arg);
                arg = arg->right;
                if (arg) emit(c, ", ");
            }
            emit(c, ");\n");
        }
    }
    else if (c->mode == OUT_ASM || c->mc) {
        // Check for user-defined function
        int fi = find_func(c, fn);
        if (fi >= 0) {
            // Compile arguments and push to stack
            ASTNode* arg = n->left;
            int argc = 0;
            while (arg) {
                compile_expr(c, arg);
                if (c->mode == OUT_ASM) emit(c, "    push rax\n");
                else encode_push_rax(c->mc);
                argc++;
                arg = arg->right;
            }
            // Pop to parameter registers (reverse order)
            for (int i = argc - 1; i >= 0; i--) {
                if (c->mode == OUT_ASM) {
                    const char* regs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
                    if (i < 6) emit(c, "    pop %s\n", regs[i]);
                    else emit(c, "    pop rax\n");  // Stack args
                } else if (c->mc) {
                    switch (i) {
                        case 0: encode_pop_rdi(c->mc); break;
                        case 1: encode_pop_rsi(c->mc); break;
                        case 2: encode_pop_rdx(c->mc); break;
                        case 3: encode_pop_rcx(c->mc); break;
                        case 4: encode_pop_r8(c->mc); break;
                        case 5: encode_pop_r9(c->mc); break;
                        default: encode_pop_rax(c->mc); break;
                    }
                }
            }
            // Call function
            if (c->mode == OUT_ASM) {
                emit(c, "    call %s\n", fn);
            } else if (c->mc) {
                int current = get_current_offset(c->mc);
                int target = c->funcs[fi].offset;
                int32_t rel = target - (current + 5);
                encode_call_rel32(c->mc, rel);
            }
        }
        else if (strcmp(fn,"println")==0 || strcmp(fn,"print")==0) {
            // Built-in print using syscall
            if (c->use_syscalls && c->mc) {
                if (n->left) {
                    compile_expr(c, n->left);
                    encode_mov_rsi_rax(c->mc);  // buf
                    encode_mov_rdx_imm32(c->mc, 64);  // len (approx)
                    encode_mov_rdi_imm32(c->mc, 1);   // stdout
                    encode_mov_rax_imm32(c->mc, SYS_WRITE);
                    encode_syscall(c->mc);
                }
            } else {
                // Use printf
                if (n->left) {
                    compile_expr(c, n->left);
                    if (c->mode == OUT_ASM) emit(c, "    mov rsi, rax\n    mov rdi, fmt_d\n    xor rax,rax\n    call printf\n");
                    else if (c->mc) {
                        encode_mov_rsi_rax(c->mc);
                        encode_mov_rdi_imm64(c->mc, 0);
                        encode_xor_rax_rax(c->mc);
                        add_relocation_entry(c->mc, 1, R_X86_64_PLT32, -4);
                        encode_call_rel32(c->mc, 0);
                    }
                }
            }
        }
    }
}

// === VARIABLE DECLARATION ===
static void compile_var(Compiler* c, ASTNode* n) {
    if (!n || !n->value) return;
    int is_str = 0;
    VivaType type = VIVA_TYPE_ENTERO;
    int size = 8;

    // Check type annotation
    VivaType elem_type = VIVA_TYPE_ENTERO;  // Default element type

    // Check type annotation
    if (n->extra && n->extra->type_info) {
        TypeDesc* td = n->extra->type_info;
        type = td->base_type;
        if (type == VIVA_TYPE_OCTETO) {
            size = 1;
        } else if (type == VIVA_TYPE_ARREGLO) {
            // Calculate array size: array_size * element_size
            int elem_size = 8;  // Default to 8 bytes
            if (td->element_type) {
                elem_type = td->element_type->base_type;
                if (elem_type == VIVA_TYPE_OCTETO) elem_size = 1;
            }
            size = (td->array_size > 0) ? td->array_size * elem_size : 8;
        }
    }

    // Global variable: register in data section, no code emission
    if (!c->in_function && c->mc) {
        // Extract constant initializer if present
        int64_t init_val = 0;
        if (n->left && n->left->type == NUMBER_NODE && n->left->value) {
            init_val = atoll(n->left->value);
        }
        add_global(c, n->value, type, elem_type, size, init_val);
        return;
    }

    if (c->mode == OUT_C) {
        ind(c);
        if (n->left && (n->left->type == STRING_LITERAL_NODE ||
            (n->left->type == IDENTIFIER_NODE && n->left->value[0]=='"'))) {
            emit(c, "char* %s = %s;\n", n->value, n->left->value);
            is_str = 1;
        } else if (n->left) {
            emit(c, "int64_t %s = ", n->value);
            compile_expr(c, n->left);
            emit(c, ";\n");
        } else {
            emit(c, "int64_t %s = 0;\n", n->value);
        }
    }
    else {
        int off = c->stack_off - size;
        if (n->left) {
            if (n->left->type == STRING_LITERAL_NODE ||
                (n->left->type == IDENTIFIER_NODE && n->left->value[0]=='"')) {
                char* l = add_str(c, n->left->value);
                if (c->mode == OUT_ASM) emit(c, "    lea rax, [%s]\n", l);
                else if (c->mc) {
                    int str_off = get_str_offset(c, l);
                    int cur_pos = c->mc->size;
                    int rip_rel = 0x10000 + str_off - cur_pos - 7;
                    encode_lea_rax_rip_rel(c->mc, rip_rel);
                }
                is_str = 1;
            } else {
                compile_expr(c, n->left);
            }
            if (c->mode == OUT_ASM) emit(c, "    mov [rbp%+d], rax\n", off);
            else if (c->mc) {
                if (type == VIVA_TYPE_OCTETO) encode_mov_memory_from_al(c->mc, off);
                else encode_mov_memory_from_rax(c->mc, off);
            }
        } else {
            if (c->mode == OUT_ASM) emit(c, "    mov qword [rbp%+d], 0\n", off);
            else if (c->mc) {
                encode_mov_rax_imm32(c->mc, 0);
                encode_mov_memory_from_rax(c->mc, off);
            }
        }
    }
    add_var(c, n->value, is_str, type, elem_type, size);
}

// === ASSIGNMENT ===
static void compile_assign(Compiler* c, ASTNode* n) {
    if (!n || !n->left) return;

    // Complex LHS (array element, field access): stored in n->extra
    if (n->extra) {
        if (n->extra->type == ARRAY_ACCESS_NODE) {
            // Array element assignment: arr[idx] = val
            ASTNode* arr = n->extra->left;
            ASTNode* idx = n->extra->right;

            if (arr && arr->type == IDENTIFIER_NODE) {
                const char* arr_name = arr->value;
                int is_global = is_global_var(c, arr_name);
                VivaType elem_type = is_global ? get_global_elem_type(c, arr_name) : get_var_elem_type(c, arr_name);

                // Determine element size based on element type
                int elem_size = (elem_type == VIVA_TYPE_OCTETO) ? 1 : 8;

                if (c->mode == OUT_C) {
                    ind(c);
                    compile_expr(c, arr);
                    emit(c, "[");
                    compile_expr(c, idx);
                    emit(c, "] = ");
                    compile_expr(c, n->left);
                    emit(c, ";\n");
                }
                else if (c->mc) {
                    // Compute address: base + index * elem_size
                    compile_expr(c, idx);               // rax = index
                    if (elem_size > 1) {
                        encode_mov_rbx_imm32(c->mc, elem_size);
                        encode_mul_rbx(c->mc);          // rax *= elem_size
                    }

                    // Get base address (global or local)
                    if (is_global) {
                        encode_push_rax(c->mc);         // save index*size
                        int goff = get_global_offset(c, arr_name);
                        int cur_pos = c->mc->size;
                        int rip_rel = 0x10000 + goff - cur_pos - 7;
                        encode_lea_rax_rip_rel(c->mc, rip_rel);  // rax = &arr[0]
                        encode_mov_rbx_rax(c->mc);               // rbx = &arr[0]
                        encode_pop_rax(c->mc);                   // restore index*size
                    } else {
                        int off = get_var_off(c, arr_name);
                        encode_lea_rbx_rbp_off(c->mc, off);      // rbx = &arr[0]
                    }

                    encode_add_rax_rbx(c->mc);          // rax = &arr[index]
                    encode_push_rax(c->mc);             // save address

                    compile_expr(c, n->left);           // rax = value
                    encode_pop_rbx(c->mc);              // rbx = address

                    if (elem_size == 1) {
                        encode_mov_rbx_ptr_from_al(c->mc);  // [rbx] = al
                    } else {
                        encode_mov_rbx_ptr_from_rax(c->mc); // [rbx] = rax
                    }
                }
            }
            return;
        }
        // TODO: Handle field access assignments (FIELD_ACCESS_NODE, ARROW_ACCESS_NODE)
        return;
    }

    // Simple variable assignment
    if (!n->value) return;

    if (c->mode == OUT_C) {
        ind(c);
        emit(c, "%s = ", n->value);
        compile_expr(c, n->left);
        emit(c, ";\n");
    }
    else if (c->mc && is_global_var(c, n->value)) {
        // Global variable assignment: use RIP-relative addressing
        compile_expr(c, n->left);  // value in rax
        int goff = get_global_offset(c, n->value);
        int cur_pos = c->mc->size;
        int base_off = 0x10000 + goff;
        int rip_rel = base_off - cur_pos - 7;
        encode_mov_rip_rel_from_rax(c->mc, rip_rel);
    }
    else {
        int off = get_var_off(c, n->value);
        VivaType type = get_var_type(c, n->value);
        compile_expr(c, n->left);
        if (c->mode == OUT_ASM) emit(c, "    mov [rbp%+d], rax\n", off);
        else if (c->mc) {
            if (type == VIVA_TYPE_OCTETO) encode_mov_memory_from_al(c->mc, off);
            else encode_mov_memory_from_rax(c->mc, off);
        }
    }
}

// === IF STATEMENT ===
// Structure: left=condition, extra=then_body, params=else_body
static void compile_if(Compiler* c, ASTNode* n) {
    static int lbl = 0;
    int id = lbl++;

    if (c->mode == OUT_C) {
        ind(c); emit(c, "if ("); compile_expr(c, n->left); emit(c, ") {\n");
        c->indent++;
        compile_node(c, n->extra);  // then_body
        c->indent--;
        ind(c); emit(c, "}");
        if (n->params) {  // else_body
            emit(c, " else {\n");
            c->indent++;
            compile_node(c, n->params);
            c->indent--;
            ind(c); emit(c, "}");
        }
        emit(c, "\n");
    }
    else if (c->mode == OUT_ASM) {
        compile_expr(c, n->left);
        emit(c, "    cmp rax, 0\n    je .Lelse%d\n", id);
        compile_node(c, n->extra);  // then_body
        emit(c, "    jmp .Lend%d\n.Lelse%d:\n", id, id);
        if (n->params) compile_node(c, n->params);  // else_body
        emit(c, ".Lend%d:\n", id);
    }
    else if (c->mc) {
        compile_expr(c, n->left);
        encode_cmp_rax_zero(c->mc);
        int je_pos = get_current_offset(c->mc);
        encode_je_rel32(c->mc, 0);
        compile_node(c, n->extra);  // then_body
        int jmp_pos = get_current_offset(c->mc);
        encode_jmp_rel32(c->mc, 0);
        int else_pos = get_current_offset(c->mc);
        patch_jump_offset(c->mc, je_pos + 2, else_pos);
        if (n->params) compile_node(c, n->params);  // else_body
        int end_pos = get_current_offset(c->mc);
        patch_jump_offset(c->mc, jmp_pos + 1, end_pos);
    }
}

// === WHILE LOOP ===
// Structure: left=condition, extra=body
static void compile_while(Compiler* c, ASTNode* n) {
    static int lbl = 0;
    int id = lbl++;

    if (c->mode == OUT_C) {
        ind(c); emit(c, "while ("); compile_expr(c, n->left); emit(c, ") {\n");
        c->indent++;
        compile_node(c, n->extra);  // body
        c->indent--;
        ind(c); emit(c, "}\n");
    }
    else if (c->mode == OUT_ASM) {
        emit(c, ".Lw%d:\n", id);
        compile_expr(c, n->left);
        emit(c, "    cmp rax,0\n    je .Lwe%d\n", id);
        compile_node(c, n->extra);  // body
        emit(c, "    jmp .Lw%d\n.Lwe%d:\n", id, id);
    }
    else if (c->mc) {
        int start = get_current_offset(c->mc);
        compile_expr(c, n->left);
        encode_cmp_rax_zero(c->mc);
        int je_pos = get_current_offset(c->mc);
        encode_je_rel32(c->mc, 0);
        compile_node(c, n->extra);  // body
        int jmp_pos = get_current_offset(c->mc);
        encode_jmp_rel32(c->mc, 0);
        patch_jump_offset(c->mc, jmp_pos + 1, start);
        int end = get_current_offset(c->mc);
        patch_jump_offset(c->mc, je_pos + 2, end);
    }
}

// === FOR LOOP ===
static void compile_for(Compiler* c, ASTNode* n) {
    static int lbl = 0;
    int id = lbl++;

    if (c->mode == OUT_C) {
        ind(c); emit(c, "for (");
        if (n->left) compile_expr(c, n->left);
        emit(c, "; ");
        if (n->params) compile_expr(c, n->params);
        emit(c, "; ");
        if (n->extra) compile_expr(c, n->extra);
        emit(c, ") {\n");
        c->indent++;
        compile_node(c, n->right);
        c->indent--;
        ind(c); emit(c, "}\n");
    }
    else if (c->mode == OUT_ASM) {
        if (n->left) compile_node(c, n->left);
        emit(c, ".Lf%d:\n", id);
        if (n->params) {
            compile_expr(c, n->params);
            emit(c, "    cmp rax,0\n    je .Lfe%d\n", id);
        }
        compile_node(c, n->right);
        if (n->extra) compile_expr(c, n->extra);
        emit(c, "    jmp .Lf%d\n.Lfe%d:\n", id, id);
    }
    else if (c->mc) {
        if (n->left) compile_node(c, n->left);
        int start = get_current_offset(c->mc);
        int je_pos = -1;
        if (n->params) {
            compile_expr(c, n->params);
            encode_cmp_rax_zero(c->mc);
            je_pos = get_current_offset(c->mc);
            encode_je_rel32(c->mc, 0);
        }
        compile_node(c, n->right);
        if (n->extra) compile_expr(c, n->extra);
        int jmp_pos = get_current_offset(c->mc);
        encode_jmp_rel32(c->mc, 0);
        patch_jump_offset(c->mc, jmp_pos + 1, start);
        if (je_pos >= 0) {
            int end = get_current_offset(c->mc);
            patch_jump_offset(c->mc, je_pos + 2, end);
        }
    }
}

// === RETURN ===
static void compile_return(Compiler* c, ASTNode* n) {
    if (c->mode == OUT_C) {
        ind(c); emit(c, "return ");
        if (n->left) compile_expr(c, n->left);
        else emit(c, "0");
        emit(c, ";\n");
    }
    else {
        if (n->left) compile_expr(c, n->left);
        else { if (c->mode == OUT_ASM) emit(c, "    xor rax,rax\n"); else encode_xor_rax_rax(c->mc); }
        if (c->mode == OUT_ASM) emit(c, "    leave\n    ret\n");
        else if (c->mc) { encode_leave(c->mc); encode_ret(c->mc); }
    }
}

// === FUNCTION DECLARATION ===
static void compile_func(Compiler* c, ASTNode* n) {
    const char* name = n->value ? n->value : "func";

    // Save old vars and mark as inside function
    Var old_vars[MAX_VARS];
    int old_nvar = c->nvar;
    int old_stack = c->stack_off;
    int old_in_func = c->in_function;
    memcpy(old_vars, c->vars, sizeof(old_vars));
    c->nvar = 0;
    c->stack_off = 0;
    c->in_function = 1;

    // Register function
    int func_offset = c->mc ? get_current_offset(c->mc) : 0;
    int param_count = 0;
    ASTNode* p = n->params;
    while (p) { param_count++; p = p->right; }
    add_func(c, name, func_offset, param_count);

    if (c->mode == OUT_C) {
        emit(c, "int64_t %s(", name);
        p = n->params;
        int i = 0;
        while (p) {
            if (i > 0) emit(c, ", ");
            emit(c, "int64_t %s", p->value);
            p = p->right;
            i++;
        }
        emit(c, ") {\n");
        c->indent++;
        compile_node(c, n->left);
        c->indent--;
        emit(c, "    return 0;\n}\n\n");
    }
    else if (c->mode == OUT_ASM) {
        emit(c, "%s:\n    push rbp\n    mov rbp,rsp\n    sub rsp,32768\n", name);
        // Copy params from registers to stack
        const char* regs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
        p = n->params;
        int i = 0;
        while (p && i < 6) {
            add_var(c, p->value, 0, VIVA_TYPE_ENTERO, VIVA_TYPE_ENTERO, 8);
            int off = get_var_off(c, p->value);
            emit(c, "    mov [rbp%+d], %s\n", off, regs[i]);
            p = p->right;
            i++;
        }
        compile_node(c, n->left);
        emit(c, "    xor rax,rax\n    leave\n    ret\n\n");
    }
    else if (c->mc) {
        encode_push_rbp(c->mc);
        encode_mov_rbp_rsp(c->mc);
        int stack_reserve_pos = get_current_offset(c->mc);
        encode_sub_rsp_imm32(c->mc, 32768);

        // Copy params from registers to stack (System V ABI)
        p = n->params;
        int i = 0;
        while (p && i < 6) {
            VivaType ptype = VIVA_TYPE_ENTERO;
            if (p->left && p->left->type_info) ptype = p->left->type_info->base_type;
            add_var(c, p->value, 0, ptype, VIVA_TYPE_ENTERO, 8);
            int off = get_var_off(c, p->value);

            switch (i) {
                case 0: encode_mov_rax_rdi(c->mc); break;
                case 1: encode_mov_rax_rsi(c->mc); break;
                case 2: encode_mov_rax_rdx(c->mc); break;
                case 3: encode_mov_rax_rcx(c->mc); break;
                case 4: encode_mov_rax_r8(c->mc); break;
                case 5: encode_mov_rax_r9(c->mc); break;
            }
            encode_mov_memory_from_rax(c->mc, off);
            p = p->right;
            i++;
        }

        compile_node(c, n->left);

        // Patch stack size
        int actual = (-c->stack_off + 15) & ~15;
        if (actual < 32768) actual = 32768;
        memcpy(c->mc->code + stack_reserve_pos + 3, &actual, 4);

        encode_xor_rax_rax(c->mc);
        encode_leave(c->mc);
        encode_ret(c->mc);
    }

    // Restore old vars and in_function flag
    memcpy(c->vars, old_vars, sizeof(old_vars));
    c->nvar = old_nvar;
    c->stack_off = old_stack;
    c->in_function = old_in_func;
}

// === NODE DISPATCHER ===
static void compile_node(Compiler* c, ASTNode* n) {
    while (n) {
        switch (n->type) {
            case PROGRAM_NODE: compile_node(c, n->left); break;
            case FN_CALL_NODE: compile_call(c, n); break;
            case FN_DECL_NODE: case FN_DECL_SPANISH_NODE: compile_func(c, n); break;
            case VAR_DECL_NODE: case VAR_DECL_SPANISH_NODE: compile_var(c, n); break;
            case ASSIGN_NODE: compile_assign(c, n); break;
            case IF_NODE: case IF_SPANISH_NODE: compile_if(c, n); break;
            case WHILE_NODE: case WHILE_SPANISH_NODE: compile_while(c, n); break;
            case FOR_NODE: case FOR_SPANISH_NODE: compile_for(c, n); break;
            case RETURN_NODE: compile_return(c, n); break;
            case BREAK_NODE:
                if (c->mode == OUT_C) { ind(c); emit(c, "break;\n"); }
                break;
            case CONTINUE_NODE:
                if (c->mode == OUT_C) { ind(c); emit(c, "continue;\n"); }
                break;
            case STRUCT_DECL_NODE: break;  // Type declaration, no code gen
            default:
                // Expression statement
                if (n->type == SYSCALL_WRITE_NODE || n->type == SYSCALL_READ_NODE ||
                    n->type == SYSCALL_OPEN_NODE || n->type == SYSCALL_CLOSE_NODE ||
                    n->type == SYSCALL_EXIT_NODE) {
                    compile_expr(c, n);
                }
                break;
        }
        n = n->right;
    }
}

// === MAIN WRAPPER FOR STANDALONE ===
static void compile_standalone(Compiler* c, ASTNode* ast) {
    if (!c->mc) return;

    // First pass: register all global variables (before generating any code)
    ASTNode* stmt = ast->left;
    while (stmt) {
        if (stmt->type == VAR_DECL_NODE || stmt->type == VAR_DECL_SPANISH_NODE) {
            compile_var(c, stmt);  // This will call add_global since in_function=0
        }
        stmt = stmt->right;
    }

    // Second pass: register all functions (so they can be called before defined)
    stmt = ast->left;
    while (stmt) {
        if (stmt->type == FN_DECL_NODE || stmt->type == FN_DECL_SPANISH_NODE) {
            // Register function with placeholder offset (will be patched later)
            int param_count = 0;
            ASTNode* p = stmt->params;
            while (p) { param_count++; p = p->right; }
            add_func(c, stmt->value, -1, param_count);  // -1 = unknown offset
        }
        stmt = stmt->right;
    }

    // Generate _start that calls main and exits
    // _start:
    //   call main
    //   mov rdi, rax
    //   mov rax, 60 (sys_exit)
    //   syscall
    encode_call_rel32(c->mc, 0);  // Placeholder
    int call_patch = get_current_offset(c->mc) - 4;

    encode_mov_rdi_rax(c->mc);
    encode_mov_rax_imm32(c->mc, SYS_EXIT);
    encode_syscall(c->mc);

    // Compile helper functions FIRST (so they're available when main calls them)
    stmt = ast->left;
    while (stmt) {
        if ((stmt->type == FN_DECL_NODE || stmt->type == FN_DECL_SPANISH_NODE) &&
            stmt->value && strcmp(stmt->value, "main") != 0 && strcmp(stmt->value, "principal") != 0) {
            compile_func(c, stmt);
        }
        stmt = stmt->right;
    }

    // Now compile main function
    int main_offset = -1;
    stmt = ast->left;
    while (stmt) {
        if ((stmt->type == FN_DECL_NODE || stmt->type == FN_DECL_SPANISH_NODE) &&
            stmt->value && (strcmp(stmt->value, "main") == 0 || strcmp(stmt->value, "principal") == 0)) {
            main_offset = get_current_offset(c->mc);
            compile_func(c, stmt);
        }
        stmt = stmt->right;
    }

    // Patch call to main
    if (main_offset >= 0) {
        int32_t rel = main_offset - (call_patch + 4);
        memcpy(c->mc->code + call_patch, &rel, 4);
    }
}

static void compile_main(Compiler* c, ASTNode* ast) {
    if (c->mode == OUT_C) {
        emit(c, "int main() {\n");
        c->indent = 1;
    } else if (c->mode == OUT_ASM) {
        // Strings written later
    } else if (c->mc) {
        encode_push_rbp(c->mc);
        encode_mov_rbp_rsp(c->mc);
        encode_sub_rsp_imm32(c->mc, 32768);
    }

    compile_node(c, ast);

    if (c->mode == OUT_C) {
        emit(c, "    return 0;\n}\n");
    } else if (c->mode == OUT_ASM) {
        emit(c, "main:\n    push rbp\n    mov rbp,rsp\n    sub rsp,32768\n");
        emit(c, "    xor rax,rax\n    leave\n    ret\n");
    } else if (c->mc) {
        encode_xor_rax_rax(c->mc);
        encode_leave(c->mc);
        encode_ret(c->mc);
    }
}

// === PUBLIC API ===
int compile_viva_to_c(const char* code, const char* outfile) {
    TokenStream* t = tokenize(code);
    ASTNode* ast = parse_program(t);
    Compiler* c = init_compiler_internal(outfile, OUT_C, 0);
    if (!c) { free_token_stream(t); free_ast_node(ast); return -1; }
    if (ast) compile_main(c, ast);
    finish_compiler(c);
    free_token_stream(t);
    free_ast_node(ast);
    return 0;
}

int compile_viva_to_asm(const char* code, const char* outfile) {
    TokenStream* t = tokenize(code);
    ASTNode* ast = parse_program(t);
    Compiler* c = init_compiler_internal(outfile, OUT_ASM, 0);
    if (!c) { free_token_stream(t); free_ast_node(ast); return -1; }
    if (ast) compile_main(c, ast);
    finish_compiler(c);
    free_token_stream(t);
    free_ast_node(ast);
    return 0;
}

int compile_viva_to_elf(const char* code, const char* outfile) {
    TokenStream* t = tokenize(code);
    ASTNode* ast = parse_program(t);
    Compiler* c = init_compiler_internal(outfile, OUT_STANDALONE, 1);  // Use syscalls
    if (!c) { free_token_stream(t); free_ast_node(ast); return -1; }
    c->plat = PLATFORM_LINUX;
    if (ast) compile_standalone(c, ast);
    finish_compiler(c);
    free_token_stream(t);
    free_ast_node(ast);
    return 0;
}

int compile_viva_to_platform(const char* code, const char* outfile, PlatformTarget plat) {
    TokenStream* t = tokenize(code);
    ASTNode* ast = parse_program(t);
    Compiler* c = init_compiler_internal(outfile, OUT_STANDALONE, 1);
    if (!c) { free_token_stream(t); free_ast_node(ast); return -1; }
    c->plat = plat;
    if (ast) compile_standalone(c, ast);
    finish_compiler(c);
    free_token_stream(t);
    free_ast_node(ast);
    return 0;
}
