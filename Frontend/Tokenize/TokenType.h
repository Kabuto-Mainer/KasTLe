#ifndef KTL_TOKEN_TYPE_H
#define KTL_TOKEN_TYPE_H

#include "TokenEnum.h"
#include "StrMapType.h"
#include "Diagnostic.h"

enum KTL_TokenStatus {
    KTL_TOKEN_THIS_OK,
    KTL_TOKEN_NOT_THIS,
    KTL_TOKEN_ERROR
};

struct KTL_Token {
    KTL_TokenKind kind;
    union {
        KTL_KeyWord key;
        KTL_Punct punct;
        KTL_StrID string;
        int64_t value;
    } data;

    KTL_SourcePos pose;
};

struct KTL_TokenContext {
    char *buffer;

    KTL_SourcePos file_pose;
    KTL_StrID file;

    int cur_pose;
    int capacity_buf;

    KTL_Token *tokens;
    int cur_token;
    int capacity_token;

    KTL_StrMap *str_map;
};

struct KTL_KeyConstBlock {
    const char *key;
    KTL_Hash hash;
    KTL_KeyWord value;
};


struct KTL_PunctConstBLock {
    const char sym;
    KTL_Punct value;
};


#endif /* KTL_TOKEN_TYPE_H */
