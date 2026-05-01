#ifndef TYPE_MAP_H
#define TYPE_MAP_H

#include "TypeMapType.h"

KTL_Error KTL_TypeMapCreate (KTL_TypeMap *map, int size);
KTL_Error KTL_TypeMapDestroy(KTL_TypeMap *map);

KTL_TypeID KTL_TypeAddBase       (KTL_TypeMap *map, const KTL_StrID name,
                                  int size, int align);

KTL_TypeID KTL_TypeAddDefine     (KTL_TypeMap *map, const KTL_TypeID base_id,
                                                    const KTL_StrID  alias);

KTL_TypeID KTL_TypeAddPointer    (KTL_TypeMap *map, const KTL_TypeID base_id);
KTL_TypeID KTL_TypeAddArray      (KTL_TypeMap *map, const KTL_TypeID base_id,
                                                    int elem_count);

KTL_TypeID KTL_TypeAddBlock      (KTL_TypeMap *map, const KTL_StrID name);
KTL_Error  KTL_TypeBlockAddField (KTL_TypeMap *map, KTL_TypeID block_id,
                                                    KTL_TypeID field_id,
                                                    KTL_StrID  name);
KTL_Error  KTL_TypeBlockFinish   (KTL_TypeMap *map, KTL_TypeID block_id);

KTL_TypeID KTL_TypeFindByName    (const KTL_TypeMap *map, const KTL_StrID name);

/**
 * @brief Get Entry from Type (!!! Check Note)
 *
 * @param map Pointer to Type Map
 * @param id ID type in Map
 * @return KTL_TypeEntry* entry in success,
 * @return KTL_TypeEntry* NULL in error
 * @note DO NOT USE IT FOR IDENTIFIER TYPE !!! ONLY FOR TYPE INFO !!!
 */
KTL_TypeEntry *KTL_TypeGetEntry(const KTL_TypeMap *map, const KTL_TypeID id);

#endif /* TYPE_MAP_H */
