#ifndef PARSE_TYPE_H
#define PARSE_TYPE_H

#include "TokenType.h"
#include "StrMap.h"
#include "ASTType.h"

struct KTL_ParseContext {
    KTL_Token *tokens;
    int cur_token;
    int cap_token;

    KTL_AstNode *root;

    KTL_StrMap *str_map;
    KTL_TypeMap *type_map;
    KTL_SymbolMap *func_map;
    KTL_SymbolMap *global_map;
};

#endif /* PARSE_TYPE_H */
