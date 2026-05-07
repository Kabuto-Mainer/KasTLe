#ifndef BACKEND_H
#define BACKEND_H

#include "BackendType.h"
#include "ASTType.h"

KTL_Error KTL_BackendInit         (KTL_BackendContext *cont,
                                   KTL_TypeMap        *type_map,
                                   KTL_StrMap         *str_map,
                                   KTL_SymbolMap      *global_scope,
                                   KTL_BackIR_Buffer  *text,
                                   KTL_BackIR_Buffer  *data,
                                   KTL_BackIR_Buffer  *rodata);
KTL_Error KTL_BackendUninit       (KTL_BackendContext *cont);
KTL_Error KTL_BackendRun          (KTL_BackendContext *cont,
                                   KTL_AstNode        *root);
KTL_Error KTL_Backend_GenerateNasm(KTL_BackIR_Buffer *text,
                                   KTL_BackIR_Buffer *data,
                                   KTL_BackIR_Buffer *rodata,
                                   FILE              *stream);

#endif /* BACKEND_H */
