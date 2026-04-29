#ifndef TOKEN_H
#define TOKEN_H

#include "TokenType.h"
#include "TokenConfig.h"
#include "Diagnostic.h"

struct KTL_TokenContext {
    KTL_StrID       source_id;
    KTL_SourcePos   source_pos;

    char           *buffer;
    int             buffer_pos;
    int             buffer_capacity;

    KTL_Token      *tokens;
    int             token_pos;
    int             token_capacity;

    KTL_StrMap     *str_map;
    KTL_Diagnostic *diag;
};

KTL_Error   KTL_TokenInit       (KTL_TokenContext *cont,    const char *file);
void        KTL_TokenAddStrMap  (KTL_TokenContext *cont,    KTL_StrMap *map);
void        KTL_TokenAddDiag    (KTL_TokenContext *cont,    KTL_Diagnostic *diag);
KTL_Error   KTL_TokenProcess    (KTL_TokenContext *cont);
KTL_Error   KTL_TokenUninit     (KTL_TokenContext *cont);
void        KTL_TokenDump       (KTL_TokenContext *cont);

#endif /* TOKEN_H */
