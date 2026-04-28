#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "Token.h"
#include "ParseType.h"
#include "ParseConfig.h"
#include "TypeMap.h"
#include "Common.h"

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
        if (equal(cont, KTL_KEY_CONST))         mod |= KTL_VAR_CONST;
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
    if ((mod & KTL_VAR_STACK) && (mod & KTL_VAR_REGISTER)) {
        KTL_DiagEmit(cont->diag, get_t_pos(cont),
                     KTL_DIAG_PARSE_BAD_MODIFIERS,
                     KTL_DIAG_SEV_ERROR, (int64_t) mod);

        mod &= ~KTL_VAR_REGISTER;
    } else if ((mod & KTL_VAR_STACK) == false &&
               (mod & KTL_VAR_REGISTER) == false) {
        mod |= KTL_VAR_REGISTER;
    }

    return mod;
}

/* Emit errors */
static KTL_TypeID ktl_parse_type(KTL_ParseContext *cont) {
    assert(cont);

    KTL_StrID type_name = ktl_parse_name(cont);
    if (StrIDCheck(type_name) == false) {
        KTL_DiagEmit(cont->diag, get_t_pos(cont),
                     KTL_DIAG_PARSE_EXPECTED_TYPE,
                     KTL_DIAG_SEV_ERROR);

        return KTL_BAD_TYPE_ID;
    }

    KTL_TypeID type = KTL_TypeFindByName(cont->type_map, type_name);
    if (TypeIDCheck(type) == false) {
        KTL_DiagEmit(cont->diag, get_t_pos(cont),
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
        }
        advance(cont);

        type = KTL_TypeAddArray(cont->type_map, type, size);
    }

    return type;
}

/* Emit errors */
static KTL_AstNode *ktl_parse_str_literal(KTL_ParseContext *cont) {
    assert(cont);

    if (equal(cont, KTL_PARSE_STR_LITERAL_LEFT) == false)   return NULL;
    advance(cont);

    KTL_SourcePos pos   = get_t_pos(cont);
    KTL_StrID string    = ktl_parse_name(cont);

    if (StrIDCheck(string) == false) {
        KTL_DiagEmit(cont->diag, get_t_pos(cont),
                     KTL_DIAG_PARSE_EXPECTED_STRING,
                     KTL_DIAG_SEV_ERROR);

        return NULL;
    }
    advance(cont);

    if (equal(cont, KTL_PARSE_STR_LITERAL_RIGHT) == false) {
        KTL_DiagEmit(cont->diag, get_t_pos(cont),
                     KTL_DIAG_PARSE_EXPECTED_TOKEN,
                     KTL_DIAG_SEV_ERROR, KTL_PARSE_STR_LITERAL_RIGHT);

        return NULL;
    }
    advance(cont);

    KTL_AstNode *node = ktl_alloc_node();
    if (node == NULL)   return NULL;

    node->kind               = KTL_AST_VALUE_STR;
    node->data.str_val.value = string;
    node->pos                = pos;

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

    KTL_StrID name = ktl_parse_name(cont);
    if (StrIDCheck(name) == true) {
        KTL_DiagEmit(cont->diag, get_t_pos(cont),
                     KTL_DIAG_PARSE_EXPECTED_NAME,
                     KTL_DIAG_SEV_ERROR);

        return NULL;
    }
    KTL_AstNode *node = ktl_alloc_node();
    if (node == NULL)   return NULL;

    node->kind                   = KTL_AST_VARIABLE;
    node->data.var.info.raw.name = name;

    KTL_AstNode *cur_node = node;

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

            if (cur_node->kind == KTL_AST_INDEX_ACCESS)     add_node_r(cur_node, field);
            else                                            add_node_u(cur_node, field);

            cur_node = field;

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
            if (cur_node->kind == KTL_AST_INDEX_ACCESS)     add_node_r(cur_node, get_idx);
            else                                            add_node_u(cur_node, get_idx);

            add_node_l(get_idx, idx);
            cur_node = get_idx;
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

    KTL_StrID name = ktl_parse_name(cont);
    if (StrIDCheck(name) == true) {
        KTL_DiagEmit(cont->diag, get_t_pos(cont),
                     KTL_DIAG_PARSE_EXPECTED_NAME,
                     KTL_DIAG_SEV_ERROR);

        return NULL;
    }
    KTL_AstNode *node = ktl_alloc_node();
    if (node == NULL)   return NULL;

    node->kind                         = KTL_AST_FUNCTION_CALL;
    node->data.func_call.info.raw.name = name;

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

/* Emit errors */
static KTL_AstNode *ktl_parse_atom(KTL_ParseContext *cont) {
    assert(cont);

    KTL_AstNode *node = NULL;

    if (equal(cont, KTL_PARSE_PAREN_LEFT)) {
        advance(cont);

        node = ktl_parse_expr(cont);
        if (node == NULL)   return NULL;
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

static KTL_AstNode *ktl_parse_unary(KTL_ParseContext *cont) {
    assert(cont);

    KTL_AstNode *node     = NULL;
    KTL_AstNode *cur_node = node;

    while (true) {
        if (equal(cont, KTL_PARSE_PTR_ADDR)  ||
            equal(cont, KTL_PARSE_PTR_DEREF) ||
            equal(cont, KTL_PARSE_OP_NEG))  break;

        KTL_Oper oper = KTL_OPER_GET_PTR;
        if (equal(cont, KTL_PARSE_PTR_DEREF))   oper = KTL_OPER_UNGET_PTR;
        if (equal(cont, KTL_PARSE_OP_NEG))      oper = KTL_OPER_NEG;

        if (node = NULL) {
            node = ktl_alloc_node();

            if (node == NULL)   return NULL;

            node->kind         = KTL_AST_UNARY_OPER;
            node->data.oper.op = oper;
            cur_node           = node;

        } else {
            KTL_AstNode *new_oper = ktl_alloc_node();
            if (new_oper == NULL)   return NULL;

            new_oper->kind         = KTL_AST_UNARY_OPER;
            new_oper->data.oper.op = oper;

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

static KTL_AstNode *ktl_parse_mul_step(KTL_ParseContext *cont) {
    assert(cont);

    KTL_AstNode *node = ktl_parse_unary(cont);
    if (node == NULL)   return NULL;

    KTL_Oper oper = KTL_OPER_SUB;
    if      (equal(cont, KTL_PARSE_OP_MUL))  oper = KTL_OPER_MUL;
    else if (equal(cont, KTL_PARSE_OP_DIV))  oper = KTL_OPER_DIV;
    else if (equal(cont, KTL_PARSE_OP_MOD))  oper = KTL_OPER_MOD;
    else    return node;

    advance(cont);

    KTL_AstNode *oper_node = ktl_alloc_node();
    if (oper_node == NULL)  {
        ktl_destroy_node(node);
        return NULL;
    }

    oper_node->kind         = KTL_AST_BINARY_OPER;
    oper_node->data.oper.op = oper;

    add_node_l(oper_node, node);

    KTL_AstNode *node_r = ktl_parse_mul_step(cont);
    if (node_r == NULL) {
        ktl_destroy_node(node);
        free(oper_node);

        return NULL;
    }

    add_node_r(oper_node, node_r);
    return oper_node;
}

static KTL_AstNode *ktl_parse_add_step(KTL_ParseContext *cont) {
    assert(cont);

    KTL_AstNode *node = ktl_parse_mul_step(cont);
    if (node == NULL)   return NULL;

    KTL_Oper oper = KTL_OPER_SUB;
    if      (equal(cont, KTL_PARSE_OP_PLUS))   oper = KTL_OPER_ADD;
    else if (equal(cont, KTL_PARSE_OP_MINUS))  oper = KTL_OPER_SUB;
    else    return node;

    advance(cont);

    KTL_AstNode *oper_node = ktl_alloc_node();
    if (oper_node == NULL)  {
        ktl_destroy_node(node);
        return NULL;
    }

    oper_node->kind         = KTL_AST_BINARY_OPER;
    oper_node->data.oper.op = oper;

    add_node_l(oper_node, node);

    KTL_AstNode *node_r = ktl_parse_add_step(cont);
    if (node_r == NULL) {
        ktl_destroy_node(node);
        free(oper_node);

        return NULL;
    }

    add_node_r(oper_node, node_r);
    return oper_node;
}

static KTL_AstNode *ktl_parse_cmp_step(KTL_ParseContext *cont) {
    assert(cont);

    KTL_AstNode *node = ktl_parse_add_step(cont);
    if (node == NULL)   return NULL;

    KTL_Oper oper = KTL_OPER_SUB;
    if      (equal(cont, KTL_PARSE_OP_EQ))  oper = KTL_OPER_COMP_E;
    else if (equal(cont, KTL_PARSE_OP_NEQ)) oper = KTL_OPER_COMP_NE;
    else if (equal(cont, KTL_PARSE_OP_LT))  oper = KTL_OPER_COMP_L;
    else if (equal(cont, KTL_PARSE_OP_GT))  oper = KTL_OPER_COMP_B;
    else if (equal(cont, KTL_PARSE_OP_GE))  oper = KTL_OPER_COMP_BE;
    else if (equal(cont, KTL_PARSE_OP_LE))  oper = KTL_OPER_COMP_LE;
    else    return node;

    advance(cont);

    KTL_AstNode *oper_node = ktl_alloc_node();
    if (oper_node == NULL)  {
        ktl_destroy_node(node);
        return NULL;
    }

    oper_node->kind         = KTL_AST_BINARY_OPER;
    oper_node->data.oper.op = oper;

    add_node_l(oper_node, node);

    KTL_AstNode *node_r = ktl_parse_cmp_step(cont);
    if (node_r == NULL) {
        ktl_destroy_node(node);
        free(oper_node);

        return NULL;
    }

    add_node_r(oper_node, node_r);
    return oper_node;
}

static KTL_AstNode *ktl_parse_expr(KTL_ParseContext *cont) {
    assert(cont);

    KTL_AstNode *node = ktl_parse_add_step(cont);
    if (node == NULL)   return NULL;

    KTL_Oper oper = KTL_OPER_SUB;
    if      (equal(cont, KTL_PARSE_OP_AND))  oper = KTL_OPER_ADD;
    else if (equal(cont, KTL_PARSE_OP_OR))   oper = KTL_OPER_OR;
    else    return node;

    advance(cont);

    KTL_AstNode *oper_node = ktl_alloc_node();
    if (oper_node == NULL)  {
        ktl_destroy_node(node);
        return NULL;
    }

    oper_node->kind         = KTL_AST_BINARY_OPER;
    oper_node->data.oper.op = oper;

    add_node_l(oper_node, node);

    KTL_AstNode *node_r = ktl_parse_expr(cont);
    if (node_r == NULL) {
        ktl_destroy_node(node);
        free(oper_node);

        return NULL;
    }

    add_node_r(oper_node, node_r);
    return oper_node;
}





// =======================================================================
// TINY HELPER FUNCTIONS
// =======================================================================


static inline KTL_Token * get_t(KTL_ParseContext *cont) {
    return cont->tokens + cont->cur_token;
}

static inline KTL_Token * get_nt(KTL_ParseContext *cont) {
    return cont->tokens + cont->cur_token + (cont->cap_token < cont->cap_token);
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

    switch (node->kind) {
        /* Kinds with n children */
        case KTL_AST_FILE:
        case KTL_AST_MAIN:

        case KTL_AST_FUNCTION_DECL:
        case KTL_AST_FUNCTION_CALL:

        case KTL_AST_BLOCK:
        case KTL_AST_COND_BLOCK:
        case KTL_AST_FOR_BLOCK:
            for (int i = 0; i < node->move.n.amount; i++) {
                ktl_destroy_node(node->move.n.children[i]);
            }
            free(node->move.n.children);
            break;

        /* Kinds with 2 children */
        case KTL_AST_IF_BRANCH:
        case KTL_AST_WHILE_BLOCK:

        case KTL_AST_BINARY_OPER:
        case KTL_AST_ASSIGN:
        case KTL_AST_INDEX_ACCESS:
            ktl_destroy_node(node->move.binary.left);
            ktl_destroy_node(node->move.binary.right);
            break;

        /* One child */
        case KTL_AST_ELSE_BRANCH:
        case KTL_AST_UNARY_OPER:
        case KTL_AST_RETURN:
        case KTL_AST_VARIABLE_DECL:
        case KTL_AST_FIELD_ACCESS:
            ktl_destroy_node(node->move.unary.next);
            break;

        case KTL_AST_VARIABLE:
        case KTL_AST_VALUE_INT:
        case KTL_AST_VALUE_STR:
        case KTL_AST_TYPEDEF:
        case KTL_AST_STRUCT_DECL:
        case KTL_AST_BREAK:
        case KTL_AST_CONTINUE:
        case KTL_AST_EXIT:
        default:
            break;
    }

    if (node->kind == KTL_AST_BLOCK) {
        KTL_SymbolMapUninit(node->data.block.map);
    }
    else if (node->kind == KTL_AST_FUNCTION_DECL) {
        KTL_SymbolMapUninit(node->data.func_decl.map);
    }
    free(node);
}

static inline KTL_SourcePos get_t_pos(KTL_ParseContext *cont) {
    return get_t(cont)->pos;
}


