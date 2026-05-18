#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "Backend.h"
#include "StrMap.h"
#include "TypeMap.h"
#include "ASTCommon.h"
#include "BackMap.h"
#include "BackIR.h"
#include "BackIR_DSL.h"

// =======================================================================
// CONSTANTS
// =======================================================================

constexpr const char *KTL_GLOBAL_PREFIX = "__global__";
constexpr const char *KTL_FUNC_PREFIX   = "__func__";
constexpr const char *KTL_STRING_PREFIX = "__string__";

const KTL_RegID KTL_PARAM_REGS[6] = {
    KTL_REG_RDI, KTL_REG_RSI, KTL_REG_RDX,
    KTL_REG_RCX, KTL_REG_R8,  KTL_REG_R9,
};


// =======================================================================
// HELPER FUNCTION'S DECLARATION
// =======================================================================

static void layout_global       (KTL_BackendContext *cont, KTL_AstNode *root);
static void layout_all_functions(KTL_BackendContext *cont, KTL_AstNode *root);
static void layout_func         (KTL_BackendContext *cont, KTL_AstNode *node);
static void layout_body         (KTL_BackendContext *cont, KTL_AstNode *node);

static int get_size            (KTL_TypeMap        *map,      KTL_TypeID  type);
static int get_align           (KTL_TypeMap        *map,      KTL_TypeID  type);
static int align_up            (int                 offset,   int         align);
static int get_offset_field    (KTL_TypeEntry      *block,    KTL_StrID   name);
KTL_TypeEntryKind get_type_kind(KTL_BackendContext *cont,     KTL_AstNode *node);
KTL_TypeEntry *   get_type     (KTL_BackendContext *cont,     KTL_AstNode *node);
KTL_StrID get_global_name      (KTL_StrMap         *str_map,  const char  *prefix, KTL_StrID name);
KTL_StrID get_func_name        (KTL_StrMap         *str_map,  const char  *prefix, KTL_StrID name);
KTL_StrID get_string_name      (KTL_StrMap         *str_map,  const char  *prefix, KTL_StrID name);
KTL_StrID get_name             (KTL_StrMap         *str_map,  const char  *prefix_1,
                                const char         *prefix_2, KTL_StrID    name);

static void emit_header    (KTL_BackendContext *cont, KTL_AstNode *root);
/*
static void emit_globals   (KTL_BackendContext *cont, KTL_AstNode *root);*/
static void emit_text      (KTL_BackendContext *cont, KTL_AstNode *root);
static void emit_function  (KTL_BackendContext *cont, KTL_AstNode *node);
static void emit_global_var(KTL_BackendContext *cont, KTL_AstNode *node);

static void emit_global_value   (KTL_BackendContext *cont,
                                 KTL_AstNode       *init,
                                 KTL_TypeEntry     *type);
static void emit_strings        (KTL_BackendContext *cont, KTL_AstNode *root);
static void emit_string_literal (KTL_BackendContext *cont, KTL_AstNode *node);
static void fprintf_string_value(KTL_BackendContext *cont, KTL_StrID value);
static void emit_function_header(KTL_BackendContext *cont, KTL_AstNode *node);
static void emit_var_decl       (KTL_BackendContext *cont, KTL_AstNode *node);
static void emit_var_decl_array (KTL_BackendContext *cont, KTL_AstNode *node);
static void emit_var_decl_zero  (KTL_BackendContext *cont, KTL_AstNode *node);
static void emit_load_address   (KTL_BackendContext *cont, KTL_AstNode *node);
static void emit_cond_block     (KTL_BackendContext *cont, KTL_AstNode *node);
static void emit_for_block      (KTL_BackendContext *cont, KTL_AstNode *node);
static void emit_while_block    (KTL_BackendContext *cont, KTL_AstNode *node);
static void emit_continue       (KTL_BackendContext *cont, KTL_AstNode *node);
static void emit_break          (KTL_BackendContext *cont, KTL_AstNode *node);
static void emit_exit           (KTL_BackendContext *cont);
static void emit_return         (KTL_BackendContext *cont, KTL_AstNode *node);
static void emit_assign         (KTL_BackendContext *cont, KTL_AstNode *node);
static int  get_assign_size     (KTL_BackendContext *cont, KTL_TypeEntry *type);
static void emit_assign_struct  (KTL_BackendContext *cont,
                                 KTL_AstNode       *l_val,
                                 KTL_AstNode       *r_val,
                                 KTL_TypeEntry     *type);
static void emit_func_call      (KTL_BackendContext *cont, KTL_AstNode *node);
static void emit_expr           (KTL_BackendContext *cont, KTL_AstNode *node);
static void emit_cast           (KTL_BackendContext *cont,
                                 KTL_TypeID          src_id,
                                 KTL_TypeID          dst_id);
static void emit_oper           (KTL_BackendContext *cont, KTL_AstNode *node);
static void emit_block          (KTL_BackendContext *cont, KTL_AstNode *node);
static void emit_body           (KTL_BackendContext *cont, KTL_AstNode *node);
static void emit_push           (KTL_BackendContext *cont, KTL_RegID    reg);
static void emit_pop            (KTL_BackendContext *cont, KTL_RegID    reg);

KTL_StrID get_global_name(KTL_StrMap *str_map, const char *prefix, KTL_StrID name);
KTL_StrID get_func_name  (KTL_StrMap *str_map, const char *prefix, KTL_StrID name);
KTL_StrID get_string_name(KTL_StrMap *str_map, const char *prefix, KTL_StrID name);
KTL_StrID get_func_plt   (KTL_StrMap *str_map, const char *prefix, KTL_StrID name);
KTL_StrID get_name       (KTL_StrMap *str_map, const char *prefix_1,
                          const char *prefix_2, const KTL_StrID name);


// =======================================================================
// API
// =======================================================================

#ifdef EMIT_DEBUG
KTL_Error KTL_BackendInit(KTL_BackendContext *cont,
                          KTL_TypeMap        *type_map,
                          KTL_StrMap         *str_map,
                          KTL_SymbolMap      *global_scope,
                          KTL_BackIR_Buffer  *text,
                          KTL_BackIR_Buffer  *data,
                          KTL_BackIR_Buffer  *rodata,
                          FILE               *debug_out) {
    assert(cont);
    assert(type_map);
    assert(str_map);
    assert(global_scope);
    assert(text);
    assert(data);
    assert(rodata);
    assert(debug_out);

    cont->type_map     = type_map;
    cont->str_map      = str_map;
    cont->global_scope = global_scope;

    cont->output.text   = text;
    cont->output.data   = data;
    cont->output.rodata = rodata;

    cont->current_func        = NULL;
    cont->loop_label_break    = -1;
    cont->loop_label_continue = -1;
    cont->label_counter       = 0;

    cont->symbol_prefix = "";
    cont->debug_emit    = debug_out;

    return KTL_BackendTableInit(&cont->table);
}
#else
KTL_Error KTL_BackendInit(KTL_BackendContext *cont,
                          KTL_TypeMap        *type_map,
                          KTL_StrMap         *str_map,
                          KTL_SymbolMap      *global_scope,
                          KTL_BackIR_Buffer  *text,
                          KTL_BackIR_Buffer  *data,
                          KTL_BackIR_Buffer  *rodata) {
    assert(cont);
    assert(type_map);
    assert(str_map);
    assert(global_scope);
    assert(text);
    assert(data);
    assert(rodata);

    cont->type_map     = type_map;
    cont->str_map      = str_map;
    cont->global_scope = global_scope;

    cont->output.text   = text;
    cont->output.data   = data;
    cont->output.rodata = rodata;

    cont->current_func        = NULL;
    cont->loop_label_break    = -1;
    cont->loop_label_continue = -1;
    cont->label_counter       = 0;

    cont->symbol_prefix = "";

    return KTL_BackendTableInit(&cont->table);
}
#endif


KTL_Error KTL_BackendUninit(KTL_BackendContext *cont) {
    assert(cont);

    return KTL_BackendTableUninit(&cont->table);
}

KTL_Error KTL_BackendRun(KTL_BackendContext *cont, KTL_AstNode *root) {
    assert(cont);
    assert(root);

    layout_global(cont, root);
    layout_all_functions(cont, root);

    emit_header(cont, root);
    // emit_globals(cont, root);
    emit_strings(cont, root);
    emit_text(cont, root);

    return KTL_OK;
}


// =======================================================================
// LAYOUT
// =======================================================================

static void layout_global(KTL_BackendContext *cont, KTL_AstNode *root) {
    assert(cont);
    assert(root);

    for (int i = 0; i < root->move.n.amount; i++) {
        KTL_AstNode *node = root->move.n.children[i];
        if (node->kind == KTL_AST_VARIABLE_DECL) {
            KTL_StrID asm_name = get_global_name(cont->str_map,
                                    cont->symbol_prefix, node->data.var_decl.entry->str_id);

            KTL_BackendAddStaticVar(&cont->table, node->data.var_decl.entry, asm_name);
        }
        else if (node->kind == KTL_AST_FUNCTION_DECL) {
            KTL_StrID asm_name = get_func_name(cont->str_map,
                                    cont->symbol_prefix, node->data.func_decl.func->str_id);
            KTL_BackendAddFunc(&cont->table, node->data.func_decl.func, asm_name);
        }
    }
    return ;
}

static void layout_all_functions(KTL_BackendContext *cont, KTL_AstNode *root) {
    assert(cont);
    assert(root);

    for (int i = 0; i < root->move.n.amount; i++) {
        KTL_AstNode *node = root->move.n.children[i];
            if (node->kind == KTL_AST_FUNCTION_DECL) {
                layout_func(cont, node);
            }
            else if (node->kind == KTL_AST_MAIN) {
                cont->frame_offset = -1 * KTL_SYSTEM_PTR_SIZE;
                layout_body(cont, node->move.unary.next);
                cont->main_frame_size = align_up(-1 * cont->frame_offset, 2 * KTL_SYSTEM_PTR_SIZE);
                cont->main_frame_size += KTL_SYSTEM_PTR_SIZE;
            }
    }
    return ;
}

static void layout_func(KTL_BackendContext *cont, KTL_AstNode *node) {
    assert(cont);
    assert(node);

    int counter_params = 0;
                        /* rbp + _ret_adr_ */
    int offset_param   = 2 * KTL_SYSTEM_PTR_SIZE;   /* for 7+ (PARAM_REGS_COUNT + 1) params */
    int offset_frame   = 0;                         /* for 1-6 (PARAM_REGS_COUNT) params + local vars */

    KTL_SymbolMap *params = node->data.func_decl.map;
    for (int i = 0; i < params->size; i++) {
        KTL_SymbolEntry *param = params->data[i];

        if (counter_params >= KTL_PARAM_REGS_COUNT) {
            KTL_BackendAddStackVar(&cont->table, param, offset_param);
            offset_param += KTL_SYSTEM_PTR_SIZE;
        }
        else {
            offset_frame -= KTL_SYSTEM_PTR_SIZE;
            KTL_BackendAddStackVar(&cont->table, param, offset_frame);
        }
        counter_params++;
    }

    cont->frame_offset = offset_frame;
    layout_body(cont, node);

    KTL_BackendFuncInfo *func = KTL_BackendFindFunc(&cont->table, node->data.func_decl.func);
    func->frame_size = align_up(-1 * cont->frame_offset, 2 * KTL_SYSTEM_PTR_SIZE);

    return ;
}

static void layout_body(KTL_BackendContext *cont, KTL_AstNode *node) {
    assert(cont);
    if (node == NULL)   return ;

    if (node->kind == KTL_AST_BLOCK ||
        node->kind == KTL_AST_FOR_BLOCK) {
        KTL_SymbolMap *vars = node->data.for_block.map;
        for (int i = 0; i < vars->size; i++) {

            debug_out("VARIABLE: [%s]\n", vars->data[i]->str_id);
            KTL_SymbolEntry *var  = vars->data[i];
            KTL_TypeEntry   *type = KTL_TypeGetEntry(cont->type_map, var->var.type);
            if (type->kind == KTL_TYPE_BASE ||
                type->kind == KTL_TYPE_PTR) {
                cont->frame_offset -= KTL_SYSTEM_PTR_SIZE;
                KTL_BackendAddStackVar(&cont->table, var, cont->frame_offset);
            }
            else {
                int size = -1;
                if (type->kind == KTL_TYPE_BLOCK) {
                    size = align_up(type->dt.block.size, KTL_SYSTEM_PTR_SIZE);
                }
                else {
                    size = align_up(get_size(cont->type_map, var->var.type), KTL_SYSTEM_PTR_SIZE);
                }
                cont->frame_offset -= size;
                KTL_BackendAddStackVar(&cont->table, var, cont->frame_offset);
            }
        }
    }
    KTL_AstChildren mov_type = KTL_AstGetTypeChildren(node);
    switch (mov_type) {

    case KTL_AST_N_CHILDREN:
        for (int i = 0; i < node->move.n.amount; i++) {
            layout_body(cont, node->move.n.children[i]);
        }
        break;

    case KTL_AST_BINARY_CHILDREN:
        layout_body(cont, node->move.binary.left);
        layout_body(cont, node->move.binary.right);
        break;

    case KTL_AST_UNARY_CHILD:
        layout_body(cont, node->move.unary.next);
        break;

    case KTL_AST_NO_CHILDREN:
    default:
        break;
    }
    return ;
}





// =======================================================================
// HELPERS
// =======================================================================

static int get_size(KTL_TypeMap *map, KTL_TypeID type) {
    assert(map);
    assert(TypeIDCheck(type));

    KTL_TypeEntry *entry = KTL_TypeGetEntry(map, type);
    if (entry->kind == KTL_TYPE_PTR)    return KTL_SYSTEM_PTR_SIZE;
    if (entry->kind == KTL_TYPE_BASE)   return entry->dt.base.size;
    if (entry->kind == KTL_TYPE_BLOCK)  return entry->dt.block.size;
    if (entry->kind == KTL_TYPE_ARRAY)  return entry->dt.arr.elem_count *
                                               get_size(map, entry->dt.arr.base_type);
    return KTL_BAD_TYPE_ID;
}

static int get_align(KTL_TypeMap *map, KTL_TypeID type) {
    assert(map);
    assert(TypeIDCheck(type));

    KTL_TypeEntry *entry = KTL_TypeGetEntry(map, type);
    if (entry->kind == KTL_TYPE_BASE)   return entry->dt.base.align;
    if (entry->kind == KTL_TYPE_PTR)    return KTL_SYSTEM_PTR_SIZE;
    if (entry->kind == KTL_TYPE_ARRAY)  return get_align(map, entry->dt.arr.base_type);
    if (entry->kind == KTL_TYPE_BLOCK)  return entry->dt.block.align;
    return 1;
}

static int align_up(int offset, int align) {
    assert(align > 0);

    return ((offset + align - 1) / align) * align;
}

static int get_offset_field(KTL_TypeEntry *block, KTL_StrID name) {
    assert(block);
    assert(StrIDCheck(name));

    for (int i = 0; i < block->dt.block.field_count; i++) {
        if (block->dt.block.fields[i].name == name) {
            return block->dt.block.fields[i].offset;
        }
    }
    return 0;
}

KTL_TypeEntryKind get_type_kind(KTL_BackendContext *cont, KTL_AstNode *node) {
    assert(cont);
    assert(node);

    return get_type(cont, node)->kind;
}

KTL_TypeEntry * get_type(KTL_BackendContext *cont, KTL_AstNode *node) {
    assert(cont);
    assert(node);

    KTL_TypeEntry *type = NULL;

    switch (node->kind) {

    case KTL_AST_FIELD_ACCESS:
        type = KTL_TypeGetEntry(cont->type_map, node->data.field.type);
        return type;

    case KTL_AST_INDEX_ACCESS:
        type = KTL_TypeGetEntry(cont->type_map, node->data.index.type_value);
        return type;

    case KTL_AST_VARIABLE:
        type = KTL_TypeGetEntry(cont->type_map, node->data.var.info.res.entry->var.type);
        return type;

    default:
        assert(0 && "UNKNOWN NODE IN VARIABLE");
        return NULL;
    }
}

KTL_StrID get_global_name(KTL_StrMap *str_map, const char *prefix, KTL_StrID name) {
    return get_name(str_map, prefix, KTL_GLOBAL_PREFIX, name);
}

KTL_StrID get_func_name(KTL_StrMap *str_map, const char *prefix, KTL_StrID name) {
    return get_name(str_map, prefix, KTL_FUNC_PREFIX, name);
}

KTL_StrID get_func_plt(KTL_StrMap *str_map, const char *prefix, KTL_StrID name) {
    (void) prefix;
    return get_name(str_map, "", "", name);
}

KTL_StrID get_string_name(KTL_StrMap *str_map, const char *prefix, KTL_StrID name) {
    assert(str_map);
    assert(name);
    if (prefix == NULL) prefix = "";

    int len_pref_1 = (int) strlen(prefix);
    int len_pref_2 = (int) strlen(KTL_STRING_PREFIX);
    int len_ended  = len_pref_1 + len_pref_2 + 19 + 1;

    char buffer[len_ended];  /* can't be initted and emit warning, but it is too userful in my situation */
    strcpy(buffer, prefix);
    strcpy(buffer + len_pref_1, KTL_STRING_PREFIX);
    sprintf(buffer + len_pref_1 + len_pref_2, "%p", name);
    buffer[len_ended - 1] = '\0';

    return KTL_StrMapFind(str_map, buffer);
}

KTL_StrID get_name(KTL_StrMap *str_map, const char *prefix_1,
                   const char *prefix_2,
                   const KTL_StrID name) {
    assert(str_map);
    assert(name);
    if (prefix_1 == NULL) prefix_1 = "";
    if (prefix_2 == NULL) prefix_2 = "";

    int len_name   = (int) strlen(name);
    int len_pref_1 = (int) strlen(prefix_1);
    int len_pref_2 = (int) strlen(prefix_2);
    int len_ended  = len_pref_1 + len_pref_2 + len_name + 1;

    char buffer[len_ended];  /* can't be initted and emit warning, but it is too userful in my situation */
    strcpy(buffer, prefix_1);
    strcpy(buffer + len_pref_1, prefix_2);
    strcpy(buffer + len_pref_1 + len_pref_2, name);
    buffer[len_ended - 1] = '\0';

    return KTL_StrMapFind(str_map, buffer);
}



// =======================================================================
// EMIT CONVERT HELPERS
// =======================================================================


// =======================================================================
// EMIT
// =======================================================================

static void emit_header(KTL_BackendContext *cont, KTL_AstNode *root) {
    assert(cont);
    assert(root);

    _SWITCH_DATA;

    for (int i = 0; i < root->move.n.amount; i++) {
        KTL_AstNode *node = root->move.n.children[i];
        if (node->kind == KTL_AST_VARIABLE_DECL) {
            emit_global_var(cont, node);
        }
    }
    return ;
}
/*
static void emit_globals(KTL_BackendContext *cont, KTL_AstNode *root) {
    assert(cont);
    assert(root);

    for (int i = 0; i < root->move.n.amount; i++) {
        emit_global_var(cont, root->move.n.children[i]);
    }

    return ;
}
*/

static void emit_text(KTL_BackendContext *cont, KTL_AstNode *root) {
    assert(cont);
    assert(root);

    _SWITCH_TEXT;
    cont->stack_depth = 0;

    for (int i = 0; i < root->move.n.amount; i++) {
        KTL_AstNode *node = root->move.n.children[i];
        if (node->kind != KTL_AST_MAIN)  continue;

        _GLABEL(_TEXT("_start"));
        _PUSH(_REG_64(_NR(RBP)));
        _MOV(_REG_64(_NR(RBP)), _REG_64(_NR(RSP)));
        if (cont->main_frame_size > 0) {
            _SUB(_REG_64(_NR(RSP)), _IMM_64(cont->main_frame_size));
        }
        emit_function(cont, node);
        emit_exit(cont);
        break;
    }

    for (int i = 0; i < root->move.n.amount; i++) {
        KTL_AstNode *node = root->move.n.children[i];
        if (node->kind == KTL_AST_FUNCTION_DECL) {
            emit_function(cont, node);
        }
    }
    return ;
}

static void emit_function(KTL_BackendContext *cont, KTL_AstNode *node) {
    if (node->kind != KTL_AST_FUNCTION_DECL && node->kind != KTL_AST_MAIN) return ;

    if (node->kind == KTL_AST_FUNCTION_DECL) {
        emit_function_header(cont, node);
        emit_block(cont, node->move.n.children[0]);
    }
    else {
        emit_block(cont, node->move.unary.next);
    }

    if (node->kind == KTL_AST_FUNCTION_DECL) {
        _MOV(_REG_64(_NR(RSP)), _REG_64(_NR(RBP)));
        _POP(_REG_64(_NR(RBP)));
        _RET;
    }
    return ;
}

static void emit_global_var(KTL_BackendContext *cont, KTL_AstNode *node) {
    assert(cont);
    assert(node);
    if (node->kind != KTL_AST_VARIABLE_DECL)  return ;

    KTL_SymbolEntry    *sym  = node->data.var_decl.entry;
    KTL_BackendVarInfo *info = KTL_BackendFindVar(&cont->table, sym);
    KTL_TypeEntry      *type = KTL_TypeGetEntry(cont->type_map, sym->var.type);

    int size  = get_size(cont->type_map, sym->var.type);
    int align = get_align(cont->type_map, sym->var.type);

    _ALIGN(align);
    _LABEL(info->loc.stat.label);

    if (!node->data.var_decl.is_init) {
        _ZERO(size);
        return ;
    }

    KTL_AstNode *init = node->move.unary.next;
    emit_global_value(cont, init, type);
}

static void emit_global_value(KTL_BackendContext *cont,
                               KTL_AstNode       *init,
                               KTL_TypeEntry     *type) {
    assert(cont);
    assert(init);
    assert(type);

    if (type->kind == KTL_TYPE_BASE || type->kind == KTL_TYPE_PTR) {
        int size = (type->kind == KTL_TYPE_PTR)
                 ? KTL_SYSTEM_PTR_SIZE
                 : type->dt.base.size;

        int64_t value = 0;
        if (init->kind == KTL_AST_VALUE_INT) {
            value = init->data.int_val.value;
        }

        _INT(value, size); /* size value */
        return ;
    }
    if (type->kind == KTL_TYPE_ARRAY) {
        KTL_TypeEntry *elem_type = KTL_TypeGetEntry(cont->type_map,
                                                    type->dt.arr.base_type);
        int total     = type->dt.arr.elem_count;
        int elem_size = get_size(cont->type_map, type->dt.arr.base_type);

        int n_init = 0;
        if (init->kind == KTL_AST_ARRAY_INIT) {
            n_init = init->move.n.amount;
            if (n_init > total)  n_init = total;

            for (int i = 0; i < n_init; i++) {
                emit_global_value(cont, init->move.n.children[i], elem_type);
            }
        }

        if (n_init < total) {
            _ZERO((total - n_init) * elem_size);
        }
        return ;
    }
    if (type->kind == KTL_TYPE_BLOCK) {
        _ZERO(type->dt.block.size);
        return ;
    }
}

static void emit_strings(KTL_BackendContext *cont, KTL_AstNode *root) {
    assert(cont);
    assert(root);

    _SWITCH_RODATA;
    _COMMENT(_TEXT("; Constant Strings\n"));

    emit_string_literal(cont, root);

    return ;
}

static void emit_string_literal(KTL_BackendContext *cont, KTL_AstNode *node) {
    assert(cont);
    if (node == NULL)   return ;

    if (node->kind == KTL_AST_VALUE_STR) {
        fprintf_string_value(cont, node->data.str_val.value);
        return ;
    }

    KTL_AstChildren mov_type = KTL_AstGetTypeChildren(node);

    switch (mov_type) {

    case KTL_AST_N_CHILDREN:
        for (int i = 0; i < node->move.n.amount; i++) {
            emit_string_literal(cont, node->move.n.children[i]);
        }
        break;

    case KTL_AST_BINARY_CHILDREN:
        emit_string_literal(cont, node->move.binary.left);
        emit_string_literal(cont, node->move.binary.right);
        break;

    case KTL_AST_UNARY_CHILD:
        emit_string_literal(cont, node->move.unary.next);
        break;

    case KTL_AST_NO_CHILDREN:
    default:
        break;
    }
    return ;
}

static void fprintf_string_value(KTL_BackendContext *cont, KTL_StrID value) {
    assert(cont);
    assert(value);

    KTL_StrID name_label = get_string_name(cont->str_map, cont->symbol_prefix, value);

    _LABEL(name_label);
    _BYTE(value);

    return ;
}

static void emit_function_header(KTL_BackendContext *cont, KTL_AstNode *node) {
    assert(cont);
    assert(node);
    if (node->kind != KTL_AST_FUNCTION_DECL)    return ;

    KTL_BackendFuncInfo *func = KTL_BackendFindFunc(&cont->table,
                                        node->data.func_decl.func);
    if (func == NULL)   return ;

    _GLABEL(func->label);

    _PUSH(_REG_64(_NR(RBP)));
    _MOV(_REG_64(_NR(RBP)), _REG_64(_NR(RSP)));

    if (func->frame_size > 0) {
        _SUB(_REG_64(_NR(RSP)), _IMM_64(func->frame_size));
    }

    KTL_SymbolMap *params = node->data.func_decl.map;
    int n_reg = (params->size < KTL_PARAM_REGS_COUNT) ?
                params->size : KTL_PARAM_REGS_COUNT;

    for (int i = 0; i < n_reg; i++) {
        KTL_SymbolEntry    *param = params->data[i];
        KTL_BackendVarInfo *info  = KTL_BackendFindVar(&cont->table, param);

        if (info == NULL || info->storage != KTL_BACKEND_STORAGE_STACK) {
            return ;
        }

        _MOV(_MEM_IDX(_NR(RBP), info->loc.stack.offset, 8), _REG_64(KTL_PARAM_REGS[i]));
    }
    cont->stack_depth = 0;
    return ;
}

static void emit_var_decl(KTL_BackendContext *cont, KTL_AstNode *node) {
    assert(cont);
    assert(node);

    if (node->data.var_decl.is_init && node->move.unary.next->kind == KTL_AST_ARRAY_INIT) {
        emit_var_decl_array(cont, node);
        return ;
    }

    debug_out("VARIABLE: %s\n", node->data.var_decl.entry->str_id);
    /* structs can't be initted */
    KTL_BackendVarInfo *var = KTL_BackendFindVar(&cont->table, node->data.var_decl.entry);
    if (node->data.var_decl.is_init) {
        emit_expr(cont, node->move.unary.next);
        KTL_TypeEntry *type = KTL_TypeGetEntry(cont->type_map,
                            node->data.var_decl.entry->var.type);
        int size = (type->kind == KTL_TYPE_PTR)
                    ? KTL_SYSTEM_PTR_SIZE
                    : type->dt.base.size;

        // printf("%s:%d::%d\n", __FILE__,__LINE__,size);
        _MOV(_MEM_IDX(_NR(RBP), var->loc.stack.offset, size), _REG(_NR(RAX), size));
    }
    else {
        emit_var_decl_zero(cont, node);
    }
}

static void emit_var_decl_array(KTL_BackendContext *cont, KTL_AstNode *node) {
    assert(cont);
    assert(node);

    KTL_AstNode   *array      = node->move.unary.next;
    KTL_TypeEntry *type_array = KTL_TypeGetEntry(cont->type_map, node->data.var_decl.entry->var.type);
    KTL_TypeEntry *type_elem  = KTL_TypeGetEntry(cont->type_map, type_array->dt.arr.base_type);
    int            size_elem  = get_size(cont->type_map, type_array->dt.arr.base_type);
    KTL_BackendVarInfo *var   = KTL_BackendFindVar(&cont->table, node->data.var_decl.entry);

    // printf("SIZE ELEM: %d\n", size_elem);

    // TODO: Check size elem
    if (type_elem->kind == KTL_TYPE_PTR ||
        type_elem->kind == KTL_TYPE_BASE) {
        for (int i = 0; i < array->move.n.amount; i++) {
            emit_expr(cont, array->move.n.children[i]);
            // printf("%s:%d::%d\n", __FILE__,__LINE__,size_elem);
            _MOV(_MEM_IDX(_NR(RBP), var->loc.stack.offset + i*size_elem, size_elem),
                 _REG(_NR(RAX), size_elem));

        }
        return ;
    }
    for (int i = 0; i < array->move.n.amount; i++) {
        emit_load_address(cont, array->move.n.children[i]);

        _MOV(_REG_64(_NR(RSI)), _REG_64(_NR(RAX)));
        _LEA(_REG_64(_NR(RDI)), _MEM_IDX(_NR(RBP), var->loc.stack.offset + i*size_elem, 8));
        _MOV(_REG_64(_NR(RCX)), _IMM_64(size_elem));
        _REP_MOVSB;
    }
    return ;
}

static void emit_var_decl_zero(KTL_BackendContext *cont, KTL_AstNode *node) {
    assert(cont);
    assert(node);

    KTL_TypeEntry *type_var = KTL_TypeGetEntry(cont->type_map, node->data.var_decl.entry->var.type);
    KTL_BackendVarInfo *var = KTL_BackendFindVar(&cont->table, node->data.var_decl.entry);
    int                size = get_size(cont->type_map, node->data.var_decl.entry->var.type);

    if (type_var->kind == KTL_TYPE_PTR ||
        type_var->kind == KTL_TYPE_BASE) {

        // printf("%s:%d::%d\n", __FILE__,__LINE__,size);

        _XOR(_REG_64(_NR(RAX)), _REG_64(_NR(RAX)));
        _MOV(_MEM_IDX(_NR(RBP), var->loc.stack.offset, size), _REG(_NR(RAX), size));
        return ;
    }

    _LEA(_REG_64(_NR(RDI)), _MEM_IDX(_NR(RBP), var->loc.stack.offset, 8));
    _XOR(_REG(_NR(RAX), 1), _REG(_NR(RAX), 1));
    _MOV(_REG_64(_NR(RCX)), _IMM_64(size));
    _REP_STOSB;

    return ;
}

static void emit_load_address(KTL_BackendContext *cont, KTL_AstNode *node) {
    assert(cont);
    assert(node);

    switch (node->kind) {

    case KTL_AST_VARIABLE: {
        KTL_BackendVarInfo *var = KTL_BackendFindVar(&cont->table,
                                  node->data.var.info.res.entry);

        if (var->storage == KTL_BACKEND_STORAGE_STATIC) {
            _LEA(_REG_64(_NR(RAX)), _MEM_RIP_VAR(var->loc.stat.label, 8));
        }
        else {
            _LEA(_REG_64(_NR(RAX)), _MEM_IDX(_NR(RBP), var->loc.stack.offset, 8));
        }
        return ;
    }

    case KTL_AST_FIELD_ACCESS: {
        KTL_AstNode   *base      = node->move.unary.next;
        KTL_TypeEntry *base_type = get_type(cont, base);

        if (node->data.field.is_ptr) {
            emit_expr(cont, base);

            KTL_TypeEntry *struct_type = KTL_TypeGetEntry(cont->type_map,
                                                base_type->dt.ptr.prev_type);
            int offset = get_offset_field(struct_type, node->data.field.name);
            if (offset != 0) {
                _ADD(_REG_64(_NR(RAX)), _IMM_64(offset));
            }
        }
        else {
            emit_load_address(cont, base);

        // printf("BASE TYPE: %s\n", node->data.field.name);
        // printf("BASE TYPE: %d\n", base_type->kind);

            int offset = get_offset_field(base_type, node->data.field.name);
            if (offset != 0) {
                _ADD(_REG_64(_NR(RAX)), _IMM_64(offset));
            }
        }
        return ;
    }

    case KTL_AST_INDEX_ACCESS: {
        KTL_AstNode   *base      = node->move.binary.left;
        KTL_AstNode   *index     = node->move.binary.right;
        KTL_TypeEntry *base_type = get_type(cont, base);

        KTL_TypeID elem_id;
        if (base_type->kind == KTL_TYPE_ARRAY) {
            elem_id = base_type->dt.arr.base_type;
        }
        else if (base_type->kind == KTL_TYPE_PTR) {
            elem_id = base_type->dt.ptr.prev_type;
        }
        else {
            assert(false && "indexing non-array non-pointer");
            return ;
        }
        int elem_size = get_size(cont->type_map, elem_id);

        if (base_type->kind == KTL_TYPE_PTR) {
            emit_expr(cont, base);
        }
        else {
            emit_load_address(cont, base);
        }
        emit_push(cont, KTL_REG_RAX);

        emit_expr(cont, index);
        if (elem_size != 1) {
            _IMUL(_REG_64(_NR(RAX)), _IMM_64(elem_size));
        }

        emit_pop(cont, KTL_REG_RDI);
        _ADD(_REG_64(_NR(RAX)), _REG_64(_NR(RDI)));

        return ;
    }

    case KTL_AST_UNARY_OPER: {
        if (node->data.oper.op == KTL_OPER_UNGET_PTR) {
            emit_expr(cont, node->move.unary.next);
            return ;
        }
        assert(false && "non-lvalue unary in lvalue context");
        return ;
    }

    default:
        assert(false && "not a valid lvalue");
        return ;
    }
}

static void emit_cond_block(KTL_BackendContext *cont, KTL_AstNode *node) {
    assert(cont);
    assert(node);

    KTL_AstNode *branch = node->move.n.children[0];
    int else_label      = cont->label_counter++;
    int end_cond        = else_label;
    char buf[32]        = "";

    if (node->move.n.amount != 1) {
        end_cond = cont->label_counter++;
    }

    sprintf(buf, ".L%d", end_cond);
    KTL_StrID end_label = _TEXT(buf);

    emit_expr(cont, branch->move.binary.left); /* condition */

    _TEST(_REG_64(_NR(RAX)), _REG_64(_NR(RAX)));

    sprintf(buf, ".L%d", else_label);
    _JZ(_LBL(_TEXT(buf)));
    emit_block(cont, branch->move.binary.right);

    _JMP(_LBL(end_label));

    sprintf(buf, ".L%d", else_label);
    _LABEL(_TEXT(buf));  /* else label */

    bool has_else = false;
    for (int i = 1; i < node->move.n.amount; i++) {
        branch = node->move.n.children[i];
        if (branch->kind == KTL_AST_ELSE_BRANCH) {
            has_else = true;
            break;
        };

        else_label = cont->label_counter++;

        emit_expr(cont, branch->move.binary.left);
        _TEST(_REG_64(_NR(RAX)), _REG_64(_NR(RAX)));

        sprintf(buf, ".L%d", else_label);
        _JZ(_LBL(_TEXT(buf)));

        emit_block(cont, branch->move.binary.right);
        _JMP(_LBL(end_label));

        sprintf(buf, ".L%d", else_label);
        _LABEL(_TEXT(buf));
    }
    if (has_else) {
        emit_block(cont, branch->move.unary.next);
    }
    _LABEL(end_label);

    return ;
}

static void emit_for_block(KTL_BackendContext *cont, KTL_AstNode *node) {
    assert(cont);
    assert(node);

    int last_break    = cont->loop_label_break;
    int last_continue = cont->loop_label_continue;

    int cur_break    = cont->label_counter++;
    int cur_continue = cont->label_counter++;

    cont->loop_label_break    = cur_break;
    cont->loop_label_continue = cur_continue;

    char buf[32] = "";
    sprintf(buf, ".L%d", cur_continue);
    KTL_StrID L_cont = _TEXT(buf);

    sprintf(buf, ".L%d", cur_break);
    KTL_StrID L_brk  = _TEXT(buf);

    if (node->move.n.children[0]) {
        if (node->move.n.children[0]->kind == KTL_AST_ASSIGN) {
            emit_assign(cont, node->move.n.children[0]);
        } else {
            emit_var_decl(cont, node->move.n.children[0]);
        }
    }

    _LABEL(L_cont);
    if (node->move.n.children[1]) {
        emit_expr(cont, node->move.n.children[1]);
    }
    _TEST(_REG_64(_NR(RAX)), _REG_64(_NR(RAX)));
    _JZ(_LBL(L_brk));

    emit_block(cont, node->move.n.children[3]);

    if (node->move.n.children[2]) {
        emit_assign(cont, node->move.n.children[2]);
    }
    _JMP(_LBL(L_cont));
    _LABEL(L_brk);

    cont->loop_label_break    = last_break;
    cont->loop_label_continue = last_continue;

    return ;
}

static void emit_while_block(KTL_BackendContext *cont, KTL_AstNode *node) {
    assert(cont);
    assert(node);

    int last_break    = cont->loop_label_break;
    int last_continue = cont->loop_label_continue;

    int cur_break    = cont->label_counter++;
    int cur_continue = cont->label_counter++;

    cont->loop_label_break    = cur_break;
    cont->loop_label_continue = cur_continue;

    char buf[32] = "";
    sprintf(buf, ".L%d", cur_continue);
    KTL_StrID L_cont = _TEXT(buf);

    sprintf(buf, ".L%d", cur_break);
    KTL_StrID L_brk  = _TEXT(buf);

    _LABEL(L_cont);
    emit_expr(cont, node->move.binary.left);

    _TEST(_REG_64(_NR(RAX)), _REG_64(_NR(RAX)));
    _JZ(_LBL(L_brk));

    emit_block(cont, node->move.binary.right);

    _JMP(_LBL(L_cont));
    _LABEL(L_brk);

    cont->loop_label_break    = last_break;
    cont->loop_label_continue = last_continue;

    return ;
}

static void emit_continue(KTL_BackendContext *cont, KTL_AstNode *node) {
    assert(cont);
    assert(node);

    char buf[32] = "";
    sprintf(buf, ".L%d", cont->loop_label_continue);
    _JMP(_LBL(_TEXT(buf)));

    return ;
}

static void emit_break(KTL_BackendContext *cont, KTL_AstNode *node) {
    assert(cont);
    assert(node);

    char buf[32] = "";
    sprintf(buf, ".L%d", cont->loop_label_break);
    _JMP(_LBL(_TEXT(buf)));

    return ;
}

static void emit_exit(KTL_BackendContext *cont) {
    assert(cont);

    _MOV(_REG_64(_NR(RAX)), _IMM_64(60));
    _XOR(_REG_64(_NR(RDI)), _REG_64(_NR(RDI)));
    _SYSCALL;

    return ;
}

static void emit_return(KTL_BackendContext *cont, KTL_AstNode *node) {
    assert(cont);
    assert(node);

    if (node->move.unary.next != NULL) emit_expr(cont, node->move.unary.next);

    _MOV(_REG_64(_NR(RSP)), _REG_64(_NR(RBP)));
    _POP(_REG_64(_NR(RBP)));
    _RET;

    return ;
}

static void emit_assign(KTL_BackendContext *cont, KTL_AstNode *node) {
    assert(cont);
    assert(node);
    assert(node->kind == KTL_AST_ASSIGN);

    KTL_AstNode *l_val = node->move.binary.left;
    KTL_AstNode *r_val = node->move.binary.right;

    KTL_TypeEntry *type = get_type(cont, l_val);
    int size            = get_assign_size(cont, type);

    if (type->kind == KTL_TYPE_BLOCK || type->kind == KTL_TYPE_ARRAY) {
        emit_assign_struct(cont, l_val, r_val, type);
        return ;
    }

    emit_load_address(cont, l_val);
    emit_push(cont, KTL_REG_RAX);

    emit_expr(cont, r_val);
    emit_pop(cont, KTL_REG_RDI);

    /* mov [rdi], rax(size) */
    // printf("SIZE::: %d\n", size);
        // printf("%s:%d::%d\n", __FILE__,__LINE__,size);
    _MOV(_MEM_IDX(_NR(RDI), 0, size), _REG(_NR(RAX), size));

    return ;
}

static void emit_assign_struct(KTL_BackendContext *cont,
                               KTL_AstNode *l_val, KTL_AstNode *r_val,
                               KTL_TypeEntry *type) {
    assert(cont);
    assert(l_val);
    assert(r_val);
    assert(type);

    int size = -1;
    if (type->kind == KTL_TYPE_BLOCK) {
        size = type->dt.block.size;
    } else {
        int elem = get_size(cont->type_map, type->dt.arr.base_type);
        size = type->dt.arr.elem_count * elem;
    }

    emit_load_address(cont, l_val);
    emit_push(cont, KTL_REG_RAX);                          /* save dst */

    emit_load_address(cont, r_val);
    _MOV(_REG_64(_NR(RSI)), _REG_64(_NR(RAX)));            /* mov rsi, rax */

    emit_pop(cont, KTL_REG_RDI);                           /* restore dst */

    _MOV(_REG_64(_NR(RCX)), _IMM_64(size));                /* mov rcx, size */
    _REP_MOVSB;

    return ;
}

static void emit_func_call(KTL_BackendContext *cont, KTL_AstNode *node) {
    assert(cont);
    assert(node);
    assert(node->kind == KTL_AST_FUNCTION_CALL ||
           node->kind == KTL_AST_FUNCTION_STD_CALL);

    int n_par       = node->move.n.amount;
    int n_reg_par   = n_par >= KTL_PARAM_REGS_COUNT ? KTL_PARAM_REGS_COUNT : n_par;
    int n_stack_par = n_par - n_reg_par;

    for (int i = 0; i < n_par; i++) {
        emit_expr(cont, node->move.n.children[n_par - 1 - i]);
        emit_push(cont, KTL_REG_RAX);
    }

    for (int i = 0; i < n_reg_par; i++) {
        emit_pop(cont, KTL_PARAM_REGS[i]);
    }

    bool need_align = (cont->stack_depth % 16) != 0;
    if (need_align) {
        _SUB(_REG_64(_NR(RSP)), _IMM_64(8));
        cont->stack_depth += 8;
    }

    _XOR(_REG_64(_NR(RAX)), _REG_64(_NR(RAX)));

    if (node->kind == KTL_AST_FUNCTION_STD_CALL) {
        _CALL_PLT(_SYM_FUNC_GOT(node->data.func_call.info.res.entry->str_id));
    }
    else {
        KTL_BackendFuncInfo *func = KTL_BackendFindFunc(&cont->table,
                                        node->data.func_call.info.res.entry);
        _CALL(_SYM_FUNC(func->label));
    }

    int cleanup = (need_align ? 8 : 0) + n_stack_par * 8;
    if (cleanup > 0) {
        _ADD(_REG_64(_NR(RSP)), _IMM_64(cleanup));
        cont->stack_depth -= cleanup;
    }

    return ;
}

static int get_assign_size(KTL_BackendContext *cont, KTL_TypeEntry *type) {
    switch (type->kind) {
        case KTL_TYPE_BASE:  return type->dt.base.size;
        case KTL_TYPE_PTR:   return KTL_SYSTEM_PTR_SIZE;
        case KTL_TYPE_BLOCK: return type->dt.block.size;
        case KTL_TYPE_ARRAY: {
            int elem = get_size(cont->type_map, type->dt.arr.base_type);
            return type->dt.arr.elem_count * elem;
        }
        default:
            assert(0 && "Bad Size");
            return -1;
    }
}

static void emit_expr(KTL_BackendContext *cont, KTL_AstNode *node) {
    assert(cont);
    assert(node);

    switch (node->kind) {
    case KTL_AST_FUNCTION_STD_CALL:
    case KTL_AST_FUNCTION_CALL:
        emit_func_call(cont, node);
        return ;

    case KTL_AST_VARIABLE:
    case KTL_AST_FIELD_ACCESS:
    case KTL_AST_INDEX_ACCESS: {
        emit_load_address(cont, node);

        // KTL_TypeEntry *type = get_type(cont, node);
        // if (type->kind == KTL_TYPE_BLOCK || type->kind == KTL_TYPE_ARRAY) return ;

        KTL_TypeID type_id = KTL_BAD_TYPE_ID;
        switch (node->kind) {
            case KTL_AST_VARIABLE:     type_id = node->data.var.info.res.entry->var.type; break;
            case KTL_AST_FIELD_ACCESS: type_id = node->data.field.type;        break;
            case KTL_AST_INDEX_ACCESS: type_id = node->data.index.type_value;  break;
            default:
                assert(0 && "Bad Expr");
                return ;
        }

        int size = get_size(cont->type_map, type_id);
        KTL_TypeEntry *type = KTL_TypeGetEntry(cont->type_map, type_id);

        // printf("%s:%d::%d\n", __FILE__,__LINE__,size);
        if (type->kind == KTL_TYPE_BASE ||
            type->kind == KTL_TYPE_PTR) {
                _MOV(_REG(_NR(RAX), size), _MEM_IDX(_NR(RAX), 0, size));
        }
        debug_out("SIZE IN EXPR: %d\n", size);

        return ;
    }

    case KTL_AST_BINARY_OPER:
    case KTL_AST_UNARY_OPER:
        emit_oper(cont, node);
        return ;

    case KTL_AST_VALUE_INT:
        _MOV(_REG_64(_NR(RAX)), _IMM_64(node->data.int_val.value));
        return ;

    case KTL_AST_VALUE_STR: {
        KTL_StrID label = get_string_name(cont->str_map, cont->symbol_prefix,
                                          node->data.str_val.value);
        _LEA(_REG_64(_NR(RAX)), _MEM_RIP_VAR(label, 8));
        return ;
    }

    case KTL_AST_CAST: {
        emit_expr(cont, node->move.unary.next);
        KTL_AstNode *src = node->move.unary.next;

        KTL_TypeID src_id = KTL_BAD_TYPE_ID;
        switch (src->kind) {
            case KTL_AST_VARIABLE:     src_id = src->data.var.info.res.entry->var.type; break;
            case KTL_AST_FIELD_ACCESS: src_id = src->data.field.type;       break;
            case KTL_AST_INDEX_ACCESS: src_id = src->data.index.type_value; break;
            case KTL_AST_VALUE_INT:    src_id = src->data.int_val.type_res; break;
            case KTL_AST_BINARY_OPER:
            case KTL_AST_UNARY_OPER:   src_id = src->data.oper.type_res;    break;
            case KTL_AST_CAST:         src_id = src->data.cast.target;      break;
            default:                   src_id = node->data.cast.target;     break;
        }
        emit_cast(cont, src_id, node->data.cast.target);
        return ;
    }

    default:
        return ;
    }
}

static void emit_cast(KTL_BackendContext *cont,
                      KTL_TypeID src_id, KTL_TypeID dst_id) {
    assert(cont);
    if (src_id == dst_id) return ;

    KTL_TypeEntry *src = KTL_TypeGetEntry(cont->type_map, src_id);
    KTL_TypeEntry *dst = KTL_TypeGetEntry(cont->type_map, dst_id);

    if (src->kind != KTL_TYPE_BASE || dst->kind != KTL_TYPE_BASE) return ;

    int src_size = src->dt.base.size;
    int dst_size = dst->dt.base.size;

    if (dst_size == 1 && strcmp(dst->dt.base.name, "bool") == 0) {
        _TEST(_REG_64(_NR(RAX)), _REG_64(_NR(RAX)));
        _SET(NE, _REG(_NR(RAX), 1));
        _MOVZX(_REG_64(_NR(RAX)), _REG(_NR(RAX), 1));
        return ;
    }

    if (dst_size <= src_size) return ;

    if      (src_size == 1 && dst_size >= 4) _MOVSX(_REG_64(_NR(RAX)), _REG(_NR(RAX), 1));
    else if (src_size == 1 && dst_size == 2) _MOVSX(_REG(_NR(RAX), 2), _REG(_NR(RAX), 1));
    else if (src_size == 2 && dst_size >= 4) _MOVSX(_REG_64(_NR(RAX)), _REG(_NR(RAX), 2));
    else if (src_size == 4 && dst_size == 8) _CDQE;

    return ;
}


static void emit_oper(KTL_BackendContext *cont, KTL_AstNode *node) {
    assert(cont);
    assert(node);

    KTL_Oper op  = node->data.oper.op;
    char buf[32] = "";

    switch (op) {
    case KTL_OPER_ADD:
    case KTL_OPER_SUB:
    case KTL_OPER_MUL:
    case KTL_OPER_DIV:
    case KTL_OPER_MOD:
        emit_expr(cont, node->move.binary.left);
        emit_push(cont, KTL_REG_RAX);
        emit_expr(cont, node->move.binary.right);
        emit_pop(cont, KTL_REG_RDI);

        if (op == KTL_OPER_ADD) {
            _ADD(_REG_64(_NR(RAX)), _REG_64(_NR(RDI)));
        }
        else if (op == KTL_OPER_SUB) {
            _SUB(_REG_64(_NR(RDI)), _REG_64(_NR(RAX)));
            _MOV(_REG_64(_NR(RAX)), _REG_64(_NR(RDI)));
        }
        else if (op == KTL_OPER_MUL) {
            _IMUL(_REG_64(_NR(RAX)), _REG_64(_NR(RDI)));
        }
        else {
            _CQO;

            emit_push(cont, KTL_REG_RAX);
            _MOV(_REG_64(_NR(RAX)), _REG_64(_NR(RDI)));
            emit_pop(cont, KTL_REG_RDI);

            _IDIV1(_REG_64(_NR(RDI)));
            if (op == KTL_OPER_MOD) {
                _MOV(_REG_64(_NR(RAX)), _REG_64(_NR(RDX)));
            }
        }
        return ;

    case KTL_OPER_NEG:
        emit_expr(cont, node->move.unary.next);
        _NEG(_REG_64(_NR(RAX)));
        return ;

    case KTL_OPER_AND:
    case KTL_OPER_OR: {
        int label_end = cont->label_counter++;
        sprintf(buf, ".L%d", label_end); KTL_StrID L_end = _TEXT(buf);

        emit_expr(cont, node->move.binary.left);
        _TEST(_REG_64(_NR(RAX)), _REG_64(_NR(RAX)));
        if (op == KTL_OPER_AND) {
            _JZ(_LBL(L_end));
        } else {
            _JNZ(_LBL(L_end));
        }

        emit_expr(cont, node->move.binary.right);
        _LABEL(L_end);
        return ;
    }

    case KTL_OPER_COMP_BE:
    case KTL_OPER_COMP_LE:
    case KTL_OPER_COMP_L:
    case KTL_OPER_COMP_B:
    case KTL_OPER_COMP_E:
    case KTL_OPER_COMP_NE: {
        emit_expr(cont, node->move.binary.left);
        emit_push(cont, KTL_REG_RAX);
        emit_expr(cont, node->move.binary.right);
        emit_pop(cont, KTL_REG_RDI);

        _CMP(_REG_64(_NR(RDI)), _REG_64(_NR(RAX)));

        switch (op) {
            case KTL_OPER_COMP_BE: _SET(GE, _REG(_NR(RAX), 1)); break;
            case KTL_OPER_COMP_B:  _SET(G,  _REG(_NR(RAX), 1)); break;
            case KTL_OPER_COMP_LE: _SET(LE, _REG(_NR(RAX), 1)); break;
            case KTL_OPER_COMP_L:  _SET(L,  _REG(_NR(RAX), 1)); break;
            case KTL_OPER_COMP_E:  _SET(E,  _REG(_NR(RAX), 1)); break;
            case KTL_OPER_COMP_NE: _SET(NE, _REG(_NR(RAX), 1)); break;
            default:
                assert(0 && "Bad Operation");
                return ;
        }
        _MOVZX(_REG_64(_NR(RAX)), _REG(_NR(RAX), 1));
        return ;
    }

    case KTL_OPER_GET_PTR:
        emit_load_address(cont, node->move.unary.next);
        return ;

    case KTL_OPER_UNGET_PTR:
        emit_expr(cont, node->move.unary.next);
        return ;

    default:
        return ;
    }
}


static void emit_push(KTL_BackendContext *cont, KTL_RegID reg) {
    _PUSH(_REG_64(reg));
    cont->stack_depth += 8;
}

static void emit_pop(KTL_BackendContext *cont, KTL_RegID reg) {
    _POP(_REG_64(reg));
    cont->stack_depth -= 8;
}

static void emit_block(KTL_BackendContext *cont, KTL_AstNode *node) {
    assert(cont);
    if (node == NULL || node->kind != KTL_AST_BLOCK)  return ;

    debug_out("BLOCK\n");

    for (int i = 0; i < node->move.n.amount; i++) {
        emit_body(cont, node->move.n.children[i]);
    }
    return ;
}

static void emit_body(KTL_BackendContext *cont, KTL_AstNode *node) {
    assert(cont);
    if (node == NULL)  return ;

    switch (node->kind) {
    case KTL_AST_VARIABLE_DECL:
        debug_out("VARIABLE DECL\n");
        emit_var_decl(cont, node);
        return ;

    case KTL_AST_ASSIGN:
        debug_out("ASSIGN\n");
        emit_assign(cont, node);
        return ;

    case KTL_AST_COND_BLOCK:
        debug_out("COND BLOCK\n");
        emit_cond_block(cont, node);
        return ;

    case KTL_AST_WHILE_BLOCK:
        debug_out("WHILE BLOCK\n");
        emit_while_block(cont, node);
        return ;

    case KTL_AST_FOR_BLOCK:
        debug_out("FOR BLOCK\n");
        emit_for_block(cont, node);
        return ;

    case KTL_AST_RETURN:
        debug_out("RETURN\n");
        emit_return(cont, node);
        return ;

    case KTL_AST_BREAK:
        debug_out("BREAK\n");
        emit_break(cont, node);
        return ;

    case KTL_AST_CONTINUE:
        debug_out("CONTINUE\n");
        emit_continue(cont, node);
        return ;

    case KTL_AST_FUNCTION_CALL:
    case KTL_AST_FUNCTION_STD_CALL:
        debug_out("FUNC CALL\n");
        emit_func_call(cont, node);
        return ;

    default:
        return ;
    }
}




