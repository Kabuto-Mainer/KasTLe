#ifndef AST_TYPE_H
#define AST_TYPE_H

#include "StrMapType.h"
#include "TypeMapType.h"
#include "SymMapType.h"
#include "TokenEnum.h"

enum KTL_AstNodeKind {
    KTL_AST_FUNCTION_DECL,
    KTL_AST_FUNCTION_CALL,
    KTL_AST_VARIABLE,
    KTL_AST_VARIABLE_DECL,
    KTL_AST_BINARY_OPER,
    KTL_AST_UNARY_OPER,
    KTL_AST_VALUE,
    KTL_AST_BLOCK,
    KTL_AST_COND_BLOCK,
    KTL_AST_WHILE_BLOCK,
    KTL_AST_FOR_BLOCK,
    KTL_AST_TYPEDEF,
    KTL_AST_ASSIGN,
    KTL_AST_RETURN,
};

struct KTL_AstNode {
    KTL_AstNodeKind kind;

    union {
        struct {
            union {
                struct {
                    KTL_StrID name;
                } raw;

                struct {
                    KTL_SymbolEntry *entry;
                } res;
            } info;
        } var;

        struct {
            KTL_SymbolEntry *entry;
            bool is_init;
        } var_decl;

        struct {
            KTL_SymbolEntry *func;
            KTL_SymbolMap *map;
        } func_decl;

//         struct {
//             KTL_
//         } func_call;
//
//         KTL_StrID indent;
//         KTL_FuncEntry *func_entry;
//         KTL_SymbolEntry *var_entry;
//         KTL_Oper oper;
//         int value;
//
//         KTL_SymbolMap *block;
    } data;

    union {
        struct {
            KTL_AstNode *next;
        } unary;

        struct {
            KTL_AstNode *left;
            KTL_AstNode *right;
        } binary;

        struct {
            KTL_AstNode **children;
            int amount;
        } n;
    } move;
};

// struct KTL_AstTree


#endif /* AST_TYPE_H */
