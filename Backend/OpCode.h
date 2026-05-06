#ifndef OP_CODE_H
#define OP_CODE_H

#include <elf.h>

enum KTL_LinkReloc {
    KTL_REL_DIRECT,
    KTL_REL_PC32,
    KTL_REL_PLT32,
    KTL_REL_GOT,
};

enum KTL_AsmInstraction {
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
    KTL_ASM_IDIV,

    KTL_ASM_XOR,
    KTL_ASM_AND,
    KTL_ASM_NEG,

    KTL_ASM_TEST,
    KTL_ASM_CMP,
    KTL_ASM_SETNE,

    KTL_ASM_JMP,
    KTL_ASM_JZ,
    KTL_ASM_JNZ,

    KTL_ASM_REP_STOSB,
    KTL_AST_REP_MOVSB,
    KTL_ASM_LEA,
    KTL_ASM_RET,
    KTL_ASM_SYSCALL,
    KTL_ASM_CALL,
};


enum KTL_AsmOpcode {
    KTL_OP_MOV_RM8_R8,
    KTL_OP_MOV_R


    KTL_ASM_MOVZX,
    KTL_ASM_MOVSX,

    KTL_ASM_CDQE,
    KTL_ASM_CQO,

    KTL_ASM_PUSH,
    KTL_ASM_POP,

    KTL_ASM_ADD,
    KTL_ASM_SUB,
    KTL_ASM_IMUL,
    KTL_ASM_IDIV,

    KTL_ASM_XOR,
    KTL_ASM_AND,
    KTL_ASM_NEG,

    KTL_ASM_TEST,
    KTL_ASM_CMP,
    KTL_ASM_SETNE,

    KTL_ASM_JMP,
    KTL_ASM_JZ,
    KTL_ASM_JNZ,

    KTL_ASM_REP_STOSB,
    KTL_AST_REP_MOVSB,
    KTL_ASM_LEA,
    KTL_ASM_RET,
    KTL_ASM_SYSCALL,
    KTL_ASM_CALL,
};


enum KTL_AsmOperand {
    KTL_OPERAND_MEMORY,
    KTL_OPERAND_REGISTER,
    KTL_OPERAND_VALUE,
    KTL_OPERAND_SYMBOL,
};

struct KTL_AsmValue {
    KTL_AsmOperand kind;

    union {
        struct { KTL_RegId reg;   int size;   } reg;
        struct { int64_t   value; int size;   } value;
        struct { KTL_RegId base;  int offset; } mem;
        struct {
            KTL_StrID symbol;
            int64_t   added;
            KTL_LinkReloc reloc;
            int           size;
        } symbol;
    };
};




#endif /* OP_CODE_H */
