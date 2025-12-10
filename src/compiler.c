// src/compiler.c - EXPANDED for Phase 4
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include "../include/lexer.h"
#include "../include/parser.h"
#include "../include/compiler.h"
#include "../include/machine_code.h"

#define MAX_VARS 100
#define MAX_STRS 100
#define MAX_BUF 16384
#define MAX_LABELS 256
#define MAX_FUNCS 64
#define MAX_PARAMS 8

typedef enum { OUT_C, OUT_ASM, OUT_ELF } OutMode;

typedef struct { char name[64]; int is_str; int offset; int is_array; int array_size; } Var;
typedef struct { char val[256]; char lbl[32]; int offset; } Str;
typedef struct { char name[32]; int offset; } Label;
typedef struct {
    char name[64];
    int param_count;
    char params[MAX_PARAMS][64];
    int code_offset;  // offset in machine code for this function
} UserFunc;

typedef struct {
    FILE* f;
    OutMode mode;
    PlatformTarget plat;
    int indent;
    Var vars[MAX_VARS];
    int nvar;
    Str strs[MAX_STRS];
    int nstr;
    Label labels[MAX_LABELS];
    int nlbl;
    char buf[MAX_BUF];
    int bufpos;
    char outname[256];
    MachineCode* mc;
    ELFFile* elf;
    int stack_off;
    int data_size;
    // User-defined functions
    UserFunc funcs[MAX_FUNCS];
    int nfunc;
    int in_function;  // Track if we're compiling inside a function
    int saved_nvar;   // Saved variable count when entering function
    int saved_stack;  // Saved stack offset when entering function
} Compiler;

// Forward declarations
static void compile_node(Compiler* c, ASTNode* n);
static void compile_expr(Compiler* c, ASTNode* n);
static void compile_expr_to_rax(Compiler* c, ASTNode* n);

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

static void indent(Compiler* c) {
    for (int i = 0; i < c->indent; i++) emit(c, "    ");
}

// === VARIABLE MANAGEMENT ===
static int find_var(Compiler* c, const char* name) {
    for (int i = 0; i < c->nvar; i++)
        if (strcmp(c->vars[i].name, name) == 0) return i;
    return -1;
}

static int add_var(Compiler* c, const char* name, int is_str) {
    if (c->nvar >= MAX_VARS) return -1;
    strncpy(c->vars[c->nvar].name, name, 63);
    c->vars[c->nvar].is_str = is_str;
    c->vars[c->nvar].is_array = 0;
    c->vars[c->nvar].array_size = 0;
    c->stack_off -= 8;
    c->vars[c->nvar].offset = c->stack_off;
    return c->nvar++;
}

static int add_array_var(Compiler* c, const char* name, int size) {
    if (c->nvar >= MAX_VARS) return -1;
    strncpy(c->vars[c->nvar].name, name, 63);
    c->vars[c->nvar].is_str = 0;
    c->vars[c->nvar].is_array = 1;
    c->vars[c->nvar].array_size = size;
    // Allocate space for all elements (8 bytes each for 64-bit values)
    c->stack_off -= 8 * size;
    c->vars[c->nvar].offset = c->stack_off;  // Points to first element
    return c->nvar++;
}

static int is_var_array(Compiler* c, const char* name) {
    int i = find_var(c, name);
    return (i >= 0) ? c->vars[i].is_array : 0;
}

static int get_var_off(Compiler* c, const char* name) {
    int i = find_var(c, name);
    return (i >= 0) ? c->vars[i].offset : -8;
}

static int is_var_str(Compiler* c, const char* name) {
    int i = find_var(c, name);
    return (i >= 0) ? c->vars[i].is_str : 0;
}

// === FUNCTION MANAGEMENT ===
static int find_func(Compiler* c, const char* name) {
    for (int i = 0; i < c->nfunc; i++)
        if (strcmp(c->funcs[i].name, name) == 0) return i;
    return -1;
}

static int add_func(Compiler* c, const char* name, int param_count) {
    if (c->nfunc >= MAX_FUNCS) return -1;
    strncpy(c->funcs[c->nfunc].name, name, 63);
    c->funcs[c->nfunc].param_count = param_count;
    c->funcs[c->nfunc].code_offset = c->mc ? c->mc->size : 0;
    return c->nfunc++;
}

static void set_func_param(Compiler* c, int func_idx, int param_idx, const char* name) {
    if (func_idx >= 0 && func_idx < c->nfunc && param_idx < MAX_PARAMS) {
        strncpy(c->funcs[func_idx].params[param_idx], name, 63);
    }
}

static int is_user_func(Compiler* c, const char* name) {
    return find_func(c, name) >= 0;
}

// === STRING MANAGEMENT ===
static char* add_str(Compiler* c, const char* s) {
    if (c->nstr >= MAX_STRS) return "str_err";
    static int cnt = 0;
    snprintf(c->strs[c->nstr].lbl, 32, "str_%d", cnt++);
    
    const char* p = (s[0] == '"') ? s + 1 : s;
    strncpy(c->strs[c->nstr].val, p, 255);
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
static Compiler* init_compiler(const char* outfile, OutMode mode) {
    Compiler* c = calloc(1, sizeof(Compiler));
    if (!c) return NULL;
    c->mode = mode;
    c->plat = PLATFORM_LINUX;
    strncpy(c->outname, outfile, 255);
    
    if (mode == OUT_ELF) {
        c->mc = init_machine_code();
        c->elf = init_elf_file();
    }
    
    c->f = fopen(outfile, "wb");
    if (!c->f) { free(c); return NULL; }
    
    if (mode == OUT_C) {
        fprintf(c->f,
            "// Generated by Viva Colombia compiler\n"
            "#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\n\n"
            "void prt(char* s){printf(\"%%s\",s);}\n"
            "void prtln(char* s){printf(\"%%s\\n\",s);}\n"
            "void prtnum(int n){printf(\"%%d\",n);}\n"
            "void prtlnnum(int n){printf(\"%%d\\n\",n);}\n"
            "void bolivar(){printf(\"Simon Bolivar: Libertador\\n\");}\n"
            "void narino(){printf(\"Francisco Narino: Heroe\\n\");}\n"
            "void cano(){printf(\"Maria Cano: Lider\\n\");}\n"
            "void gaitan(){printf(\"Jorge Gaitan: Lider politico\\n\");}\n"
            "void garcia(){printf(\"Gabriel Garcia Marquez: Nobel\\n\");}\n\n");
    } else if (mode == OUT_ASM) {
        fprintf(c->f,
            "; Generated by Viva Colombia\n"
            "section .data\n"
            "    fmt_s db \"%%s\",10,0\n"
            "    fmt_d db \"%%d\",10,0\n");
    }
    return c;
}

static void write_asm_strings(Compiler* c) {
    if (c->mode != OUT_ASM) return;
    for (int i = 0; i < c->nstr; i++) {
        fprintf(c->f, "    %s db \"%s\",0\n", c->strs[i].lbl, c->strs[i].val);
    }
    fprintf(c->f,
        "section .text\n"
        "    global main\n"
        "    extern printf\n\n");
}

static void write_elf_data_section(Compiler* c) {
    if (c->mode != OUT_ELF || c->nstr == 0) return;
    
    uint8_t* data = calloc(1, c->data_size + 1);
    if (!data) return;
    
    int off = 0;
    for (int i = 0; i < c->nstr; i++) {
        size_t len = strlen(c->strs[i].val);
        memcpy(data + off, c->strs[i].val, len);
        data[off + len] = 0;
        off += len + 1;
    }
    
    create_data_section(c->elf, data, c->data_size);
    free(data);
}

static void finish_compiler(Compiler* c) {
    if (!c) return;
    
    if (c->mode == OUT_ASM) {
        write_asm_strings(c);
        if (c->bufpos > 0)
            fwrite(c->buf, 1, c->bufpos, c->f);
    }
    
    if (c->mode == OUT_ELF && c->elf && c->mc) {
        write_elf_data_section(c);
        create_text_section(c->elf, c->mc);
        create_symbol_table(c->elf);
        fclose(c->f);
        c->f = NULL;
        write_complete_elf_file(c->elf, c->outname);
    }
    
    if (c->f) fclose(c->f);
    if (c->mc) free_machine_code(c->mc);
    if (c->elf) free_elf_file(c->elf);
    free(c);
}

// === EXPRESSION COMPILATION ===
static void compile_expr(Compiler* c, ASTNode* n) {
    if (!n) return;
    
    if (n->type == NUMBER_NODE) {
        if (c->mode == OUT_C) emit(c, "%s", n->value);
        else if (c->mode == OUT_ASM) emit(c, "    mov rax, %s\n", n->value);
        else if (c->mc) encode_mov_rax_imm32(c->mc, atoi(n->value));
    }
    else if (n->type == STRING_LITERAL_NODE) {
        if (c->mode == OUT_C) emit(c, "%s", n->value);
        else if (c->mode == OUT_ASM) {
            char* l = add_str(c, n->value);
            emit(c, "    lea rax, [%s]\n", l);
        }
        else if (c->mc) {
            char* l = add_str(c, n->value);
            int off = get_str_offset(c, l);
            encode_lea_rax_rip_rel(c->mc, off);
        }
    }
    else if (n->type == IDENTIFIER_NODE) {
        if (n->value && n->value[0] == '"') {
            if (c->mode == OUT_C) emit(c, "%s", n->value);
            else if (c->mode == OUT_ASM) {
                char* l = add_str(c, n->value);
                emit(c, "    lea rax, [%s]\n", l);
            }
            else if (c->mc) {
                char* l = add_str(c, n->value);
                int off = get_str_offset(c, l);
                encode_lea_rax_rip_rel(c->mc, off);
            }
        } else {
            if (c->mode == OUT_C) emit(c, "%s", n->value);
            else if (c->mode == OUT_ASM) {
                int off = get_var_off(c, n->value);
                emit(c, "    mov rax, [rbp%+d]\n", off);
            }
            else if (c->mc) {
                int off = get_var_off(c, n->value);
                encode_mov_rax_from_memory(c->mc, off);
            }
        }
    }
    else if (n->type == BINARY_OP_NODE) {
        if (c->mode == OUT_C) {
            emit(c, "(");
            compile_expr(c, n->left);
            emit(c, " %s ", n->value ? n->value : "+");
            compile_expr(c, n->right);
            emit(c, ")");
        }
        else if (c->mode == OUT_ASM) {
            compile_expr(c, n->left);
            emit(c, "    push rax\n");
            compile_expr(c, n->right);
            emit(c, "    mov rbx, rax\n    pop rax\n");
            
            if (!n->value) return;
            if (strcmp(n->value, "+") == 0) emit(c, "    add rax, rbx\n");
            else if (strcmp(n->value, "-") == 0) emit(c, "    sub rax, rbx\n");
            else if (strcmp(n->value, "*") == 0) emit(c, "    imul rax, rbx\n");
            else if (strcmp(n->value, "/") == 0) emit(c, "    xor rdx,rdx\n    idiv rbx\n");
            else if (strcmp(n->value, "%") == 0) emit(c, "    xor rdx,rdx\n    idiv rbx\n    mov rax,rdx\n");
            else if (strcmp(n->value, ">") == 0) emit(c, "    cmp rax,rbx\n    setg al\n    movzx rax,al\n");
            else if (strcmp(n->value, "<") == 0) emit(c, "    cmp rax,rbx\n    setl al\n    movzx rax,al\n");
            else if (strcmp(n->value, ">=") == 0) emit(c, "    cmp rax,rbx\n    setge al\n    movzx rax,al\n");
            else if (strcmp(n->value, "<=") == 0) emit(c, "    cmp rax,rbx\n    setle al\n    movzx rax,al\n");
            else if (strcmp(n->value, "==") == 0) emit(c, "    cmp rax,rbx\n    sete al\n    movzx rax,al\n");
            else if (strcmp(n->value, "!=") == 0) emit(c, "    cmp rax,rbx\n    setne al\n    movzx rax,al\n");
            else if (strcmp(n->value, "&&") == 0) emit(c, "    and rax,rbx\n");
            else if (strcmp(n->value, "||") == 0) emit(c, "    or rax,rbx\n");
            // Bitwise operators
            else if (strcmp(n->value, "&") == 0) emit(c, "    and rax,rbx\n");
            else if (strcmp(n->value, "|") == 0) emit(c, "    or rax,rbx\n");
            else if (strcmp(n->value, "^") == 0) emit(c, "    xor rax,rbx\n");
            // Shift operators
            else if (strcmp(n->value, "<<") == 0) emit(c, "    mov rcx,rbx\n    shl rax,cl\n");
            else if (strcmp(n->value, ">>") == 0) emit(c, "    mov rcx,rbx\n    sar rax,cl\n");
        }
        else if (c->mc) {
            compile_expr(c, n->left);
            encode_push_rax(c->mc);
            compile_expr(c, n->right);
            encode_mov_rbx_rax(c->mc);
            encode_pop_rax(c->mc);
            
            if (!n->value) return;
            if (strcmp(n->value, "+") == 0) encode_add_rax_rbx(c->mc);
            else if (strcmp(n->value, "-") == 0) encode_sub_rax_rbx(c->mc);
            else if (strcmp(n->value, "*") == 0) encode_mul_rbx(c->mc);
            else if (strcmp(n->value, "/") == 0) encode_div_rbx(c->mc);
            else if (strcmp(n->value, "%") == 0) encode_mod_rbx(c->mc);
            else if (strcmp(n->value, "&&") == 0) encode_and_rax_rbx(c->mc);
            else if (strcmp(n->value, "||") == 0) encode_or_rax_rbx(c->mc);
            // Bitwise operators
            else if (strcmp(n->value, "&") == 0) encode_and_rax_rbx(c->mc);
            else if (strcmp(n->value, "|") == 0) encode_or_rax_rbx(c->mc);
            else if (strcmp(n->value, "^") == 0) encode_xor_rax_rbx(c->mc);
            // Shift operators
            else if (strcmp(n->value, "<<") == 0) {
                encode_mov_cl_bl(c->mc);  // move shift count to cl
                encode_shl_rax_cl(c->mc);
            }
            else if (strcmp(n->value, ">>") == 0) {
                encode_mov_cl_bl(c->mc);  // move shift count to cl
                encode_sar_rax_cl(c->mc); // arithmetic shift right
            }
            else if (strcmp(n->value, ">") == 0) {
                encode_cmp_rax_rbx(c->mc);
                encode_setg_al(c->mc);
                encode_movzx_rax_al(c->mc);
            }
            else if (strcmp(n->value, "<") == 0) {
                encode_cmp_rax_rbx(c->mc);
                encode_setl_al(c->mc);
                encode_movzx_rax_al(c->mc);
            }
            else if (strcmp(n->value, ">=") == 0) {
                encode_cmp_rax_rbx(c->mc);
                encode_setge_al(c->mc);
                encode_movzx_rax_al(c->mc);
            }
            else if (strcmp(n->value, "<=") == 0) {
                encode_cmp_rax_rbx(c->mc);
                encode_setle_al(c->mc);
                encode_movzx_rax_al(c->mc);
            }
            else if (strcmp(n->value, "==") == 0) {
                encode_cmp_rax_rbx(c->mc);
                encode_sete_al(c->mc);
                encode_movzx_rax_al(c->mc);
            }
            else if (strcmp(n->value, "!=") == 0) {
                encode_cmp_rax_rbx(c->mc);
                encode_setne_al(c->mc);
                encode_movzx_rax_al(c->mc);
            }
        }
    }
    else if (n->type == UNARY_OP_NODE) {
        ASTNode* op = n->right ? n->right : n->left;
        if (c->mode == OUT_C) {
            if (n->value && (strcmp(n->value,"!")==0 || strcmp(n->value,"no")==0)) emit(c, "!");
            else if (n->value && strcmp(n->value,"-")==0) emit(c, "-");
            else if (n->value && strcmp(n->value,"~")==0) emit(c, "~");
            compile_expr(c, op);
        }
        else if (c->mode == OUT_ASM) {
            compile_expr(c, op);
            if (n->value && (strcmp(n->value,"!")==0 || strcmp(n->value,"no")==0))
                emit(c, "    cmp rax,0\n    sete al\n    movzx rax,al\n");
            else if (n->value && strcmp(n->value,"-")==0)
                emit(c, "    neg rax\n");
            else if (n->value && strcmp(n->value,"~")==0)
                emit(c, "    not rax\n");
        }
        else if (c->mc) {
            compile_expr(c, op);
            if (n->value && (strcmp(n->value,"!")==0 || strcmp(n->value,"no")==0)) {
                encode_cmp_rax_zero(c->mc);
                encode_sete_al(c->mc);
                encode_movzx_rax_al(c->mc);
            }
            else if (n->value && strcmp(n->value,"-")==0) {
                encode_neg_rax(c->mc);
            }
            else if (n->value && strcmp(n->value,"~")==0) {
                encode_not_rax(c->mc);
            }
        }
    }
    else if (n->type == ARRAY_ACCESS_NODE) {
        // Array access: arr[index]
        // n->value = array name, n->left = index expression
        const char* arr_name = n->value;
        if (c->mode == OUT_C) {
            emit(c, "%s[", arr_name);
            compile_expr(c, n->left);
            emit(c, "]");
        }
        else if (c->mode == OUT_ASM) {
            // Get base address of array
            int base_off = get_var_off(c, arr_name);
            // Compile index to rax
            compile_expr(c, n->left);
            // Calculate address: base + index * 8
            emit(c, "    shl rax, 3\n");  // multiply by 8
            emit(c, "    lea rbx, [rbp%+d]\n", base_off);
            emit(c, "    add rax, rbx\n");
            emit(c, "    mov rax, [rax]\n");  // load value
        }
        else if (c->mc) {
            int base_off = get_var_off(c, arr_name);
            // Compile index to rax
            compile_expr(c, n->left);
            encode_mov_rbx_rax(c->mc);  // rbx = index
            encode_imul_rbx_8(c->mc);   // rbx = index * 8
            encode_lea_rax_rbp_off(c->mc, base_off);  // rax = base address
            encode_add_rax_rbx(c->mc);  // rax = base + index*8
            encode_mov_rax_ptr_rax(c->mc);  // rax = *rax (load value)
        }
    }
    else if (n->type == FN_CALL_NODE) {
        // Function call in expression context - result in rax
        const char* fn = n->value;
        int func_idx = find_func(c, fn);

        if (c->mode == OUT_C) {
            emit(c, "%s(", fn);
            if (n->left) compile_expr(c, n->left);
            emit(c, ")");
        }
        else if (c->mode == OUT_ASM) {
            if (func_idx >= 0) {
                if (n->left) {
                    compile_expr(c, n->left);
                    emit(c, "    mov rdi, rax\n");
                }
                emit(c, "    call %s\n", fn);
            }
        }
        else if (c->mc) {
            if (func_idx >= 0) {
                if (n->left) {
                    compile_expr(c, n->left);
                    encode_mov_rdi_rax(c->mc);
                }
                int call_pos = c->mc->size;
                int func_offset = c->funcs[func_idx].code_offset;
                int rel_offset = func_offset - (call_pos + 5);
                encode_call_rel32(c->mc, rel_offset);
            }
        }
    }
}
// === FUNCTION CALL ===
// Helper to count arguments in a function call
static int count_args(ASTNode* arg) {
    // Arguments are stored in n->left, linked by n->right?
    // Actually in parse, only n->left is used for single arg
    // For multiple args, we need to check the parsing
    // Currently, function calls only support single argument
    return arg ? 1 : 0;
}

static void compile_call(Compiler* c, ASTNode* n) {
    if (!n || !n->value) return;
    const char* fn = n->value;

    // Check if this is a user-defined function
    int func_idx = find_func(c, fn);

    if (c->mode == OUT_C) {
        indent(c);
        if (func_idx >= 0) {
            // User-defined function call
            emit(c, "%s(", fn);
            if (n->left) {
                compile_expr(c, n->left);
            }
            emit(c, ");\n");
        }
        else if (strcmp(fn,"println")==0 || strcmp(fn,"print")==0) {
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
            else if (n->left && n->left->type == BINARY_OP_NODE) {
                emit(c, "prt%snum(", ln?"ln":"");
                compile_expr(c, n->left);
                emit(c, ");\n");
            }
            else if (n->left && n->left->type == ARRAY_ACCESS_NODE) {
                // Handle array access: println(arr[i])
                emit(c, "prt%snum(", ln?"ln":"");
                compile_expr(c, n->left);
                emit(c, ");\n");
            }
            else emit(c, "prt%s(\"\");\n", ln?"ln":"");
        }
        else if (strstr(fn,"bolivar")) emit(c, "bolivar();\n");
        else if (strstr(fn,"narino")) emit(c, "narino();\n");
        else if (strstr(fn,"cano")) emit(c, "cano();\n");
        else if (strstr(fn,"gaitan")) emit(c, "gaitan();\n");
        else if (strstr(fn,"garcia")) emit(c, "garcia();\n");
        else {
            // Unknown function - try calling it anyway
            emit(c, "%s(", fn);
            if (n->left) compile_expr(c, n->left);
            emit(c, ");\n");
        }
    }
    else if (c->mode == OUT_ASM) {
        emit(c, "    ; call %s\n", fn);
        if (func_idx >= 0) {
            // User-defined function call
            if (n->left) {
                compile_expr(c, n->left);
                emit(c, "    mov rdi, rax\n");
            }
            emit(c, "    call %s\n", fn);
        }
        else if (strcmp(fn,"println")==0 || strcmp(fn,"print")==0) {
            if (n->left && n->left->type == NUMBER_NODE) {
                emit(c, "    mov rdi, fmt_d\n    mov rsi, %s\n", n->left->value);
            } else if (n->left && n->left->type == BINARY_OP_NODE) {
                compile_expr(c, n->left);
                emit(c, "    mov rsi, rax\n    mov rdi, fmt_d\n");
            } else if (n->left && n->left->type == ARRAY_ACCESS_NODE) {
                // Handle array access: println(arr[i])
                compile_expr(c, n->left);
                emit(c, "    mov rsi, rax\n    mov rdi, fmt_d\n");
            } else if (n->left && n->left->type == IDENTIFIER_NODE && !is_var_str(c, n->left->value)) {
                int off = get_var_off(c, n->left->value);
                emit(c, "    mov rsi, [rbp%+d]\n    mov rdi, fmt_d\n", off);
            } else if (n->left) {
                char* l = add_str(c, n->left->value);
                emit(c, "    mov rdi, fmt_s\n    lea rsi, [%s]\n", l);
            }
            emit(c, "    xor rax,rax\n    call printf\n");
        }
        else {
            // Unknown function
            if (n->left) {
                compile_expr(c, n->left);
                emit(c, "    mov rdi, rax\n");
            }
            emit(c, "    call %s\n", fn);
        }
    }
    else if (c->mc) {
        if (func_idx >= 0) {
            // User-defined function call
            // Evaluate argument and put in rdi
            if (n->left) {
                compile_expr(c, n->left);
                encode_mov_rdi_rax(c->mc);
            }
            // Calculate relative offset to function
            int call_pos = c->mc->size;
            int func_offset = c->funcs[func_idx].code_offset;
            int rel_offset = func_offset - (call_pos + 5);  // +5 for call instruction size
            encode_call_rel32(c->mc, rel_offset);
        }
        else if (strcmp(fn,"syscall_exit")==0) {
            // syscall_exit(code) - Linux exit syscall (60)
            if (n->left) {
                compile_expr(c, n->left);
                encode_mov_rdi_rax(c->mc);
            } else {
                encode_xor_rax_rax(c->mc);
                encode_mov_rdi_rax(c->mc);
            }
            encode_mov_rax_imm32(c->mc, 60);  // syscall number for exit
            encode_syscall(c->mc);
        }
        else if (strcmp(fn,"syscall_write")==0) {
            // syscall_write(fd, buf, len) - Linux write syscall (1)
            // For now, simplified: write to stdout
            // Full version needs 3 args
            if (n->left) {
                compile_expr(c, n->left);  // Get the string/buffer
                encode_mov_rsi_rax(c->mc);  // buf -> rsi
            }
            encode_mov_rax_imm32(c->mc, 1);   // stdout fd
            encode_mov_rdi_rax(c->mc);        // fd -> rdi
            encode_mov_rax_imm32(c->mc, 1);   // len = 1 (simplified)
            encode_mov_rdx_rax(c->mc);        // len -> rdx
            encode_mov_rax_imm32(c->mc, 1);   // syscall number for write
            encode_syscall(c->mc);
        }
        else if (strcmp(fn,"println")==0 || strcmp(fn,"print")==0) {
            if (n->left && n->left->type == NUMBER_NODE) {
                encode_mov_rdi_imm64(c->mc, 0);  // fmt placeholder
                encode_mov_rax_imm32(c->mc, atoi(n->left->value));
                encode_mov_rsi_rax(c->mc);
            } else if (n->left && n->left->type == BINARY_OP_NODE) {
                compile_expr(c, n->left);
                encode_mov_rsi_rax(c->mc);
                encode_mov_rdi_imm64(c->mc, 0);
            } else if (n->left && n->left->type == ARRAY_ACCESS_NODE) {
                // Handle array access: println(arr[i])
                compile_expr(c, n->left);
                encode_mov_rsi_rax(c->mc);
                encode_mov_rdi_imm64(c->mc, 0);
            } else if (n->left && n->left->type == IDENTIFIER_NODE) {
                int off = get_var_off(c, n->left->value);
                encode_mov_rax_from_memory(c->mc, off);
                encode_mov_rsi_rax(c->mc);
                encode_mov_rdi_imm64(c->mc, 0);
            }
            encode_xor_rax_rax(c->mc);
            add_relocation_entry(c->mc, 1, R_X86_64_PLT32, -4);
            encode_call_rel32(c->mc, 0);
        } else {
            // External/unknown function
            if (n->left) {
                compile_expr(c, n->left);
                encode_mov_rdi_rax(c->mc);
            }
            encode_call_external(c->mc);
        }
    }
}

// === VARIABLE DECLARATION ===
static void compile_var(Compiler* c, ASTNode* n, int spanish) {
    if (!n || !n->value) return;
    int is_str = 0;
    
    if (c->mode == OUT_C) {
        indent(c);
        if (n->left && (n->left->type == STRING_LITERAL_NODE ||
            (n->left->type == IDENTIFIER_NODE && n->left->value[0]=='"'))) {
            emit(c, "char* %s = %s;\n", n->value, n->left->value);
            is_str = 1;
        } else if (n->left) {
            emit(c, "int %s = ", n->value);
            compile_expr(c, n->left);
            emit(c, ";\n");
        } else {
            emit(c, "int %s = 0;\n", n->value);
        }
    }
    else if (c->mode == OUT_ASM) {
        int off = c->stack_off - 8;
        emit(c, "    ; var %s\n", n->value);
        if (n->left) {
            if (n->left->type == STRING_LITERAL_NODE ||
                (n->left->type == IDENTIFIER_NODE && n->left->value[0]=='"')) {
                char* l = add_str(c, n->left->value);
                emit(c, "    lea rax, [%s]\n", l);
                is_str = 1;
            } else {
                compile_expr(c, n->left);
            }
            emit(c, "    mov [rbp%+d], rax\n", off);
        } else {
            emit(c, "    mov qword [rbp%+d], 0\n", off);
        }
    }
    else if (c->mc) {
        int off = c->stack_off - 8;
        if (n->left) {
            if (n->left->type == STRING_LITERAL_NODE ||
                (n->left->type == IDENTIFIER_NODE && n->left->value[0]=='"')) {
                char* l = add_str(c, n->left->value);
                int str_off = get_str_offset(c, l);
                encode_lea_rax_rip_rel(c->mc, str_off);
                is_str = 1;
            } else {
                compile_expr(c, n->left);
            }
            encode_mov_memory_from_rax(c->mc, off);
        } else {
            encode_mov_rax_imm32(c->mc, 0);
            encode_mov_memory_from_rax(c->mc, off);
        }
    }
    add_var(c, n->value, is_str);
}

// === ARRAY DECLARATION ===
static void compile_array_decl(Compiler* c, ASTNode* n) {
    if (!n || !n->value) return;

    // Get array size from extra node
    int arr_size = 10;  // default size
    if (n->extra && n->extra->type == NUMBER_NODE && n->extra->value) {
        arr_size = atoi(n->extra->value);
    }

    // First register the array variable (allocates stack space)
    add_array_var(c, n->value, arr_size);

    if (c->mode == OUT_C) {
        indent(c);
        emit(c, "int %s[%d];\n", n->value, arr_size);
    }
    else if (c->mode == OUT_ASM) {
        emit(c, "    ; array %s[%d]\n", n->value, arr_size);
        // Arrays are stored on stack, space already allocated by add_array_var
    }
    else if (c->mc) {
        // Arrays are stored on stack, space already allocated by add_array_var
        // Initialize to zero
        int base_off = get_var_off(c, n->value);
        for (int i = 0; i < arr_size; i++) {
            encode_mov_rax_imm32(c->mc, 0);
            encode_mov_memory_from_rax(c->mc, base_off + i * 8);
        }
    }
}

// === ASSIGNMENT ===
static void compile_assign(Compiler* c, ASTNode* n) {
    if (!n || !n->value || !n->left) return;

    // Check if this is an array assignment (n->extra contains index)
    if (n->extra) {
        // Array assignment: arr[index] = value
        if (c->mode == OUT_C) {
            indent(c);
            emit(c, "%s[", n->value);
            compile_expr(c, n->extra);  // index
            emit(c, "] = ");
            compile_expr(c, n->left);   // value
            emit(c, ";\n");
        }
        else if (c->mode == OUT_ASM) {
            int base_off = get_var_off(c, n->value);
            // Compute value and save to rbx
            compile_expr(c, n->left);
            emit(c, "    push rax\n");  // save value
            // Compute index
            compile_expr(c, n->extra);
            emit(c, "    imul rax, 8\n");  // index * 8
            emit(c, "    lea rbx, [rbp%+d]\n", base_off);
            emit(c, "    add rax, rbx\n");  // base + index*8
            emit(c, "    pop rbx\n");  // restore value to rbx
            emit(c, "    mov [rax], rbx\n");  // store value
        }
        else if (c->mc) {
            int base_off = get_var_off(c, n->value);
            // Compute value and save to stack
            compile_expr(c, n->left);
            encode_push_rax(c->mc);     // save value
            // Compute index
            compile_expr(c, n->extra);
            encode_mov_rbx_rax(c->mc);  // rbx = index
            encode_imul_rbx_8(c->mc);   // rbx = index * 8
            encode_lea_rax_rbp_off(c->mc, base_off);  // rax = base address
            encode_add_rax_rbx(c->mc);  // rax = base + index*8
            encode_pop_rbx(c->mc);      // rbx = value to store
            encode_mov_ptr_rax_rbx(c->mc);  // [rax] = rbx (store value)
        }
    }
    else {
        // Regular variable assignment
        if (c->mode == OUT_C) {
            indent(c);
            emit(c, "%s = ", n->value);
            compile_expr(c, n->left);
            emit(c, ";\n");
        }
        else if (c->mode == OUT_ASM) {
            int off = get_var_off(c, n->value);
            compile_expr(c, n->left);
            emit(c, "    mov [rbp%+d], rax\n", off);
        }
        else if (c->mc) {
            int off = get_var_off(c, n->value);
            compile_expr(c, n->left);
            encode_mov_memory_from_rax(c->mc, off);
        }
    }
}

// === IF STATEMENT ===
static void compile_if(Compiler* c, ASTNode* n) {
    static int lbl = 0;
    int id = lbl++;
    
    if (c->mode == OUT_C) {
        indent(c); emit(c, "if ("); compile_expr(c, n->left); emit(c, ") {\n");
        c->indent++;
        compile_node(c, n->right);
        c->indent--;
        indent(c); emit(c, "}");
        if (n->extra) {
            emit(c, " else {\n");
            c->indent++;
            compile_node(c, n->extra);
            c->indent--;
            indent(c); emit(c, "}");
        }
        emit(c, "\n");
    }
    else if (c->mode == OUT_ASM) {
        compile_expr(c, n->left);
        emit(c, "    cmp rax, 0\n    je .Lelse%d\n", id);
        compile_node(c, n->right);
        emit(c, "    jmp .Lend%d\n.Lelse%d:\n", id, id);
        if (n->extra) compile_node(c, n->extra);
        emit(c, ".Lend%d:\n", id);
    }
    else if (c->mc) {
        compile_expr(c, n->left);
        encode_cmp_rax_zero(c->mc);
        
        int je_pos = c->mc->size;
        encode_je_rel32(c->mc, 0);  // placeholder
        
        compile_node(c, n->right);
        
        int jmp_pos = c->mc->size;
        encode_jmp_rel32(c->mc, 0);  // placeholder
        
        int else_pos = c->mc->size;
        patch_jump_offset(c->mc, je_pos + 2, else_pos);
        
        if (n->extra) compile_node(c, n->extra);
        
        int end_pos = c->mc->size;
        patch_jump_offset(c->mc, jmp_pos + 1, end_pos);
    }
}

// === WHILE LOOP ===
static void compile_while(Compiler* c, ASTNode* n) {
    static int lbl = 0;
    int id = lbl++;
    
    if (c->mode == OUT_C) {
        indent(c); emit(c, "while ("); compile_expr(c, n->left); emit(c, ") {\n");
        c->indent++;
        compile_node(c, n->right);
        c->indent--;
        indent(c); emit(c, "}\n");
    }
    else if (c->mode == OUT_ASM) {
        emit(c, ".Lw%d:\n", id);
        compile_expr(c, n->left);
        emit(c, "    cmp rax,0\n    je .Lwe%d\n", id);
        compile_node(c, n->right);
        emit(c, "    jmp .Lw%d\n.Lwe%d:\n", id, id);
    }
    else if (c->mc) {
        int start_pos = c->mc->size;
        
        compile_expr(c, n->left);
        encode_cmp_rax_zero(c->mc);
        
        int je_pos = c->mc->size;
        encode_je_rel32(c->mc, 0);  // placeholder
        
        compile_node(c, n->right);
        
        int jmp_pos = c->mc->size;
        encode_jmp_rel32(c->mc, 0);  // placeholder
        patch_jump_offset(c->mc, jmp_pos + 1, start_pos);
        
        int end_pos = c->mc->size;
        patch_jump_offset(c->mc, je_pos + 2, end_pos);
    }
}

// === FOR LOOP ===
static void compile_for(Compiler* c, ASTNode* n) {
    static int lbl = 0;
    int id = lbl++;
    
    if (c->mode == OUT_C) {
        indent(c); emit(c, "for (");
        if (n->left) compile_expr(c, n->left);
        emit(c, "; ");
        if (n->extra && n->extra->left) compile_expr(c, n->extra->left);
        emit(c, "; ");
        if (n->extra && n->extra->right) compile_expr(c, n->extra->right);
        emit(c, ") {\n");
        c->indent++;
        compile_node(c, n->right);
        c->indent--;
        indent(c); emit(c, "}\n");
    }
    else if (c->mode == OUT_ASM) {
        if (n->left) compile_node(c, n->left);
        emit(c, ".Lf%d:\n", id);
        if (n->extra && n->extra->left) {
            compile_expr(c, n->extra->left);
            emit(c, "    cmp rax,0\n    je .Lfe%d\n", id);
        }
        compile_node(c, n->right);
        if (n->extra && n->extra->right) compile_node(c, n->extra->right);
        emit(c, "    jmp .Lf%d\n.Lfe%d:\n", id, id);
    }
    else if (c->mc) {
        if (n->left) compile_node(c, n->left);
        
        int start_pos = c->mc->size;
        
        int je_pos = -1;
        if (n->extra && n->extra->left) {
            compile_expr(c, n->extra->left);
            encode_cmp_rax_zero(c->mc);
            je_pos = c->mc->size;
            encode_je_rel32(c->mc, 0);
        }
        
        compile_node(c, n->right);
        
        if (n->extra && n->extra->right) compile_node(c, n->extra->right);
        
        int jmp_pos = c->mc->size;
        encode_jmp_rel32(c->mc, 0);
        patch_jump_offset(c->mc, jmp_pos + 1, start_pos);
        
        if (je_pos >= 0) {
            int end_pos = c->mc->size;
            patch_jump_offset(c->mc, je_pos + 2, end_pos);
        }
    }
}

// === RETURN ===
static void compile_return(Compiler* c, ASTNode* n) {
    if (c->mode == OUT_C) {
        indent(c); emit(c, "return ");
        if (n->left) compile_expr(c, n->left);
        else emit(c, "0");
        emit(c, ";\n");
    }
    else if (c->mode == OUT_ASM) {
        if (n->left) compile_expr(c, n->left);
        else emit(c, "    xor rax,rax\n");
        emit(c, "    leave\n    ret\n");
    }
    else if (c->mc) {
        if (n->left) compile_expr(c, n->left);
        else encode_xor_rax_rax(c->mc);
        encode_leave(c->mc);
        encode_ret(c->mc);
    }
}

// === FUNCTION DECLARATION ===
static void compile_func(Compiler* c, ASTNode* n) {
    const char* name = n->value ? n->value : "func";

    // Count parameters and register function
    int param_count = 0;
    ASTNode* param = n->left;
    while (param) {
        param_count++;
        param = param->right;
    }

    // Save compiler state for function scope
    c->saved_nvar = c->nvar;
    c->saved_stack = c->stack_off;
    c->in_function = 1;
    c->stack_off = 0;  // Reset stack for function

    // Register function
    int func_idx = add_func(c, name, param_count);

    // Function body is in n->extra (n->right is for statement linking)
    ASTNode* body = n->extra;

    if (c->mode == OUT_C) {
        // Generate C function with parameters
        emit(c, "int %s(", name);
        param = n->left;
        int first = 1;
        while (param) {
            if (!first) emit(c, ", ");
            emit(c, "int %s", param->value);
            first = 0;
            param = param->right;
        }
        emit(c, ") {\n");
        c->indent++;

        // Parameters are already available as C variables
        param = n->left;
        int param_idx = 0;
        while (param) {
            add_var(c, param->value, 0);  // Track in var table
            set_func_param(c, func_idx, param_idx++, param->value);
            param = param->right;
        }

        compile_node(c, body);
        c->indent--;
        emit(c, "    return 0;\n}\n\n");
    }
    else if (c->mode == OUT_ASM) {
        emit(c, "%s:\n    push rbp\n    mov rbp,rsp\n    sub rsp,256\n", name);

        // Store parameters on stack (System V AMD64: rdi, rsi, rdx, rcx, r8, r9)
        param = n->left;
        int param_idx = 0;
        const char* param_regs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
        while (param && param_idx < 6) {
            int off = add_var(c, param->value, 0);
            off = c->vars[c->nvar - 1].offset;
            emit(c, "    mov [rbp%+d], %s  ; param %s\n", off, param_regs[param_idx], param->value);
            set_func_param(c, func_idx, param_idx++, param->value);
            param = param->right;
        }

        compile_node(c, body);
        emit(c, "    xor rax,rax\n    leave\n    ret\n\n");
    }
    else if (c->mc) {
        // Record function start offset
        if (func_idx >= 0) c->funcs[func_idx].code_offset = c->mc->size;

        encode_push_rbp(c->mc);
        encode_mov_rbp_rsp(c->mc);
        encode_sub_rsp_imm8(c->mc, 128);

        // Store parameters on stack (System V AMD64: rdi, rsi, rdx, rcx)
        param = n->left;
        int param_idx = 0;
        while (param && param_idx < 4) {
            add_var(c, param->value, 0);
            int off = c->vars[c->nvar - 1].offset;
            set_func_param(c, func_idx, param_idx, param->value);

            // Store parameter register to stack
            switch (param_idx) {
                case 0: encode_mov_memory_from_rdi(c->mc, off); break;
                case 1: encode_mov_memory_from_rsi(c->mc, off); break;
                case 2: encode_mov_memory_from_rdx(c->mc, off); break;
                case 3: encode_mov_memory_from_rcx(c->mc, off); break;
            }
            param_idx++;
            param = param->right;
        }

        compile_node(c, body);
        encode_xor_rax_rax(c->mc);
        encode_leave(c->mc);
        encode_ret(c->mc);
    }

    // Restore compiler state
    c->nvar = c->saved_nvar;
    c->stack_off = c->saved_stack;
    c->in_function = 0;
}

// === NODE DISPATCHER ===
static void compile_node(Compiler* c, ASTNode* n) {
    while (n) {
        switch (n->type) {
            case PROGRAM_NODE: compile_node(c, n->left); break;
            case FN_CALL_NODE: compile_call(c, n); break;
            case FN_DECL_NODE: case FN_DECL_SPANISH_NODE: compile_func(c, n); break;
            case VAR_DECL_NODE: compile_var(c, n, 0); break;
            case VAR_DECL_SPANISH_NODE: compile_var(c, n, 1); break;
            case ARRAY_DECL_NODE: compile_array_decl(c, n); break;
            case ASSIGN_NODE: compile_assign(c, n); break;
            case IF_NODE: case IF_SPANISH_NODE: compile_if(c, n); break;
            case WHILE_NODE: case WHILE_SPANISH_NODE: compile_while(c, n); break;
            case FOR_NODE: case FOR_SPANISH_NODE: compile_for(c, n); break;
            case RETURN_NODE: compile_return(c, n); break;
            case BREAK_NODE:
                if (c->mode == OUT_C) { indent(c); emit(c, "break;\n"); }
                break;
            case CONTINUE_NODE:
                if (c->mode == OUT_C) { indent(c); emit(c, "continue;\n"); }
                break;
            default: break;
        }
        n = n->right;
    }
}

// === Helper: compile only function declarations from AST ===
static void compile_functions_only(Compiler* c, ASTNode* n) {
    while (n) {
        if (n->type == FN_DECL_NODE || n->type == FN_DECL_SPANISH_NODE) {
            compile_func(c, n);
        }
        n = n->right;
    }
}

// === Helper: compile non-function statements from AST ===
static void compile_statements_only(Compiler* c, ASTNode* n) {
    while (n) {
        if (n->type != FN_DECL_NODE && n->type != FN_DECL_SPANISH_NODE) {
            switch (n->type) {
                case PROGRAM_NODE: compile_statements_only(c, n->left); break;
                case FN_CALL_NODE: compile_call(c, n); break;
                case VAR_DECL_NODE: compile_var(c, n, 0); break;
                case VAR_DECL_SPANISH_NODE: compile_var(c, n, 1); break;
                case ARRAY_DECL_NODE: compile_array_decl(c, n); break;
                case ASSIGN_NODE: compile_assign(c, n); break;
                case IF_NODE: case IF_SPANISH_NODE: compile_if(c, n); break;
                case WHILE_NODE: case WHILE_SPANISH_NODE: compile_while(c, n); break;
                case FOR_NODE: case FOR_SPANISH_NODE: compile_for(c, n); break;
                case RETURN_NODE: compile_return(c, n); break;
                case BREAK_NODE:
                    if (c->mode == OUT_C) { indent(c); emit(c, "break;\n"); }
                    break;
                case CONTINUE_NODE:
                    if (c->mode == OUT_C) { indent(c); emit(c, "continue;\n"); }
                    break;
                default: break;
            }
        }
        n = n->right;
    }
}

// === MAIN WRAPPER ===
static void compile_main(Compiler* c, ASTNode* ast) {
    if (c->mode == OUT_C) {
        // First pass: compile function declarations outside main
        ASTNode* prog = ast;
        if (prog && prog->type == PROGRAM_NODE) {
            compile_functions_only(c, prog->left);
        }

        // Now emit main and compile statements
        emit(c, "int main() {\n");
        c->indent = 1;

        if (prog && prog->type == PROGRAM_NODE) {
            compile_statements_only(c, prog->left);
        } else {
            compile_statements_only(c, ast);
        }

        emit(c, "    return 0;\n}\n");
    } else if (c->mode == OUT_ASM) {
        // Strings written later
        compile_node(c, ast);
        emit(c, "main:\n    push rbp\n    mov rbp,rsp\n    sub rsp,256\n");
        // Code already in buffer, append ending
        emit(c, "    xor rax,rax\n    leave\n    ret\n");
    } else if (c->mc) {
        encode_push_rbp(c->mc);
        encode_mov_rbp_rsp(c->mc);
        encode_sub_rsp_imm8(c->mc, 128);
        compile_node(c, ast);
        encode_xor_rax_rax(c->mc);
        encode_leave(c->mc);
        encode_ret(c->mc);
    }
}

// === PUBLIC API ===
int compile_viva_to_c(const char* code, const char* outfile) {
    TokenStream* t = tokenize(code);
    ASTNode* ast = parse_program(t);
    Compiler* c = init_compiler(outfile, OUT_C);
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
    Compiler* c = init_compiler(outfile, OUT_ASM);
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
    Compiler* c = init_compiler(outfile, OUT_ELF);
    if (!c) { free_token_stream(t); free_ast_node(ast); return -1; }
    c->plat = PLATFORM_LINUX;
    if (ast) compile_main(c, ast);
    finish_compiler(c);
    free_token_stream(t);
    free_ast_node(ast);
    return 0;
}

int compile_viva_to_platform(const char* code, const char* outfile, PlatformTarget plat) {
    TokenStream* t = tokenize(code);
    ASTNode* ast = parse_program(t);
    Compiler* c = init_compiler(outfile, OUT_ELF);
    if (!c) { free_token_stream(t); free_ast_node(ast); return -1; }
    c->plat = plat;
    if (ast) compile_main(c, ast);
    finish_compiler(c);
    free_token_stream(t);
    free_ast_node(ast);
    return 0;
}
