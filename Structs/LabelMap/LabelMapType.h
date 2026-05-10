#ifndef LABEL_MAP_TYPE_H
#define LABEL_MAP_TYPE_H

#include "StrMapType.h"
#include "BackIRType.h"

struct KTL_LabelDecl_Entry {
    KTL_StrID name;
    int       offset;
};

struct KTL_LabelDecl_Map {
    KTL_LabelDecl_Entry *data;
    int                  size;
    int                  capacity;
};

struct KTL_LabelFix_Entry {
    KTL_BackIR_SymbolKind kind;
    KTL_StrID             target;
    int32_t               index;        /* in KTL_BackIR_Buffer */
    int32_t               inner_offset; /* in KTL_BackIR_Item (byte_15) */
    int32_t               ads_offset;   /* from start of text */
    int32_t               size;
};

struct KTL_LabelFix_Map {
    KTL_LabelFix_Entry *data;
    int                 size;
    int                 capacity;
};



#endif /* LABEL_MAP_TYPE_H */
