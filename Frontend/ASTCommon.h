#ifndef AST_COMMON_H
#define AST_COMMON_H

#include "ASTType.h"

KTL_AstChildren KTL_AstGetTypeChildren(KTL_AstNode *node);
KTL_AstNode *   ktl_alloc_node        ();
void            ktl_destroy_node      (KTL_AstNode *node);



#endif /* AST_COMMON_H */
