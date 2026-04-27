#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "SymMap.h"

constexpr static int KTL_SYM_INITIAL_CAP = 8;
constexpr static int KTL_SYM_GROW_MOD    = 2;

// =======================================================================
// HELPER FUNCTIONS DECLARATION
// =======================================================================

static KTL_Error          ktl_sym_grow         (KTL_SymbolMap *map);
static KTL_SymbolEntry *  ktl_sym_alloc_entry  (KTL_StrID            name,
                                                KTL_SymbolEntryKind  kind);
static KTL_Error          ktl_sym_push         (KTL_SymbolMap   *map,
                                                KTL_SymbolEntry *entry);

// =======================================================================
// API FUNCTIONS
// =======================================================================

KTL_SymbolMap * KTL_SymbolMapInit(KTL_SymbolMap *parent) {
    // there is not 'assert(parent)' because global_map has not parent

    KTL_SymbolMap *map = (KTL_SymbolMap *)calloc(1, sizeof(KTL_SymbolMap));
    if (map == NULL)    ExitF("NULL Calloc", NULL);

    map->data = (KTL_SymbolEntry **)calloc(KTL_SYM_INITIAL_CAP,
                                           sizeof(KTL_SymbolEntry *));
    if (map->data == NULL) {
        free(map);
        ExitF("NULL Calloc", NULL);
    }

    map->parent   = parent;
    map->size     = 0;
    map->capacity = KTL_SYM_INITIAL_CAP;

    return map;
}

KTL_Error KTL_SymbolMapUninit(KTL_SymbolMap *map) {
    if (map == NULL)    return KTL_OK;

    for (int i = 0; i < map->size; i++) {
        KTL_SymbolEntry *entry = map->data[i];
        if (entry == NULL)  continue;

        if (entry->kind == KTL_SYMBOL_FUNC) {
            free(entry->func.params);
        }
        free(entry);
    }
    free(map->data);
    free(map);

    return KTL_OK;
}

KTL_SymbolEntry * KTL_SymbolInsertVar(KTL_SymbolMap *map,
                                      KTL_StrID      name,
                                      KTL_TypeID     type,
                                      int            mod) {
    assert(map);
    assert(StrIDCheck(name));
    assert(TypeIDCheck(type));

    if (KTL_SymbolFindLocal(map, name, KTL_SYMBOL_VAR) != NULL)  return NULL;

    KTL_SymbolEntry *entry = ktl_sym_alloc_entry(name, KTL_SYMBOL_VAR);
    if (entry == NULL)  return NULL;

    entry->var.type = type;
    entry->var.mod  = mod;

    if (ktl_sym_push(map, entry) != KTL_OK) {
        free(entry);
        return NULL;
    }

    return entry;
}

KTL_SymbolEntry * KTL_SymbolInsertFunc(KTL_SymbolMap *map,
                                       KTL_StrID      name,
                                       KTL_TypeID     ret_type) {
    assert(map);
    assert(StrIDCheck(name));
    assert(TypeIDCheck(ret_type));

    if (KTL_SymbolFindLocal(map, name, KTL_SYMBOL_FUNC) != NULL)  return NULL;

    KTL_SymbolEntry *entry = ktl_sym_alloc_entry(name, KTL_SYMBOL_FUNC);
    if (entry == NULL)  return NULL;

    entry->func.amount   = 0;
    entry->func.params   = NULL;
    entry->func.ret_type = ret_type;

    if (ktl_sym_push(map, entry) != KTL_OK) {
        free(entry);
        return NULL;
    }

    return entry;
}

KTL_Error KTL_SymbolFuncSetParams(KTL_SymbolEntry  *func,
                                  KTL_SymbolEntry **params,
                                  int               amount) {
    assert(func);
    assert(func->kind == KTL_SYMBOL_FUNC);
    assert(amount >= 0);

    if (amount == 0) {
        func->func.params = NULL;
        func->func.amount = 0;
        return KTL_OK;
    }
    assert(params);

    KTL_SymbolEntry **buf = (KTL_SymbolEntry **)calloc((size_t) amount,
                                          sizeof(KTL_SymbolEntry *));
    if (buf == NULL)    ExitF("NULL Calloc", KTL_MEMORY_ERR);

    memcpy(buf, params, (size_t) amount * sizeof(KTL_SymbolEntry *));
    func->func.params = buf;
    func->func.amount = amount;

    return KTL_OK;
}

KTL_SymbolEntry * KTL_SymbolFindLocal(const KTL_SymbolMap *map,
                                      KTL_StrID            name,
                                      KTL_SymbolEntryKind  kind) {
    assert(map);
    assert(StrIDCheck(name));

    for (int i = 0; i < map->size; i++) {
        if (map->data[i]->str_id == name &&
            map->data[i]->kind   == kind) {
            return map->data[i];
        }
    }

    return NULL;
}

KTL_SymbolEntry * KTL_SymbolFind(const KTL_SymbolMap *map,
                                 KTL_StrID            name,
                                 KTL_SymbolEntryKind  kind) {
    assert(StrIDCheck(name));

    for (const KTL_SymbolMap *cur = map; cur != NULL; cur = cur->parent) {
        KTL_SymbolEntry *entry = KTL_SymbolFindLocal(cur, name, kind);
        if (entry != NULL)  return entry;
    }

    return NULL;
}

// =======================================================================
// HELPER FUNCTIONS
// =======================================================================

static KTL_Error ktl_sym_grow(KTL_SymbolMap *map) {
    assert(map);

    int new_cap = map->capacity * KTL_SYM_GROW_MOD;
    KTL_SymbolEntry **buf = (KTL_SymbolEntry **)realloc(map->data,
                            (size_t) new_cap * sizeof(KTL_SymbolEntry *));
    if (buf == NULL)    ExitF("NULL Realloc", KTL_MEMORY_ERR);

    map->data     = buf;
    map->capacity = new_cap;

    return KTL_OK;
}

static KTL_SymbolEntry * ktl_sym_alloc_entry(KTL_StrID            name,
                                             KTL_SymbolEntryKind  kind) {
    assert(StrIDCheck(name));

    KTL_SymbolEntry *entry = (KTL_SymbolEntry *)calloc(1,
                                                       sizeof(KTL_SymbolEntry));
    if (entry == NULL)  ExitF("NULL Calloc", NULL);

    entry->str_id = name;
    entry->kind   = kind;

    return entry;
}

static KTL_Error ktl_sym_push(KTL_SymbolMap *map, KTL_SymbolEntry *entry) {
    assert(map);
    assert(entry);

    if (map->size == map->capacity) {
        if (ktl_sym_grow(map) != KTL_OK)    return KTL_MEMORY_ERR;
    }
    map->data[map->size++] = entry;

    return KTL_OK;
}
