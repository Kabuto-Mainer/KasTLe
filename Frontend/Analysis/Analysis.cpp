#include "AnalysisType.h"
#include "Common.h"
#include "ASTCommon.h"
#include "Analysis.h"
#include "StandardType.h"

constexpr int MAX_TYPE_LEN = 256;

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

    switch (get_k(node)) {
        case KTL_AST_VARIABLE: {
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
        }

        case KTL_AST_FUNCTION_CALL: {
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
        }

        case KTL_AST_BLOCK:
        case KTL_AST_FOR_BLOCK: {
            bool           is_correct = false;
            KTL_SymbolMap *prev_scope = cont->current_scope;
            cont->current_scope       = (get_k(node) == KTL_AST_FOR_BLOCK) ?
                                        node->data.for_block.map : node->data.block.map;


            is_correct = process_resolving_children(cont, node);

            cont->current_scope       = prev_scope;
            return is_correct;
        }

        default:
            return process_resolving_children(cont, node);
    }
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




/*
static bool typing_operation(KTL_AnalysisContext *cont,
                             KTL_AstNode         *node) {
    assert(cont);
    assert(node);

    KTL_AstChildren type_mov = KTL_AstGetTypeChildren(node);

    if      (get_k(node) == KTL_AST_ASSIGN) {
        KTL_TypeID type_l = get_type_oper(cont, node->move.binary.left);
        KTL_TypeID type_r = get_type_oper(cont, node->move.binary.right);

        if ()
    }
    else if (get_k(node) == KTL_AST_VARIABLE_DECL) {
        KTL_TypeID type_v = get_type(cont, )
    }
    else if (get_k(node) == KTL_AST_FUNCTION_CALL) {

    }
    else if ()
}

static bool check_function_decl(KTL_AnalysisContext *cont,
                                KTL_AstNode         *node) {
    assert(cont);
    assert(node);

    if (get_k(node) != KTL_AST_FUNCTION_DECL)   return false;
    KTL_TypeID return_type = node->data.func_decl.func->func.ret_type;

    bool is_correct = false;
    bool ret_res    = true;

    for (int i = 0; i < node->move.n.amount; i++) {
        is_correct = check_func_body(cont, node->move.n.children[i], return_type);

        if (is_correct == false) {
            ret_res = false;
        }
    }


}

static bool check_function_body(KTL_AnalysisContext *cont,
                                KTL_AstNode         *node, KTL_TypeID ret_type) {
    assert(cont);
    assert(node);
    assert(TypeIDCheck(ret_type));

    KTL_AstNodeKind kind  = get_k(node);
    bool            ret_v = true;

    if (kind == KTL_AST_RETURN) {
        if (node->move.unary.next == NULL) {
            if (ret_type != KTL_VOID_TYPE_ID) {
                KTL_DiagEmit(cont->diag, get_pos(node),
                             KTL_DIAG_SEM_TYPE_MISMATCH,
                             KTL_DIAG_SEV_ERROR);
                ret_v = false;
            }
        } else {
            KTL_TypeID type =  get_type_expr(cont, node->mode.unary.next);
            if (type == ret_type)
        }

    }
}




static KTL_TypeID get_type(KTL_AnalysisContext *cont,
                           KTL_AstNode         *node) {
    assert(cont);
    assert(node);

    if (KTL_AST_FILE) {
        check child
    }
    if (KTL_AST_MAIN) {
        check child
    }
    if (KTL_AST_FUNCTION_DECL) {
        save return type
        check child
            check return types
    }
    if (KTL_AST_FUNCTION_CALL) {
        get arg types
        check chold
            check arg types
    }
    if (KTL_AST_VARIABLE) {
        return type
    }
    if (KTL_AST_VARIABLE_DECL) {
        check left type
        check right type
        cmp types
    }
    if (KTL_AST_FIELD_ACCESS) {
        check fields
        add entries to node
        return type
    }
    if (KTL_AST_INDEX_ACCESS) {
        check type array
        return type value
    }
    if (KTL_AST_BINARY_OPER) {
        check types
        return type
    }
    if (KTL_AST_UNARY_OPER) {
        check type
        return type
    }
    if (KTL_AST_VALUE_INT) {
        return type
    }
    if (KTL_AST_VALUE_STR) {
        return type
    }
    if (KTL_AST_BLOCK) {
        check children
    }
    if (KTL_AST_COND_BLOCK) {
        check children
    }
    if (KTL_AST_ELSE_BRANCH) {
    if (KTL_AST_IF_BRANCH) {
    if (KTL_AST_WHILE_BLOCK) {
    if (KTL_AST_FOR_BLOCK) {
        check children
    }
    if (KTL_AST_TYPEDEF) {
        ktl_destroy_node
    }
    if (KTL_AST_STRUCT_DECL) {
        ktl_destroy_node
    }
    if (KTL_AST_ASSIGN) {
        check left and right
        cmd
    }
    if (KTL_AST_RETURN) {
        return type
    }
    if (KTL_AST_BREAK) {
        null
    }
    if (KTL_AST_BREAK) {
        null
    }

    switch (get_k(node)) {
        case KTL_AST_VARIABLE:
            return node->data.var.info.res.entry->var.type;

        case KTL_AST_FUNCTION_CALL:

        case KTL_AST_ARRAY_INIT:
        case KTL_AST_FIELD_ACCESS:
        case KTL_AST_INDEX_ACCESS:
        case KTL_AST_BINARY_OPER:
        case KTL_AST_UNARY_OPER:
        case KTL_AST_VALUE_INT:
        case KTL_AST_VALUE_STR:


        case KTL_AST_COND_BLOCK:
        case KTL_AST_IF_BRANCH:
        case KTL_AST_ELSE_BRANCH:
        case KTL_AST_WHILE_BLOCK:
        case KTL_AST_FOR_BLOCK:
        case KTL_AST_TYPEDEF:
        case KTL_AST_STRUCT_DECL:
        case KTL_AST_ASSIGN:
        case KTL_AST_BLOCK:
        case KTL_AST_RETURN:
        case KTL_AST_BREAK:
        case KTL_AST_VARIABLE_DECL:
        case KTL_AST_FUNCTION_DECL:
        case KTL_AST_CONTINUE:
        case KTL_AST_MAIN:
        case KTL_AST_EXIT:
        case KTL_AST_FILE:
        case KTL_AST_CAST:
    }
}


static KTL_TypeID get_type_oper(KTL_AnalysisContext *cont,
                                KTL_AstNode         *node) {
    assert(cont);
    assert(node);

    if (get_k(node) == KTL_AST_UNARY_OPER) {
        return get_type_unary_oper(cont, node);
    }
    return get_type_binary_oper(cont, node);
}

*/


static bool analyze_file(KTL_AnalysisContext *cont,
                         KTL_AstNode         *root) {
    assert(cont);
    assert(root);

    bool is_correct = true;

    for (int i = 0; i < root->move.n.amount; i++) {
        KTL_AstNode *node = root->move.n.children[i];

        switch (get_k(node)) {
            case KTL_AST_MAIN:
            case KTL_AST_FUNCTION_DECL: {
                cont->currect_type_func = node->data.func_decl.func->func.ret_type;
                for (int j = 0; j < node->move.n.amount; j++) {
                    is_correct &= analyze_line(cont, node->move.n.children[j]);
                }
                break;
            }

            case KTL_AST_VARIABLE_DECL:
                is_correct &= analyze_var_decl(cont, node);
                break;

            case KTL_AST_TYPEDEF:
            case KTL_AST_STRUCT_DECL:
                break;

            default:
                is_correct = false;
                KTL_DiagEmit(cont->diag, get_pos(node),
                             KTL_DIAG_SEM_NOT_A_FUNCTION,
                             KTL_DIAG_SEV_ERROR);
        }
    }
}

static bool analyze_line(KTL_AnalysisContext *cont,
                         KTL_AstNode         *node) {
    assert(cont);
    assert(node);

    bool is_correct = true;

    switch (get_k(node)) {
        case KTL_AST_FUNCTION_CALL:
            return analyze_func_call_param(cont, node);

        case KTL_AST_VARIABLE_DECL:
            return analyze_var_decl(cont, node);

        case KTL_AST_ASSIGN:
            return analyze_assign(cont, node);

        case KTL_AST_WHILE_BLOCK:
        case KTL_AST_IF_BRANCH:
        case KTL_AST_ELSE_BRANCH:
            is_correct &= analyze_line(cont, node->move.binary.left);
            is_correct &= analyze_line(cont, node->move.binary.right);
            return is_correct;

        case KTL_AST_COND_BLOCK:
        case KTL_AST_FOR_BLOCK:
        case KTL_AST_BLOCK:
            for (int i = 0; i < node->move.n.amount; i++) {
                is_correct &= analyze_line(cont, node->move.n.children[i]);
            }
            return is_correct;

        case KTL_AST_RETURN:
            if (cont->in_func == false) {
                KTL_DiagEmit(cont->diag, get_pos(node),
                             KTL_DIAG_SEM_RETURN_OUTSIDE_FUNC,
                             KTL_DIAG_SEV_ERROR);
            }
            return analyze_return(cont, node);

        case KTL_AST_BREAK:
        case KTL_AST_CONTINUE:
        case KTL_AST_EXIT:
            return true;

        default:
            return false;
    }
}


static KTL_TypeID get_type_binary_oper(KTL_AnalysisContext *cont, KTL_AstNode *node) {
    assert(cont);
    assert(node);

    KTL_TypeID type_l = analyze_type_expr(cont, node->move.binary.left);
    KTL_TypeID type_r = analyze_type_expr(cont, node->move.binary.right);
    KTL_TypeID result = KTL_BAD_TYPE_ID;

    KTL_AstNode *needed_to_cast = NULL;

    if (is_base_or_ptr(cont, node, type_l) == false)    return KTL_BAD_TYPE_ID;
    if (is_base_or_ptr(cont, node, type_r) == false)    return KTL_BAD_TYPE_ID;

    bool is_l_ptr = cmp_type_kind(cont, type_l, KTL_TYPE_PTR);
    bool is_r_ptr = cmp_type_kind(cont, type_r, KTL_TYPE_PTR);

    switch (node->data.oper.op) {
        case KTL_OPER_ADD:
        case KTL_OPER_SUB:
            if (is_l_ptr && is_r_ptr)       goto fail;
            result = type_res_oper(cont->type_map, type_l, type_r);
            if (result == KTL_BAD_TYPE_ID)  goto fail;

            apply_type_compat(cont, node->move.binary.left,  result, type_l);
            apply_type_compat(cont, node->move.binary.right, result, type_r);
            break;

        case KTL_OPER_COMP_BE:
        case KTL_OPER_COMP_B:
        case KTL_OPER_COMP_LE:
        case KTL_OPER_COMP_L:
        case KTL_OPER_COMP_E:
        case KTL_OPER_COMP_NE:
            if (is_l_ptr || is_r_ptr)   goto fail;
            result = KTL_BOOL_TYPE_ID;
            break;

        case KTL_OPER_MUL:
        case KTL_OPER_DIV:
        case KTL_OPER_MOD:
            if (is_r_ptr || is_l_ptr)   goto fail;
            result = type_res_oper(cont->type_map, type_l, type_r);
            if (result == KTL_BAD_TYPE_ID)  goto fail;

            apply_type_compat(cont, node->move.binary.left,  result, type_l);
            apply_type_compat(cont, node->move.binary.right, result, type_r);

            break;

        case KTL_OPER_AND:
        case KTL_OPER_OR:
            result = KTL_BOOL_TYPE_ID;
            break;

        case KTL_OPER_GET_PTR:
        case KTL_OPER_UNGET_PTR:
        case KTL_OPER_ASSIGN:
        case KTL_OPER_NEG:
        case KTL_OPER_THIS_ERROR:
        default:
            return KTL_BAD_TYPE_ID;
    }
    node->data.oper.type_res = result;
    return result;

fail:
    KTL_DiagEmit(cont->diag, get_pos(node),
                    KTL_DIAG_SEM_UNSUPPORTED_TYPE_OPER,
                    KTL_DIAG_SEV_ERROR);
    return KTL_BAD_TYPE_ID;
}

static KTL_TypeID get_type_unary_oper(KTL_AnalysisContext *cont, KTL_AstNode *node) {
    assert(cont);
    assert(node);

    KTL_Oper   oper = node->data.oper.op;
    KTL_TypeID type = analyze_type_expr(cont, node->move.unary.next);
    if (type == KTL_BAD_TYPE_ID)    return KTL_BAD_TYPE_ID;

    if      (oper == KTL_OPER_GET_PTR) {
        type = KTL_TypeAddPointer(cont->type_map, type);
    }
    else if (oper == KTL_OPER_NEG) {
        type = type;
    }
    else if (oper == KTL_OPER_UNGET_PTR) {
        KTL_TypeEntry *entry = KTL_TypeGetEntry(cont->type_map, type);
        if (entry == NULL)          return KTL_BAD_TYPE_ID;

        if (entry->kind != KTL_TYPE_PTR) {
            KTL_DiagEmit(cont->diag, get_pos(node),
                         KTL_DIAG_SEM_TYPE_MISMATCH,
                         KTL_DIAG_SEV_ERROR);
            return KTL_BAD_TYPE_ID;
        }

        type = entry->dt.ptr.prev_type;
    }

    return type;
}

static KTL_TypeID type_res_oper(KTL_TypeMap *map,
                                KTL_TypeID type_1,
                                KTL_TypeID type_2) {
    assert(map);

    KTL_TypeEntry *e_1 = KTL_TypeGetEntry(map, type_1);
    KTL_TypeEntry *e_2 = KTL_TypeGetEntry(map, type_2);

    if (e_1 == NULL || e_2 == NULL)     return KTL_BAD_TYPE_ID;

    bool is_1_ptr = e_1->kind == KTL_TYPE_PTR;
    bool is_2_ptr = e_2->kind == KTL_TYPE_PTR;

    if (is_1_ptr && is_2_ptr) return KTL_BAD_TYPE_ID;
    if (is_1_ptr || is_2_ptr) return is_1_ptr ? type_1 : type_2;

    return e_1->dt.base.size > e_2->dt.base.size ? type_1 : type_2;
}

static KTL_TypeID analyze_type_expr(KTL_AnalysisContext *cont,
                                    KTL_AstNode         *node) {
    assert(cont);
    if (node == NULL)  return KTL_BAD_TYPE_ID;

    switch (get_k(node)) {

    case KTL_AST_VALUE_INT: {
        node->data.int_val.type_res = KTL_I32_TYPE_ID;
        return KTL_I32_TYPE_ID;
    }

    case KTL_AST_VALUE_STR: {
        KTL_TypeID type = KTL_TypeAddPointer(cont->type_map, KTL_CHAR_TYPE_ID);
        node->data.str_val.type_res = type;
        return type;
    }

    case KTL_AST_VARIABLE: {
        KTL_SymbolEntry *entry = node->data.var.info.res.entry;
        if (entry == NULL)  return KTL_BAD_TYPE_ID;
        return entry->var.type;
    }

    case KTL_AST_FUNCTION_CALL: {
        analyze_func_call_param(cont, node);

        KTL_SymbolEntry *entry = node->data.func_call.info.res.entry;
        if (entry == NULL)  return KTL_BAD_TYPE_ID;
        return entry->func.ret_type;
    }

    case KTL_AST_FIELD_ACCESS: {
        KTL_TypeID base_type = analyze_type_expr(cont, node->move.unary.next);
        if (base_type == KTL_BAD_TYPE_ID)  return KTL_BAD_TYPE_ID;

        KTL_TypeEntry *base_entry = KTL_TypeGetEntry(cont->type_map, base_type);
        if (base_entry == NULL)  return KTL_BAD_TYPE_ID;

        bool is_ptr_access = node->data.field.is_ptr;
        KTL_TypeEntry *block_entry = NULL;

        if (is_ptr_access) {
            if (base_entry->kind != KTL_TYPE_PTR) {
                emit_field_mismatch(cont, node);
                return KTL_BAD_TYPE_ID;
            }
            block_entry = KTL_TypeGetEntry(cont->type_map,
                                           base_entry->dt.ptr.prev_type);
            if (block_entry == NULL || block_entry->kind != KTL_TYPE_BLOCK) {
                emit_field_mismatch(cont, node);
                return KTL_BAD_TYPE_ID;
            }
        } else {
            if (base_entry->kind != KTL_TYPE_BLOCK) {
                emit_field_mismatch(cont, node);
                return KTL_BAD_TYPE_ID;
            }
            block_entry = base_entry;
        }

        KTL_TypeID field_type = find_type_by_name_field(block_entry,
                                                        node->data.field.name);
        if (field_type == KTL_BAD_TYPE_ID) {
            KTL_DiagEmit(cont->diag, get_pos(node),
                         KTL_DIAG_SEM_BAD_FIELD,
                         KTL_DIAG_SEV_ERROR,
                         node->data.field.name);
            return KTL_BAD_TYPE_ID;
        }

        node->data.field.type = field_type;
        return field_type;
    }

    case KTL_AST_INDEX_ACCESS: {
        KTL_TypeID base_type = analyze_type_expr(cont, node->move.binary.left);
        if (base_type == KTL_BAD_TYPE_ID)  return KTL_BAD_TYPE_ID;

        KTL_TypeEntry *base_entry = KTL_TypeGetEntry(cont->type_map, base_type);
        if (base_entry == NULL)  return KTL_BAD_TYPE_ID;

        KTL_TypeID elem_type = KTL_BAD_TYPE_ID;
        if (base_entry->kind == KTL_TYPE_ARRAY) {
            elem_type = base_entry->dt.arr.base_type;
        } else if (base_entry->kind == KTL_TYPE_PTR) {
            elem_type = base_entry->dt.ptr.prev_type;
        } else {
            KTL_DiagEmit(cont->diag, get_pos(node),
                         KTL_DIAG_SEM_NOT_INDEXABLE,
                         KTL_DIAG_SEV_ERROR);
            return KTL_BAD_TYPE_ID;
        }

        KTL_TypeID idx_type = analyze_type_expr(cont, node->move.binary.right);
        if (idx_type == KTL_BAD_TYPE_ID)  return KTL_BAD_TYPE_ID;

        if (cmp_type_kind(cont, idx_type, KTL_TYPE_BASE) == false) {
            KTL_DiagEmit(cont->diag, get_pos(node),
                         KTL_DIAG_SEM_TYPE_MISMATCH,
                         KTL_DIAG_SEV_ERROR);
            return KTL_BAD_TYPE_ID;
        }

        node->data.index.type_value = elem_type;
        return elem_type;
    }

    case KTL_AST_BINARY_OPER:
        return get_type_binary_oper(cont, node);

    case KTL_AST_UNARY_OPER:
        return get_type_unary_oper(cont, node);

    case KTL_AST_CAST:
        assert(TypeIDCheck(node->data.cast.target));
        return node->data.cast.target;

    default:
        assert(false && "analyze_type_expr called on non-expression");
        return KTL_BAD_TYPE_ID;
    }
}

static bool analyze_func_call_param(KTL_AnalysisContext *cont,
                                    KTL_AstNode         *node) {
    assert(cont);
    assert(node);

    if (get_k(node) != KTL_AST_FUNCTION_CALL)   return false;

    bool             is_correct = true;
    KTL_SymbolEntry *func       = node->data.func_call.info.res.entry;

    if (node->move.n.amount != func->func.amount) {
        KTL_DiagEmit(cont->diag, get_pos(node),
                     KTL_DIAG_SEM_BAD_ARG_COUNT,
                     KTL_DIAG_SEV_ERROR);
        return false;
    }

    for (int i = 0; i < node->move.n.amount; i++) {
        KTL_TypeID type_arg = analyze_type_expr(cont, node->move.n.children[i]);
        if (type_arg == KTL_BAD_TYPE_ID)  {
            is_correct = false;
            continue;
        }

        KTL_TypeID needed_type = func->func.params[i]->var.type;
        if (needed_type == KTL_BAD_TYPE_ID) {
            is_correct = false;
            continue;
        }

        if (apply_type_compat(cont, node->move.n.children[i], needed_type, type_arg) == false) {
            is_correct = false;
        }
    }
    return is_correct;
}

static bool analyze_var_decl(KTL_AnalysisContext *cont,
                             KTL_AstNode         *node) {
    assert(cont);
    assert(node);

    if (get_k(node) != KTL_AST_VARIABLE_DECL)   return false;
    if (node->data.var_decl.is_init == false)   return true;

    KTL_TypeID type_var      = node->data.var_decl.entry->var.type;
    cont->currect_type_array = type_var;
    return analyze_type_var_init(cont, node->move.unary.next);
}

static bool analyze_type_var_init(KTL_AnalysisContext *cont,
                                  KTL_AstNode         *node) {
    assert(cont);
    assert(node);

    KTL_TypeID type_var  = cont->currect_type_array;
    KTL_TypeEntry *entry = KTL_TypeGetEntry(cont->type_map, type_var);
    if (entry == NULL)  return false;

    bool is_correct = true;

    if (node->kind == KTL_AST_ARRAY_INIT) {
        if (entry->kind != KTL_TYPE_ARRAY) {
            KTL_DiagEmit(cont->diag, get_pos(node),
                         KTL_DIAG_SEM_TYPE_MISMATCH,
                         KTL_DIAG_SEV_ERROR);
            return false;
        }
        int len_init = node->move.n.amount;
        int len_var  = entry->dt.arr.elem_count;

        // #TODO: Добавить инициализацию нулями
        if (len_init != len_var) {
            KTL_DiagEmit(cont->diag, get_pos(node),
                         KTL_DIAG_SEM_TYPE_MISMATCH,
                         KTL_DIAG_SEV_ERROR);
            return false;
        }

        KTL_TypeID needed_type = entry->dt.arr.base_type;

        for (int i = 0; i < len_init; i++) {
            KTL_TypeID elem_type = analyze_type_expr(cont, node->move.n.children[i]);
            if (elem_type == KTL_BAD_TYPE_ID)   continue;

            is_correct &= apply_type_compat(cont, node->move.n.children[i], needed_type, elem_type);
        }
        return is_correct;
    } else {
        return analyze_type_expr(cont, node);
    }
}

static bool analyze_return(KTL_AnalysisContext *cont,
                           KTL_AstNode         *node) {
    assert(cont);
    assert(node);

    KTL_AstNodeKind kind  = get_k(node);
    if (kind != KTL_AST_RETURN)     return false;
    bool            ret_v = true;

    if (node->move.unary.next == NULL) {
        if (cont->currect_type_func != KTL_VOID_TYPE_ID) {
            KTL_DiagEmit(cont->diag, get_pos(node),
                            KTL_DIAG_SEM_TYPE_MISMATCH,
                            KTL_DIAG_SEV_ERROR);
            ret_v = false;
        }
    } else {
        KTL_TypeID type =  analyze_type_expr(cont, node->move.unary.next);
        ret_v = apply_type_compat(cont, node->move.unary.next, cont->currect_type_func, type);
    }
    return ret_v;
}

static bool analyze_assign(KTL_AnalysisContext *cont,
                           KTL_AstNode         *node) {
    assert(cont);
    assert(node);

    if (get_k(node) != KTL_AST_ASSIGN)  return false;
    KTL_TypeID type_l = analyze_type_expr(cont, node->move.binary.left);
    KTL_TypeID type_r = analyze_type_expr(cont, node->move.binary.right);

    return apply_type_compat(cont, node->move.binary.right, type_l, type_r);
}

static void emit_field_mismatch(KTL_AnalysisContext *cont,
                                KTL_AstNode *node) {
    KTL_DiagEmit(cont->diag, get_pos(node),
                 KTL_DIAG_SEM_TYPE_MISMATCH,
                 KTL_DIAG_SEV_ERROR);
}

static KTL_TypeID find_type_by_name_field(KTL_TypeEntry *entry, KTL_StrID name) {
    assert(entry);
    assert(StrIDCheck(name));

    if (entry->kind != KTL_TYPE_BLOCK)  return KTL_BAD_TYPE_ID;
    for (int i = 0; i < entry->dt.block.field_count; i++) {
        if (entry->dt.block.fields[i].name == name)     return entry->dt.block.fields[i].base_type;
    }
    return KTL_BAD_TYPE_ID;
}

static KTL_TypeCastKind check_type_compat_cast(KTL_TypeMap *map,
                                               KTL_TypeID   target,
                                               KTL_TypeID   source) {
    assert(map);

    if (TypeIDCheck(target) == false ||
        TypeIDCheck(source) == false)  return KTL_TYPE_CAST_INCOMPATIBLE;

    if (target == source)              return KTL_TYPE_CAST_SAME;

    KTL_TypeEntry *t_entry = KTL_TypeGetEntry(map, target);
    KTL_TypeEntry *s_entry = KTL_TypeGetEntry(map, source);
    if (t_entry == NULL || s_entry == NULL)  return KTL_TYPE_CAST_INCOMPATIBLE;

    if (t_entry->kind == KTL_TYPE_BASE && s_entry->kind == KTL_TYPE_BASE) {
        if (t_entry->dt.base.size >= s_entry->dt.base.size) {
            return KTL_TYPE_CAST_IMPLICIT;
        }
        return KTL_TYPE_CAST_INCOMPATIBLE;
    }

    if (t_entry->kind == KTL_TYPE_PTR && s_entry->kind == KTL_TYPE_PTR) {
        if (t_entry->dt.ptr.prev_type == s_entry->dt.ptr.prev_type) {
            return KTL_TYPE_CAST_SAME;
        }
        return KTL_TYPE_CAST_INCOMPATIBLE;
    }

    if (t_entry->kind == KTL_TYPE_PTR && s_entry->kind == KTL_TYPE_ARRAY) {
        if (t_entry->dt.ptr.prev_type == s_entry->dt.arr.base_type) {
            return KTL_TYPE_CAST_IMPLICIT;
        }
        return KTL_TYPE_CAST_INCOMPATIBLE;
    }

    return KTL_TYPE_CAST_INCOMPATIBLE;
}

static bool apply_type_compat(KTL_AnalysisContext *cont,
                              KTL_AstNode         *node,
                              KTL_TypeID           target,
                              KTL_TypeID           source) {

    KTL_TypeCastKind kind = check_type_compat_cast(cont->type_map,
                                                   target, source);

    switch (kind) {
        case KTL_TYPE_CAST_IMPLICIT:
            return cast_to_type(node, target);

        case KTL_TYPE_CAST_INCOMPATIBLE:
            KTL_DiagEmit(cont->diag, get_pos(node),
                         KTL_DIAG_SEM_TYPE_MISMATCH,
                         KTL_DIAG_SEV_ERROR);
            return false;

        case KTL_TYPE_CAST_SAME:
            return true;

        default:
            return false;
    }
}

static bool cast_to_type(KTL_AstNode *node, KTL_TypeID target) {
    assert(node);
    assert(TypeIDCheck(target));

    KTL_AstNode *new_node = ktl_alloc_node();
    if (new_node == NULL)  return false;

   *new_node               = *node;
    node->kind             =  KTL_AST_CAST;
    node->data.cast.target =  target;

    node->move.unary.next  = new_node;

    return true;
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

static inline bool cmp_type_kind(KTL_AnalysisContext *cont,
                                 KTL_TypeID type,    KTL_TypeEntryKind kind) {
    KTL_TypeEntry *entry = KTL_TypeGetEntry(cont->type_map, type);
    return entry->kind == kind;
}

static inline bool is_base_or_ptr(KTL_AnalysisContext *cont,
                                  KTL_AstNode *node,  KTL_TypeID type) {
    if (cmp_type_kind(cont, type, KTL_TYPE_BASE) == false &&
        cmp_type_kind(cont, type, KTL_TYPE_PTR)  == false) {
        KTL_DiagEmit(cont->diag, get_pos(node),
                     KTL_DIAG_SEM_UNSUPPORTED_TYPE_OPER,
                     KTL_DIAG_SEV_ERROR);
        return false;
    }
    return true;
}

// static KTL_StrID get_name_type(KTL_AnalysisContext *cont, KTL_TypeID type) {
//     char buffer[MAX_TYPE_LEN] = "";
//
// }
//
// static void print_name_type_to_buffer(char *buffer, int pos, KTL_TypeID type) {
//     switch ()
// }
