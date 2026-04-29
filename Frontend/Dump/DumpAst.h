#ifndef DUMP_AST_H
#define DUMP_AST_H

#include "DumpAstType.h"

void KTL_AstDumpRaw(KTL_AstNode   *root,
                    KTL_StrMap    *str_map,
                    KTL_SymbolMap *global_map,
                    KTL_TypeMap   *type_map,
                    const char    *file);

#endif /* DUMP_AST_H */
