#include "AnalysisType.h"
#include "Common.h"
#include "ASTCommon.h"
#include "Analysis.h"

static inline KTL_AstNodeKind get_k              (KTL_AstNode *node);
static inline KTL_StrID       get_raw_var_name   (KTL_AstNode *node);
static inline KTL_StrID       get_raw_func_name  (KTL_AstNode *node);
static inline KTL_SourcePos   get_pos            (KTL_AstNode *node);
static inline void            set_var_with_entry (KTL_AstNode *node,
                                                  KTL_SymbolEntry *entry);
static inline void            set_func_with_entry(KTL_AstNode *node,
                                                  KTL_SymbolEntry *entry);

static bool resolving_name            (KTL_AnalysisContext *cont,
                                       KTL_AstNode         *node);
static bool process_resolving_children(KTL_AnalysisContext *cont,
                                       KTL_AstNode         *node);


KTL_Error KTL_AnalysisInit(KTL_AnalysisContext *cont,
                           KTL_StrMap          *str_map,
                           KTL_TypeMap         *type_map,
                           KTL_SymbolMap       *global_scope,
                           KTL_Diagnostic      *diag,
                           KTL_AstNode         *root) {

    assert(cont);
    assert(str_map);
    assert(type_map);
    assert(global_scope);
    assert(diag);
    assert(root);

    cont->global_scope  = global_scope;
    cont->str_map       = str_map;
    cont->type_map      = type_map;
    cont->diag          = diag;
    cont->root          = root;
    cont->current_scope = global_scope;

    return KTL_OK;
}

KTL_Error KTL_AnalysisProcess(KTL_AnalysisContext *cont) {
    assert(cont);

    bool is_correct = resolving_name(cont, cont->root);
    if (is_correct == false)    return KTL_LOGICAL_ERR;

    return KTL_OK;
}

static bool resolving_name(KTL_AnalysisContext *cont,
                           KTL_AstNode         *node) {
    assert(cont);
    if (node == NULL)   return true;

    if (get_k(node) == KTL_AST_VARIABLE) {
        KTL_StrID        name  = get_raw_var_name(node);

        KTL_SymbolEntry *entry = KTL_SymbolFind(cont->current_scope, name, KTL_SYMBOL_VAR);
        if (entry == NULL) {
            KTL_DiagEmit(cont->diag, get_pos(node),
                         KTL_DIAG_SEM_UNDECLARED_NAME,
                         KTL_DIAG_SEV_ERROR, name);
            return false;
        }
        set_var_with_entry(node, entry);

        return true;
    } else if (get_k(node) == KTL_AST_FUNCTION_CALL) {
        KTL_StrID        name  = get_raw_func_name(node);
        KTL_SymbolEntry *entry = KTL_SymbolFindLocal(cont->global_scope, name, KTL_SYMBOL_FUNC);
        if (entry == NULL) {
            KTL_DiagEmit(cont->diag, get_pos(node),
                         KTL_DIAG_SEM_UNDECLARED_NAME,
                         KTL_DIAG_SEV_ERROR, name);
            return false;
        }
        set_func_with_entry(node, entry);

        return process_resolving_children(cont, node);
    } else if (get_k(node) == KTL_AST_BLOCK ||
               get_k(node) == KTL_AST_FOR_BLOCK) {
        bool           is_correct = false;
        KTL_SymbolMap *prev_scope = cont->current_scope;
        cont->current_scope       = (get_k(node) == KTL_AST_FOR_BLOCK) ?
                                    node->data.for_block.map : node->data.block.map;


        is_correct = process_resolving_children(cont, node);

        cont->current_scope       = prev_scope;
        return is_correct;
    } else {
        return process_resolving_children(cont, node);
    }
    return false;
}


static bool process_resolving_children(KTL_AnalysisContext *cont,
                                       KTL_AstNode         *node) {
    bool is_correct = false;

    switch (KTL_AstGetTypeChildren(node)) {
        case KTL_AST_N_CHILDREN:
            for (int i = 0; i < node->move.n.amount; i++) {
                is_correct = resolving_name(cont, node->move.n.children[i]);
                if (is_correct == false)    return is_correct;
            }
            return is_correct;

        case KTL_AST_UNARY_CHILD:
            return resolving_name(cont, node->move.unary.next);

        case KTL_AST_BINARY_CHILDREN:
            is_correct                 = resolving_name(cont, node->move.binary.left);
            if (is_correct) is_correct = resolving_name(cont, node->move.binary.right);
            return is_correct;

        case KTL_AST_NO_CHILDREN:
            return true;

        default:
            return false;
    }
}



static inline KTL_AstNodeKind get_k(KTL_AstNode *node) {
    return node->kind;
}

static inline KTL_StrID get_raw_var_name(KTL_AstNode *node) {
    return node->data.var.info.raw.name;
}

static inline KTL_StrID get_raw_func_name(KTL_AstNode *node) {
    return node->data.func_call.info.raw.name;
}

static inline KTL_SourcePos get_pos(KTL_AstNode *node) {
    return node->pos;
}

static inline void set_var_with_entry(KTL_AstNode *node,
                                      KTL_SymbolEntry *entry) {
    node->data.var.info.res.entry = entry;
    node->data.var.is_raw         = false;
}

static inline void set_func_with_entry(KTL_AstNode *node,
                                      KTL_SymbolEntry *entry) {
    node->data.func_call.info.res.entry = entry;
    node->data.func_call.is_raw         = false;
}

