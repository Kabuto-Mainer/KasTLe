#ifndef BACKEND_H
#define BACKEND_H

#include "BackendType.h"
#include "ASTType.h"

KTL_Error KTL_BackendInit  (KTL_BackendContext *bctx,
                            KTL_TypeMap        *type_map,
                            KTL_StrMap         *str_map,
                            KTL_SymbolMap      *global_scope,
                            FILE               *output);

KTL_Error KTL_BackendUninit(KTL_BackendContext *cont);
KTL_Error KTL_BackendRun(KTL_BackendContext *cont, KTL_AstNode *root);

// =======================================================================
// API
// =======================================================================

KTL_Error KTL_BackendTableInit  (KTL_BackendTable *table);
KTL_Error KTL_BackendTableUninit(KTL_BackendTable *table);

KTL_Error KTL_BackendAddStackVar (KTL_BackendTable *table,
                                  KTL_SymbolEntry  *origin,
                                  int offset);

KTL_Error KTL_BackendAddStaticVar(KTL_BackendTable *table,
                                  KTL_SymbolEntry  *origin,
                                  KTL_StrID         label);

KTL_Error KTL_BackendAddFunc     (KTL_BackendTable *table,
                                  KTL_SymbolEntry  *origin,
                                  KTL_StrID         label);

KTL_BackendVarInfo  * KTL_BackendFindVar (KTL_BackendTable *table,
                                          KTL_SymbolEntry  *origin);

KTL_BackendFuncInfo * KTL_BackendFindFunc(KTL_BackendTable *table,
                                          KTL_SymbolEntry  *origin);


const char * KTL_BackendRegName(KTL_RegId reg, int size);
const char * KTL_BackendSizePrefix(int size);

#endif /* BACKEND_H */
