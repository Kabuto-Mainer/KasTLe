#ifndef SYM_MAP_TYPE_H
#define SYM_MAP_TYPE_H

#include "Common.h"
#include "StrMapType.h"
#include "TypeMapType.h"

enum KTL_VarModifier {
    KTL_VAR_CONST           = 0x0001,
    KTL_VAR_REGISTER        = 0x0002,
    KTL_VAR_INITIAL         = 0x0004,
};

enum KTL_SymbolEntryKind {
    KTL_SYMBOL_VAR,
    KTL_SYMBOL_FUNC,
    KTL_SYMBOL_PARAM,
};

struct KTL_ParamInfo {
    KTL_TypeID type_id;
    int mod;
};

struct KTL_SymbolEntry {
    KTL_StrID str_id;
    KTL_SymbolEntryKind kind;

    union {
        struct {
            KTL_TypeID type;
            int mod;
        } var;

        struct {
            int amount;
            KTL_SymbolEntry **params;
            KTL_TypeID ret_type;
        } func;
    };

};


struct KTL_SymbolMap {
    KTL_SymbolMap *parent;

    int size;
    int capacity;

    KTL_SymbolEntry *data;
};






#endif /* SYM_MAP_TYPE_H */
