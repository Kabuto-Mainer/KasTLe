#ifndef ANALYSIS_H
#define ANALYSIS_H

#include "AnalysisType.h"

KTL_Error KTL_AnalysisInit(KTL_AnalysisContext *cont,
                           KTL_StrMap          *str_map,
                           KTL_TypeMap         *type_map,
                           KTL_SymbolMap       *global_scope,
                           KTL_Diagnostic      *diag,
                           KTL_AstNode         *root);

KTL_Error KTL_AnalysisProcess(KTL_AnalysisContext *cont);


#endif /* ANALYSIS_H */
