#include <stdlib.h>
#include <assert.h>

#include "Common.h"
#include "LabelMap.h"



constexpr int KTL_START_LABEL_DECL_SIZE = 10;
constexpr int KTL_START_LABEL_FIX_SIZE  = 10;

constexpr int KTL_LABEL_DECL_GROW_MOD = 2;
constexpr int KTL_LABEL_FIX_GROW_MOD  = 2;

// =======================================================================
// HELPER DECLARATION
// =======================================================================

static void grow_decl(KTL_LabelDecl_Map *map);
static void grow_fix (KTL_LabelFix_Map *map);

// =======================================================================
// API
// =======================================================================

KTL_Error KTL_LabelDecl_Init(KTL_LabelDecl_Map *map) {
    assert(map);

    map->data = (KTL_LabelDecl_Entry *)calloc(KTL_START_LABEL_DECL_SIZE,
                                sizeof(KTL_LabelDecl_Entry));
    if (map->data == NULL)  ExitF("NULL Calloc", KTL_MEMORY_ERR);

    map->capacity = KTL_START_LABEL_DECL_SIZE;
    map->size     = 0;

    return KTL_OK;
}

KTL_Error KTL_LabelFix_Init(KTL_LabelFix_Map *map) {
    assert(map);

    map->data = (KTL_LabelFix_Entry *)calloc(KTL_START_LABEL_FIX_SIZE,
                                sizeof(KTL_LabelFix_Entry));
    if (map->data == NULL)  ExitF("NULL Calloc", KTL_MEMORY_ERR);

    map->capacity = KTL_START_LABEL_FIX_SIZE;
    map->size     = 0;

    return KTL_OK;
}

KTL_Error KTL_LabelDecl_Uninit(KTL_LabelDecl_Map *map) {
    assert(map);

    if (map->data != NULL)  free(map->data);
    map->data     = NULL;
    map->capacity = 0;
    map->size     = 0;

    return KTL_OK;
}

KTL_Error KTL_LabelFix_Uninit(KTL_LabelFix_Map *map) {
    assert(map);

    if (map->data != NULL)  free(map->data);
    map->data     = NULL;
    map->capacity = 0;
    map->size     = 0;

    return KTL_OK;
}

void KTL_LabelDecl_Add(KTL_LabelDecl_Map *map,
                       KTL_StrID          name,
                       int                offset) {
    assert(map);
    assert(StrIDCheck(name));

    grow_decl(map);

    KTL_LabelDecl_Entry e = {};
    e.name   = name;
    e.offset = offset;

    map->data[map->size++] = e;
    return ;
}

void KTL_LabelFix_AddLocal(KTL_LabelFix_Map *map,
                           KTL_StrID         target,
                           int32_t           index,
                           int32_t           inner_offset,
                           int32_t           ads_offset,
                           int32_t           size) {
    assert(map);
    assert(StrIDCheck(target));

    grow_fix(map);

    KTL_LabelFix_Entry e = {};
    e.kind   = (KTL_BackIR_SymbolKind) -1;
    e.index  = index;
    e.inner_offset       = inner_offset;
    e.ads_offset         = ads_offset;
    e.target = target;
    e.size   = size;

    map->data[map->size++] = e;
    return ;
}

void KTL_LabelFix_AddGlobal(KTL_LabelFix_Map *map,
                            KTL_BackIR_SymbolKind kind,
                            KTL_StrID         target,
                            int32_t           index,
                            int32_t           inner_offset,
                            int32_t           ads_offset,
                            int32_t           size) {
    assert(map);
    assert(StrIDCheck(target));

    grow_fix(map);

    KTL_LabelFix_Entry e = {};
    e.kind               = kind;
    e.index              = index;
    e.inner_offset       = inner_offset;
    e.ads_offset         = ads_offset;
    e.target             = target;
    e.size               = size;

    map->data[map->size++] = e;
    return ;
}

KTL_LabelDecl_Entry *KTL_LabelDecl_Find(KTL_LabelDecl_Map *map, KTL_StrID name) {
    assert(map);
    assert(StrIDCheck(name));

    for (int i = 0; i < map->size; i++) {
        if (map->data[i].name == name)  return map->data + i;
    }
    return NULL;
}


static void grow_decl(KTL_LabelDecl_Map *map) {
    assert(map);

    if (map->size == map->capacity) {
        KTL_LabelDecl_Entry *buffer = (KTL_LabelDecl_Entry *)realloc(map->data,
                        (size_t) map->capacity * sizeof(KTL_LabelDecl_Entry) * KTL_LABEL_DECL_GROW_MOD);
        if (buffer == NULL) ExitF("NULL Realloc", );
        map->data     = buffer;
        map->capacity = KTL_LABEL_DECL_GROW_MOD;
    }
    return ;
}

static void grow_fix(KTL_LabelFix_Map *map) {
    assert(map);

    if (map->size == map->capacity) {
        KTL_LabelFix_Entry *buffer = (KTL_LabelFix_Entry *)realloc(map->data,
                        (size_t) map->capacity * sizeof(KTL_LabelFix_Entry) * KTL_LABEL_FIX_GROW_MOD);
        if (buffer == NULL) ExitF("NULL Realloc", );
        map->data     = buffer;
        map->capacity = KTL_LABEL_FIX_GROW_MOD;
    }
    return ;
}

