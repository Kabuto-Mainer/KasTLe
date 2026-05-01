#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "ASTType.h"
#include "ASTCommon.h"
#include "SymMap.h"
#include "TypeMap.h"

KTL_AstChildren KTL_AstGetTypeChildren(KTL_AstNode *node) {
    assert(node);

    switch (node->kind) {
        /* Kinds with n children */
        case KTL_AST_FILE:

        case KTL_AST_FUNCTION_DECL:
        case KTL_AST_FUNCTION_CALL:
        case KTL_AST_ARRAY_INIT:

        case KTL_AST_BLOCK:
        case KTL_AST_COND_BLOCK:
        case KTL_AST_FOR_BLOCK:
            return KTL_AST_N_CHILDREN;

        /* Kinds with 2 children */
        case KTL_AST_IF_BRANCH:
        case KTL_AST_WHILE_BLOCK:

        case KTL_AST_BINARY_OPER:
        case KTL_AST_ASSIGN:
        case KTL_AST_INDEX_ACCESS:
            return KTL_AST_BINARY_CHILDREN;

        /* One child */
        case KTL_AST_CAST:
        case KTL_AST_MAIN:
        case KTL_AST_ELSE_BRANCH:
        case KTL_AST_UNARY_OPER:
        case KTL_AST_RETURN:
        case KTL_AST_VARIABLE_DECL:
        case KTL_AST_FIELD_ACCESS:
            return KTL_AST_UNARY_CHILD;

        case KTL_AST_VARIABLE:
        case KTL_AST_VALUE_INT:
        case KTL_AST_VALUE_STR:
        case KTL_AST_TYPEDEF:
        case KTL_AST_STRUCT_DECL:
        case KTL_AST_BREAK:
        case KTL_AST_CONTINUE:
        case KTL_AST_EXIT:
        default:
            return KTL_AST_NO_CHILDREN;
    }
};

KTL_AstNode * ktl_alloc_node() {
    KTL_AstNode *node = (KTL_AstNode *)calloc(1, sizeof(KTL_AstNode));
    if (node == NULL) {
        ExitF("NULL Calloc", NULL);
    }
    return node;
}

void ktl_destroy_node(KTL_AstNode *node) {
    if (node == NULL)   return ;

    switch (KTL_AstGetTypeChildren(node)) {
        case KTL_AST_N_CHILDREN:
            for (int i = 0; i < node->move.n.amount; i++) {
                ktl_destroy_node(node->move.n.children[i]);
            }
            free(node->move.n.children);
            break;

        case KTL_AST_BINARY_CHILDREN:
            ktl_destroy_node(node->move.binary.left);
            ktl_destroy_node(node->move.binary.right);
            break;

        case KTL_AST_UNARY_CHILD:
            ktl_destroy_node(node->move.unary.next);
            break;

        case KTL_AST_NO_CHILDREN:
        default:
            break;
    }

    if (node->kind == KTL_AST_BLOCK) {
        KTL_SymbolMapUninit(node->data.block.map);
    } else if (node->kind == KTL_AST_FUNCTION_DECL) {
        KTL_SymbolMapUninit(node->data.func_decl.map);
    } else if (node->kind == KTL_AST_FOR_BLOCK) {
        KTL_SymbolMapUninit(node->data.for_block.map);
    }
    free(node);
}

int ktl_print_type(KTL_TypeMap *map, KTL_TypeID type, char *buffer) {
    assert(map);
    assert(TypeIDCheck(type));
    assert(buffer);

    KTL_TypeEntry *e = KTL_TypeGetEntry(map, type);
    if (e == NULL)  return 0;

    switch (e->kind) {
        case KTL_TYPE_ARRAY:
            buffer += ktl_print_type(map, e->dt.arr.base_type, buffer);
            return sprintf(buffer, "[%d]", e->dt.arr.elem_count);

        case KTL_TYPE_BLOCK:
            return sprintf(buffer, "%s", e->dt.block.name);

        case KTL_TYPE_PTR:
            buffer += ktl_print_type(map, e->dt.ptr.prev_type, buffer);
            return sprintf(buffer, "*");

        case KTL_TYPE_BASE:
            return sprintf(buffer, "%s", e->dt.base.name);
    }
}
