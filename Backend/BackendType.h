#ifndef BACKEND_TYPE_H
#define BACKEND_TYPE_H

#include <stdio.h>

#include "Common.h"
#include "StrMapType.h"
#include "TypeMapType.h"
#include "SymMapType.h"

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


enum KTL_BackendStorage {
    KTL_BACKEND_STORAGE_NONE = 0,
    KTL_BACKEND_STORAGE_STACK,
    KTL_BACKEND_STORAGE_STATIC,
};

struct KTL_BackendVarInfo {
    KTL_SymbolEntry    *origin;
    KTL_BackendStorage  storage;

    union {
        struct { int       offset; } stack;
        struct { KTL_StrID label;  } stat;
    } loc;
};

struct KTL_BackendFuncInfo {
    KTL_SymbolEntry *origin;
    KTL_StrID        label;
    int              frame_size; /* mod 16 = 0 */

    int label_counter;
};


struct KTL_BackendTable {
    KTL_BackendVarInfo *vars;
    int                 vars_size;
    int                 vars_capacity;

    KTL_BackendFuncInfo *funcs;
    int                  funcs_size;
    int                  funcs_capacity;
};

struct KTL_BackendContext {
    KTL_TypeMap   *type_map;
    KTL_StrMap    *str_map;
    KTL_SymbolMap *global_scope;

    KTL_BackendTable table;
    FILE            *output;

    KTL_BackendFuncInfo *current_func;

    int loop_label_break;
    int loop_label_continue;
    int string_counter;

    int frame_offset;

    const char *symbol_prefix;  /* for MacOS users */
};

#endif /* BACKEND_TYPE_H */
