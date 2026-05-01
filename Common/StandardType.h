#ifndef STANDARD_TYPE_H
#define STANDARD_TYPE_H

#include "TypeMapType.h"

const KTL_TypeStandard KTL_STANDARD_TYPES[] = {
    {"bool", 1,  1},
    {"void", 0,  1},
    {"i32",  4,  4},
    {"i8",   1,  1},
    {"i16",  2,  2},
    {"i64",  8,  8},
};

constexpr int KTL_BOOL_TYPE_ID = 0;
constexpr int KTL_VOID_TYPE_ID = 1;
constexpr int KTL_I32_TYPE_ID  = 2;
constexpr int KTL_I8_TYPE_ID   = 3;

const KTL_TypeAlias KTL_STANDARD_ALIAS[] = {
    {"char", KTL_I8_TYPE_ID},
};
constexpr int KTL_CHAR_TYPE_ID = KTL_I8_TYPE_ID;

#endif /* STANDARD_TYE_H */
