#ifndef TOKEN_TYPE_H
#define TOKEN_TYPE_H

#include "TokenEnum.h"
#include "StrMapType.h"

enum KTL_TokenStatus {
    KTL_TOKEN_THIS_OK,
    KTL_TOKEN_NOT_THIS,
    KTL_TOKEN_ERROR
};

struct KTL_SourcePos {
    int line;
    int column;
};

struct KTL_Token {
    KTL_TokenKind   kind;
    union {
        KTL_KeyWord key;
        KTL_Punct   punct;
        KTL_StrID   string;
        KTL_StrID   str_literal;
        int64_t     value;
    } data;

    KTL_SourcePos   pos;
};

struct KTL_KeyConstBlock {
    const char     *string;
    KTL_Hash        hash;
    KTL_KeyWord     value_key;
};


struct KTL_PunctConstBLock {
    const char  sym;
    KTL_Punct   value_punct;
};

struct KTL_PunctConst2Block {
    const char  sym[2];

    KTL_Punct   value_punct;
};

struct KTL_ParseTokenRef {
    KTL_TokenKind kind;            /* KTL_TOKEN_PUNCT или KTL_TOKEN_KEY */
    union {
        KTL_Punct   punct;
        KTL_KeyWord key;
    } as;
};


#endif /* TOKEN_TYPE_H */
