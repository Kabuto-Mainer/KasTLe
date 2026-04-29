#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "Token.h"
#include "ParseType.h"
#include "ParseConfig.h"
#include "TypeMap.h"
#include "Common.h"
#include "StandardType.h"
#include "DumpAstType.h"
#include "ASTCommon.h"

static int AMOUNT_IMAGES = 0;

void KTL_AstDumpRaw(KTL_AstNode   *root,
                    KTL_StrMap    *str_map,
                    KTL_SymbolMap *global_map,
                    KTL_TypeMap   *type_map,
                    const char    *file) {
    assert(root);
    assert(str_map);
    assert(global_map);
    assert(type_map);
    assert(file);

    KTL_DumpAstContext cont = {};
    cont.global_map         = global_map;
    cont.str_map            = str_map;
    cont.type_map           = type_map;
    cont.stream             = fopen(file, "rb");

    if (cont.stream == NULL)    ExitF("NULL File", );

    ktl_dump_init        (&cont);
    ktl_dump_create_block(&cont, root);
    ktl_dump_create_line (&cont, root);
    ktl_dump_create_img  (&cont);
}

static void ktl_dump_init(KTL_DumpAstContext *cont) {
    assert(cont);

    fprintf (cont->stream,
             "digraph {\n"
             "  rankdir=UD;\n"
             "  bgcolor=\"#1e1e1e\"\n"
             "  splines=spline;\n"
             "  nodesep=0.4;\n"
             "  ranksep=0.6;\n"
             "  node [shape=plaintext, style=filled, fontname=\"Helvetica\"];\n"
             "  edge [arrowhead=vee, arrowsize=0.6, penwidth=1.2];\n\n");
    return ;
}

static void ktl_dump_block(KTL_DumpAstContext *cont,
                           KTL_AstNode        *node) {
    assert(cont);
    if (node == NULL)   return ;

    switch (KTL_AstGetTypeChildren(node)) {
        case KTL_AST_N_CHILDREN:
            for (int i = 0; i < node->move.n.amount; i++) {
                ktl_dump_block(cont, node->move.n.children[i]);
            }
            free(node->move.n.children);
            break;

        case KTL_AST_BINARY_CHILDREN:
            ktl_dump_block(cont, node->move.binary.left);
            ktl_dump_block(cont, node->move.binary.right);
            break;

        case KTL_AST_UNARY_CHILD:
            ktl_dump_block(cont, node->move.unary.next);
            break;

        case KTL_AST_NO_CHILDREN:
        default:
            break;
    }


}

static void ktl_dump_block_info_raw(KTL_DumpAstContext *cont,
                                    KTL_AstNode        *node) {
    assert(cont);
    if (node == NULL)   return ;

    char name[100] = "";
    char info[1000] = "";


    switch (node->kind) {
        case KTL_AST_FILE:
            strcpy(name, "FILE");
        case KTL_AST_MAIN:
            strcpy(name, "MAIN");

        case KTL_AST_FUNCTION_DECL:
            strcpy(name, "FUNC DECL");

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
}
