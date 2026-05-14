#ifndef BACK_IR_TYPE_H
#define BACK_IR_TYPE_H

#include <stdint.h>
#include "StrMapType.h"


// =====================================================================

/* Used Assembly Instructions */
enum KTL_AsmInstr {
    KTL_ASM_MOV,
    KTL_ASM_MOVZX,
    KTL_ASM_MOVSX,

    KTL_ASM_CDQE,
    KTL_ASM_CQO,

    KTL_ASM_PUSH,
    KTL_ASM_POP,

    KTL_ASM_ADD,
    KTL_ASM_SUB,
    KTL_ASM_IMUL,
    KTL_ASM_IDIV1,

    KTL_ASM_XOR,
    KTL_ASM_AND,
    KTL_ASM_NEG,

    KTL_ASM_TEST,
    KTL_ASM_CMP,

    KTL_ASM_SETNE,
    KTL_ASM_SETGE,
    KTL_ASM_SETG,
    KTL_ASM_SETLE,
    KTL_ASM_SETL,
    KTL_ASM_SETE,

    KTL_ASM_JMP,
    KTL_ASM_JZ,
    KTL_ASM_JNZ,

    KTL_ASM_CALL,
    KTL_ASM_CALL_PLT,
    KTL_ASM_LEA,
    KTL_ASM_SYSCALL,

    KTL_ASM_REP_STOSB,
    KTL_ASM_REP_MOVSB,
    KTL_ASM_RET,

    KTL_ASM_DEBUG,
};

/* System Registers */
enum KTL_RegID {
    KTL_REG_RAX = 0,
    KTL_REG_RCX = 1,
    KTL_REG_RDX = 2,
    KTL_REG_RBX = 3,
    KTL_REG_RSP = 4,
    KTL_REG_RBP = 5,
    KTL_REG_RSI = 6,
    KTL_REG_RDI = 7,
    KTL_REG_R8  = 8,
    KTL_REG_R9  = 9,
    KTL_REG_R10 = 10,
    KTL_REG_R11 = 11,
    KTL_REG_R12 = 12,
    KTL_REG_R13 = 13,
    KTL_REG_R14 = 14,
    KTL_REG_R15 = 15,

    KTL_REG_COUNT   = 16,
    KTL_REG_INVALID = -1,
};


// =====================================================================
/* Operand                                                              */
// =====================================================================

/* ELF relocation type */
enum KTL_BackIR_SymbolKind {
    KTL_BACK_IR_SYM_LOCAL_FUNC,
    KTL_BACK_IR_SYM_GOT_FUNC,
    KTL_BACK_IR_SYM_LOCAL_VAR,
    KTL_BACK_IR_SYM_GOT_VAR,
};

/* Operand kind */
enum KTL_BackIR_OperandKind {
    KTL_BACK_IR_OP_REG,
    KTL_BACK_IR_OP_IMM,
    KTL_BACK_IR_OP_SYMBOL,
    KTL_BACK_IR_OP_MEM,
    KTL_BACK_IR_OP_MEM_RIP,
    KTL_BACK_IR_OP_LABEL,
};

/* Operand */
struct KTL_BackIR_InstrOperand {
    KTL_BackIR_OperandKind kind;
    union {
        struct {
            KTL_RegID reg;
            uint8_t   size;
        } reg;
        struct {
            int64_t imm;
            uint8_t size;
        } imm;
        struct {
            KTL_StrID             sym;
            KTL_BackIR_SymbolKind kind;
            int                   size;
        } sym;
        struct {
            KTL_RegID base;
            KTL_RegID idx;
            uint8_t   scale;
            int       offset;
            int       size;
        } mem;
        struct {
            KTL_StrID             sym;
            KTL_BackIR_SymbolKind kind;
            uint8_t               size;
        } mem_rip;
        struct {
            KTL_StrID name;
        } label;
    };
};


/* Supported directives */
enum KTL_BackIR_Directive {
    KTL_BACK_IR_DIR_ALIGN,
};

/* Data item subkind */
enum KTL_BackIR_DataKind {
    KTL_BACK_IR_DATA_ZERO,
    KTL_BACK_IR_DATA_INT,
    KTL_BACK_IR_DATA_BYTES,
    KTL_BACK_IR_DATA_SYMBOL,
};

/* Item kind */
enum KTL_BackIR_ItemKind {
    KTL_BACK_IR_ITEM_INSTR,
    KTL_BACK_IR_ITEM_LABEL,
    KTL_BACK_IR_ITEM_COMMENT,
    KTL_BACK_IR_ITEM_DIRECTIVE,
    KTL_BACK_IR_ITEM_DATA,
    KTL_BACK_IR_ITEM_BYTE_15
};

/* Backend IR item */
struct KTL_BackIR_Item {
    KTL_BackIR_ItemKind kind;
    union {
        struct {
            KTL_AsmInstr cmd;
            int          amount_args;
            union {
                KTL_BackIR_InstrOperand one;
                struct {
                    KTL_BackIR_InstrOperand dst;
                    KTL_BackIR_InstrOperand src;
                } two;
            };
        } instr;

        struct {
            KTL_StrID name;
            bool      is_global;
        } label_decl;

        struct {
            KTL_StrID text;
        } comment;

        struct {
            KTL_BackIR_Directive kind;
            union {
                struct { int size; } align;
            };
        } direct;

        struct {
            KTL_BackIR_DataKind kind;
            union {
                struct { int       count;                  } zero;
                struct { int64_t   value;   uint8_t  size; } int_val;
                struct { KTL_StrID bytes;   int      len;  } bytes;
                struct {
                    KTL_StrID             sym;
                    KTL_BackIR_SymbolKind sym_kind;
                    int64_t               addend;
                    uint8_t               size;
                } symbol;
            };
        } data;

        struct {
            uint8_t byte[15];
            uint8_t len;
        } byte_15;
    };
};


// =====================================================================

/* Buffer */
struct KTL_BackIR_Buffer {
    KTL_BackIR_Item *data;
    int              size;
    int              capacity;
};


#endif /* BACK_IR_TYPE_H */
