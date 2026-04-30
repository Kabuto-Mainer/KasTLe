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
    KTL_AST_ARRAY_INIT,

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

    KTL_AST_CAST,
};

enum KTL_Oper {
    KTL_OPER_ADD,
    KTL_OPER_SUB,
    KTL_OPER_MUL,
    KTL_OPER_DIV,
    KTL_OPER_MOD,

    KTL_OPER_NEG,
    KTL_OPER_AND,
    KTL_OPER_OR,

    KTL_OPER_COMP_BE,
    KTL_OPER_COMP_B,
    KTL_OPER_COMP_LE,
    KTL_OPER_COMP_L,
    KTL_OPER_COMP_E,
    KTL_OPER_COMP_NE,

    KTL_OPER_GET_PTR,
    KTL_OPER_UNGET_PTR,
    KTL_OPER_ASSIGN,


    KTL_OPER_THIS_ERROR,
};

struct KTL_AstNode {
    KTL_AstNodeKind kind;
    KTL_SourcePos pos;

    union {
        struct {
            union {
                struct { KTL_StrID        name;  } raw;
                struct { KTL_SymbolEntry *entry; } res;
            } info;
            bool         is_raw;
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
                struct { KTL_StrID        name;  } raw;
                struct { KTL_SymbolEntry *entry; } res;
            } info;
            bool         is_raw;
        } func_call;

        struct {
            KTL_StrID name;
            bool      is_ptr;
        } field;

        struct { KTL_Oper op; } oper;

        struct { int64_t   value; } int_val;
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

        struct {
            KTL_SymbolMap *map;
        } for_block;

        struct {
            KTL_TypeID target;
        } cast;
    } data;

    /* Связи с потомками:
     *
     *   FILE, MAIN, BLOCK, FUNCTION_DECL    -> move.n        (список инструкций/деклараций)
     *   FUNCTION_CALL, ARRAY                -> move.n        (аргументы)
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

enum KTL_AstChildren {
    KTL_AST_NO_CHILDREN,
    KTL_AST_UNARY_CHILD,
    KTL_AST_BINARY_CHILDREN,
    KTL_AST_N_CHILDREN,
};


#endif /* AST_TYPE_H */
