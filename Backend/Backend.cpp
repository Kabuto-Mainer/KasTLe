#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "Backend.h"
#include "StrMap.h"
#include "TypeMap.h"

// =======================================================================
// CONSTANTS
// =======================================================================

constexpr static int KTL_BACKEND_VARS_INIT  = 16;
constexpr static int KTL_BACKEND_FUNCS_INIT = 8;
constexpr static int KTL_BACKEND_GROW_MOD   = 2;

constexpr char *KTL_GLOBAL_PREFIX = "__global__";
constexpr char *KTL_FUNC_PREFIX   = "__func__";


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

const char * KTL_BackendRegName(KTL_RegId reg, int size) {
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

const char * KTL_BackendSizePrefix(int size) {
    switch (size) {
        case 1: return "byte ptr";
        case 2: return "word ptr";
        case 4: return "dword ptr";
        case 8: return "qword ptr";
        default:
            assert(false && "unsupported memory operand size");
            return "?";
    }
}

static KTL_Error grow_vars(KTL_BackendTable *table) {
    assert(table);

    int new_cap = table->vars_capacity * KTL_BACKEND_GROW_MOD;
    KTL_BackendVarInfo *buf = (KTL_BackendVarInfo *)realloc(table->vars,
        (size_t) new_cap * sizeof(KTL_BackendVarInfo));

    if (buf == NULL)  ExitF("NULL Realloc", KTL_MEMORY_ERR);

    table->vars          = buf;
    table->vars_capacity = new_cap;

    return KTL_OK;
}

static KTL_Error grow_funcs(KTL_BackendTable *table) {
    assert(table);

    int new_cap = table->funcs_capacity * KTL_BACKEND_GROW_MOD;
    KTL_BackendFuncInfo *buf = (KTL_BackendFuncInfo *)realloc(table->funcs,
        (size_t) new_cap * sizeof(KTL_BackendFuncInfo));

    if (buf == NULL)  ExitF("NULL Realloc", KTL_MEMORY_ERR);

    table->funcs = buf;
    table->funcs_capacity = new_cap;

    return KTL_OK;
}

// =======================================================================
// API
// =======================================================================

KTL_Error KTL_BackendTableInit(KTL_BackendTable *table) {
    assert(table);

    table->vars = (KTL_BackendVarInfo *)calloc(KTL_BACKEND_VARS_INIT,
                                               sizeof(KTL_BackendVarInfo));
    if (table->vars == NULL)  ExitF("NULL Calloc", KTL_MEMORY_ERR);
    table->vars_size = 0;
    table->vars_capacity = KTL_BACKEND_VARS_INIT;

    table->funcs = (KTL_BackendFuncInfo *)calloc(KTL_BACKEND_FUNCS_INIT,
                                                 sizeof(KTL_BackendFuncInfo));
    if (table->funcs == NULL) {
        free(table->vars);
        ExitF("NULL Calloc", KTL_MEMORY_ERR);
    }
    table->funcs_size = 0;
    table->funcs_capacity = KTL_BACKEND_FUNCS_INIT;

    return KTL_OK;
}

KTL_Error KTL_BackendTableUninit(KTL_BackendTable *table) {
    assert(table);

    free(table->vars);
    free(table->funcs);

    table->vars = NULL;
    table->funcs = NULL;

    table->vars_size = 0;
    table->vars_capacity = 0;

    table->funcs_size = 0;
    table->funcs_capacity = 0;

    return KTL_OK;
}

KTL_Error KTL_BackendAddStackVar(KTL_BackendTable *table,
                                 KTL_SymbolEntry  *origin,
                                 int offset) {
    assert(table);
    assert(origin);

    if (KTL_BackendFindVar(table, origin) != NULL) {
        return KTL_LOGICAL_ERR;
    }

    if (table->vars_size == table->vars_capacity) {
        if (grow_vars(table) != KTL_OK)  return KTL_MEMORY_ERR;
    }

    KTL_BackendVarInfo *info = &table->vars[table->vars_size++];
    info->origin             = origin;
    info->storage            = KTL_BACKEND_STORAGE_STACK;
    info->loc.stack.offset   = offset;

    return KTL_OK;
}

KTL_Error KTL_BackendAddStaticVar(KTL_BackendTable *table,
                                  KTL_SymbolEntry  *origin,
                                  KTL_StrID         label) {
    assert(table);
    assert(origin);
    assert(StrIDCheck(label));

    if (KTL_BackendFindVar(table, origin) != NULL)  return KTL_LOGICAL_ERR;

    if (table->vars_size == table->vars_capacity) {
        if (grow_vars(table) != KTL_OK)  return KTL_MEMORY_ERR;
    }

    KTL_BackendVarInfo *info = &table->vars[table->vars_size++];
    info->origin             = origin;
    info->storage            = KTL_BACKEND_STORAGE_STATIC;
    info->loc.stat.label     = label;

    return KTL_OK;
}

KTL_Error KTL_BackendAddFunc(KTL_BackendTable *table,
                             KTL_SymbolEntry  *origin,
                             KTL_StrID         label) {
    assert(table);
    assert(origin);
    assert(StrIDCheck(label));

    if (KTL_BackendFindFunc(table, origin) != NULL)  return KTL_LOGICAL_ERR;

    if (table->funcs_size == table->funcs_capacity) {
        if (grow_funcs(table) != KTL_OK)  return KTL_MEMORY_ERR;
    }

    KTL_BackendFuncInfo *info = &table->funcs[table->funcs_size++];
    info->origin              = origin;
    info->label               = label;
    info->frame_size          = 0;
    info->label_counter       = 0;

    return KTL_OK;
}

KTL_BackendVarInfo * KTL_BackendFindVar(KTL_BackendTable *table,
                                        KTL_SymbolEntry  *origin) {
    assert(table);
    if (origin == NULL)  return NULL;

    for (int i = 0; i < table->vars_size; i++) {
        if (table->vars[i].origin == origin)  return &table->vars[i];
    }
    return NULL;
}

KTL_BackendFuncInfo * KTL_BackendFindFunc(KTL_BackendTable *table,
                                          KTL_SymbolEntry  *origin) {
    assert(table);
    if (origin == NULL)  return NULL;

    for (int i = 0; i < table->funcs_size; i++) {
        if (table->funcs[i].origin == origin)  return &table->funcs[i];
    }
    return NULL;
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
    cont->output       = output;

    cont->current_func        = NULL;
    cont->loop_label_break    = -1;
    cont->loop_label_continue = -1;
    cont->string_counter      = 0;

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

    /* TODO: реализуется следующим сообщением.
     * Этапы:
     *   1. layout_globals(bctx, root) — глобальные переменные и метки функций
     *   2. layout_all_functions(bctx, root) — стековые слоты для каждой функции
     *   3. emit_header(bctx) — секции и .globl
     *   4. emit_globals(bctx, root) — .data / .bss
     *   5. emit_all_functions(bctx, root) — код функций
     */

    return KTL_OK;
}


KTL_Error layout_global(KTL_BackendContext *cont, KTL_AstNode *root) {
    assert(cont);
    assert(root);

    bool is_correct = true;
    for (int i = 0; i < root->move.n.amount; i++) {
        KTL_AstNode *node = root->move.n.children[i];
        if (node->kind == KTL_AST_VARIABLE_DECL) {
            KTL_StrID asm_name = get_global_name(cont->str_map,
                                    cont->symbol_prefix, node->data.var_decl.entry->str_id);

            is_correct &= (KTL_BackendAddStaticVar(&cont->table, node->data.var_decl.entry, asm_name) == KTL_OK);
        }
        else if (node->kind == KTL_AST_FUNCTION_DECL) {
            KTL_StrID asm_name = get_func_name(cont->str_map,
                                    cont->symbol_prefix, node->data.func_decl.func->str_id);
            is_correct &= (KTL_BackendAddFunc(&cont->table, node->data.func_decl.func, asm_name) == KTL_OK);
        }
    }
    return is_correct ? KTL_OK : KTL_MEMORY_ERR;
}

KTL_Error layout_all_functions(KTL_BackendContext *cont, KTL_AstNode *root) {
    assert(cont);
    assert(root);

    bool is_correct = true;
    for (int i = 0; i < root->move.n.amount; i++) {
        KTL_AstNode *node = root->move.n.children[i];
            if (node->kind == KTL_AST_FUNCTION_DECL) {
            is_correct &= layout_func(cont, node);
        }
    }
    return is_correct ? KTL_OK : KTL_MEMORY_ERR;
}


bool layout_func(KTL_BackendContext *cont, KTL_AstNode *node) {
    assert(cont);
    assert(node);

    int counter_params = 0;
    int offset_frame   = -1 * KTL_SYSTEM_PTR_SIZE; /* for 7+ (PARAM_REGS_COUNT + 1) params */
    int offset_param   = KTL_SYSTEM_PTR_SIZE;      /* for 1-6 (PARAM_REGS_COUNT) params + local vars */

    KTL_SymbolMap *params = node->data.func_decl.map;
    for (int i = 0; i < params->size; i++) {
        KTL_SymbolEntry *param = params->data[i];

        if (counter_params >= KTL_PARAM_REGS_COUNT) {
            KTL_BackendAddStackVar(&cont->table, param, offset_param);
            offset_param += KTL_SYSTEM_PTR_SIZE;
        }
        else {
            KTL_BackendAddStackVar(&cont->table, param, offset_frame);
            offset_param -= KTL_SYSTEM_PTR_SIZE;
        }
    }

    cont->frame_offset = offset_frame;

}

bool layout_body(KTL_BackendContext *cont, KTL_AstNode *node) {
    assert(cont);
    assert(node);

    if (node->kind == KTL_AST_BLOCK ||
        node->kind == KTL_AST_FOR_BLOCK) {
        KTL_SymbolMap *vars = node->data.for_block.map;
        for (int i = 0; i < vars->size; i++) {

            KTL_SymbolEntry *var  = vars->data[i];
            KTL_TypeEntry   *type = KTL_TypeGetEntry(cont->type_map, var->var.type);
            if (type->kind == KTL_TYPE_BASE ||
                type->kind == KTL_TYPE_PTR) {
                KTL_BackendAddStackVar(&cont->table, var, cont->frame_offset);
                cont->frame_offset -= KTL_SYSTEM_PTR_SIZE;
            }

        }
    }
}


KTL_StrID get_global_name(KTL_StrMap *str_map, const char *prefix, KTL_StrID name) {
    assert(str_map);
    assert(name);
    if (prefix == NULL) prefix = "";

    int len_name   = strlen(name);
    int len_global = strlen(KTL_GLOBAL_PREFIX);
    int len_prefix = strlen(prefix);
    int len_ended  = len_prefix + len_global + len_name + 1;

    char buffer[len_ended];  /* can't be initted */
    strcpy(buffer, prefix);
    strcpy(buffer + len_prefix, KTL_GLOBAL_PREFIX);
    strcpy(buffer + len_prefix + len_global, name);
    buffer[len_ended - 1] = '\0';

    return KTL_StrMapFind(str_map, buffer);
}

KTL_StrID get_func_name(KTL_StrMap *str_map, const char *prefix, KTL_StrID name) {
    assert(str_map);
    assert(name);
    if (prefix == NULL) prefix = "";

    int len_name   = strlen(name);
    int len_func   = strlen(KTL_FUNC_PREFIX);
    int len_prefix = strlen(prefix);
    int len_ended  = len_prefix + len_func + len_name + 1;

    char buffer[len_ended];  /* can't be initted */
    strcpy(buffer, prefix);
    strcpy(buffer + len_prefix, KTL_FUNC_PREFIX);
    strcpy(buffer + len_prefix + len_func, name);
    buffer[len_ended - 1] = '\0';

    return KTL_StrMapFind(str_map, buffer);
}
