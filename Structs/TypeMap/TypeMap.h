#ifndef TYPE_MAP_H
#define TYPE_MAP_H

#include "TypeMapType.h"


//-------------------------------------------------------------------------
/**
 * @brief Create Type Map with start Size
 *
 * @param map Pointer to Type Map
 * @param size Start Size
 * @return KTL_Error KTL_OK in correct
 */
KTL_Error KTL_TypeMapCreate(KTL_TypeMap *map, int size);


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
                                                  const KTL_StrID def_name);

//-------------------------------------------------------------------------
/**
 * @brief Find Base Type in Map
 *
 * @param map Pointer to Type Map
 * @param name Name of Base Type
 * @return KTL_TypeID ID in correct
 * @return KTL_TypeID KTL_BAD_TYPE_ID in error
 */
KTL_TypeID KTL_TypeFindBase(KTL_TypeMap *map, const KTL_StrID name);

//-------------------------------------------------------------------------
/**
 * @brief Add Pointer Type to Map
 *
 * @param map Pointer to Type Map
 * @param base_id ID of prev type
 * @return KTL_TypeID New ID in correct
 * @return KTL_TypeID KTL_BAD_TYPE_ID in error
 */
KTL_TypeID KTL_TypeAddPointer(KTL_TypeMap *map, const KTL_TypeID base_id);


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
KTL_TypeID KTL_TypeAddArray(KTL_TypeMap *map, const KTL_TypeID base_id, int elem_count);


//-------------------------------------------------------------------------
/**
 * @brief Add Block Type to Map
 *
 * @param map Pointer to Type Map
 * @param name Name of Block
 * @return KTL_TypeID New IF in correct
 * @return KTL_TypeID KTL_BAD_TYPE_ID in error
 */
KTL_TypeID KTL_TypeAddBlock(KTL_TypeMap *map, const KTL_StrID name);


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
                                                  KTL_StrID name);

//-------------------------------------------------------------------------
/**
 * @brief Finish creating Block
 *
 * @param map Pointer to Type Map
 * @param block_id ID of block type
 * @return KTL_Error KTL_OK in correct
 */
KTL_Error KTL_TypeBlockFinish(KTL_TypeMap *map, KTL_TypeID block_id);




#endif /* TYPE_MAP_H */
