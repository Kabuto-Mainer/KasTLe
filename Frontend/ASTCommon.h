#ifndef AST_COMMON_H
#define AST_COMMON_H

#include "ASTType.h"

KTL_AstChildren KTL_AstGetTypeChildren(KTL_AstNode *node);
KTL_AstNode *   ktl_alloc_node        ();
void            ktl_destroy_node      (KTL_AstNode *node);


/**
 * Use it careful. Buffer does not checked
 */
int             ktl_print_type        (KTL_TypeMap *map, KTL_TypeID type, char *buffer);


#endif /* AST_COMMON_H */
