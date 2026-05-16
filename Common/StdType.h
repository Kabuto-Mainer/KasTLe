#ifndef STD_TYPE_H
#define STD_TYPE_H

#include "TypeMapType.h"

const KTL_TypeStandard KTL_STANDARD_TYPES[] = {
    {"bool", 1,  1},
    {"void", 0,  1},
    {"i8",   1,  1},
    {"i16",  2,  2},
    {"i32",  4,  4},
    {"i64",  8,  8},
};

constexpr int KTL_BOOL_TYPE_ID = 0;
constexpr int KTL_VOID_TYPE_ID = 1;
constexpr int KTL_I8_TYPE_ID   = 2;
constexpr int KTL_I16_TYPE_ID  = 3;
constexpr int KTL_I32_TYPE_ID  = 4;
constexpr int KTL_I64_TYPE_ID  = 5;

struct KTL_TypeStandardAlias {
    const char *name;
    const int   target;
};

const KTL_TypeStandardAlias KTL_STANDARD_ALIAS[] = {
    {"char", KTL_I8_TYPE_ID},
};
constexpr int KTL_CHAR_TYPE_ID = KTL_I8_TYPE_ID;

constexpr int KTL_STANDARD_MAX_PARAM = 6;


#define _VOID     {KTL_VOID_TYPE_ID,  0}

#define _I8       {KTL_I8_TYPE_ID,  0}
#define _I16      {KTL_I16_TYPE_ID, 0}
#define _I32      {KTL_I32_TYPE_ID, 0}
#define _I64      {KTL_I64_TYPE_ID, 0}

#define _I8_ptr   {KTL_I8_TYPE_ID,  1}
#define _I16_ptr  {KTL_I16_TYPE_ID, 1}
#define _I32_ptr  {KTL_I32_TYPE_ID, 1}
#define _I64_ptr  {KTL_I64_TYPE_ID, 1}





struct KTL_StandardFunc_Type {
    KTL_TypeID base_type;
    int        ptr_lvl;
};

struct KTL_StandardFunc_Gen {
    const char *f_name;
    const char *b_name;

    KTL_StandardFunc_Type r_type;
    KTL_StandardFunc_Type p_types[KTL_STANDARD_MAX_PARAM];

    int  amount_param;
    bool has_optional_param;
};



#endif /* STD_TYPE_H */
