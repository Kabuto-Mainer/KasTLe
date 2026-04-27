#ifndef TOKEN_H
#define TOKEN_H

#include "TokenType.h"
#include "TokenConfig.h"

KTL_Error   KTL_TokenInit       (KTL_TokenContext *cont,    const char *file);
void        KTL_TokenAddStrMap  (KTL_TokenContext *cont,    KTL_StrMap *map);
KTL_Error   KTL_TokenProcess    (KTL_TokenContext *cont);
KTL_Error   KTL_TokenUninit     (KTL_TokenContext *cont);
void        KTL_TokenDump       (KTL_TokenContext *cont);

#endif /* TOKEN_H */
