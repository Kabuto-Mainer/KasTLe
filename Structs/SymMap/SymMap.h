#ifndef SYM_MAP_H
#define SYM_MAP_H

#include "SymMapType.h"

KTL_SymbolMap * KTL_SymbolMapInit  (KTL_SymbolMap *parent);
KTL_Error       KTL_SymbolMapUninit(KTL_SymbolMap *map);


KTL_SymbolEntry * KTL_SymbolInsertVar (KTL_SymbolMap *map,
                                       KTL_StrID name,
                                       KTL_TypeID type,
                                       int mod);

KTL_SymbolEntry * KTL_SymbolInsertParam(KTL_SymbolMap *map,
                                        KTL_StrID name,
                                        KTL_TypeID type,
                                        int mod);

KTL_SymbolEntry * KTL_SymbolInsertFunc(KTL_SymbolMap *map,
                                       KTL_StrID name,
                                       KTL_TypeID ret_type);

KTL_Error KTL_SymbolFuncSetParams(KTL_SymbolEntry *func,
                                  KTL_SymbolEntry **params,
                                  int amount);

KTL_SymbolEntry * KTL_SymbolFindLocal(const KTL_SymbolMap *map, KTL_StrID name);

KTL_SymbolEntry * KTL_SymbolFind(const KTL_SymbolMap *map, KTL_StrID name);

#endif /* SYM_MAP_H */
