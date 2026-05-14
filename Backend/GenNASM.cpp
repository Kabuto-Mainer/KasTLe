#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "BackIR.h"
#include "BackIRType.h"
#include "Common.h"
#include "Gen.h"

// =====================================================================
// TABLES
// =====================================================================

/*   idx:   0=8byte, 1=4byte, 2=2byte, 3=1byte */
static const char *KTL_REG_NAMES[KTL_REG_COUNT][4] = {
    /* RAX */ {"rax",  "eax",  "ax",   "al"  },
    /* RCX */ {"rcx",  "ecx",  "cx",   "cl"  },
    /* RDX */ {"rdx",  "edx",  "dx",   "dl"  },
    /* RBX */ {"rbx",  "ebx",  "bx",   "bl"  },
    /* RSP */ {"rsp",  "esp",  "sp",   "spl" },
    /* RBP */ {"rbp",  "ebp",  "bp",   "bpl" },
    /* RSI */ {"rsi",  "esi",  "si",   "sil" },
    /* RDI */ {"rdi",  "edi",  "di",   "dil" },
    /* R8  */ {"r8",   "r8d",  "r8w",  "r8b" },
    /* R9  */ {"r9",   "r9d",  "r9w",  "r9b" },
    /* R10 */ {"r10",  "r10d", "r10w", "r10b"},
    /* R11 */ {"r11",  "r11d", "r11w", "r11b"},
    /* R12 */ {"r12",  "r12d", "r12w", "r12b"},
    /* R13 */ {"r13",  "r13d", "r13w", "r13b"},
    /* R14 */ {"r14",  "r14d", "r14w", "r14b"},
    /* R15 */ {"r15",  "r15d", "r15w", "r15b"},
};

static const char *KTL_DATA_PREFIX[4] = {
    "db", "dw",
    "dd", "dq"
};

static const char *KTL_INSTR_NAMES[] = {
    [KTL_ASM_MOV]       = "mov",
    [KTL_ASM_MOVZX]     = "movzx",
    [KTL_ASM_MOVSX]     = "movsx",
    [KTL_ASM_CDQE]      = "cdqe",
    [KTL_ASM_CQO]       = "cqo",
    [KTL_ASM_PUSH]      = "push",
    [KTL_ASM_POP]       = "pop",
    [KTL_ASM_ADD]       = "add",
    [KTL_ASM_SUB]       = "sub",
    [KTL_ASM_IMUL]      = "imul",
    [KTL_ASM_IDIV1]     = "idiv",
    [KTL_ASM_XOR]       = "xor",
    [KTL_ASM_AND]       = "and",
    [KTL_ASM_NEG]       = "neg",
    [KTL_ASM_TEST]      = "test",
    [KTL_ASM_CMP]       = "cmp",
    [KTL_ASM_SETE]      = "sete",
    [KTL_ASM_SETNE]     = "setne",
    [KTL_ASM_SETL]      = "setl",
    [KTL_ASM_SETLE]     = "setle",
    [KTL_ASM_SETG]      = "setg",
    [KTL_ASM_SETGE]     = "setge",
    [KTL_ASM_JMP]       = "jmp",
    [KTL_ASM_JZ]        = "jz",
    [KTL_ASM_JNZ]       = "jnz",
    [KTL_ASM_LEA]       = "lea",
    [KTL_ASM_CALL]      = "call",
    [KTL_ASM_CALL_PLT]  = "call",
    [KTL_ASM_REP_STOSB] = "rep stosb",
    [KTL_ASM_REP_MOVSB] = "rep movsb",
    [KTL_ASM_RET]       = "ret",
    [KTL_ASM_SYSCALL]   = "syscall",
    [KTL_ASM_DEBUG]     = "int3",
};


// =====================================================================
// HELPERS
// =====================================================================

static const char *reg_name(KTL_RegID reg, int size) {
    assert(reg >= 0 && reg < KTL_REG_COUNT);
    int idx;
    switch (size) {
        case 8: idx = 0; break;
        case 4: idx = 1; break;
        case 2: idx = 2; break;
        case 1: idx = 3; break;
        default: assert(false && "unsupported register size"); return "?";
    }
    return KTL_REG_NAMES[reg][idx];
}

static const char *size_prefix(int size) {
    switch (size) {
        case 1: return "byte";
        case 2: return "word";
        case 4: return "dword";
        case 8: return "qword";
        default: assert(false && "unsupported memory size"); return "?";
    }
}

static const char *data_prefix(int size) {
    switch (size) {
        case 1: return KTL_DATA_PREFIX[0];
        case 2: return KTL_DATA_PREFIX[1];
        case 4: return KTL_DATA_PREFIX[2];
        case 8: return KTL_DATA_PREFIX[3];
        default: assert(false && "unsupported data size"); return "?";
    }
}


// =====================================================================
// OPERAND
// =====================================================================

static void print_mem(FILE *file, KTL_RegID base, KTL_RegID idx,
                      int scale, int offset, int size) {
    assert(file);
    fprintf(file, "%s [", size_prefix(size));

    bool has_base = (base != KTL_REG_INVALID);
    bool has_idx  = (idx  != KTL_REG_INVALID);

    if (has_base) {
        fprintf(file, "%s", reg_name(base, 8));
    }
    if (has_idx) {
        fprintf(file, "%s%s*%d", has_base ? "+" : "", reg_name(idx, 8), scale);
    }
    if (offset != 0 || (!has_base && !has_idx)) {
        if (has_base || has_idx) {
            fprintf(file, "%+d", offset);
        } else {
            fprintf(file, "%d", offset);
        }
    }
    fputc(']', file);
}

static void print_operand(FILE *file, const KTL_BackIR_InstrOperand *op) {
    assert(file);
    assert(op);

    switch (op->kind) {
    case KTL_BACK_IR_OP_REG:
        fprintf(file, "%s", reg_name(op->reg.reg, op->reg.size));
        return ;

    case KTL_BACK_IR_OP_IMM:
        fprintf(file, "%lld", (long long) op->imm.imm);
        return ;

    case KTL_BACK_IR_OP_SYMBOL:
        if (op->sym.kind == KTL_BACK_IR_SYM_GOT_FUNC) {
            fprintf(file, "%s WRT ..plt", op->sym.sym);
        } else {
            fprintf(file, "%s", op->sym.sym);
        }
        return ;

    case KTL_BACK_IR_OP_LABEL:
        fprintf(file, "%s", op->label.name);
        return ;

    case KTL_BACK_IR_OP_MEM:
        print_mem(file, op->mem.base, op->mem.idx, op->mem.scale,
                     op->mem.offset, op->mem.size);
        return ;

    case KTL_BACK_IR_OP_MEM_RIP:
        if (op->mem_rip.kind == KTL_BACK_IR_SYM_GOT_VAR) {
            fprintf(file, "%s [rel %s WRT ..got]",
                    size_prefix(op->mem_rip.size), op->mem_rip.sym);
        } else {
            fprintf(file, "%s [rel %s]",
                    size_prefix(op->mem_rip.size), op->mem_rip.sym);
        }
        return ;

    default:
        assert(false && "unknown operand kind");
        return ;
    }
}


// =====================================================================
// ITEMS
// =====================================================================

static void print_instr(FILE *file, const KTL_BackIR_Item *item) {
    assert(file);
    assert(item);

    if (item->instr.amount_args == 0) {
        fprintf(file, "    %s\n", KTL_INSTR_NAMES[item->instr.cmd]);
        return;
    }

    fprintf(file, "    %-4s ", KTL_INSTR_NAMES[item->instr.cmd]);

    if (item->instr.amount_args == 1) {
        print_operand(file, &item->instr.one);
    } else {
        print_operand(file, &item->instr.two.dst);
        fputs(", ", file);
        print_operand(file, &item->instr.two.src);
    }
    fputc('\n', file);

    return ;
}

static void print_label(FILE *file, const KTL_BackIR_Item *item) {
    assert(file);
    assert(item);

    if (item->label_decl.is_global) {
        fprintf(file, "global %s\n", item->label_decl.name);
    }
    fprintf(file, "%s:\n", item->label_decl.name);

    return ;
}

static void print_directive(FILE *file, const KTL_BackIR_Item *item) {
    assert(file);
    assert(item);

    switch (item->direct.kind) {
    case KTL_BACK_IR_DIR_ALIGN:
        fprintf(file, "align %d\n", item->direct.align.size);
        return ;
    default:
        assert(false && "unknown directive");
        return ;
    }
}

static void print_bytes(FILE *file, const char *bytes, int len) {
    assert(file);
    assert(bytes);
    fputs("    db ", file);

    bool in_quotes = false;
    bool first     = true;

    for (int i = 0; i < len; i++) {
        unsigned char c = (unsigned char) bytes[i];
        bool printable  = (c >= 0x20 && c < 0x7F && c != '"' && c != '\\');

        if (printable) {
            if (!in_quotes) {
                fprintf(file, "%s\"", first ? "" : ", ");
                in_quotes = true;
            }
            fputc((char) c, file);
        } else {
            if (in_quotes) {
                fputc('"', file);
                in_quotes = false;
            }
            fprintf(file, "%s0x%02X", first ? "" : ", ", c);
        }
        first = false;
    }
    if (in_quotes) fputc('"', file);
    fputc('\n', file);

    return ;
}

static void print_data(FILE *file, const KTL_BackIR_Item *item) {
    assert(file);
    assert(item);

    switch (item->data.kind) {
    case KTL_BACK_IR_DATA_ZERO:
        fprintf(file, "    times %d db 0\n", item->data.zero.count);
        return ;

    case KTL_BACK_IR_DATA_INT:
        fprintf(file, "    %s %lld\n",
                data_prefix(item->data.int_val.size),
                (long long) item->data.int_val.value);
        return ;

    case KTL_BACK_IR_DATA_BYTES:
        print_bytes(file, item->data.bytes.bytes, item->data.bytes.len);
        return ;

    case KTL_BACK_IR_DATA_SYMBOL:
        if (item->data.symbol.addend != 0) {
            fprintf(file, "    %s %s %+lld\n",
                    data_prefix(item->data.symbol.size),
                    item->data.symbol.sym,
                    (long long) item->data.symbol.addend);
        } else {
            fprintf(file, "    %s %s\n",
                    data_prefix(item->data.symbol.size),
                    item->data.symbol.sym);
        }
        return ;

    default:
        assert(false && "unknown data kind");
        return ;
    }
}

static void print_item(FILE *file, const KTL_BackIR_Item *item) {
    assert(file);
    assert(item);

    switch (item->kind) {
    case KTL_BACK_IR_ITEM_INSTR:     print_instr(file, item);         return;
    case KTL_BACK_IR_ITEM_LABEL:     print_label(file, item);         return;
    case KTL_BACK_IR_ITEM_COMMENT:   fputs(item->comment.text, file); return;
    case KTL_BACK_IR_ITEM_DIRECTIVE: print_directive(file, item);     return;
    case KTL_BACK_IR_ITEM_DATA:      print_data(file, item);          return;
    default:
        assert(false && "unknown item kind");
        return ;
    }
}

static void print_buffer(FILE *file, KTL_BackIR_Buffer *buf) {
    assert(file);
    assert(buf);

    if (buf == NULL) return;
    for (int i = 0; i < buf->size; i++) {
        print_item(file, &buf->data[i]);
    }
    return ;
}


// =====================================================================
// API
// =====================================================================

KTL_Error KTL_Backend_GenerateNasm(KTL_BackIR_Buffer *text,
                                   KTL_BackIR_Buffer *data,
                                   KTL_BackIR_Buffer *rodata,
                                   FILE              *stream) {
    assert(stream);

    fputs(";============================================\n"
          "; THIS FILE IS AUTOGENERATED BY KasTLe\n"
          "; DO NOT MODIFY IT\n"
          ";============================================\n\n", stream);
    fputs("section .data\n", stream);
    print_buffer(stream, data);
    fputc('\n', stream);

    fputs("section .rodata\n", stream);
    print_buffer(stream, rodata);
    fputc('\n', stream);

    fputs("section .text\n", stream);
    fputs("    extern printf\n"
          "    extern scanf\n"
          "    extern calloc\n"
          "    extern free\n", stream);
    print_buffer(stream, text);

    return KTL_OK;
}
