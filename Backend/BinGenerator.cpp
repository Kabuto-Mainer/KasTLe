#include <assert.h>

#include "OpCode.h"
#include "BackendType.h"

static void emit_istr(KTL_BackendContext *cont,
                      KTL_AsmInstraction  comand,
                      KTL_AsmValue        oper) {
    assert(cont);

    switch (comand) {

    case KTL_ASM_MOV:
    case KTL_ASM_MOVZX:
    case KTL_ASM_MOVSX:
    case KTL_ASM_CDQE:
    case KTL_ASM_CQO:
    case KTL_ASM_PUSH:
    case KTL_ASM_POP:
    case KTL_ASM_ADD:
    case KTL_ASM_SUB:
    case KTL_ASM_IMUL:
    case KTL_ASM_IDIV:
    case KTL_ASM_XOR:
    case KTL_ASM_AND:
    case KTL_ASM_NEG:
    case KTL_ASM_TEST:
    case KTL_ASM_CMP:
    case KTL_ASM_SETNE:
    case KTL_ASM_JMP:
    case KTL_ASM_JZ:
    case KTL_ASM_JNZ:
    case KTL_ASM_REP_STOSB:
    case KTL_AST_REP_MOVSB:
    case KTL_ASM_LEA:
    case KTL_ASM_RET:
    case KTL_ASM_SYSCALL:
    case KTL_ASM_CALL:
    }
}

static void emit_istr(KTL_BackendContext *cont,
                      KTL_AsmInstraction  comand,
                      KTL_AsmValue        oper_1,
                      KTL_AsmValue        oper_2) {
    assert(cont);

    switch (comand) {

    case KTL_ASM_MOV:
    case KTL_ASM_MOVZX:
    case KTL_ASM_MOVSX:
    case KTL_ASM_CDQE:
    case KTL_ASM_CQO:
    case KTL_ASM_PUSH:
    case KTL_ASM_POP:
    case KTL_ASM_ADD:
    case KTL_ASM_SUB:
    case KTL_ASM_IMUL:
    case KTL_ASM_IDIV:
    case KTL_ASM_XOR:
    case KTL_ASM_AND:
    case KTL_ASM_NEG:
    case KTL_ASM_TEST:
    case KTL_ASM_CMP:
    case KTL_ASM_SETNE:
    case KTL_ASM_JMP:
    case KTL_ASM_JZ:
    case KTL_ASM_JNZ:
    case KTL_ASM_REP_STOSB:
    case KTL_AST_REP_MOVSB:
    case KTL_ASM_LEA:
    case KTL_ASM_RET:
    case KTL_ASM_SYSCALL:
    case KTL_ASM_CALL:
    }
}

static void emit_istr(KTL_BackendContext *cont,
                      KTL_AsmInstraction  comand) {
    assert(cont);

    switch (comand) {

    case KTL_ASM_CDQE:
        emit_byte(cont, )

    case KTL_ASM_CQO:
    case KTL_ASM_REP_STOSB:
    case KTL_AST_REP_MOVSB:
    case KTL_ASM_RET:
    case KTL_ASM_NOP:
    case KTL_ASM_SYSCALL:
    }
}


static void emit_mov(KTL_BackendContext *cont,
                     )

