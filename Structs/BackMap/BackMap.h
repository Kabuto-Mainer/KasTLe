#ifndef BACK_MAP_H
#define BACK_MAP_H

#include "BackMapType.h"

KTL_Error KTL_BackendTableInit   (KTL_BackendTable *table);
KTL_Error KTL_BackendTableUninit (KTL_BackendTable *table);

KTL_Error KTL_BackendAddStackVar (KTL_BackendTable *table,
                                  KTL_SymbolEntry  *origin,
                                  int offset);
KTL_Error KTL_BackendAddStaticVar(KTL_BackendTable *table,
                                  KTL_SymbolEntry  *origin,
                                  KTL_StrID         label);
KTL_Error KTL_BackendAddFunc     (KTL_BackendTable *table,
                                  KTL_SymbolEntry  *origin,
                                  KTL_StrID         label);

KTL_BackendVarInfo * KTL_BackendFindVar  (KTL_BackendTable *table,
                                          KTL_SymbolEntry  *origin);
KTL_BackendFuncInfo * KTL_BackendFindFunc(KTL_BackendTable *table,
                                          KTL_SymbolEntry  *origin);


#endif /* BACK_MAP_H */
