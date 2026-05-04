#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "Backend.h"
#include "StrMap.h"
#include "TypeMap.h"
#include "ASTCommon.h"
#include "BackMap.h"

constexpr static int KTL_BACKEND_VARS_INIT  = 16;
constexpr static int KTL_BACKEND_FUNCS_INIT = 8;
constexpr static int KTL_BACKEND_GROW_MOD   = 2;


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
        if (grow_vars(table) != KTL_OK)     return KTL_MEMORY_ERR;
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

