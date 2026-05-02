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
    KTL_TypeID      currect_type_func;
    KTL_TypeID      currect_type_array;
    bool            in_func;
};

enum KTL_AnalysisConst {
    KTL_ANALYSIS_CONST_VALUE,
    KTL_ANALYSIS_CONST_PTR,
    KTL_ANALYSIS_MUTABLE,
};



#endif /* ANALYSES_TYPE_H */
