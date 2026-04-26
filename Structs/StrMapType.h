#ifndef STR_MAP_TYPE_H
#define STR_MAP_TYPE_H

#include "Common.h"

struct KTL_StrList {
    char *string;
    KTL_Hash hash_list;
    KTL_StrList *next;
};

struct KTL_StrMap {
    KTL_Hash (*get_hash_list)(const char *string);
    KTL_Hash (*get_hash_cell)(const char *string);

    int size;

    KTL_StrList *data;
};

typedef char * KTL_StrID;
bool inline StrIDCheck(const KTL_StrID id) {
    return !!id;
}

constexpr KTL_StrID KTL_BAD_STR_ID = NULL;


#endif /* STR_MAP_TYPE_H */
