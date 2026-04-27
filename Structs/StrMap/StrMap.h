#ifndef STRMAP_H
#define STRMAP_H

#include "StrMapType.h"

KTL_Error KTL_StrMapCreate      (KTL_StrMap *map,   int size);
KTL_StrID KTL_StrMapFind        (KTL_StrMap *map,   const char *string);
KTL_Error KTL_StrMapDestroy     (KTL_StrMap *map);


#endif /* STRMAP_H */

