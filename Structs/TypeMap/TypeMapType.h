#ifndef TYPE_MAP_TYPE_H
#define TYPE_MAP_TYPE_H

#include "Common.h"
#include "StrMapType.h"

typedef int32_t KTL_TypeID;
constexpr KTL_TypeID KTL_BAD_TYPE_ID = -1;

// =======================================================================
enum KTL_TypeEntryKind {
    KTL_TYPE_BASE,
    KTL_TYPE_BLOCK,
    KTL_TYPE_PTR,
    KTL_TYPE_ARRAY,
};

enum KTL_TypeCastKind {
    KTL_TYPE_CAST_SAME,
    KTL_TYPE_CAST_IMPLICIT,
    KTL_TYPE_CAST_INCOMPATIBLE,
};

// =======================================================================
struct KTL_TypeField {
    KTL_TypeID base_type;
    int        offset;
    KTL_StrID  name;
};

struct KTL_TypeBase {
    KTL_StrID name;
    int       size;
    int       align;
};

struct KTL_TypeBlock {
    KTL_TypeField *fields;
    int            field_cap;
    int            field_count;
    KTL_StrID      name;

    int            size;
    int            align;
    bool           complete;
};

struct KTL_TypePointer {
    KTL_TypeID prev_type;
};

struct KTL_TypeArray {
    int        elem_count;
    KTL_TypeID base_type;
};

struct KTL_TypeEntry {
    union {
        KTL_TypeBase    base;
        KTL_TypeBlock   block;
        KTL_TypePointer ptr;
        KTL_TypeArray   arr;
    } dt;

    KTL_TypeEntryKind kind;
};

struct KTL_TypeAlias {
    KTL_StrID  name;
    KTL_TypeID target;
};

struct KTL_TypeMap {
    KTL_TypeEntry *data;
    int            size;
    int            capacity;

    KTL_TypeAlias *aliases;
    int            alias_size;
    int            alias_capacity;
};

struct KTL_TypeStandard {
    const char *name;
    int         size;
    int         align;
};

struct KTL_AliasStandard {
    const char *name;
    int         size;
    int         align;
};

bool inline TypeIDCheck(const KTL_TypeID id) {
    return id >= 0;
}

bool inline TypeIDCheck(const KTL_TypeMap *map, const KTL_TypeID id) {
    return id >= 0 && id < map->size;
}

#endif /* TYPE_MAP_TYPE_H */
