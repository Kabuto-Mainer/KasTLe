#ifndef BACKEND_H
#define BACKEND_H

#include "BackendType.h"
#include "ASTType.h"

KTL_Error KTL_BackendInit  (KTL_BackendContext *cont,
                            KTL_TypeMap        *type_map,
                            KTL_StrMap         *str_map,
                            KTL_SymbolMap      *global_scope,
                            FILE               *output);

KTL_Error KTL_BackendUninit(KTL_BackendContext *cont);
KTL_Error KTL_BackendRun(KTL_BackendContext *cont, KTL_AstNode *root);


#endif /* BACKEND_H */
