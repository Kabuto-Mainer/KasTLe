#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "TypeMapType.h"



constexpr static int ktl_field_size_mod = 2;
constexpr static int ktl_map_size_mod = 10;

extern const int KTL_POINTER_SIZE;

// =======================================================================
// HELPER FUNCTIONS DECLARATION
// =======================================================================

static void ktl_add_standard(KTL_TypeMap *map);
static void ktl_check_size(KTL_TypeMap *map);

static KTL_TypeEntry * ktl_find(const KTL_TypeMap *map, const KTL_StrID id);
static KTL_TypeEntry * ktl_get_entry(const KTL_TypeMap *map, const KTL_TypeID id);
static KTL_TypeID ktl_get_id(const KTL_TypeMap *map, const KTL_TypeEntry *entry);

static int ktl_get_type_size(const KTL_TypeMap *map, const KTL_TypeID id);
static int ktl_get_type_align(const KTL_TypeMap *map, const KTL_TypeID id);
static int ktl_align_up(int offset, int align);


// =======================================================================
// API FUNCTIONS
// =======================================================================

//-------------------------------------------------------------------------
/**
 * @brief Create Type Map with start Size
 *
 * @param map Pointer to Type Map
 * @param size Start Size
 * @return KTL_Error KTL_OK in correct
 */
KTL_Error KTL_TypeMapCreate(KTL_TypeMap *map, int size) {
    assert(map);
    if (size <= 0)  ExitF("Bad Size of Map", KTL_BAD_ARG_ERR);

    map->data = (KTL_TypeEntry *)calloc((size_t) size, sizeof(KTL_TypeEntry));
    if (map->data == NULL)    ExitF("NULL Calloc", KTL_MEMORY_ERR);

    map->size = 0;
    map->capacity = size;

    ktl_add_standard(map);
    return KTL_OK;
}

//-------------------------------------------------------------------------
/**
 * @brief Add alias to Base Type
 *
 * @param map Pointer to Type Map
 * @param base_id Id of Base Type
 * @param def_name Alias
 * @return KTL_TypeID New ID in correct
 * @return KTL_TypeID KTL_BAD_TYPE_ID in error
 */
KTL_TypeID KTL_TypeAddDefine(KTL_TypeMap *map, const KTL_TypeID base_id,
                                               const KTL_StrID def_name) {
    assert(map);
    assert(TypeIDCheck(map, base_id));
    assert(StrIDCheck(def_name));

    /* Check redefination */
    KTL_TypeEntry *def_type = ktl_find(map, def_name);
    if (def_type != NULL) {
        return ktl_get_id(map, def_type);
    }

    /* Check correct of type */
    KTL_TypeEntry *base_type = ktl_get_entry(map, base_id);
    if (base_type == NULL) {
        return KTL_BAD_TYPE_ID;
    }

    /* Control size */
    ktl_check_size(map);

    /* Add new type */
    KTL_TypeEntry *new_type = &(map->data[map->size++]);
    new_type->kind = KTL_TYPE_DEFINE;
    new_type->dt.def.name = def_name;
    new_type->dt.def.base_type = base_id;

    return map->size - 1;
}

//-------------------------------------------------------------------------
/**
 * @brief Add Base Type to Map
 *
 * @param map Pointer to Type Map
 * @param name Name of new type
 * @param size SIze of new type
 * @return KTL_TypeID New ID in correct
 * @return KTL_TypeID KTL_BAD_TYPE_ID in error
 */
KTL_TypeID KTL_TypeMapAddBase(KTL_TypeMap *map, const KTL_StrID name,
                                                int size, int align) {
    assert(map);
    assert(StrIDCheck(name));
    assert(size > 0);
    assert(align > 0);

    /* Check correct of type and its copies */
    KTL_TypeEntry *check_type = ktl_find(map, name);
    if (check_type != NULL) {
        return ktl_get_id(map, check_type);
    }

    /* Control size */
    ktl_check_size(map);

    /* Add new type */
    KTL_TypeEntry *new_type = &(map->data[map->size++]);
    new_type->kind = KTL_TYPE_BASE;

    KTL_TypeBase *base = &new_type->dt.base;
    base->name = name;
    base->size = size;
    base->align = align;

    return map->size - 1;
}

//-------------------------------------------------------------------------
/**
 * @brief Find Base Type in Map
 *
 * @param map Pointer to Type Map
 * @param name Name of Base Type
 * @return KTL_TypeID ID in correct
 * @return KTL_TypeID KTL_BAD_TYPE_ID in error
 */
KTL_TypeID KTL_TypeFindBase(KTL_TypeMap *map, const KTL_StrID name) {
    assert(map);
    assert(StrIDCheck(name));

    KTL_TypeEntry *check_type = ktl_find(map, name);
    if (check_type != NULL) {
        return ktl_get_id(map, check_type);
    }
    return KTL_BAD_TYPE_ID;
};

//-------------------------------------------------------------------------
/**
 * @brief Add Pointer Type to Map
 *
 * @param map Pointer to Type Map
 * @param base_id ID of prev type
 * @return KTL_TypeID New ID in correct
 * @return KTL_TypeID KTL_BAD_TYPE_ID in error
 */
KTL_TypeID KTL_TypeAddPointer(KTL_TypeMap *map, const KTL_TypeID base_id) {
    assert(map);
    assert(TypeIDCheck(map, base_id));

    /* Check correct of type */
    KTL_TypeEntry *base_type = ktl_get_entry(map, base_id);
    if (base_type == NULL)  return KTL_BAD_TYPE_ID;

    /* Check copies */
    for (int i = 0; i < map->size; i++) {
        KTL_TypeEntry *entry = &map->data[i];
        if (entry->kind != KTL_TYPE_PTR) {
            continue;
        }
        if (entry->dt.ptr.prev_type == base_id) {
            return i;
        }
    }

    /* Control size */
    ktl_check_size(map);

    /* Add new type */
    KTL_TypeEntry *new_type = &(map->data[map->size++]);
    new_type->kind = KTL_TYPE_PTR;
    new_type->dt.ptr.prev_type = base_id;

    return map->size - 1;
}

//-------------------------------------------------------------------------
/**
 * @brief Add Array Type to Map
 *
 * @param map Pointer to Type Map
 * @param base_id ID of base type
 * @param elem_count Amount elemets
 * @return KTL_TypeID New ID in correct
 * @return KTL_TypeID KTL_BAD_TYPE_ID in error
 */
KTL_TypeID KTL_TypeAddArray(KTL_TypeMap *map, const KTL_TypeID base_id, int elem_count) {
    assert(map);
    assert(TypeIDCheck(map, base_id));
    assert(elem_count > 0);

    /* Check correct of type */
    KTL_TypeEntry *base_type = ktl_get_entry(map, base_id);
    if (base_type == NULL)  return KTL_BAD_TYPE_ID;

    /* Check copies */
    for (int i = 0; i < map->size; i++) {
        KTL_TypeEntry *entry = &map->data[i];
        if (entry->kind != KTL_TYPE_ARRAY) {
            continue;
        }
        if (entry->dt.arr.base_type == base_id && entry->dt.arr.elem_count == elem_count) {
            return i;
        }
    }

    /* Control size */
    ktl_check_size(map);

    /* Add new type */
    KTL_TypeEntry *new_type = &(map->data[map->size++]);
    new_type->kind = KTL_TYPE_ARRAY;
    new_type->dt.arr.elem_count = elem_count;
    new_type->dt.arr.base_type = base_id;

    return map->size - 1;
}

//-------------------------------------------------------------------------
/**
 * @brief Add Block Type to Map
 *
 * @param map Pointer to Type Map
 * @param name Name of Block
 * @return KTL_TypeID New IF in correct
 * @return KTL_TypeID KTL_BAD_TYPE_ID in error
 */
KTL_TypeID KTL_TypeAddBlock(KTL_TypeMap *map, const KTL_StrID name) {
    assert(map);
    assert(StrIDCheck(name));

    /* Check correct of type */
    KTL_TypeEntry *check_type = ktl_find(map, name);
    if (check_type != NULL)  return KTL_BAD_TYPE_ID;

    /* Control size */
    ktl_check_size(map);

    /* Add new type */
    KTL_TypeEntry *new_type = &(map->data[map->size++]);
    new_type->kind = KTL_TYPE_BLOCK;

    KTL_TypeBlock *block = &new_type->dt.block;

    block->name = name;
    block->fields = NULL;
    block->field_cap = 0;
    block->field_count = 0;
    block->size = 0;
    block->align = 0;
    block->complete = false;

    return map->size - 1;
}

//-------------------------------------------------------------------------
/**
 * @brief Add Field to Block
 *
 * @param map Pointer to Type Map
 * @param block_id ID of block type
 * @param field_id ID of filed type
 * @param name Name of Field
 * @return KTL_Error KTL_OK in correct
 */
KTL_Error KTL_TypeBlockAddField(KTL_TypeMap *map, KTL_TypeID block_id,
                                                  KTL_TypeID field_id,
                                                  KTL_StrID name) {
    assert(map);
    assert(TypeIDCheck(map, block_id));
    assert(TypeIDCheck(map, field_id));
    assert(StrIDCheck(name));

    /* Check correct */
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

    /* Check size */
    KTL_TypeBlock *block = &block_entry->dt.block;
    if (block->fields == NULL) {
        block->fields = (KTL_TypeField *)calloc(ktl_field_size_mod, sizeof(KTL_TypeField));
        if (block->fields == NULL)  ExitF("NULL Calloc", KTL_MEMORY_ERR);

        block->field_cap = ktl_field_size_mod;
        block->field_count = 0;
    }
    else if (block->field_cap == block->field_count) {
        KTL_TypeField *buffer = (KTL_TypeField *)realloc(block->fields,
                                        (block->field_cap + ktl_field_size_mod) * sizeof(KTL_TypeField));
        if (buffer == NULL)  ExitF("NULL Calloc", KTL_MEMORY_ERR);

        block->fields = buffer;
        block->field_cap += ktl_field_size_mod;
    }

    /* Add field */
    KTL_TypeField *field = &block->fields[block->field_count++];
    field->name = name;
    field->base_type = field_id;
    field->offset = 0;

    return KTL_OK;
}

//-------------------------------------------------------------------------
/**
 * @brief Finish creating Block
 *
 * @param map Pointer to Type Map
 * @param block_id ID of block type
 * @return KTL_Error KTL_OK in correct
 */
KTL_Error KTL_TypeBlockFinish(KTL_TypeMap *map, KTL_TypeID block_id) {
    assert(map);
    assert(TypeIDCheck(map, block_id));

    /* Check correct */
    KTL_TypeEntry *block_entry = ktl_get_entry(map, block_id);
    if (block_entry == NULL || block_entry->kind != KTL_TYPE_BLOCK) {
        return KTL_BAD_ARG_ERR;
    }

    KTL_TypeBlock *block = &block_entry->dt.block;
    if (block->fields == NULL) {
        return KTL_BAD_ARG_ERR;
    }

    /* Fill algin */
    int cur_offset = 0;
    int max_align = 1;

    for (int i = 0; i < block->field_count; i++) {
        KTL_TypeField *field = &block->fields[i];

        int field_size  = ktl_get_type_size(map, field->base_type);
        int field_align = ktl_get_type_align(map, field->base_type);

        if (field_size <= 0 || field_align <= 0) {
            return KTL_LOGICAL_ERR;
        }

        cur_offset = ktl_align_up(cur_offset, field_align);
        field->offset = cur_offset;
        cur_offset += field_size;

        if (field_align > max_align) {
            max_align = field_align;
        }
    }

    block->align = max_align;
    block->size = ktl_align_up(cur_offset, block->align);
    block->complete = true;

    return KTL_OK;
}

// =======================================================================
// HELPER FUNCTIONS
// =======================================================================

static void ktl_add_standard(KTL_TypeMap *map) {
    assert(map);

}

static void ktl_check_size(KTL_TypeMap *map) {
    assert(map);

    if (map->data == NULL) {
        return ;
    }

    if (map->size != map->capacity) {
        return ;
    }

    KTL_TypeEntry *buffer = (KTL_TypeEntry *)realloc(map->data, sizeof(KTL_TypeEntry) * (map->capacity + ktl_map_size_mod));
    if (buffer == NULL) {
        ExitF("NULL Realloc", );
    }
    map->data = buffer;
    map->capacity += ktl_map_size_mod;
    return ;
}

static KTL_TypeEntry * ktl_get_entry(const KTL_TypeMap *map, const KTL_TypeID id) {
    if (map == NULL || map->data == NULL) return NULL;
    if (!TypeIDCheck(map, id)) return NULL;
    return &map->data[id];
}

static KTL_TypeEntry * ktl_find(const KTL_TypeMap *map, const KTL_StrID id) {
    assert(map);
    assert(StrIDCheck(id));

    for (int i = 0; i < map->size; i++) {
        KTL_TypeEntry *entry = &map->data[i];

        switch (entry->kind) {
            case KTL_TYPE_BASE: {
                if (entry->dt.base.name == id) {
                    return entry;
                }
                break;
            }

            case KTL_TYPE_BLOCK: {
                if (entry->dt.block.name == id) {
                    return entry;
                }
                break;
            }

            case KTL_TYPE_DEFINE: {
                if (entry->dt.def.name == id) {
                    return entry;
                }
                break;
            }

            case KTL_TYPE_ARRAY:
            case KTL_TYPE_PTR:
            default:
        }
    }
    return NULL;
}

static KTL_TypeID ktl_get_id(const KTL_TypeMap *map, const KTL_TypeEntry *entry) {
    assert(map);
    assert(entry);

    KTL_TypeEntry *start = map->data;
    return (KTL_TypeID) (entry - start);
}

static int ktl_get_type_size(const KTL_TypeMap *map, const KTL_TypeID id) {
    assert(map);
    assert(TypeIDCheck(map, id));

    KTL_TypeEntry *entry = ktl_get_entry(map, id);
    if (entry == NULL) {
        return -1;
    }

    switch (entry->kind) {
        case KTL_TYPE_BLOCK: {
            if (entry->dt.block.complete) {
                return entry->dt.block.size;
            }
            return -1;
        }
        case KTL_TYPE_BASE: {
            return entry->dt.base.size;
        }
        case KTL_TYPE_PTR: {
            return KTL_POINTER_SIZE;
        }
        case KTL_TYPE_ARRAY: {
            int size_elem = ktl_get_type_size(map, entry->dt.arr.base_type);
            return entry->dt.arr.elem_count * size_elem;
        }
        case KTL_TYPE_DEFINE: {
            int size = ktl_get_type_size(map, entry->dt.def.base_type);
            return size;
        }

        default:
            return -1;
    }
    return -1;
}

static int ktl_get_type_align(const KTL_TypeMap *map, const KTL_TypeID id) {
    assert(map);
    assert(TypeIDCheck(map, id));

    KTL_TypeEntry *entry = ktl_get_entry(map, id);
    if (entry == NULL) {
        return -1;
    }

    switch (entry->kind) {
        case KTL_TYPE_BASE: {
            return entry->dt.base.align;
        }

        case KTL_TYPE_PTR: {
            return KTL_POINTER_SIZE;
        }

        case KTL_TYPE_ARRAY: {
            return ktl_get_type_align(map, entry->dt.arr.base_type);
        }

        case KTL_TYPE_DEFINE: {
            return ktl_get_type_align(map, entry->dt.def.base_type);
        }

        case KTL_TYPE_BLOCK: {
            if (!entry->dt.block.complete) {
                return -1;
            }
            return entry->dt.block.align;
        }
    }
    return -1;
}

static int ktl_align_up(int offset, int align) {
    assert(align > 0);
    return ((offset + align - 1) / align) * align;
}
