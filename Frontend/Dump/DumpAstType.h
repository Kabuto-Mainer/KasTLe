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

    FILE           *stream;

    KTL_SymbolEntry **all_syms;
    int               n_syms;
    int               cap_syms;
};



#endif /* DUMP_AST_TYPE_H */
