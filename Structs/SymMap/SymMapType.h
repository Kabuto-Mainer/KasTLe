#ifndef SYM_MAP_TYPE_H
#define SYM_MAP_TYPE_H

#include "Common.h"
#include "StrMapType.h"
#include "TypeMapType.h"

enum KTL_VarModifier {
    KTL_VAR_NONE      = 0x0000,

    /* User */
    KTL_VAR_CONST     = 0x0001,
    KTL_VAR_MUTABLE   = 0x0002,

    /* Parser and Analyze */
    KTL_VAR_INITIAL   = 0x0010,
    KTL_VAR_STACK     = 0x0008,
    KTL_VAR_REGISTER  = 0x0004,
};

enum KTL_SymbolEntryKind {
    KTL_SYMBOL_VAR,
    KTL_SYMBOL_FUNC,
};

struct KTL_SymbolEntry {
    KTL_StrID            str_id;
    KTL_SymbolEntryKind  kind;

    union {
        struct {
            KTL_TypeID type;
            int        mod;
        } var;

        struct {
            int               amount;
            KTL_SymbolEntry **params;
            KTL_TypeID        ret_type;
            bool              is_ret_const;
            bool              is_external;
            bool              has_optional;
        } func;
    };
};

struct KTL_SymbolMap {
    KTL_SymbolMap     *parent;
    KTL_SymbolEntry  **data;
    int                size;
    int                capacity;
};

#endif /* SYM_MAP_TYPE_H */
