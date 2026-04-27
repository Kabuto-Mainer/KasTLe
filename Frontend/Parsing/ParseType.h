#ifndef PARSE_TYPE_H
#define PARSE_TYPE_H

#include "TokenType.h"
#include "StrMap.h"
#include "TypeMap.h"
#include "SymMap.h"
#include "ASTType.h"
#include "Diagnostic.h"

struct KTL_ParseContext {
    KTL_Token   *tokens;
    int          cur_token;
    int          cap_token;

    KTL_AstNode *root;

    KTL_StrMap  *str_map;
    KTL_TypeMap *type_map;

    /* Functions and Global Variables */
    KTL_SymbolMap  *global_map;

    KTL_SymbolMap  *current_scope;

    KTL_Diagnostic *diag;

    int  loop_depth;
    bool main_seen;
};

#endif /* PARSE_TYPE_H */
