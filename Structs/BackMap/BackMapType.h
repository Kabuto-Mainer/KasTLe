#ifndef BACK_MAP_TYPE_H
#define BACK_MAP_TYPE_H

#include "SymMapType.h"

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


#endif /* BACK_MAP_TYPE_H */
