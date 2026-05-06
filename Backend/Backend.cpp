#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "Backend.h"
#include "StrMap.h"
#include "TypeMap.h"
#include "ASTCommon.h"
#include "BackMap.h"
#include "OpCode.h"

// =======================================================================
// CONSTANTS
// =======================================================================

constexpr static int KTL_BACKEND_VARS_INIT  = 16;
constexpr static int KTL_BACKEND_FUNCS_INIT = 8;
constexpr static int KTL_BACKEND_GROW_MOD   = 2;

constexpr char *KTL_GLOBAL_PREFIX = "__global__";
constexpr char *KTL_FUNC_PREFIX   = "__func__";
constexpr char *KTL_STRING_PREFIX = "__string__";


const KTL_RegId KTL_PARAM_REGS[6] = {
    KTL_REG_RDI, KTL_REG_RSI, KTL_REG_RDX,
    KTL_REG_RCX, KTL_REG_R8,  KTL_REG_R9,
};



// =======================================================================
// REGISTER NAMES TABLE
// =======================================================================

/*           0=8byte, 1=4byte, 2=2byte, 3=1byte. */
static const char *KTL_REG_NAMES[KTL_REG_COUNT][4] = {
    /* RAX */ {"rax",  "eax",  "ax",   "al"  },
    /* RCX */ {"rcx",  "ecx",  "cx",   "cl"  },
    /* RDX */ {"rdx",  "edx",  "dx",   "dl"  },
    /* RBX */ {"rbx",  "ebx",  "bx",   "bl"  },
    /* RSP */ {"rsp",  "esp",  "sp",   "spl" },
    /* RBP */ {"rbp",  "ebp",  "bp",   "bpl" },
    /* RSI */ {"rsi",  "esi",  "si",   "sil" },
    /* RDI */ {"rdi",  "edi",  "di",   "dil" },
    /* R8  */ {"r8",   "r8d",  "r8w",  "r8b" },
    /* R9  */ {"r9",   "r9d",  "r9w",  "r9b" },
    /* R10 */ {"r10",  "r10d", "r10w", "r10b"},
    /* R11 */ {"r11",  "r11d", "r11w", "r11b"},
    /* R12 */ {"r12",  "r12d", "r12w", "r12b"},
    /* R13 */ {"r13",  "r13d", "r13w", "r13b"},
    /* R14 */ {"r14",  "r14d", "r14w", "r14b"},
    /* R15 */ {"r15",  "r15d", "r15w", "r15b"},
};

static const char *KTL_STATIC_SIZE_NAME[4] = {
    "db",
    "dw",
    "dd",
    "dq",
};

const char * reg_name          (KTL_RegId reg, int size);
const char * size_prefix       (int size);
const char * global_size_prefix(int size);


static void layout_global       (KTL_BackendContext *cont, KTL_AstNode *root);
static void layout_all_functions(KTL_BackendContext *cont, KTL_AstNode *root);
static void layout_func         (KTL_BackendContext *cont, KTL_AstNode *node);
static void layout_body         (KTL_BackendContext *cont, KTL_AstNode *node);

static int get_size (KTL_TypeMap *map, KTL_TypeID type);
static int get_align(KTL_TypeMap *map, KTL_TypeID type);
static int align_up (int offset,       int align);

static void emit_header    (KTL_BackendContext *cont, KTL_AstNode *root);
static void emit_globals   (KTL_BackendContext *cont, KTL_AstNode *root);
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
/* static void emit_variable(KTL_BackendContext *cont, KTL_AstNode *node); */
static int  get_offset_field    (KTL_TypeEntry *block, KTL_StrID name);
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
KTL_TypeEntryKind get_type_kind (KTL_BackendContext *cont, KTL_AstNode *node);
KTL_TypeEntry *   get_type      (KTL_BackendContext *cont, KTL_AstNode *node);
static void emit_func_call      (KTL_BackendContext *cont, KTL_AstNode *node);
static void emit_expr           (KTL_BackendContext *cont, KTL_AstNode *node);
static void emit_cast           (KTL_BackendContext *cont,
                                 KTL_TypeID          src_id,
                                 KTL_TypeID          dst_id);
static void emit_oper           (KTL_BackendContext *cont, KTL_AstNode *node);
static void emit_block          (KTL_BackendContext *cont, KTL_AstNode *node);
static void emit_body           (KTL_BackendContext *cont, KTL_AstNode *node);
static void emit_push           (KTL_BackendContext *cont, const char *reg);
static void emit_pop            (KTL_BackendContext *cont, const char *reg);

KTL_StrID get_global_name(KTL_StrMap *str_map, const char *prefix, KTL_StrID name);
KTL_StrID get_func_name  (KTL_StrMap *str_map, const char *prefix, KTL_StrID name);
KTL_StrID get_string_name(KTL_StrMap *str_map, const char *prefix, KTL_StrID name);
KTL_StrID get_name       (KTL_StrMap *str_map, const char *prefix_1,
                          const char *prefix_2, KTL_StrID name);



const char * reg_name(KTL_RegId reg, int size) {
    assert(reg >= 0 && reg < KTL_REG_COUNT);

    int idx;
    switch (size) {
        case 8: idx = 0; break;
        case 4: idx = 1; break;
        case 2: idx = 2; break;
        case 1: idx = 3; break;
        default:
            assert(false && "unsupported register size");
            return "?";
    }
    return KTL_REG_NAMES[reg][idx];
}

const char * size_prefix(int size) {
    switch (size) {
        case 1: return "byte";
        case 2: return "word";
        case 4: return "dword";
        case 8: return "qword";
        default:
            assert(false && "unsupported memory operand size");
            return "?";
    }
}

const char * global_size_prefix(int size) {
    switch (size) {
        case 1: return KTL_STATIC_SIZE_NAME[0];
        case 2: return KTL_STATIC_SIZE_NAME[1];
        case 4: return KTL_STATIC_SIZE_NAME[2];
        case 8: return KTL_STATIC_SIZE_NAME[3];
        default:
            assert(false && "unsupported memory operand size");
            return "?";
    }
}

KTL_Error KTL_BackendInit(KTL_BackendContext *cont,
                          KTL_TypeMap        *type_map,
                          KTL_StrMap         *str_map,
                          KTL_SymbolMap      *global_scope,
                          FILE               *output) {
    assert(cont);
    assert(type_map);
    assert(str_map);
    assert(global_scope);
    assert(output);

    cont->type_map     = type_map;
    cont->str_map      = str_map;
    cont->global_scope = global_scope;
    cont->file         = output;

    cont->current_func        = NULL;
    cont->loop_label_break    = -1;
    cont->loop_label_continue = -1;
    cont->label_counter       = 0;

    cont->symbol_prefix = "";

    return KTL_BackendTableInit(&cont->table);
}

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
                layout_body(cont, node->move.unary.next);
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



static void emit_header(KTL_BackendContext *cont, KTL_AstNode *root) {
    assert(cont);
    assert(root);

    print_asm(";============================================\n"
              "; THIS FILE IS AUTOGENERATED BY KasTLe\n"
              "; DO NOT MODIFY IT\n"
              ";============================================\n\n");

    print_asm("section .data\n");

    for (int i = 0; i < root->move.n.amount; i++) {
        KTL_AstNode *node = root->move.n.children[i];
        if (node->kind == KTL_AST_VARIABLE_DECL) {
            emit_global_var(cont, node);
        }
    }

    print_asm("\n");
    return ;
}

static void emit_globals(KTL_BackendContext *cont, KTL_AstNode *root) {
    assert(cont);
    assert(root);

    for (int i = 0; i < root->move.n.amount; i++) {
        emit_global_var(cont, root->move.n.children[i]);
    }

    return ;
}

static void emit_text(KTL_BackendContext *cont, KTL_AstNode *root) {
    print_asm("section .text\n");
    print_asm("global _start\n\n");
    cont->stack_depth = 0;

    for (int i = 0; i < root->move.n.amount; i++) {
        KTL_AstNode *node = root->move.n.children[i];
        if (node->kind != KTL_AST_MAIN)  continue;

        print_asm("_start:\n");
        print_asm("    push rbp\n");
        print_asm("    mov  rbp, rsp\n");
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
    if (node->kind != KTL_AST_FUNCTION_DECL && node->kind != KTL_AST_MAIN) return;

    if (node->kind == KTL_AST_FUNCTION_DECL) {
        print_asm("\n; Function: %s\n", node->data.func_decl.func->str_id);
        emit_function_header(cont, node);
        emit_block(cont, node->move.n.children[0]);
    }
    else {
        emit_block(cont, node->move.unary.next);
    }

    if (node->kind == KTL_AST_FUNCTION_DECL) {
        print_asm("    mov  rsp, rbp\n");
        print_asm("    pop  rbp\n");
        print_asm("    ret\n\n");
    }
}

static void emit_global_var(KTL_BackendContext *cont, KTL_AstNode *node) {
    assert(cont);
    assert(node);
    if (node->kind != KTL_AST_VARIABLE_DECL)  return;

    KTL_SymbolEntry    *sym  = node->data.var_decl.entry;
    KTL_BackendVarInfo *info = KTL_BackendFindVar(&cont->table, sym);
    KTL_TypeEntry      *type = KTL_TypeGetEntry(cont->type_map, sym->var.type);

    int size  = get_size(cont->type_map, sym->var.type);
    int align = get_align(cont->type_map, sym->var.type);

    print_asm("align %d\n", align);
    print_asm("%s:\n", info->loc.stat.label);

    if (!node->data.var_decl.is_init) {
        print_asm("    times %d db 0\n", size);
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

        print_asm("    %s %lld\n", global_size_prefix(size),
                  (long long) value);
        return;
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
            print_asm("    times %d db 0\n", (total - n_init) * elem_size);
        }
        return;
    }
    if (type->kind == KTL_TYPE_BLOCK) {
        print_asm("    times %d db 0\n", type->dt.block.size);
        return;
    }
}


static void emit_strings(KTL_BackendContext *cont, KTL_AstNode *root) {
    assert(cont);
    assert(root);

    print_asm("section .rodata  ; constant strings\n");
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

    print_asm("    %s db", get_string_name(cont->str_map, cont->symbol_prefix, value));
    fputc('\"', cont->output);

    for (int i = 0; i < strlen(value); i++) {
        char c = value[i];
        if (c == '\n' || c == '\t' || c == '\r') {
            print_asm("\", 0x%X, \"", c);
        }
        else {
            fputc(c, cont->file);
        }
    }

    fputc('\"',  cont->file);
    fputs(", 0", cont->file);
    fputc('\n',  cont->file);

    return ;
}



static void emit_function_header(KTL_BackendContext *cont, KTL_AstNode *node) {
    assert(cont);
    assert(node);
    if (node->kind != KTL_AST_FUNCTION_DECL)    return ;

    KTL_BackendFuncInfo *func = KTL_BackendFindFunc(&cont->table,
                                        node->data.func_decl.func);
    if (func == NULL)   return ;

    print_asm("global %s\n", func->label);
    print_asm("%s:\n",       func->label);

    print_asm("    push rbp\n");
    print_asm("    mov  rbp, rsp\n");

    if (func->frame_size > 0) {
        print_asm("    sub  rsp, %d\n",  func->frame_size);
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

        const char *reg = reg_name(KTL_PARAM_REGS[i],
                                             KTL_SYSTEM_PTR_SIZE);
        print_asm("    mov  [rbp%+d], %s\n",
                info->loc.stack.offset, reg);
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

        print_asm("    mov  %s [rbp%+d], %s\n",
                size_prefix(size),
                var->loc.stack.offset,
                reg_name(KTL_REG_RAX, size));
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

    if (type_elem->kind == KTL_TYPE_PTR ||
        type_elem->kind == KTL_TYPE_BASE) {
        for (int i = 0; i < array->move.n.amount; i++) {
            emit_expr(cont, array->move.n.children[i]);
            print_asm("    mov  %s [rbp%+d], %s\n",
                    size_prefix(size_elem),
                    var->loc.stack.offset + i * size_elem,
                    reg_name(KTL_REG_RAX, size_elem));
        }
        return ;
    }
    for (int i = 0; i < array->move.n.amount; i++) {
        emit_load_address(cont, array->move.n.children[i]);
        print_asm("    mov  rsi, rax\n");                                             /* src */
        print_asm("    lea  rdi, [rbp%+d]\n", var->loc.stack.offset + i * size_elem); /* dst */
        print_asm("    mov  rcx, %d\n", size_elem);
        print_asm("    rep  movsb\n");
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
        print_asm("    xor  rax, rax\n");
        print_asm("    mov  [rbp%+d], %s\n", var->loc.stack.offset, reg_name(KTL_REG_RAX,
                                get_size(cont->type_map, node->data.var_decl.entry->var.type)));
        return ;
    }

    print_asm("    lea  rdi, [rbp%+d]\n", var->loc.stack.offset);
    print_asm("    xor  al, al\n");
    print_asm("    mov  rcx, %d\n", size);
    print_asm("    rep  stosb\n");

    return ;
}0x88



static void emit_load_address(KTL_BackendContext *cont, KTL_AstNode *node) {
    assert(cont);
    assert(node);

    switch (node->kind) {

    case KTL_AST_VARIABLE: {
        KTL_BackendVarInfo *var = KTL_BackendFindVar(&cont->table,
                                  node->data.var.info.res.entry);

        if (var->storage == KTL_BACKEND_STORAGE_STATIC) {
            print_asm("    lea  rax, [rel %s]\n", var->loc.stat.label);
        }
        else {
            print_asm("    lea  rax, [rbp%+d]\n", var->loc.stack.offset);
        }
        return;
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
                print_asm("    add  rax, %d\n", offset);
            }
        }
        else {
            emit_load_address(cont, base);

            int offset = get_offset_field(base_type, node->data.field.name);
            if (offset != 0) {
                print_asm("    add  rax, %d\n", offset);
            }
        }
        return;
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
            return;
        }
        int elem_size = get_size(cont->type_map, elem_id);

        if (base_type->kind == KTL_TYPE_PTR) {
            emit_expr(cont, base);
        }
        else {
            emit_load_address(cont, base);
        }
        emit_push(cont, "rax");

        emit_expr(cont, index);
        if (elem_size != 1) {
            print_asm("    imul rax, %d\n", elem_size);
        }

        emit_pop(cont, "rdi");
        print_asm("    add  rax, rdi\n");
        return;
    }

    case KTL_AST_UNARY_OPER: {
        if (node->data.oper.op == KTL_OPER_UNGET_PTR) {
            emit_expr(cont, node->move.unary.next);
            return;
        }
        assert(false && "non-lvalue unary in lvalue context");
        return;
    }

    default:
        assert(false && "not a valid lvalue");
        return;
    }
}

/*
static void emit_variable(KTL_BackendContext *cont, KTL_AstNode *node) {
    assert(cont);
    assert(node);

    switch (node->kind) {

    case KTL_AST_VARIABLE: {
        KTL_BackendVarInfo *var = KTL_BackendFindVar(&cont->table, node->data.var.info.res.entry);
        if (var->storage == KTL_BACKEND_STORAGE_STATIC) {
            print_asm("    mov  rax, %s\n", get_global_name(cont->str_map, cont->symbol_prefix,
                                                        var->origin->str_id));
        }
        else {
            print_asm("    mov  rax, rsp\n");
            if (var->loc.stack.offset > 0) {
                print_asm("    add  rax, %d\n", var->loc.stack.offset);
            }
            else {
                print_asm("    sub  rax, %d\n", (-1) * var->loc.stack.offset);
            }
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
                print_asm("    add  rax, %d\n", offset);
            }
        }
        else {
            emit_load_address(cont, base);

            int offset = get_offset_field(base_type, node->data.field.name);
            if (offset != 0) {
                print_asm("    add  rax, %d\n", offset);
            }
        }
        return;
    }

    case KTL_AST_INDEX_ACCESS: {
        KTL_AstNode   *base      = node->move.binary.left;
        KTL_AstNode   *index     = node->move.binary.right;
        KTL_TypeEntry *base_type = get_type(cont, base);

        KTL_TypeID elem_id;
        if (base_type->kind == KTL_TYPE_ARRAY) {
            elem_id = base_type->dt.arr.base_type;
        } else if (base_type->kind == KTL_TYPE_PTR) {
            elem_id = base_type->dt.ptr.prev_type;
        } else {
            assert(false && "indexing non-array non-pointer");
            return;
        }
        int elem_size = get_size(cont->type_map, elem_id);

        if (base_type->kind == KTL_TYPE_PTR) {
            emit_expr(cont, base);
        } else {
            emit_load_address(cont, base);
        }
        emit_push(cont, "rax");

        emit_expr(cont, index);
        if (elem_size != 1) {
            print_asm("    imul rax, %d\n", elem_size);
        }
        emit_pop(cont, "rdi");
        print_asm("    add  rax, rdi\n");
        return;
    }

    case KTL_AST_UNARY_OPER:
        if (node->data.oper.op == KTL_OPER_UNGET_PTR) {
            emit_expr(cont, node->move.unary.next);
            return;
        }
        assert(false && "non-lvalue unary operator");
        return;

    }
    return ;
}
*/

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



static void emit_cond_block(KTL_BackendContext *cont, KTL_AstNode *node) {
    assert(cont);
    assert(node);

    KTL_AstNode *branch = node->move.n.children[0];
    int else_label         = cont->label_counter++;
    int end_cond           = else_label;

    if (node->move.n.amount != 1) {
        end_cond = cont->label_counter++;
    }

    emit_expr(cont, branch->move.binary.left); /* condition */
    print_asm("    test rax, rax\n");
    print_asm("    jz .L%d\n", else_label);

    emit_block(cont, branch->move.binary.right);
    print_asm("    jmp .L%d\n", end_cond);

    print_asm(".L%d:\n", else_label);

    bool has_else = false;
    for (int i = 1; i < node->move.n.amount; i++) {
        branch = node->move.n.children[i];
        if (branch->kind == KTL_AST_ELSE_BRANCH) {
            has_else = true;
            break;
        };

        else_label = cont->label_counter++;

        emit_expr(cont, branch->move.binary.left);
        print_asm("    test rax, rax\n");
        print_asm("    jz .L%d\n", else_label);

        emit_block(cont, branch->move.binary.right);
        print_asm("    jmp .L%d\n", end_cond);

        print_asm(".L%d:\n", else_label);
    }
    if (has_else) {
        emit_block(cont, branch->move.unary.next);
    }
    print_asm(".L%d:\n", end_cond);

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

    if (node->move.n.children[0]) {
        if (node->move.n.children[0]->kind == KTL_AST_ASSIGN) {
            emit_assign(cont, node->move.n.children[0]);
        }
        else {
            emit_var_decl(cont, node->move.n.children[0]);
        }
    }
    print_asm(".L%d:\n", cur_continue);
    if (node->move.n.children[1]) {
        emit_expr(cont, node->move.n.children[1]);
    }
    print_asm("    test rax, rax\n");
    print_asm("    jz   .L%d\n", cur_break);
    emit_block(cont, node->move.n.children[3]);

    if (node->move.n.children[2]) {
        emit_assign(cont, node->move.n.children[2]);
    }
    print_asm("    jmp  .L%d\n", cur_continue);
    print_asm(".L%d:\n", cur_break);

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

    print_asm(".L%d:\n", cur_continue);
    emit_expr(cont, node->move.binary.left);

    print_asm("    test rax, rax\n");
    print_asm("    jz   .L%d\n", cur_break);
    emit_block(cont, node->move.binary.right);

    print_asm("    jmp  .L%d\n", cur_continue);
    print_asm(".L%d:\n", cur_break);

    cont->loop_label_break    = last_break;
    cont->loop_label_continue = last_continue;

    return ;
}

static void emit_continue(KTL_BackendContext *cont, KTL_AstNode *node) {
    assert(cont);
    assert(node);

    print_asm("    jmp .L%d\n", cont->loop_label_continue);
    return ;
}

static void emit_break(KTL_BackendContext *cont, KTL_AstNode *node) {
    assert(cont);
    assert(node);

    print_asm("    jmp .L%d\n", cont->loop_label_break);
    return ;
}

static void emit_exit(KTL_BackendContext *cont) {
    assert(cont);

    print_asm("    mov  rax, 60\n");
    print_asm("    xor rdi, rdi\n");
    print_asm("    syscall\n");

    return ;
}

static void emit_return(KTL_BackendContext *cont, KTL_AstNode *node) {
    assert(cont);
    assert(node);

    if (node->move.unary.next != NULL)   emit_expr(cont, node->move.unary.next);

    print_asm("    mov  rsp, rbp\n");
    print_asm("    pop  rbp\n");
    print_asm("    ret\n");

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

    if (type->kind == KTL_TYPE_BLOCK ||
        type->kind == KTL_TYPE_ARRAY) {
        emit_assign_struct(cont, l_val, r_val, type);
        return;
    }

    emit_load_address(cont, l_val);
    emit_push(cont, "rax");

    emit_expr(cont, r_val);
    emit_pop(cont, "rdi");

    print_asm("    mov  %s [rdi], %s\n",
            size_prefix(size), reg_name(KTL_REG_RAX, size));
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
        default: assert(false); return -1;
    }
}

static void emit_assign_struct(KTL_BackendContext *cont,
                                KTL_AstNode       *l_val,
                                KTL_AstNode       *r_val,
                                KTL_TypeEntry     *type) {
    assert(cont);
    assert(r_val);
    assert(l_val);
    assert(type);

    int size = -1;
    if (type->kind == KTL_TYPE_BLOCK) {
        size = type->dt.block.size;
    } else {
        int elem = get_size(cont->type_map, type->dt.arr.base_type);
        size = type->dt.arr.elem_count * elem;
    }

    emit_load_address(cont, l_val);
    emit_push(cont, "rax");

    emit_load_address(cont, r_val);
    print_asm("    mov  rsi, rax\n");   /* src */

    emit_pop(cont, "rdi");              /* dst */

    print_asm("    mov  rcx, %d\n", size);
    print_asm("    rep  movsb\n");

    return ;
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


static void emit_func_call(KTL_BackendContext *cont, KTL_AstNode *node) {
    assert(cont);
    assert(node);
    assert(node->kind == KTL_AST_FUNCTION_CALL);

    int n_par = node->move.n.amount;
    int n_reg_par   = n_par >= KTL_PARAM_REGS_COUNT    ? KTL_PARAM_REGS_COUNT : n_par;
    int n_stack_par = n_par - n_reg_par;

    for (int i = 0; i < n_par; i++) {
        emit_expr(cont, node->move.n.children[n_par - 1 - i]);
        emit_push(cont, reg_name(KTL_REG_RAX, KTL_SYSTEM_PTR_SIZE));
    }

    for (int i = 0; i < n_reg_par; i++) {
        const char *reg = reg_name(KTL_PARAM_REGS[i], KTL_SYSTEM_PTR_SIZE);
        emit_pop(cont, reg);
    }

    bool need_align = (cont->stack_depth % 16) != 0;
    if (need_align) {
        print_asm("    sub  rsp, 8\n");
        cont->stack_depth += 8;
    }

    KTL_BackendFuncInfo *func = KTL_BackendFindFunc(&cont->table, node->data.func_call.info.res.entry);
    print_asm("    call %s\n", func->label);

    int cleanup = (need_align ? 8 : 0) + n_stack_par * 8;
    if (cleanup > 0) {
        print_asm("    add  rsp, %d\n", cleanup);
        cont->stack_depth -= cleanup;
    }


    return ;
}


static void emit_expr(KTL_BackendContext *cont, KTL_AstNode *node) {
    assert(cont);
    assert(node);

    switch (node->kind) {

    case KTL_AST_FUNCTION_CALL:
        emit_func_call(cont, node);
        return ;

    case KTL_AST_VARIABLE:
    case KTL_AST_FIELD_ACCESS:
    case KTL_AST_INDEX_ACCESS: {
        emit_load_address(cont, node);

        KTL_TypeEntry *type = get_type(cont, node);
        if (type->kind == KTL_TYPE_BLOCK || type->kind == KTL_TYPE_ARRAY) {
            return;
        }

        KTL_TypeID type_id = KTL_BAD_TYPE_ID;
        switch (node->kind) {
            case KTL_AST_VARIABLE:     type_id = node->data.var.info.res.entry->var.type; break;
            case KTL_AST_FIELD_ACCESS: type_id = node->data.field.type;        break;
            case KTL_AST_INDEX_ACCESS: type_id = node->data.index.type_value;  break;
            default: assert(false); return;
        }

        int size = get_size(cont->type_map, type_id);
        const char *reg    = reg_name(KTL_REG_RAX, size);
        const char *prefix = size_prefix(size);
        print_asm("    mov  %s, %s [rax]\n", reg, prefix);
        return;
    }
    case KTL_AST_BINARY_OPER:
    case KTL_AST_UNARY_OPER:
        emit_oper(cont, node);
        return ;

    case KTL_AST_VALUE_INT:
        print_asm("    mov  rax, %lld\n", node->data.int_val.value);
        return;

    case KTL_AST_VALUE_STR: {
        KTL_StrID label = get_string_name(cont->str_map, cont->symbol_prefix,
                                        node->data.str_val.value);
        print_asm("    lea  rax, [rel %s]\n", label);
        return;
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
        return;
    }

    default:
        break;
    }
    return ;
}

static void emit_cast(KTL_BackendContext *cont,
                      KTL_TypeID src_id, KTL_TypeID dst_id)
{
    if (src_id == dst_id)  return;

    KTL_TypeEntry *src = KTL_TypeGetEntry(cont->type_map, src_id);
    KTL_TypeEntry *dst = KTL_TypeGetEntry(cont->type_map, dst_id);

    if (src->kind != KTL_TYPE_BASE || dst->kind != KTL_TYPE_BASE)  return;

    int src_size = src->dt.base.size;
    int dst_size = dst->dt.base.size;

    if (dst_size == 1 && strcmp(dst->dt.base.name, "bool") == 0) {
        print_asm("    test rax, rax\n");
        print_asm("    setne al\n");
        print_asm("    movzx rax, al\n");
        return;
    }

    if (dst_size <= src_size)  return;

    if      (src_size == 1 && dst_size >= 4) print_asm("    movsx rax, al\n");
    else if (src_size == 1 && dst_size == 2) print_asm("    movsx ax, al\n");
    else if (src_size == 2 && dst_size >= 4) print_asm("    movsx rax, ax\n");
    else if (src_size == 4 && dst_size == 8) print_asm("    cdqe\n");

    return ;
}

static void emit_oper(KTL_BackendContext *cont, KTL_AstNode *node) {
    assert(cont);
    assert(node);

    KTL_Oper op = node->data.oper.op;

    switch (op) {

    case KTL_OPER_ADD:
    case KTL_OPER_SUB:
    case KTL_OPER_MUL:
    case KTL_OPER_DIV:
    case KTL_OPER_MOD:
        emit_expr(cont, node->move.binary.left);
        emit_push(cont, "rax");
        emit_expr(cont, node->move.binary.right);
        emit_pop(cont, "rdi");
        if     (op == KTL_OPER_ADD) {
            print_asm("    add  rax, rdi\n");
        }
        else if (op == KTL_OPER_SUB) {
            print_asm("    sub  rdi, rax\n");
            print_asm("    mov  rax, rdi\n");
        }
        else {
            if (op == KTL_OPER_MUL)     print_asm("    imul rax, rdi\n");
            else {
                print_asm("    cqo\n");
                print_asm("    idiv rdi\n");
                if (op == KTL_OPER_MOD)     print_asm("    mov  rax, rdx\n");
            }
        }
        return ;

    case KTL_OPER_NEG:
        emit_expr(cont, node->move.unary.next);
        print_asm("    neg  rax\n");
        return ;

    case KTL_OPER_AND:
    case KTL_OPER_OR: {
        int label_end = cont->label_counter++;

        emit_expr(cont, node->move.binary.left);
        print_asm("    test rax, rax\n");
        if (op == KTL_OPER_AND) {
            print_asm("    jz   .L%d\n", label_end);
        } else {
            print_asm("    jnz  .L%d\n", label_end);
        }
        emit_expr(cont, node->move.binary.right);
        print_asm(".L%d:\n", label_end);
        return;
    }

    case KTL_OPER_COMP_BE:
    case KTL_OPER_COMP_LE:
    case KTL_OPER_COMP_L:
    case KTL_OPER_COMP_B:
    case KTL_OPER_COMP_E:
    case KTL_OPER_COMP_NE: {
        emit_expr(cont, node->move.binary.left);
        emit_push(cont, "rax");
        emit_expr(cont, node->move.binary.right);
        emit_pop(cont, "rdi");

        print_asm("    cmp  rdi, rax\n");

        const char *setcc = NULL;
        switch (op) {
            case KTL_OPER_COMP_BE: setcc = "setge"; break;
            case KTL_OPER_COMP_B:  setcc = "setg";  break;
            case KTL_OPER_COMP_LE: setcc = "setle"; break;
            case KTL_OPER_COMP_L:  setcc = "setl";  break;
            case KTL_OPER_COMP_E:  setcc = "sete";  break;
            case KTL_OPER_COMP_NE: setcc = "setne"; break;
            default: assert(false); return;
        }

        print_asm("    %s al\n", setcc);
        print_asm("    movzx rax, al\n");
        return;
    }
    case KTL_OPER_GET_PTR:
        emit_load_address(cont, node->move.unary.next);
        return ;

    case KTL_OPER_UNGET_PTR:
        emit_expr(cont, node->move.unary.next);
        return ;
    }
    return ;
}


static void emit_block(KTL_BackendContext *cont, KTL_AstNode *node) {
    assert(cont);
    if (node == NULL || node->kind != KTL_AST_BLOCK)  return;

    debug_out("BLOCK\n");

    for (int i = 0; i < node->move.n.amount; i++) {
        emit_body(cont, node->move.n.children[i]);
    }
}

static void emit_body(KTL_BackendContext *cont, KTL_AstNode *node) {
    assert(cont);
    if (node == NULL)  return;

    switch (node->kind) {
    case KTL_AST_VARIABLE_DECL:
        debug_out("VARIABLE DECL\n");
        emit_var_decl(cont, node);
        return;

    case KTL_AST_ASSIGN:
        debug_out("ASSIGN\n");
        emit_assign(cont, node);
        return;

    case KTL_AST_COND_BLOCK:
        debug_out("COND BLOCK\n");
        emit_cond_block(cont, node);
        return;

    case KTL_AST_WHILE_BLOCK:
        debug_out("WHILE BLOCK\n");
        emit_while_block(cont, node);
        return;

    case KTL_AST_FOR_BLOCK:
        debug_out("FOR BLOCK\n");
        emit_for_block(cont, node);
        return;

    case KTL_AST_RETURN:
        debug_out("RETURN\n");
        emit_return(cont, node);
        return;

    case KTL_AST_BREAK:
        debug_out("BREAK\n");
        emit_break(cont, node);
        return;

    case KTL_AST_CONTINUE:
        debug_out("CONTINUE\n");
        emit_continue(cont, node);
        return;

    case KTL_AST_FUNCTION_CALL:
        debug_out("FUNC CALL\n");
        emit_func_call(cont, node);
        return;

    default:
        return;
    }
}


static void emit_push(KTL_BackendContext *cont, const char *reg) {
    print_asm("    push %s\n", reg);
    cont->stack_depth += 8;
}

static void emit_pop(KTL_BackendContext *cont, const char *reg) {
    print_asm("    pop  %s\n", reg);
    cont->stack_depth -= 8;
}




KTL_StrID get_global_name(KTL_StrMap *str_map, const char *prefix, KTL_StrID name) {
    return get_name(str_map, prefix, KTL_GLOBAL_PREFIX, name);
}

KTL_StrID get_func_name(KTL_StrMap *str_map, const char *prefix, KTL_StrID name) {
    return get_name(str_map, prefix, KTL_FUNC_PREFIX, name);
}

KTL_StrID get_string_name(KTL_StrMap *str_map, const char *prefix, KTL_StrID name) {
    assert(str_map);
    assert(name);
    if (prefix == NULL) prefix = "";

    int len_pref_1 = strlen(prefix);
    int len_pref_2 = strlen(KTL_STRING_PREFIX);
    int len_ended  = len_pref_1 + len_pref_2 + 19 + 1;

    char buffer[len_ended];  /* can't be initted */
    strcpy(buffer, prefix);
    strcpy(buffer + len_pref_1, KTL_STRING_PREFIX);
    sprintf(buffer + len_pref_1 + len_pref_2, "%p", name);
    buffer[len_ended - 1] = '\0';

    return KTL_StrMapFind(str_map, buffer);
}

KTL_StrID get_name(KTL_StrMap *str_map, const char *prefix_1, const char *prefix_2, KTL_StrID name) {
    assert(str_map);
    assert(name);
    if (prefix_1 == NULL) prefix_1 = "";
    if (prefix_2 == NULL) prefix_2 = "";

    int len_name   = strlen(name);
    int len_pref_1 = strlen(prefix_1);
    int len_pref_2 = strlen(prefix_2);
    int len_ended  = len_pref_1 + len_pref_2 + len_name + 1;

    char buffer[len_ended];  /* can't be initted */
    strcpy(buffer, prefix_1);
    strcpy(buffer + len_pref_1, prefix_2);
    strcpy(buffer + len_pref_1 + len_pref_2, name);
    buffer[len_ended - 1] = '\0';

    return KTL_StrMapFind(str_map, buffer);
}
