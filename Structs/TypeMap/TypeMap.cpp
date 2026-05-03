#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "TypeMap.h"
#include "CallingConvention.h"

constexpr static int KTL_TYPE_FIELD_GROW   = 2;
constexpr static int KTL_TYPE_MAP_GROW     = 10;
constexpr static int KTL_TYPE_ALIAS_INIT   = 4;

// =======================================================================
// HELPER FUNCTIONS DECLARATION
// =======================================================================

static KTL_Error       ktl_add_standard     (KTL_TypeMap *map);
static KTL_Error       ktl_check_size       (KTL_TypeMap *map);
static KTL_Error       ktl_check_alias_size (KTL_TypeMap *map);

static KTL_TypeEntry * ktl_find_in_main     (const KTL_TypeMap *map,
                                             const KTL_StrID    name);
static KTL_TypeID      ktl_find_in_aliases  (const KTL_TypeMap *map,
                                             const KTL_StrID    name);

static KTL_TypeEntry * ktl_get_entry        (const KTL_TypeMap *map,
                                             const KTL_TypeID   id);
static KTL_TypeID      ktl_get_id           (const KTL_TypeMap *map,
                                             const KTL_TypeEntry *entry);

static int             ktl_get_type_size    (const KTL_TypeMap *map,
                                             const KTL_TypeID   id);
static int             ktl_get_type_align   (const KTL_TypeMap *map,
                                             const KTL_TypeID   id);
static int             ktl_align_up         (int offset, int align);

// =======================================================================
// API FUNCTIONS
// =======================================================================

KTL_Error KTL_TypeMapCreate(KTL_TypeMap *map, int size) {
    assert(map);
    if (size <= 0)  ExitF("Bad Size of Map", KTL_BAD_ARG_ERR);

    map->data = (KTL_TypeEntry *)calloc((size_t) size, sizeof(KTL_TypeEntry));
    if (map->data == NULL)    ExitF("NULL Calloc", KTL_MEMORY_ERR);

    map->size     = 0;
    map->capacity = size;

    map->aliases = (KTL_TypeAlias *)calloc(KTL_TYPE_ALIAS_INIT,
                                           sizeof(KTL_TypeAlias));
    if (map->aliases == NULL) {
        free(map->data);
        ExitF("NULL Calloc", KTL_MEMORY_ERR);
    }
    map->alias_size     = 0;
    map->alias_capacity = KTL_TYPE_ALIAS_INIT;

    return ktl_add_standard(map);
}

KTL_Error KTL_TypeMapDestroy(KTL_TypeMap *map) {
    assert(map);

    for (int i = 0; i < map->size; i++) {
        if (map->data[i].kind == KTL_TYPE_BLOCK) {
            free(map->data[i].dt.block.fields);
        }
    }
    free(map->data);
    free(map->aliases);

    map->data           = NULL;
    map->aliases        = NULL;
    map->size           = 0;
    map->capacity       = 0;
    map->alias_size     = 0;
    map->alias_capacity = 0;

    return KTL_OK;
}

KTL_TypeID KTL_TypeAddBase(KTL_TypeMap *map, const KTL_StrID name,
                              int size, int align) {
    assert(map);
    assert(StrIDCheck(name));
    assert(size >= 0);
    assert(align > 0);

    if (ktl_find_in_main(map, name) != NULL)                       return KTL_BAD_TYPE_ID;
    if (TypeIDCheck(ktl_find_in_aliases(map, name)))               return KTL_BAD_TYPE_ID;

    if (ktl_check_size(map) != KTL_OK)  return KTL_BAD_TYPE_ID;

    KTL_TypeEntry *entry = &map->data[map->size++];
    entry->kind            = KTL_TYPE_BASE;
    entry->dt.base.name    = name;
    entry->dt.base.size    = size;
    entry->dt.base.align   = align;

    return map->size - 1;
}

KTL_TypeID KTL_TypeAddDefine(KTL_TypeMap *map, const KTL_TypeID base_id,
                                               const KTL_StrID  alias) {
    assert(map);
    assert(StrIDCheck(alias));
    if (!TypeIDCheck(map, base_id))  return KTL_BAD_TYPE_ID;

    if (ktl_find_in_main(map, alias) != NULL)                  return KTL_BAD_TYPE_ID;
    if (TypeIDCheck(ktl_find_in_aliases(map, alias)))          return KTL_BAD_TYPE_ID;

    if (ktl_check_alias_size(map) != KTL_OK)  return KTL_BAD_TYPE_ID;

    KTL_TypeAlias *al_entry = &map->aliases[map->alias_size++];
    al_entry->name   = alias;
    al_entry->target = base_id;

    return base_id;
}

KTL_TypeID KTL_TypeAddPointer(KTL_TypeMap *map, const KTL_TypeID base_id) {
    assert(map);
    if (!TypeIDCheck(map, base_id))  return KTL_BAD_TYPE_ID;

    /* Check Duplicate */
    for (int i = 0; i < map->size; i++) {
        if (map->data[i].kind == KTL_TYPE_PTR &&
            map->data[i].dt.ptr.prev_type == base_id) {
            return i;
        }
    }

    if (ktl_check_size(map) != KTL_OK)  return KTL_BAD_TYPE_ID;

    KTL_TypeEntry *entry = &map->data[map->size++];
    entry->kind              = KTL_TYPE_PTR;
    entry->dt.ptr.prev_type  = base_id;

    return map->size - 1;
}

KTL_TypeID KTL_TypeAddArray(KTL_TypeMap *map, const KTL_TypeID base_id,
                            int elem_count) {
    assert(map);
    assert(elem_count > 0);
    if (!TypeIDCheck(map, base_id))  return KTL_BAD_TYPE_ID;

    /* Check Duplicate */
    for (int i = 0; i < map->size; i++) {
        if (map->data[i].kind == KTL_TYPE_ARRAY &&
            map->data[i].dt.arr.base_type  == base_id &&
            map->data[i].dt.arr.elem_count == elem_count) {
            return i;
        }
    }

    if (ktl_check_size(map) != KTL_OK)  return KTL_BAD_TYPE_ID;

    KTL_TypeEntry *entry = &map->data[map->size++];
    entry->kind                = KTL_TYPE_ARRAY;
    entry->dt.arr.base_type    = base_id;
    entry->dt.arr.elem_count   = elem_count;

    return map->size - 1;
}

KTL_TypeID KTL_TypeAddBlock(KTL_TypeMap *map, const KTL_StrID name) {
    assert(map);
    assert(StrIDCheck(name));

    if (ktl_find_in_main(map, name) != NULL)                   return KTL_BAD_TYPE_ID;
    if (TypeIDCheck(ktl_find_in_aliases(map, name)))           return KTL_BAD_TYPE_ID;

    if (ktl_check_size(map) != KTL_OK)  return KTL_BAD_TYPE_ID;

    KTL_TypeEntry *entry = &map->data[map->size++];
    entry->kind = KTL_TYPE_BLOCK;

    KTL_TypeBlock *block = &entry->dt.block;
    block->name        = name;
    block->fields      = NULL;
    block->field_cap   = 0;
    block->field_count = 0;
    block->size        = 0;
    block->align       = 0;
    block->complete    = false;

    return map->size - 1;
}

KTL_Error KTL_TypeBlockAddField(KTL_TypeMap *map, KTL_TypeID    block_id,
                                                  KTL_TypeID    field_id,
                                                  KTL_StrID     name) {
    assert(map);
    assert(StrIDCheck(name));
    if (!TypeIDCheck(map, block_id))  return KTL_BAD_ARG_ERR;
    if (!TypeIDCheck(map, field_id))  return KTL_BAD_ARG_ERR;

    KTL_TypeEntry *block_entry = ktl_get_entry(map, block_id);
    if (block_entry == NULL ||
        block_entry->kind != KTL_TYPE_BLOCK ||
        block_entry->dt.block.complete) {
        return KTL_LOGICAL_ERR;
    }

    KTL_TypeEntry *field_entry = ktl_get_entry(map, field_id);
    if (field_entry == NULL || field_entry == block_entry) {
        return KTL_LOGICAL_ERR;
    }

    KTL_TypeBlock *block = &block_entry->dt.block;
    if (block->fields == NULL) {
        block->fields = (KTL_TypeField *)calloc(KTL_TYPE_FIELD_GROW,
                                                sizeof(KTL_TypeField));
        if (block->fields == NULL)  ExitF("NULL Calloc", KTL_MEMORY_ERR);

        block->field_cap   = KTL_TYPE_FIELD_GROW;
        block->field_count = 0;
    }
    else if (block->field_cap == block->field_count) {
        int new_cap = block->field_cap + KTL_TYPE_FIELD_GROW;
        KTL_TypeField *buf = (KTL_TypeField *)realloc(block->fields,
                              (size_t) new_cap * sizeof(KTL_TypeField));
        if (buf == NULL)  ExitF("NULL Realloc", KTL_MEMORY_ERR);

        block->fields    = buf;
        block->field_cap = new_cap;
    }

    KTL_TypeField *field = &block->fields[block->field_count++];
    field->name      = name;
    field->base_type = field_id;
    field->offset    = 0;

    return KTL_OK;
}

KTL_Error KTL_TypeBlockFinish(KTL_TypeMap *map, KTL_TypeID block_id) {
    assert(map);
    if (!TypeIDCheck(map, block_id))  return KTL_BAD_ARG_ERR;

    KTL_TypeEntry *block_entry = ktl_get_entry(map, block_id);
    if (block_entry == NULL || block_entry->kind != KTL_TYPE_BLOCK) {
        return KTL_BAD_ARG_ERR;
    }

    KTL_TypeBlock *block = &block_entry->dt.block;
    if (block->fields == NULL || block->field_count == 0) {
        return KTL_BAD_ARG_ERR;
    }

    int cur_offset = 0;
    int max_align  = 1;

    for (int i = 0; i < block->field_count; i++) {
        KTL_TypeField *field = &block->fields[i];

        int field_size  = ktl_get_type_size (map, field->base_type);
        int field_align = ktl_get_type_align(map, field->base_type);
        if (field_size <= 0 || field_align <= 0)  return KTL_LOGICAL_ERR;

        cur_offset    = ktl_align_up(cur_offset, field_align);
        field->offset = cur_offset;
        cur_offset   += field_size;

        if (field_align > max_align)  max_align = field_align;
    }

    block->align    = max_align;
    block->size     = ktl_align_up(cur_offset, block->align);
    block->complete = true;

    return KTL_OK;
}

KTL_TypeID KTL_TypeFindByName(const KTL_TypeMap *map, const KTL_StrID name) {
    assert(map);
    assert(StrIDCheck(name));

    KTL_TypeEntry *entry = ktl_find_in_main(map, name);
    if (entry != NULL)  return ktl_get_id(map, entry);

    return ktl_find_in_aliases(map, name);
}

/**
 * @brief Get Entry from Type (!!! Check Note)
 *
 * @param map Pointer to Type Map
 * @param id ID type in Map
 * @return KTL_TypeEntry* entry in success,
 * @return KTL_TypeEntry* NULL in error
 * @note DO NOT USE IT FOR IDENTIFIER TYPE !!! ONLY FOR GET TYPE INFO !!!
 */
KTL_TypeEntry *KTL_TypeGetEntry(const KTL_TypeMap *map, const KTL_TypeID id) {
    assert(map);
    if (!TypeIDCheck(map, id))  return NULL;

    return map->data + id;

}

// =======================================================================
// HELPER FUNCTIONS
// =======================================================================

static KTL_Error ktl_add_standard(KTL_TypeMap *map) {
    (void) map;

    return KTL_OK;
}

static KTL_Error ktl_check_size(KTL_TypeMap *map) {
    assert(map);
    if (map->size != map->capacity)  return KTL_OK;

    int new_cap = map->capacity + KTL_TYPE_MAP_GROW;
    KTL_TypeEntry *buf = (KTL_TypeEntry *)realloc(map->data,
                            (size_t) new_cap * sizeof(KTL_TypeEntry));
    if (buf == NULL)  ExitF("NULL Realloc", KTL_MEMORY_ERR);

    map->data     = buf;
    map->capacity = new_cap;

    return KTL_OK;
}

static KTL_Error ktl_check_alias_size(KTL_TypeMap *map) {
    assert(map);
    if (map->alias_size != map->alias_capacity)  return KTL_OK;

    int new_cap = map->alias_capacity * 2;
    KTL_TypeAlias *buf = (KTL_TypeAlias *)realloc(map->aliases,
                            (size_t) new_cap * sizeof(KTL_TypeAlias));
    if (buf == NULL)  ExitF("NULL Realloc", KTL_MEMORY_ERR);

    map->aliases        = buf;
    map->alias_capacity = new_cap;

    return KTL_OK;
}

static KTL_TypeEntry * ktl_get_entry(const KTL_TypeMap *map,
                                     const KTL_TypeID   id) {
    if (map == NULL || map->data == NULL)  return NULL;
    if (!TypeIDCheck(map, id))             return NULL;

    return &map->data[id];
}

static KTL_TypeID ktl_get_id(const KTL_TypeMap *map,
                             const KTL_TypeEntry *entry) {
    assert(map);
    assert(entry);

    return (KTL_TypeID) (entry - map->data);
}

static KTL_TypeEntry * ktl_find_in_main(const KTL_TypeMap *map,
                                        const KTL_StrID    name) {
    assert(map);
    assert(StrIDCheck(name));

    for (int i = 0; i < map->size; i++) {
        KTL_TypeEntry *entry = &map->data[i];
        switch (entry->kind) {
            case KTL_TYPE_BASE: {
                if (entry->dt.base.name == name)   return entry;
                break;
            }
            case KTL_TYPE_BLOCK: {
                if (entry->dt.block.name == name)  return entry;
                break;
            }
            case KTL_TYPE_ARRAY:
            case KTL_TYPE_PTR:
            default:
                break;
        }
    }

    return NULL;
}

static KTL_TypeID ktl_find_in_aliases(const KTL_TypeMap *map,
                                      const KTL_StrID    name) {
    assert(map);
    assert(StrIDCheck(name));

    for (int i = 0; i < map->alias_size; i++) {
        if (map->aliases[i].name == name)  return map->aliases[i].target;
    }

    return KTL_BAD_TYPE_ID;
}

static int ktl_get_type_size(const KTL_TypeMap *map, const KTL_TypeID id) {
    KTL_TypeEntry *entry = ktl_get_entry(map, id);
    if (entry == NULL)  return -1;

    switch (entry->kind) {
        case KTL_TYPE_BASE:  return entry->dt.base.size;
        case KTL_TYPE_PTR:   return KTL_SYSTEM_PTR_SIZE;

        case KTL_TYPE_ARRAY: {
            int elem_size = ktl_get_type_size(map, entry->dt.arr.base_type);
            if (elem_size <= 0)  return -1;
            return entry->dt.arr.elem_count * elem_size;
        }

        case KTL_TYPE_BLOCK:
            if (!entry->dt.block.complete)  return -1;
            return entry->dt.block.size;

        default:  return -1;
    }
}

static int ktl_get_type_align(const KTL_TypeMap *map, const KTL_TypeID id) {
    KTL_TypeEntry *entry = ktl_get_entry(map, id);
    if (entry == NULL)  return -1;

    switch (entry->kind) {
        case KTL_TYPE_BASE:  return entry->dt.base.align;
        case KTL_TYPE_PTR:   return KTL_SYSTEM_PTR_SIZE;
        case KTL_TYPE_ARRAY: return ktl_get_type_align(map, entry->dt.arr.base_type);

        case KTL_TYPE_BLOCK:
            if (!entry->dt.block.complete)  return -1;
            return entry->dt.block.align;

        default:  return -1;
    }
}

static int ktl_align_up(int offset, int align) {
    assert(align > 0);

    return ((offset + align - 1) / align) * align;
}


