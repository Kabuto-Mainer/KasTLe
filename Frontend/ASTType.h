#ifndef AST_TYPE_H
#define AST_TYPE_H

#include "StrMapType.h"
#include "TypeMapType.h"
#include "SymMapType.h"
#include "TokenEnum.h"
#include "Diagnostic.h"

enum KTL_AstNodeKind {
    KTL_AST_FILE,
    KTL_AST_MAIN,

    KTL_AST_FUNCTION_DECL,
    KTL_AST_FUNCTION_CALL,

    KTL_AST_VARIABLE,
    KTL_AST_VARIABLE_DECL,

    KTL_AST_FIELD_ACCESS,
    KTL_AST_INDEX_ACCESS,

    KTL_AST_BINARY_OPER,
    KTL_AST_UNARY_OPER,
    KTL_AST_VALUE_INT,
    KTL_AST_VALUE_STR,

    KTL_AST_BLOCK,
    KTL_AST_COND_BLOCK,

    KTL_AST_IF_BRANCH,
    KTL_AST_ELSE_BRANCH,

    KTL_AST_WHILE_BLOCK,
    KTL_AST_FOR_BLOCK,

    KTL_AST_TYPEDEF,
    KTL_AST_STRUCT_DECL,

    KTL_AST_ASSIGN,
    KTL_AST_RETURN,
    KTL_AST_BREAK,
    KTL_AST_CONTINUE,
    KTL_AST_EXIT,
};

struct KTL_AstNode {
    KTL_AstNodeKind kind;
    KTL_SourcePos pos;

    union {
        struct {
            union {
                struct { KTL_StrID name; } raw;
                struct { KTL_SymbolEntry *entry; } res;
            } info;
        } var;

        struct {
            KTL_SymbolEntry *entry;
            bool is_init;
        } var_decl;

        struct {
            KTL_SymbolEntry *func;
            KTL_SymbolMap   *map;
        } func_decl;

        struct {
            union {
                struct { KTL_StrID name; } raw;
                struct { KTL_SymbolEntry *entry; } res;
            } info;
        } func_call;

        struct {
            KTL_StrID name;
        } field;

        struct { KTL_Oper op; } oper;

        struct { int64_t value; } int_val;
        struct { KTL_StrID value; } str_val;

        struct {
            KTL_TypeID base_id;
            KTL_StrID  alias;
        } typedef_;

        struct {
            KTL_TypeID type_id;
        } struct_decl;

        struct {
            KTL_SymbolMap *map;
        } block;
    } data;

    /* Связи с потомками:
     *
     *   FILE, MAIN, BLOCK, FUNCTION_DECL    -> move.n        (список инструкций/деклараций)
     *   FUNCTION_CALL                       -> move.n        (аргументы)
     *
     *   COND_BLOCK                          -> move.n        (IF_BRANCH, [IF_BRANCH...], [ELSE_BRANCH])
     *   IF_BRANCH                           -> move.binary   (left = условие, right = BLOCK с телом)
     *   ELSE_BRANCH                         -> move.unary    (next = BLOCK с телом)
     *
     *   WHILE_BLOCK                         -> move.binary   (left = условие, right = BLOCK с телом)
     *   FOR_BLOCK                           -> move.n        (init, cond, step, BLOCK)
     *
     *   ASSIGN, BINARY_OPER, INDEX_ACCESS   -> move.binary
     *   UNARY_OPER, RETURN, VARIABLE_DECL,
     *   FIELD_ACCESS                        -> move.unary
     *
     *   VARIABLE, VALUE_*, BREAK, CONTINUE,
     *   EXIT, TYPEDEF, STRUCT_DECL          -> листья, move не используется
     */
    union {
        struct { KTL_AstNode *next;  } unary;
        struct { KTL_AstNode *left;
                 KTL_AstNode *right; } binary;
        struct { KTL_AstNode **children;
                 int amount; }         n;
    } move;
};

#endif /* AST_TYPE_H */
