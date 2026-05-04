#ifndef BACKEND_TYPE_H
#define BACKEND_TYPE_H

#include <stdio.h>

#include "Common.h"
#include "StrMapType.h"
#include "TypeMapType.h"
#include "SymMapType.h"
#include "BackMapType.h"

enum KTL_RegId {
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

    KTL_REG_COUNT = 16,
    KTL_REG_INVALID = -1,
};

extern const KTL_RegId KTL_PARAM_REGS[6];
constexpr int KTL_PARAM_REGS_COUNT = 6;
constexpr int KTL_SYSTEM_PTR_SIZE = 8;


struct KTL_BackendContext {
    KTL_TypeMap   *type_map;
    KTL_StrMap    *str_map;
    KTL_SymbolMap *global_scope;

    KTL_BackendTable table;
    FILE            *output;

    KTL_BackendFuncInfo *current_func;

    int loop_label_break;
    int loop_label_continue;
    int label_counter;
    int stack_depth;

    int frame_offset;

    const char *symbol_prefix;  /* for MacOS users */
};

#define print_asm(_fmt_, ...)     fprintf(cont->output, _fmt_,  ##__VA_ARGS__)


#endif /* BACKEND_TYPE_H */
