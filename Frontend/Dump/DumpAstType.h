#ifndef DUMP_AST_TYPE_H
#define DUMP_AST_TYPE_H

#include "TokenType.h"
#include "StrMap.h"
#include "TypeMap.h"
#include "SymMap.h"
#include "ASTType.h"

struct KTL_DumpAstContext {
    KTL_StrMap     *str_map;
    KTL_TypeMap    *type_map;
    KTL_SymbolMap  *global_map;

    KTL_AstNode    *cur_node;
    FILE           *stream;
};



#endif /* DUMP_AST_TYPE_H */
