#include <assert.h>

#include "ASTType.h"
#include "ASTCommon.h"

KTL_AstChildren KTL_AstGetTypeChildren(KTL_AstNode *node) {
    assert(node);

    switch (node->kind) {
        /* Kinds with n children */
        case KTL_AST_FILE:
        case KTL_AST_MAIN:

        case KTL_AST_FUNCTION_DECL:
        case KTL_AST_FUNCTION_CALL:

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
