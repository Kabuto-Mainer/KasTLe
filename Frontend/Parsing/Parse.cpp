#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "Token.h"
#include "ParseType.h"
#include "ParseConfig.h"
#include "TypeMap.h"
#include "Common.h"
#include "StandardType.h"
#include "ASTCommon.h"

constexpr int KTL_PARSE_NCHILDREN_INIT = 4;


// =======================================================================
// HELPER FUNCTIONS DECLARATIONS
// =======================================================================

static inline KTL_Token * get_t      (KTL_ParseContext *cont);
static inline void        advance    (KTL_ParseContext *cont);
static inline bool        equal      (KTL_ParseContext *cont, KTL_ParseTokenRef ref);
static inline bool        equal      (KTL_ParseContext *cont, KTL_TokenKind kind);
static inline bool        equal      (KTL_ParseContext *cont, KTL_KeyWord key);


static inline void        add_node_l (KTL_AstNode *parent, KTL_AstNode *child);
static inline void        add_node_r (KTL_AstNode *parent, KTL_AstNode *child);
static inline void        add_node_u (KTL_AstNode *parent, KTL_AstNode *child);
static void               add_node_n (KTL_AstNode *parent, KTL_AstNode *child);

static inline int         get_pos    (KTL_ParseContext *cont);
static inline void        set_pos    (KTL_ParseContext *cont, int pos);

static KTL_AstNode *      ktl_alloc_node    ();
static void               ktl_destroy_node  (KTL_AstNode *node);

static inline KTL_SourcePos get_t_pos       (KTL_ParseContext *cont);

// =======================================================================
// PARSE FUNCTIONS
// =======================================================================




// -------------------------------------------------------------------------

/* Don't emit error */
static KTL_StrID ktl_parse_name(KTL_ParseContext *cont) {
    assert(cont);

    if (equal(cont, KTL_TOKEN_STRING) == false)     return KTL_BAD_STR_ID;

    KTL_StrID name = get_t(cont)->data.string;
    advance(cont);

    return name;
}

/* Emit modifier error */
static int ktl_parse_type_mod(KTL_ParseContext *cont) {
    assert(cont);

    int mod = KTL_VAR_NONE;

    while (true) {
        if      (equal(cont, KTL_KEY_CONST))    mod |= KTL_VAR_CONST;
        else if (equal(cont, KTL_KEY_MUTABLE))  mod |= KTL_VAR_MUTABLE;
        else if (equal(cont, KTL_KEY_STACK))    mod |= KTL_VAR_STACK;
        else if (equal(cont, KTL_KEY_REGISTER)) mod |= KTL_VAR_REGISTER;
        else    break;

        advance(cont);
        continue;
    }
    if (mod & KTL_VAR_CONST && mod & KTL_VAR_MUTABLE) {
        KTL_DiagEmit(cont->diag, get_t_pos(cont),
                     KTL_DIAG_PARSE_BAD_MODIFIERS,
                     KTL_DIAG_SEV_ERROR, (int64_t) mod);

        mod &= ~KTL_VAR_MUTABLE;
    }
    if (mod & KTL_VAR_STACK && mod & KTL_VAR_REGISTER) {
        KTL_DiagEmit(cont->diag, get_t_pos(cont),
                     KTL_DIAG_PARSE_BAD_MODIFIERS,
                     KTL_DIAG_SEV_ERROR, (int64_t) mod);

        mod &= ~KTL_VAR_REGISTER;
    } else if ((mod & KTL_VAR_STACK)    == false &&
               (mod & KTL_VAR_REGISTER) == false) {
        mod |= KTL_VAR_REGISTER;
    }

    return mod;
}

/* Emit errors */
static KTL_TypeID ktl_parse_type(KTL_ParseContext *cont) {
    assert(cont);

    KTL_SourcePos pos   = get_t_pos(cont);
    KTL_StrID type_name = ktl_parse_name(cont);

    if (StrIDCheck(type_name) == false) {
        KTL_DiagEmit(cont->diag, pos,
                     KTL_DIAG_PARSE_EXPECTED_TYPE,
                     KTL_DIAG_SEV_ERROR);

        return KTL_BAD_TYPE_ID;
    }

    KTL_TypeID type = KTL_TypeFindByName(cont->type_map, type_name);
    if (TypeIDCheck(type) == false) {
        KTL_DiagEmit(cont->diag, pos,
                     KTL_DIAG_PARSE_UNKNOWN_TYPE,
                     KTL_DIAG_SEV_ERROR, type_name);

        return KTL_BAD_TYPE_ID;
    }

    while (true) {
        if (equal(cont, KTL_PARSE_TYPE_PTR)) {
            type = KTL_TypeAddPointer(cont->type_map, type);
            advance(cont);
            continue;
        }
        break;
    }

    while (true) {
        if (equal(cont, KTL_PARSE_INDEX_LEFT) == false)     break;
        advance(cont);

        /* TODO заменить чтение литерала на
        ktl_parse_expr + ktl_const_eval. */

        if (equal(cont, KTL_TOKEN_VALUE) == false) {
            KTL_DiagEmit(cont->diag, get_t_pos(cont),
                         KTL_DIAG_PARSE_EXPECTED_NUMBER,
                         KTL_DIAG_SEV_ERROR);

            return KTL_BAD_TYPE_ID;
        }

        int64_t size = get_t(cont)->data.value;
        if (size <= 0) {
            KTL_DiagEmit(cont->diag, get_t_pos(cont),
                         KTL_DIAG_PARSE_BAD_ARRAY_SIZE,
                         KTL_DIAG_SEV_ERROR, size);

            return KTL_BAD_TYPE_ID;
        }
        advance(cont);

        if (equal(cont, KTL_PARSE_INDEX_RIGHT) == false) {
            KTL_DiagEmit(cont->diag, get_t_pos(cont),
                         KTL_DIAG_PARSE_EXPECTED_TOKEN,
                         KTL_DIAG_SEV_ERROR, KTL_PARSE_INDEX_RIGHT);

            return KTL_BAD_TYPE_ID;
        }
        advance(cont);

        type = KTL_TypeAddArray(cont->type_map, type, size);
    }

    return type;
}

/* Emit errors */
static KTL_AstNode *ktl_parse_str_literal(KTL_ParseContext *cont) {
    assert(cont);

    if (equal(cont, KTL_TOKEN_STR_LITERAL) == false)    return NULL;
    KTL_StrID string = get_t(cont)->data.str_literal;

    if (StrIDCheck(string) == false) {
        KTL_DiagEmit(cont->diag, get_t_pos(cont),
                     KTL_DIAG_PARSE_EXPECTED_STRING,
                     KTL_DIAG_SEV_ERROR);

        return NULL;
    }

    KTL_AstNode *node = ktl_alloc_node();
    if (node == NULL)   return NULL;

    node->kind               = KTL_AST_VALUE_STR;
    node->data.str_val.value = string;
    node->pos                = get_t_pos(cont);

    advance(cont);

    return node;
}

/* Not emit errors */
static KTL_AstNode *ktl_parse_number(KTL_ParseContext *cont) {
    assert(cont);

    if (equal(cont, KTL_TOKEN_VALUE) == false)  return NULL;

    KTL_AstNode *node = ktl_alloc_node();
    if (node == NULL)   return NULL;

    node->kind               = KTL_AST_VALUE_INT;
    node->data.int_val.value = get_t(cont)->data.value;
    node->pos                = get_t_pos(cont);

    advance(cont);

    return node;
}

/* Emit errors */
static KTL_AstNode *ktl_parse_var(KTL_ParseContext *cont) {
    assert(cont);

    if (equal  (cont, KTL_TOKEN_STRING)     == false ||
        equal_n(cont, KTL_PARSE_PAREN_LEFT) == true) {
        return NULL;
    }

    KTL_StrID name    = ktl_parse_name(cont);
    KTL_SourcePos pos = get_t_pos(cont);

    if (StrIDCheck(name) == false) {
        KTL_DiagEmit(cont->diag, get_t_pos(cont),
                     KTL_DIAG_PARSE_EXPECTED_NAME,
                     KTL_DIAG_SEV_ERROR);

        return NULL;
    }
    KTL_AstNode *node = ktl_alloc_node();
    if (node == NULL)   return NULL;

    node->kind                   = KTL_AST_VARIABLE;
    node->data.var.info.raw.name = name;
    node->pos                    = pos;

    while (equal(cont, KTL_PARSE_FIELD_DOT) || equal(cont, KTL_PARSE_INDEX_LEFT)) {
        if (equal(cont, KTL_PARSE_FIELD_DOT)) {
            advance(cont);

            name = ktl_parse_name(cont);
            if (StrIDCheck(name) == false) {
                KTL_DiagEmit(cont->diag, get_t_pos(cont),
                             KTL_DIAG_PARSE_EXPECTED_NAME,
                             KTL_DIAG_SEV_ERROR);

                ktl_destroy_node(node);
                return NULL;
            }

            KTL_AstNode *field = ktl_alloc_node();
            if (field == NULL) {
                ktl_destroy_node(node);
                return NULL;
            }

            field->kind            = KTL_AST_FIELD_ACCESS;
            field->data.field.name = name;

            add_node_u(field, node);
            node = field;
        } else {
            advance(cont);

            KTL_SourcePos pos = get_t_pos(cont);
            KTL_AstNode *idx = ktl_parse_expr(cont);
            if (idx == NULL) {
                ktl_destroy_node(node);
                return NULL;
            }

            if (equal(cont, KTL_PARSE_INDEX_RIGHT) == false) {
                KTL_DiagEmit(cont->diag, pos,
                             KTL_DIAG_PARSE_EXPECTED_TOKEN,
                             KTL_DIAG_SEV_ERROR, KTL_PARSE_INDEX_RIGHT);

                ktl_destroy_node(node);
                ktl_destroy_node(idx);

                return NULL;
            }

            KTL_AstNode *get_idx = ktl_alloc_node();
            if (get_idx == NULL) {
                ktl_destroy_node(node);
                ktl_destroy_node(idx);

                return NULL;
            }

            get_idx->kind = KTL_AST_INDEX_ACCESS;
            add_node_l(get_idx, node);
            add_node_r(get_idx, idx);

            node = get_idx;
        }
    }

    return node;
}

/* Emit errors */
static KTL_AstNode *ktl_parse_call(KTL_ParseContext *cont) {
    assert(cont);

    if (equal  (cont, KTL_TOKEN_STRING)     == false ||
        equal_n(cont, KTL_PARSE_PAREN_LEFT) == false) {
        return NULL;
    }

    KTL_SourcePos pos = get_t_pos(cont);
    KTL_StrID name    = ktl_parse_name(cont);

    if (StrIDCheck(name) == false) {
        KTL_DiagEmit(cont->diag, get_t_pos(cont),
                     KTL_DIAG_PARSE_EXPECTED_NAME,
                     KTL_DIAG_SEV_ERROR);

        return NULL;
    }
    KTL_AstNode *node = ktl_alloc_node();
    if (node == NULL)   return NULL;

    node->kind                         = KTL_AST_FUNCTION_CALL;
    node->data.func_call.info.raw.name = name;
    node->pos                          = pos;

    // // Do not need this check
    // if (equal(cont, KTL_PARSE_PAREN_LEFT) == false) {}
    advance(cont);

    while (equal(cont, KTL_PARSE_PAREN_RIGHT) == false) {
        KTL_AstNode *arg = ktl_parse_expr(cont);
        if (arg == NULL) {
            ktl_destroy_node(node);
            return NULL;
        }

        add_node_n(node, arg);
        if (equal(cont, KTL_PARSE_ARG_SEP) == false)    break;

        advance(cont);
    }
    if (equal(cont, KTL_PARSE_PAREN_RIGHT) == false) {
        KTL_DiagEmit(cont->diag, get_t_pos(cont),
                     KTL_DIAG_PARSE_EXPECTED_TOKEN,
                     KTL_DIAG_SEV_ERROR, KTL_PARSE_PAREN_RIGHT);

        ktl_destroy_node(node);
        return NULL;
    }

    advance(cont);

    return node;
}

// -------------------------------------------------------------------------

/* Emit errors */
static KTL_AstNode *ktl_parse_atom(KTL_ParseContext *cont) {
    assert(cont);

    KTL_AstNode *node = NULL;

    if (equal(cont, KTL_PARSE_PAREN_LEFT)) {
        advance(cont);

        node = ktl_parse_expr(cont);
        if (node == NULL)   return NULL;

        if (equal(cont, KTL_PARSE_PAREN_RIGHT) == false) {
            KTL_DiagEmit(cont->diag, get_t_pos(cont),
                         KTL_DIAG_PARSE_EXPECTED_TOKEN,
                         KTL_DIAG_SEV_ERROR, KTL_PARSE_PAREN_RIGHT);

            ktl_destroy_node(node);
            return NULL;
        }
        advance(cont);
        return node;
    }

    node = ktl_parse_call(cont);
    if (node == NULL)   node = ktl_parse_var        (cont);
    if (node == NULL)   node = ktl_parse_number     (cont);
    if (node == NULL)   node = ktl_parse_str_literal(cont);
    if (node == NULL) {
        KTL_DiagEmit(cont->diag, get_t_pos(cont),
                     KTL_DIAG_PARSE_EXPECTED_EXPR,
                     KTL_DIAG_SEV_ERROR);

        return NULL;
    }

    return node;
}

/* Emit errors */
static KTL_AstNode *ktl_parse_unary(KTL_ParseContext *cont) {
    assert(cont);

    KTL_AstNode *node     = NULL;
    KTL_AstNode *cur_node = node;

    while (true) {
        if (equal(cont, KTL_PARSE_PTR_ADDR)  == false &&
            equal(cont, KTL_PARSE_PTR_DEREF) == false &&
            equal(cont, KTL_PARSE_OP_NEG)    == false )  break;

        KTL_Oper oper     = KTL_OPER_GET_PTR;
        KTL_SourcePos pos = get_t_pos(cont);

        if      (equal(cont, KTL_PARSE_PTR_DEREF)) oper = KTL_OPER_UNGET_PTR;
        else if (equal(cont, KTL_PARSE_OP_NEG))    oper = KTL_OPER_NEG;
        else if (equal(cont, KTL_PARSE_PTR_ADDR))  oper = KTL_OPER_GET_PTR;

        if (node == NULL) {
            node = ktl_alloc_node();

            if (node == NULL)   return NULL;

            node->kind         = KTL_AST_UNARY_OPER;
            node->data.oper.op = oper;
            node->pos          = pos;

            cur_node           = node;
        } else {
            KTL_AstNode *new_oper = ktl_alloc_node();
            if (new_oper == NULL) {
                ktl_destroy_node(node);
                return NULL;
            }

            new_oper->kind         = KTL_AST_UNARY_OPER;
            new_oper->data.oper.op = oper;
            new_oper->pos          = pos;

            add_node_u(cur_node, new_oper);
            cur_node = new_oper;
        }

        advance(cont);
    }

    KTL_AstNode *atom = ktl_parse_atom(cont);
    if (atom == NULL) {
        if (node != NULL)   ktl_destroy_node(node);
        return NULL;
    }

    if (node == NULL)   return atom;

    add_node_u(cur_node, atom);
    return node;
}

/* Emit errors */
static KTL_AstNode *ktl_parse_mul_step(KTL_ParseContext *cont) {
    assert(cont);

    KTL_AstNode *node = ktl_parse_unary(cont);
    if (node == NULL)   return NULL;

    while (true) {
        KTL_Oper oper     = KTL_OPER_THIS_ERROR;
        KTL_SourcePos pos = get_t_pos(cont);

        if      (equal(cont, KTL_PARSE_OP_MUL))  oper = KTL_OPER_MUL;
        else if (equal(cont, KTL_PARSE_OP_DIV))  oper = KTL_OPER_DIV;
        else if (equal(cont, KTL_PARSE_OP_MOD))  oper = KTL_OPER_MOD;
        else    break;

        advance(cont);
        KTL_AstNode *node_r = ktl_parse_unary(cont);
        if (node_r == NULL) {
            ktl_destroy_node(node);
            return NULL;
        }

        KTL_AstNode *oper_node = ktl_alloc_node();
        if (oper_node == NULL)  {
            ktl_destroy_node(node);
            ktl_destroy_node(node_r);

            return NULL;
        }

        oper_node->kind         = KTL_AST_BINARY_OPER;
        oper_node->data.oper.op = oper;
        oper_node->pos          = pos;

        add_node_l(oper_node, node);
        add_node_r(oper_node, node_r);
        node = oper_node;
    }
    return node;
}

/* Emit errors */
static KTL_AstNode *ktl_parse_add_step(KTL_ParseContext *cont) {
    assert(cont);

    KTL_AstNode *node = ktl_parse_mul_step(cont);
    if (node == NULL)   return NULL;

    while (true) {
        KTL_Oper oper     = KTL_OPER_THIS_ERROR;
        KTL_SourcePos pos = get_t_pos(cont);

        if      (equal(cont, KTL_PARSE_OP_PLUS))   oper = KTL_OPER_ADD;
        else if (equal(cont, KTL_PARSE_OP_MINUS))  oper = KTL_OPER_SUB;
        else    break;

        advance(cont);
        KTL_AstNode *node_r = ktl_parse_mul_step(cont);
        if (node_r == NULL) {
            ktl_destroy_node(node);
            return NULL;
        }

        KTL_AstNode *oper_node = ktl_alloc_node();
        if (oper_node == NULL)  {
            ktl_destroy_node(node);
            ktl_destroy_node(node_r);

            return NULL;
        }

        oper_node->kind         = KTL_AST_BINARY_OPER;
        oper_node->data.oper.op = oper;
        oper_node->pos          = pos;

        add_node_l(oper_node, node);
        add_node_r(oper_node, node_r);
        node = oper_node;
    }
    return node;
}

/* Emit errors */
static KTL_AstNode *ktl_parse_cmp_step(KTL_ParseContext *cont) {
    assert(cont);

    KTL_AstNode *node = ktl_parse_add_step(cont);
    if (node == NULL)   return NULL;

    while (true) {
        KTL_Oper oper     = KTL_OPER_THIS_ERROR;
        KTL_SourcePos pos = get_t_pos(cont);

        if      (equal(cont, KTL_PARSE_OP_EQ))  oper = KTL_OPER_COMP_E;
        else if (equal(cont, KTL_PARSE_OP_NEQ)) oper = KTL_OPER_COMP_NE;
        else if (equal(cont, KTL_PARSE_OP_LT))  oper = KTL_OPER_COMP_L;
        else if (equal(cont, KTL_PARSE_OP_GT))  oper = KTL_OPER_COMP_B;
        else if (equal(cont, KTL_PARSE_OP_GE))  oper = KTL_OPER_COMP_BE;
        else if (equal(cont, KTL_PARSE_OP_LE))  oper = KTL_OPER_COMP_LE;
        else    break;

        advance(cont);
        KTL_AstNode *node_r = ktl_parse_add_step(cont);
        if (node_r == NULL) {
            ktl_destroy_node(node);
            return NULL;
        }

        KTL_AstNode *oper_node = ktl_alloc_node();
        if (oper_node == NULL)  {
            ktl_destroy_node(node);
            ktl_destroy_node(node_r);

            return NULL;
        }

        oper_node->kind         = KTL_AST_BINARY_OPER;
        oper_node->data.oper.op = oper;
        oper_node->pos          = pos;

        add_node_l(oper_node, node);
        add_node_r(oper_node, node_r);
        node = oper_node;
    }
    return node;
}

/* Emit errors */
static KTL_AstNode *ktl_parse_expr(KTL_ParseContext *cont) {
    assert(cont);

    KTL_AstNode *node = ktl_parse_cmp_step(cont);
    if (node == NULL)   return NULL;

    while (true) {
        KTL_Oper oper     = KTL_OPER_THIS_ERROR;
        KTL_SourcePos pos = get_t_pos(cont);

        if      (equal(cont, KTL_PARSE_OP_AND))  oper = KTL_OPER_AND;
        else if (equal(cont, KTL_PARSE_OP_OR))   oper = KTL_OPER_OR;
        else    break;

        advance(cont);
        KTL_AstNode *node_r = ktl_parse_cmp_step(cont);
        if (node_r == NULL) {
            ktl_destroy_node(node);
            return NULL;
        }

        KTL_AstNode *oper_node = ktl_alloc_node();
        if (oper_node == NULL)  {
            ktl_destroy_node(node);
            ktl_destroy_node(node_r);

            return NULL;
        }

        oper_node->kind         = KTL_AST_BINARY_OPER;
        oper_node->data.oper.op = oper;
        oper_node->pos          = pos;

        add_node_l(oper_node, node);
        add_node_r(oper_node, node_r);
        node = oper_node;
    }
    return node;
}


static KTL_AstNode *ktl_parse_line       (KTL_ParseContext *cont);
static KTL_AstNode *ktl_parse_var_decl   (KTL_ParseContext *cont);
static KTL_AstNode *ktl_parse_assign     (KTL_ParseContext *cont);
static KTL_AstNode *ktl_parse_condition  (KTL_ParseContext *cont);
static KTL_AstNode *ktl_parse_while      (KTL_ParseContext *cont);
static KTL_AstNode *ktl_parse_for        (KTL_ParseContext *cont);
static KTL_AstNode *ktl_parse_return     (KTL_ParseContext *cont);
static KTL_AstNode *ktl_parse_break      (KTL_ParseContext *cont);
static KTL_AstNode *ktl_parse_continue   (KTL_ParseContext *cont);
static KTL_AstNode *ktl_parse_exit       (KTL_ParseContext *cont);
static KTL_AstNode *ktl_parse_body       (KTL_ParseContext *cont);
static KTL_AstNode *ktl_parse_loop_common(KTL_ParseContext *cont, bool is_loop);
static KTL_AstNode *ktl_parse_typedef    (KTL_ParseContext *cont);
static KTL_AstNode *ktl_parse_block_decl (KTL_ParseContext *cont);
static KTL_AstNode *ktl_parse_func_decl  (KTL_ParseContext *cont);
static KTL_AstNode *ktl_parse_main       (KTL_ParseContext *cont);
static KTL_AstNode *ktl_parse_top_decl   (KTL_ParseContext *cont);


static KTL_AstNode *ktl_parse_break(KTL_ParseContext *cont) {
    assert(cont);

    if (equal(cont, KTL_KEY_BREAK) == false)    return NULL;

    KTL_SourcePos pos = get_t_pos(cont);

    if (cont->loop_depth == 0) {
        KTL_DiagEmit(cont->diag, pos,
                     KTL_DIAG_SEM_BREAK_OUTSIDE_LOOP,
                     KTL_DIAG_SEV_ERROR);
    }
    advance(cont);

    KTL_AstNode *node = ktl_alloc_node();
    if (node == NULL)   return NULL;

    node->kind = KTL_AST_BREAK;
    node->pos  = pos;

    return node;
}

static KTL_AstNode *ktl_parse_continue(KTL_ParseContext *cont) {
    assert(cont);

    if (equal(cont, KTL_KEY_CONTINUE) == false)     return NULL;

    KTL_SourcePos pos = get_t_pos(cont);

    if (cont->loop_depth == 0) {
        KTL_DiagEmit(cont->diag, pos,
                     KTL_DIAG_SEM_CONTINUE_OUTSIDE_LOOP,
                     KTL_DIAG_SEV_ERROR);
    }
    advance(cont);

    KTL_AstNode *node = ktl_alloc_node();
    if (node == NULL)   return NULL;

    node->kind = KTL_AST_CONTINUE;
    node->pos  = pos;

    return node;
}

static KTL_AstNode *ktl_parse_exit(KTL_ParseContext *cont) {
    assert(cont);

    if (equal(cont, KTL_KEY_EXIT) == false)     return NULL;

    KTL_SourcePos pos = get_t_pos(cont);
    advance(cont);

    KTL_AstNode *node = ktl_alloc_node();
    if (node == NULL)   return NULL;

    node->kind = KTL_AST_EXIT;
    node->pos  = pos;

    return node;
}

static KTL_AstNode *ktl_parse_return(KTL_ParseContext *cont) {
    assert(cont);

    if (equal(cont, KTL_KEY_RETURN) == false)   return NULL;

    KTL_SourcePos pos = get_t_pos(cont);

    if (cont->in_func == false) {
        KTL_DiagEmit(cont->diag, pos,
                     KTL_DIAG_SEM_RETURN_OUTSIDE_FUNC,
                     KTL_DIAG_SEV_ERROR);
    }
    advance(cont);

    KTL_AstNode *node = ktl_alloc_node();
    if (node == NULL)   return NULL;

    node->kind = KTL_AST_RETURN;
    node->pos  = pos;

    if (equal(cont, KTL_PARSE_END_LINE) == false) {
        KTL_AstNode *value = ktl_parse_expr(cont);
        if (value == NULL) {
            ktl_destroy_node(node);
            return NULL;
        }
        add_node_u(node, value);
    }

    return node;
}

static KTL_AstNode *ktl_parse_var_decl(KTL_ParseContext *cont) {
    assert(cont);

    if (equal(cont, KTL_KEY_VAR_DECL) == false)     return NULL;

    KTL_SourcePos pos = get_t_pos(cont);
    advance(cont);

    int        mod  = ktl_parse_type_mod(cont);
    KTL_TypeID type = ktl_parse_type(cont);
    if (TypeIDCheck(type) == false)     return NULL;

    KTL_SourcePos name_pos = get_t_pos(cont);
    KTL_StrID     name     = ktl_parse_name(cont);
    if (StrIDCheck(name) == false) {
        KTL_DiagEmit(cont->diag, name_pos,
                     KTL_DIAG_PARSE_EXPECTED_NAME,
                     KTL_DIAG_SEV_ERROR);
        return NULL;
    }

    KTL_AstNode *init = NULL;
    if (equal(cont, KTL_PARSE_ASSIGN)) {
        advance(cont);
        init = ktl_parse_expr(cont);
        if (init == NULL)   return NULL;
        mod |= KTL_VAR_INITIAL;
    }

    KTL_AstNode *node = ktl_alloc_node();
    if (node == NULL) {
        if (init != NULL)   ktl_destroy_node(init);
        return NULL;
    }

    node->kind                  = KTL_AST_VARIABLE_DECL;
    node->pos                   = pos;
    node->data.var_decl.is_init = (init != NULL);

    KTL_SymbolEntry *entry = KTL_SymbolInsertVar(cont->current_scope,
                                                  name, type, mod);
    if (entry == NULL) {
        KTL_DiagEmit(cont->diag, name_pos,
                     KTL_DIAG_SEM_REDECLARATION,
                     KTL_DIAG_SEV_ERROR, name);
    }
    node->data.var_decl.entry = entry;

    if (init != NULL)   add_node_u(node, init);

    return node;
}

static KTL_AstNode *ktl_parse_lvalue(KTL_ParseContext *cont) {
    assert(cont);

    KTL_AstNode *root = NULL;
    KTL_AstNode *tail = NULL;

    while (equal(cont, KTL_PARSE_PTR_DEREF)) {
        KTL_SourcePos pos = get_t_pos(cont);
        advance(cont);

        KTL_AstNode *deref = ktl_alloc_node();
        if (deref == NULL) {
            if (root != NULL)   ktl_destroy_node(root);
            return NULL;
        }
        deref->kind         = KTL_AST_UNARY_OPER;
        deref->data.oper.op = KTL_OPER_UNGET_PTR;
        deref->pos          = pos;

        if (root == NULL) {
            root = deref;
            tail = deref;
        } else {
            add_node_u(tail, deref);
            tail = deref;
        }
    }

    KTL_AstNode *var = ktl_parse_var(cont);
    if (var == NULL) {
        if (root != NULL)   ktl_destroy_node(root);
        return NULL;
    }

    if (root == NULL)   return var;

    add_node_u(tail, var);
    return root;
}

static KTL_AstNode *ktl_parse_assign(KTL_ParseContext *cont) {
    assert(cont);

    KTL_AstNode *lvalue = ktl_parse_lvalue(cont);
    if (lvalue == NULL)     return NULL;

    if (equal(cont, KTL_PARSE_ASSIGN) == false) {
        KTL_DiagEmit(cont->diag, get_t_pos(cont),
                     KTL_DIAG_PARSE_EXPECTED_TOKEN,
                     KTL_DIAG_SEV_ERROR, KTL_PARSE_ASSIGN);
        ktl_destroy_node(lvalue);
        return NULL;
    }

    KTL_SourcePos pos = get_t_pos(cont);
    advance(cont);

    KTL_AstNode *value = ktl_parse_expr(cont);
    if (value == NULL) {
        ktl_destroy_node(lvalue);
        return NULL;
    }

    KTL_AstNode *node = ktl_alloc_node();
    if (node == NULL) {
        ktl_destroy_node(lvalue);
        ktl_destroy_node(value);
        return NULL;
    }

    node->kind = KTL_AST_ASSIGN;
    node->pos  = pos;
    add_node_l(node, lvalue);
    add_node_r(node, value);

    return node;
}



static KTL_AstNode *ktl_parse_paren_cond(KTL_ParseContext *cont) {
    assert(cont);

    if (equal(cont, KTL_PARSE_PAREN_LEFT) == false) {
        KTL_DiagEmit(cont->diag, get_t_pos(cont),
                     KTL_DIAG_PARSE_EXPECTED_TOKEN,
                     KTL_DIAG_SEV_ERROR, KTL_PARSE_PAREN_LEFT);
        return NULL;
    }
    advance(cont);

    KTL_AstNode *cond = ktl_parse_expr(cont);
    if (cond == NULL)   return NULL;

    if (equal(cont, KTL_PARSE_PAREN_RIGHT) == false) {
        KTL_DiagEmit(cont->diag, get_t_pos(cont),
                     KTL_DIAG_PARSE_EXPECTED_TOKEN,
                     KTL_DIAG_SEV_ERROR, KTL_PARSE_PAREN_RIGHT);
        ktl_destroy_node(cond);
        return NULL;
    }
    advance(cont);

    return cond;
}

static KTL_AstNode *ktl_make_if_branch(KTL_AstNode  *cond,
                                       KTL_AstNode  *body,
                                       KTL_SourcePos pos) {
    assert(cond);
    assert(body);

    KTL_AstNode *branch = ktl_alloc_node();
    if (branch == NULL) {
        ktl_destroy_node(cond);
        ktl_destroy_node(body);
        return NULL;
    }

    branch->kind = KTL_AST_IF_BRANCH;
    branch->pos  = pos;
    add_node_l(branch, cond);
    add_node_r(branch, body);

    return branch;
}


static KTL_AstNode *ktl_parse_condition(KTL_ParseContext *cont) {
    assert(cont);

    if (equal(cont, KTL_KEY_IF) == false)   return NULL;

    KTL_SourcePos block_pos = get_t_pos(cont);
    advance(cont);

    KTL_AstNode *block = ktl_alloc_node();
    if (block == NULL)  return NULL;

    block->kind = KTL_AST_COND_BLOCK;
    block->pos  = block_pos;

    KTL_AstNode *cond = ktl_parse_paren_cond(cont);
    if (cond == NULL) {
        ktl_destroy_node(block);
        return NULL;
    }

    KTL_AstNode *body = ktl_parse_body(cont);
    if (body == NULL) {
        ktl_destroy_node(cond);
        ktl_destroy_node(block);
        return NULL;
    }

    KTL_AstNode *branch = ktl_make_if_branch(cond, body, block_pos);
    if (branch == NULL) {
        ktl_destroy_node(block);
        return NULL;
    }
    add_node_n(block, branch);

    while (equal(cont, KTL_KEY_ELIF)) {
        KTL_SourcePos elif_pos = get_t_pos(cont);
        advance(cont);

        cond = ktl_parse_paren_cond(cont);
        if (cond == NULL) {
            ktl_destroy_node(block);
            return NULL;
        }

        body = ktl_parse_body(cont);
        if (body == NULL) {
            ktl_destroy_node(cond);
            ktl_destroy_node(block);
            return NULL;
        }

        branch = ktl_make_if_branch(cond, body, elif_pos);
        if (branch == NULL) {
            ktl_destroy_node(block);
            return NULL;
        }
        add_node_n(block, branch);
    }

    if (equal(cont, KTL_KEY_ELSE)) {
        KTL_SourcePos else_pos = get_t_pos(cont);
        advance(cont);

        body = ktl_parse_body(cont);
        if (body == NULL) {
            ktl_destroy_node(block);
            return NULL;
        }

        KTL_AstNode *else_branch = ktl_alloc_node();
        if (else_branch == NULL) {
            ktl_destroy_node(body);
            ktl_destroy_node(block);
            return NULL;
        }

        else_branch->kind = KTL_AST_ELSE_BRANCH;
        else_branch->pos  = else_pos;
        add_node_u(else_branch, body);

        add_node_n(block, else_branch);
    }

    return block;
}


static KTL_AstNode *ktl_parse_while(KTL_ParseContext *cont) {
    assert(cont);

    if (equal(cont, KTL_KEY_WHILE) == false)    return NULL;

    KTL_SourcePos pos = get_t_pos(cont);
    advance(cont);

    KTL_AstNode *cond = ktl_parse_paren_cond(cont);
    if (cond == NULL)   return NULL;

    cont->loop_depth++;
    KTL_AstNode *body = ktl_parse_body(cont);
    cont->loop_depth--;

    if (body == NULL) {
        ktl_destroy_node(cond);
        return NULL;
    }

    KTL_AstNode *node = ktl_alloc_node();
    if (node == NULL) {
        ktl_destroy_node(cond);
        ktl_destroy_node(body);
        return NULL;
    }

    node->kind = KTL_AST_WHILE_BLOCK;
    node->pos  = pos;
    add_node_l(node, cond);
    add_node_r(node, body);

    return node;
}

static KTL_AstNode *ktl_parse_for(KTL_ParseContext *cont) {
    assert(cont);

    if (equal(cont, KTL_KEY_FOR) == false)  return NULL;

    /* c++ goto need it */
    KTL_AstNode *init = NULL;
    KTL_AstNode *cond = NULL;
    KTL_AstNode *step = NULL;
    KTL_AstNode *body = NULL;

    KTL_SourcePos pos = get_t_pos(cont);
    advance(cont);

    if (equal(cont, KTL_PARSE_PAREN_LEFT) == false) {
        KTL_DiagEmit(cont->diag, get_t_pos(cont),
                     KTL_DIAG_PARSE_EXPECTED_TOKEN,
                     KTL_DIAG_SEV_ERROR, KTL_PARSE_PAREN_LEFT);
        return NULL;
    }
    advance(cont);

    KTL_SymbolMap *scope     = KTL_SymbolMapInit(cont->current_scope);
    if (scope == NULL)  return NULL;
    KTL_SymbolMap *prev_scope = cont->current_scope;
    cont->current_scope       = scope;

    KTL_AstNode *node = ktl_alloc_node();
    if (node == NULL) {
        cont->current_scope = prev_scope;
        KTL_SymbolMapUninit(scope);
        return NULL;
    }
    node->kind                = KTL_AST_FOR_BLOCK;
    node->pos                 = pos;
    node->data.for_block.map  = scope;

    init = NULL;
    if (equal(cont, KTL_PARSE_END_LINE) == false) {
        if (equal(cont, KTL_KEY_VAR_DECL))  init = ktl_parse_var_decl(cont);
        else                                init = ktl_parse_assign  (cont);

        if (init == NULL)   goto fail;
    }
    if (equal(cont, KTL_PARSE_END_LINE) == false) {
        KTL_DiagEmit(cont->diag, get_t_pos(cont),
                     KTL_DIAG_PARSE_EXPECTED_TOKEN,
                     KTL_DIAG_SEV_ERROR, KTL_PARSE_END_LINE);
        ktl_destroy_node(init);
        goto fail;
    }
    advance(cont);

    cond = NULL;
    if (equal(cont, KTL_PARSE_END_LINE) == false) {
        cond = ktl_parse_expr(cont);
        if (cond == NULL) {
            ktl_destroy_node(init);
            goto fail;
        }
    }
    if (equal(cont, KTL_PARSE_END_LINE) == false) {
        KTL_DiagEmit(cont->diag, get_t_pos(cont),
                     KTL_DIAG_PARSE_EXPECTED_TOKEN,
                     KTL_DIAG_SEV_ERROR, KTL_PARSE_END_LINE);
        ktl_destroy_node(init);
        ktl_destroy_node(cond);
        goto fail;
    }
    advance(cont);

    step = NULL;
    if (equal(cont, KTL_PARSE_PAREN_RIGHT) == false) {
        step = ktl_parse_assign(cont);
        if (step == NULL) {
            ktl_destroy_node(init);
            ktl_destroy_node(cond);
            goto fail;
        }
    }
    if (equal(cont, KTL_PARSE_PAREN_RIGHT) == false) {
        KTL_DiagEmit(cont->diag, get_t_pos(cont),
                     KTL_DIAG_PARSE_EXPECTED_TOKEN,
                     KTL_DIAG_SEV_ERROR, KTL_PARSE_PAREN_RIGHT);
        ktl_destroy_node(init);
        ktl_destroy_node(cond);
        ktl_destroy_node(step);
        goto fail;
    }
    advance(cont);

    /* body */
    cont->loop_depth++;
    body = ktl_parse_body(cont);
    cont->loop_depth--;

    if (body == NULL) {
        ktl_destroy_node(init);
        ktl_destroy_node(cond);
        ktl_destroy_node(step);
        goto fail;
    }

    cont->current_scope = prev_scope;

    add_node_n(node, init);
    add_node_n(node, cond);
    add_node_n(node, step);
    add_node_n(node, body);

    return node;

fail:
    cont->current_scope = prev_scope;
    ktl_destroy_node(node);
    return NULL;
}

static KTL_AstNode *ktl_parse_line(KTL_ParseContext *cont) {
    assert(cont);

    if (equal(cont, KTL_KEY_VAR_DECL))      return ktl_parse_var_decl(cont);
    if (equal(cont, KTL_KEY_IF))            return ktl_parse_condition(cont);
    if (equal(cont, KTL_KEY_WHILE))         return ktl_parse_while(cont);
    if (equal(cont, KTL_KEY_FOR))           return ktl_parse_for(cont);
    if (equal(cont, KTL_KEY_RETURN))        return ktl_parse_return(cont);
    if (equal(cont, KTL_KEY_BREAK))         return ktl_parse_break(cont);
    if (equal(cont, KTL_KEY_CONTINUE))      return ktl_parse_continue(cont);
    if (equal(cont, KTL_KEY_EXIT))          return ktl_parse_exit(cont);

    if (equal(cont, KTL_PARSE_PTR_DEREF))   return ktl_parse_assign(cont);

    if (equal(cont, KTL_TOKEN_STRING)) {
        if (equal_n(cont, KTL_PARSE_PAREN_LEFT))    return ktl_parse_call(cont);
        else                                        return ktl_parse_assign(cont);
    }

    KTL_DiagEmit(cont->diag, get_t_pos(cont),
                 KTL_DIAG_PARSE_UNEXPECTED_TOKEN,
                 KTL_DIAG_SEV_ERROR);
    return NULL;
}

static KTL_AstNode *ktl_parse_loop_common(KTL_ParseContext *cont, bool is_loop) {
    assert(cont);

    if (equal(cont, KTL_PARSE_BLOCK_LEFT) == false) {
        KTL_DiagEmit(cont->diag, get_t_pos(cont),
                     KTL_DIAG_PARSE_EXPECTED_TOKEN,
                     KTL_DIAG_SEV_ERROR, KTL_PARSE_BLOCK_LEFT);
        return NULL;
    }
    KTL_SourcePos block_pos = get_t_pos(cont);
    advance(cont);

    KTL_SymbolMap *scope      = KTL_SymbolMapInit(cont->current_scope);
    if (scope == NULL)  return NULL;
    KTL_SymbolMap *prev_scope = cont->current_scope;
    cont->current_scope       = scope;

    KTL_AstNode *block = ktl_alloc_node();
    if (block == NULL) {
        cont->current_scope = prev_scope;
        KTL_SymbolMapUninit(scope);
        return NULL;
    }
    block->kind           = KTL_AST_BLOCK;
    block->pos            = block_pos;
    block->data.block.map = scope;

    if (is_loop)    cont->loop_depth++;

    while (equal(cont, KTL_PARSE_BLOCK_RIGHT) == false) {
        KTL_AstNode *node = ktl_parse_line(cont);
        if (node == NULL)   goto fail;

        if (equal(cont, KTL_PARSE_END_LINE) == false) {
            KTL_DiagEmit(cont->diag, get_t_pos(cont),
                         KTL_DIAG_PARSE_EXPECTED_TOKEN,
                         KTL_DIAG_SEV_ERROR, KTL_PARSE_END_LINE);
            ktl_destroy_node(node);
            goto fail;
        }
        advance(cont);
        add_node_n(block, node);
    }
    advance(cont);   /* съедаем "}" */

    cont->current_scope = prev_scope;
    if (is_loop)    cont->loop_depth--;
    return block;

fail:
    cont->current_scope = prev_scope;
    if (is_loop)    cont->loop_depth--;
    ktl_destroy_node(block);   /* уничтожит scope через data.block.map */
    return NULL;
}

static KTL_AstNode *ktl_parse_body(KTL_ParseContext *cont) {
    assert(cont);
    return ktl_parse_loop_common(cont, false);
}


static KTL_AstNode *ktl_parse_typedef(KTL_ParseContext *cont) {
    assert(cont);

    if (equal(cont, KTL_KEY_TYPEDEF) == false)  return NULL;

    KTL_SourcePos pos = get_t_pos(cont);
    advance(cont);

    KTL_TypeID base = ktl_parse_type(cont);
    if (TypeIDCheck(base) == false)     return NULL;

    if (equal(cont, KTL_PARSE_TYPEDEF_ARROW) == false) {
        KTL_DiagEmit(cont->diag, get_t_pos(cont),
                     KTL_DIAG_PARSE_EXPECTED_TOKEN,
                     KTL_DIAG_SEV_ERROR, KTL_PARSE_TYPEDEF_ARROW);
        return NULL;
    }
    advance(cont);

    KTL_SourcePos name_pos = get_t_pos(cont);
    KTL_StrID     alias    = ktl_parse_name(cont);
    if (StrIDCheck(alias) == false) {
        KTL_DiagEmit(cont->diag, name_pos,
                     KTL_DIAG_PARSE_EXPECTED_NAME,
                     KTL_DIAG_SEV_ERROR);
        return NULL;
    }

    KTL_TypeID result = KTL_TypeAddDefine(cont->type_map, base, alias);
    if (TypeIDCheck(result) == false) {
        KTL_DiagEmit(cont->diag, name_pos,
                     KTL_DIAG_SEM_REDECLARATION,
                     KTL_DIAG_SEV_ERROR, alias);
    }

    KTL_AstNode *node = ktl_alloc_node();
    if (node == NULL)   return NULL;

    node->kind                  = KTL_AST_TYPEDEF;
    node->pos                   = pos;
    node->data.typedef_.base_id = base;
    node->data.typedef_.alias   = alias;

    return node;
}

static KTL_TypeID ktl_parse_field(KTL_ParseContext *cont,
                                  KTL_StrID        *out_name,
                                  KTL_SourcePos    *out_pos) {
    assert(cont);
    assert(out_name);
    assert(out_pos);

    /* Skip modifiers. Fields don't use it */
    (void) ktl_parse_type_mod(cont);

    KTL_TypeID type = ktl_parse_type(cont);
    if (TypeIDCheck(type) == false)     return KTL_BAD_TYPE_ID;

    *out_pos  = get_t_pos     (cont);
    *out_name = ktl_parse_name(cont);
    if (StrIDCheck(*out_name) == false) {
        KTL_DiagEmit(cont->diag, *out_pos,
                     KTL_DIAG_PARSE_EXPECTED_NAME,
                     KTL_DIAG_SEV_ERROR);
        return KTL_BAD_TYPE_ID;
    }

    return type;
}

static KTL_AstNode *ktl_parse_block_decl(KTL_ParseContext *cont) {
    assert(cont);

    if (equal(cont, KTL_KEY_STRUCT) == false)   return NULL;

    KTL_SourcePos pos = get_t_pos(cont);
    advance(cont);

    KTL_SourcePos name_pos = get_t_pos     (cont);
    KTL_StrID     name     = ktl_parse_name(cont);
    if (StrIDCheck(name) == false) {
        KTL_DiagEmit(cont->diag, name_pos,
                     KTL_DIAG_PARSE_EXPECTED_NAME,
                     KTL_DIAG_SEV_ERROR);
        return NULL;
    }

    if (equal(cont, KTL_PARSE_BLOCK_LEFT) == false) {
        KTL_DiagEmit(cont->diag, get_t_pos(cont),
                     KTL_DIAG_PARSE_EXPECTED_TOKEN,
                     KTL_DIAG_SEV_ERROR, KTL_PARSE_BLOCK_LEFT);
        return NULL;
    }
    advance(cont);

    KTL_TypeID block_id = KTL_TypeAddBlock(cont->type_map, name);
    if (TypeIDCheck(block_id) == false) {
        KTL_DiagEmit(cont->diag, name_pos,
                     KTL_DIAG_SEM_REDECLARATION,
                     KTL_DIAG_SEV_ERROR, name);
    }

    bool has_field = false;
    while (equal(cont, KTL_PARSE_BLOCK_RIGHT) == false) {
        KTL_StrID     field_name;
        KTL_SourcePos field_pos;
        KTL_TypeID    field_type = ktl_parse_field(cont, &field_name, &field_pos);

        if (TypeIDCheck(field_type) == false)   return NULL;

        if (TypeIDCheck(block_id)) {
            KTL_Error err = KTL_TypeBlockAddField(cont->type_map, block_id,
                                                  field_type, field_name);
            if (err != KTL_OK) {
                KTL_DiagEmit(cont->diag, field_pos,
                             KTL_DIAG_SEM_REDECLARATION,
                             KTL_DIAG_SEV_ERROR, field_name);
            }
        }
        has_field = true;

        if (equal(cont, KTL_PARSE_END_LINE) == false) {
            KTL_DiagEmit(cont->diag, get_t_pos(cont),
                         KTL_DIAG_PARSE_EXPECTED_TOKEN,
                         KTL_DIAG_SEV_ERROR, KTL_PARSE_END_LINE);
            return NULL;
        }
        advance(cont);
    }
    advance(cont);   /* "}" */

    if (has_field == false) {
        KTL_DiagEmit(cont->diag, pos,
                     KTL_DIAG_PARSE_UNEXPECTED_TOKEN,
                     KTL_DIAG_SEV_ERROR);
    }

    /*
    if (equal(cont, KTL_PARSE_END_LINE) == false) {
        KTL_DiagEmit(cont->diag, get_t_pos(cont),
                     KTL_DIAG_PARSE_EXPECTED_TOKEN,
                     KTL_DIAG_SEV_ERROR, KTL_PARSE_END_LINE);
        return NULL;
    }
    advance(cont);
    */

    if (TypeIDCheck(block_id)) {
        KTL_TypeBlockFinish(cont->type_map, block_id);
    }

    KTL_AstNode *node = ktl_alloc_node();
    if (node == NULL)   return NULL;

    node->kind                     = KTL_AST_STRUCT_DECL;
    node->pos                      = pos;
    node->data.struct_decl.type_id = block_id;

    return node;
}

static KTL_SymbolEntry *ktl_parse_param(KTL_ParseContext *cont) {
    assert(cont);

    int        mod      = ktl_parse_type_mod(cont);
    KTL_TypeID type     = ktl_parse_type(cont);
    if (TypeIDCheck(type) == false)     return NULL;

    KTL_SourcePos name_pos = get_t_pos(cont);
    KTL_StrID     name     = ktl_parse_name(cont);
    if (StrIDCheck(name) == false) {
        KTL_DiagEmit(cont->diag, name_pos,
                     KTL_DIAG_PARSE_EXPECTED_NAME,
                     KTL_DIAG_SEV_ERROR);
        return NULL;
    }

    KTL_SymbolEntry *entry = KTL_SymbolInsertVar(cont->current_scope,
                                                 name, type, mod);
    if (entry == NULL) {
        KTL_DiagEmit(cont->diag, name_pos,
                     KTL_DIAG_SEM_REDECLARATION,
                     KTL_DIAG_SEV_ERROR, name);
    }
    return entry;
}

static KTL_AstNode *ktl_parse_func_decl(KTL_ParseContext *cont) {
    assert(cont);

    if (equal(cont, KTL_KEY_FUNC_DECL) == false)    return NULL;

    KTL_SourcePos pos = get_t_pos(cont);
    advance(cont);

    int mod = ktl_parse_type_mod(cont);
    (void) mod;

    KTL_TypeID ret_type = ktl_parse_type(cont);
    if (TypeIDCheck(ret_type) == false)     return NULL;

    KTL_SourcePos name_pos = get_t_pos(cont);
    KTL_StrID     name     = ktl_parse_name(cont);
    if (StrIDCheck(name) == false) {
        KTL_DiagEmit(cont->diag, name_pos,
                     KTL_DIAG_PARSE_EXPECTED_NAME,
                     KTL_DIAG_SEV_ERROR);
        return NULL;
    }

    KTL_SymbolEntry *func = KTL_SymbolInsertFunc(cont->global_map,
                                                  name, ret_type);
    if (func == NULL) {
        KTL_DiagEmit(cont->diag, name_pos,
                     KTL_DIAG_SEM_REDECLARATION,
                     KTL_DIAG_SEV_ERROR, name);
    }

    /* "(" */
    if (equal(cont, KTL_PARSE_PAREN_LEFT) == false) {
        KTL_DiagEmit(cont->diag, get_t_pos(cont),
                     KTL_DIAG_PARSE_EXPECTED_TOKEN,
                     KTL_DIAG_SEV_ERROR, KTL_PARSE_PAREN_LEFT);

        return NULL;
    }
    advance(cont);

    /* Scope for params */
    KTL_SymbolMap *param_scope = KTL_SymbolMapInit(cont->global_map);
    if (param_scope == NULL)    return NULL;
    KTL_SymbolMap *prev_scope  = cont->current_scope;
    cont->current_scope        = param_scope;

    KTL_SymbolEntry *params[64];   /* TODO: dynamic, if needed */
    int param_count = 0;

    while (equal(cont, KTL_PARSE_PAREN_RIGHT) == false) {
        if (param_count >= 64) {
            KTL_DiagEmit(cont->diag, get_t_pos(cont),
                         KTL_DIAG_PARSE_UNEXPECTED_TOKEN,
                         KTL_DIAG_SEV_ERROR);

            cont->current_scope = prev_scope;
            KTL_SymbolMapUninit(param_scope);
            return NULL;
        }

        KTL_SymbolEntry *par = ktl_parse_param(cont);
        if (par == NULL) {
            cont->current_scope = prev_scope;
            KTL_SymbolMapUninit(param_scope);
            return NULL;
        }
        params[param_count++] = par;

        if (equal(cont, KTL_PARSE_ARG_SEP) == false)    break;
        advance(cont);
    }

    if (equal(cont, KTL_PARSE_PAREN_RIGHT) == false) {
        KTL_DiagEmit(cont->diag, get_t_pos(cont),
                     KTL_DIAG_PARSE_EXPECTED_TOKEN,
                     KTL_DIAG_SEV_ERROR, KTL_PARSE_PAREN_RIGHT);

        cont->current_scope = prev_scope;
        KTL_SymbolMapUninit(param_scope);
        return NULL;
    }
    advance(cont);

    if (func != NULL) {
        KTL_SymbolFuncSetParams(func, params, param_count);
    }

    bool prev_in_func = cont->in_func;
    cont->in_func = true;

    KTL_AstNode *body = ktl_parse_body(cont);

    cont->in_func        = prev_in_func;
    cont->current_scope  = prev_scope;

    if (body == NULL) {
        KTL_SymbolMapUninit(param_scope);
        return NULL;
    }

    KTL_AstNode *node = ktl_alloc_node();
    if (node == NULL) {
        ktl_destroy_node(body);
        KTL_SymbolMapUninit(param_scope);
        return NULL;
    }

    node->kind                 = KTL_AST_FUNCTION_DECL;
    node->pos                  = pos;
    node->data.func_decl.func  = func;
    node->data.func_decl.map   = param_scope;

    add_node_n(node, body);

    return node;
}

static KTL_AstNode *ktl_parse_main(KTL_ParseContext *cont) {
    assert(cont);

    if (equal(cont, KTL_KEY_MAIN) == false)     return NULL;

    KTL_SourcePos pos = get_t_pos(cont);

    if (cont->main_seen) {
        KTL_DiagEmit(cont->diag, pos,
                     KTL_DIAG_SEM_DUPLICATE_MAIN,
                     KTL_DIAG_SEV_ERROR);
    }
    cont->main_seen = true;
    advance(cont);

    KTL_AstNode *body = ktl_parse_body(cont);
    if (body == NULL)   return NULL;

    KTL_AstNode *node = ktl_alloc_node();
    if (node == NULL) {
        ktl_destroy_node(body);
        return NULL;
    }

    node->kind = KTL_AST_MAIN;
    node->pos  = pos;
    add_node_n(node, body);

    return node;
}

static KTL_AstNode *ktl_parse_top_decl(KTL_ParseContext *cont) {
    assert(cont);

    if (equal(cont, KTL_KEY_VAR_DECL)) {
        KTL_AstNode *node = ktl_parse_var_decl(cont);
        if (node == NULL)   return NULL;

        if (equal(cont, KTL_PARSE_END_LINE) == false) {
            KTL_DiagEmit(cont->diag, get_t_pos(cont),
                         KTL_DIAG_PARSE_EXPECTED_TOKEN,
                         KTL_DIAG_SEV_ERROR, KTL_PARSE_END_LINE);
            ktl_destroy_node(node);
            return NULL;
        }
        advance(cont);
        return node;
    }

    if (equal(cont, KTL_KEY_FUNC_DECL))     return ktl_parse_func_decl(cont);
    if (equal(cont, KTL_KEY_TYPEDEF))       return ktl_parse_typedef(cont);
    if (equal(cont, KTL_KEY_STRUCT))        return ktl_parse_block_decl(cont);

    return NULL;
}



static KTL_Error ktl_register_standard_types(KTL_ParseContext *cont) {
    assert(cont);
    assert(cont->str_map);
    assert(cont->type_map);


    int count = sizeof(KTL_STANDARD_TYPES) / sizeof(KTL_STANDARD_TYPES[0]);
    for (int i = 0; i < count; i++) {
        KTL_StrID name_id = KTL_StrMapFind(cont->str_map, KTL_STANDARD_TYPES[i].name);
        if (StrIDCheck(name_id) == false)   return KTL_MEMORY_ERR;

        KTL_TypeID id = KTL_TypeMapAddBase(cont->type_map, name_id,
                                            KTL_STANDARD_TYPES[i].size,
                                            KTL_STANDARD_TYPES[i].align);
        if (TypeIDCheck(id) == false)       return KTL_LOGICAL_ERR;
    }

    return KTL_OK;
}

static KTL_AstNode *ktl_parse_file(KTL_ParseContext *cont) {
    assert(cont);

    KTL_SourcePos pos = get_t_pos(cont);

    KTL_AstNode *file = ktl_alloc_node();
    if (file == NULL)   return NULL;

    file->kind = KTL_AST_FILE;
    file->pos  = pos;

    /* Infinity circle check */
    while (cont->cur_token < cont->cap_token) {
        int pos_before = cont->cur_token;

        KTL_AstNode *node = NULL;
        if (equal(cont, KTL_KEY_MAIN))      node = ktl_parse_main(cont);
        else                                node = ktl_parse_top_decl(cont);

        if (node == NULL) {
            if (cont->cur_token == pos_before) {
                KTL_DiagEmit(cont->diag, get_t_pos(cont),
                             KTL_DIAG_PARSE_UNEXPECTED_TOKEN,
                             KTL_DIAG_SEV_ERROR);
                advance(cont);
            }

            if (cont->diag->fatal_count > 0)    break;
            continue;
        }

        add_node_n(file, node);
    }

    if (cont->main_seen == false) {
        KTL_DiagEmit(cont->diag, pos,
                     KTL_DIAG_SEM_NO_MAIN,
                     KTL_DIAG_SEV_ERROR);
    }

    return file;
}

// =======================================================================
// API
// =======================================================================

KTL_Error KTL_ParseInit(KTL_ParseContext *cont,
                        KTL_Token        *tokens,
                        int               token_count,
                        KTL_StrMap       *str_map,
                        KTL_TypeMap      *type_map,
                        KTL_Diagnostic   *diag) {
    assert(cont);
    assert(tokens);
    assert(str_map);
    assert(type_map);
    assert(diag);
    if (token_count <= 0)   ExitF("Bad token count", KTL_BAD_ARG_ERR);

    cont->tokens     = tokens;
    cont->cur_token  = 0;
    cont->cap_token  = token_count;

    cont->str_map    = str_map;
    cont->type_map   = type_map;
    cont->diag       = diag;

    cont->root       = NULL;

    cont->global_map    = KTL_SymbolMapInit(NULL);
    if (cont->global_map == NULL)   ExitF("global_map init failed", KTL_MEMORY_ERR);

    cont->current_scope = cont->global_map;

    cont->loop_depth = 0;
    cont->main_seen  = false;
    cont->in_func    = false;

    KTL_Error err = ktl_register_standard_types(cont);
    if (err != KTL_OK) {
        KTL_SymbolMapUninit(cont->global_map);
        cont->global_map = NULL;
        return err;
    }

    return KTL_OK;
}

KTL_Error KTL_ParseProcess(KTL_ParseContext *cont) {
    assert(cont);
    assert(cont->tokens);
    assert(cont->global_map);

    cont->root = ktl_parse_file(cont);
    if (cont->root == NULL)     return KTL_LOGICAL_ERR;

    if (cont->diag->fatal_count > 0)    return KTL_LOGICAL_ERR;

    return KTL_OK;
}

KTL_Error KTL_ParseUninit(KTL_ParseContext *cont) {
    assert(cont);

    if (cont->root != NULL) {
        ktl_destroy_node(cont->root);
        cont->root = NULL;
    }

    if (cont->global_map != NULL) {
        KTL_SymbolMapUninit(cont->global_map);
        cont->global_map = NULL;
    }

    cont->current_scope = NULL;
    cont->tokens        = NULL;
    cont->str_map       = NULL;
    cont->type_map      = NULL;
    cont->diag          = NULL;

    return KTL_OK;
}



// =======================================================================
// TINY HELPER FUNCTIONS
// =======================================================================


static inline KTL_Token * get_t(KTL_ParseContext *cont) {
    return cont->tokens + cont->cur_token;
}

static inline KTL_Token * get_nt(KTL_ParseContext *cont) {
    return cont->tokens + cont->cur_token + (cont->cur_token < cont->cap_token);
}

static inline void advance(KTL_ParseContext *cont) {
    cont->cur_token += cont->cur_token < cont->cap_token;
}



static inline bool equal(KTL_ParseContext *cont, KTL_ParseTokenRef ref) {
    if (ref.kind == KTL_TOKEN_PUNCT) {
        return  get_t(cont)->kind == KTL_TOKEN_PUNCT &&
                get_t(cont)->data.punct == ref.as.punct;
    }
    return  get_t(cont)->kind == KTL_TOKEN_KEY &&
            get_t(cont)->data.key == ref.as.key;
}

static inline bool equal(KTL_ParseContext *cont, KTL_TokenKind kind) {
    return get_t(cont)->kind == kind;
}

static inline bool equal(KTL_ParseContext *cont, KTL_KeyWord key) {
    return equal(cont, KTL_TOKEN_KEY) && get_t(cont)->data.key == key;
}


static inline bool equal_n(KTL_ParseContext *cont, KTL_ParseTokenRef ref) {
    if (ref.kind == KTL_TOKEN_PUNCT) {
        return  get_nt(cont)->kind == KTL_TOKEN_PUNCT &&
                get_nt(cont)->data.punct == ref.as.punct;
    }
    return  get_nt(cont)->kind == KTL_TOKEN_KEY &&
            get_nt(cont)->data.key == ref.as.key;
}

static inline bool equal_n(KTL_ParseContext *cont, KTL_TokenKind kind) {
    return get_nt(cont)->kind == kind;
}

static inline bool equal_n(KTL_ParseContext *cont, KTL_KeyWord key) {
    return equal_n(cont, KTL_TOKEN_KEY) && get_nt(cont)->data.key == key;
}


static inline void add_node_l(KTL_AstNode *parent, KTL_AstNode *child) {
    parent->move.binary.left = child;
}

static inline void add_node_r(KTL_AstNode *parent, KTL_AstNode *child) {
    parent->move.binary.right = child;
}

static inline void add_node_u(KTL_AstNode *parent, KTL_AstNode *child) {
    parent->move.unary.next = child;
}

static void add_node_n(KTL_AstNode *parent, KTL_AstNode *child) {
    assert(parent);
    assert(child);

    if (parent->move.n.children == NULL) {
        parent->move.n.children = (KTL_AstNode **)calloc(
            KTL_PARSE_NCHILDREN_INIT, sizeof(KTL_AstNode *));
        if (parent->move.n.children == NULL)  ExitF("NULL Calloc", );

        parent->move.n.amount = 0;
    }

    /* Cap always = 2^n */
    int amount = parent->move.n.amount;
    if (amount >= KTL_PARSE_NCHILDREN_INIT &&
        (amount & (amount - 1)) == 0) {
        int new_cap = amount * 2;
        KTL_AstNode **buf = (KTL_AstNode **)realloc(
            parent->move.n.children,
            (size_t) new_cap * sizeof(KTL_AstNode *));

        if (buf == NULL)  ExitF("NULL Realloc", );
        parent->move.n.children = buf;
    }

    parent->move.n.children[parent->move.n.amount++] = child;
}

static inline int get_pos(KTL_ParseContext *cont) {
    return cont->cur_token;
}

static inline void set_pos(KTL_ParseContext *cont, int pos) {
    assert(pos >= 0);
    cont->cur_token = pos;
}

static KTL_AstNode * ktl_alloc_node() {
    KTL_AstNode *node = (KTL_AstNode *)calloc(1, sizeof(KTL_AstNode));
    if (node == NULL) {
        ExitF("NULL Calloc", NULL);
    }
    return node;
}

static void ktl_destroy_node(KTL_AstNode *node) {
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

static inline KTL_SourcePos get_t_pos(KTL_ParseContext *cont) {
    return get_t(cont)->pos;
}


