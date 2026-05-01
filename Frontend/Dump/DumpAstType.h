#ifndef DUMP_AST_TYPE_H
#define DUMP_AST_TYPE_H

#include "TokenType.h"
#include "StrMap.h"
#include "TypeMap.h"
#include "SymMap.h"
#include "ASTType.h"

struct KTL_DumpStats {
    int total;
    int raw_vars,    resolved_vars;
    int raw_calls,   resolved_calls;
    int exprs_total, exprs_typed;
    int bad_types;
};


struct KTL_DumpAstContext {
    KTL_StrMap     *str_map;
    KTL_TypeMap    *type_map;
    KTL_SymbolMap  *global_map;

    FILE           *stream;

    KTL_SymbolEntry **all_syms;
    int               n_syms;
    int               cap_syms;

    KTL_DumpStats stats;
    int           stage;
};



#endif /* DUMP_AST_TYPE_H */
