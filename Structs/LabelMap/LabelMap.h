#ifndef LABEL_MAP_H
#define LABEL_MAP_H

#include "LabelMapType.h"

KTL_Error KTL_LabelDecl_Init  (KTL_LabelDecl_Map *map);
KTL_Error KTL_LabelFix_Init   (KTL_LabelFix_Map  *map);
KTL_Error KTL_LabelDecl_Uninit(KTL_LabelDecl_Map *map);
KTL_Error KTL_LabelFix_Uninit (KTL_LabelFix_Map  *map);

void KTL_LabelDecl_Add        (KTL_LabelDecl_Map *map,
                               KTL_StrID          name,
                               int                offset);
void KTL_LabelFix_AddLocal    (KTL_LabelFix_Map *map,
                               KTL_StrID         target,
                               int32_t           offset,
                               int32_t           size);
void KTL_LabelFix_AddGlobal   (KTL_LabelFix_Map *map,
                               KTL_BackIR_SymbolKind kind,
                               KTL_StrID         target,
                               int32_t           offset,
                               int32_t           size);



#endif /* LABEL_MAP_H */
