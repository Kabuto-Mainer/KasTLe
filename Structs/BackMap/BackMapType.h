#ifndef BACK_MAP_H
#define BACK_MAP_H

#include "SymMapType.h"

enum KTL_BackendStorage {
    KTL_BACKEND_STORAGE_NONE,
    KTL_BACKEND_STORAGE_STACK,
    KTL_BACKEND_STORAGE_REGISTER,
    KTL_BACKEND_STORAGE_GLOBAL,
    KTL_BACKEND_STORAGE_PARAM_REG,
    KTL_BACKEND_STORAGE_PARAM_STACK,
};

struct KTL_BackendVarInfo {
    KTL_SymbolEntry   *origin;
    KTL_BackendStorage storage;

    union {
        struct { int       offset; } stack;
        struct { int       reg_id; } reg;
        struct { KTL_StrID label;  } stat;
    } loc;
};

struct KTL_BackendFuncInfo {
    KTL_SymbolEntry *origin;
    KTL_StrID        label;
    int              frame_size;
    int              label_counter;
};

struct KTL_BackendTable {
    KTL_BackendVarInfo *vars;
    int                 var_size;
    int                 var_capacity;

    KTL_BackendFuncInfo *funcs;
    int                  func_size;
    int                  func_capacity;
};

struct KTL_BackendAllocVar {
    KTL_SymbolEntry  *origin;
    bool              has_ptr;
    bool              can_be_reg;
    int               used;
    union {
        int           reg_id;
        int           stack_offset;
    } place;
};

struct KTL_BackendAllocTable {
    KTL_BackendAllocVar *vars;
    int                  capacity;
    int                  size;
};


#endif /* BACK_MAP_H */
