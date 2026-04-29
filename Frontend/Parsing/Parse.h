#ifndef PARSE_H
#define PARSE_H

#include "ParseType.h"

KTL_Error KTL_ParseInit     (KTL_ParseContext *cont,
                             KTL_Token        *tokens,
                             int               token_count,
                             KTL_StrMap       *str_map,
                             KTL_TypeMap      *type_map,
                             KTL_Diagnostic   *diag);

KTL_Error KTL_ParseProcess  (KTL_ParseContext *cont);

KTL_Error KTL_ParseUninit   (KTL_ParseContext *cont);

#endif /* PARSE_H */
