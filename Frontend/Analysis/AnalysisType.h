#ifndef ANALYSIS_TYPE_H
#define ANALYSIS_TYPE_H

#include "StrMap.h"
#include "TypeMap.h"
#include "SymMap.h"
#include "ASTType.h"
#include "Diagnostic.h"

struct KTL_AnalysisContext {
    KTL_StrMap     *str_map;
    KTL_TypeMap    *type_map;
    KTL_SymbolMap  *global_scope;
    KTL_Diagnostic *diag;

    KTL_AstNode    *root;
    KTL_SymbolMap  *current_scope;
};


#endif /* ANALYSES_TYPE_H */
