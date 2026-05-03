#include <stdlib.h>
#include <assert.h>

#include "Common.h"
#include "BackMapType.h"

constexpr int KTL_FUNC_SIZE_INIT      = 3;
constexpr int KTL_VAR_SIZE_INIT       = 3;
constexpr int KTL_ALLOC_SIZE_INIT     = 5;



KTL_Error KTL_BackendTableInit(KTL_BackendTable *table) {
    assert(table);

    table->funcs = (KTL_BackendFuncInfo *)calloc(KTL_FUNC_SIZE_INIT,
                                            sizeof(KTL_BackendFuncInfo));
    if (table->funcs == NULL)   ExitF("NULL Calloc", KTL_MEMORY_ERR);
    table->func_capacity = KTL_FUNC_SIZE_INIT;
    table->func_size     = 0;

    table->vars = (KTL_BackendVarInfo *)calloc(KTL_VAR_SIZE_INIT,
                                            sizeof(KTL_BackendVarInfo));
    if (table->vars == NULL)  {
        free(table->funcs);
        ExitF("NULL Calloc", KTL_MEMORY_ERR);
    }
    table->var_capacity = KTL_VAR_SIZE_INIT;
    table->var_size     = 0;

    return KTL_OK;
}

KTL_BackendFuncInfo * KTL_BackendTableAddFunc(KTL_BackendTable *table, KTL_SymbolEntry *symbol) {
    assert(table);
    assert(symbol);

    if (symbol->kind != KTL_SYMBOL_FUNC)    return NULL;

    if (table->func_size == table->func_capacity) {
        KTL_BackendFuncInfo *buffer = (KTL_BackendFuncInfo *)realloc(table->funcs,
                                            sizeof(KTL_BackendFuncInfo) * table->func_capacity * 2);
        if (buffer == NULL)     ExitF("NULL Calloc", NULL);
        table->funcs         =  buffer;
        table->func_capacity *= 2;
    }
    table->funcs[table->func_size++].origin = symbol;
    return table->funcs + table->func_size - 1;
}

KTL_BackendVarInfo * KTL_BackendTableAddVar(KTL_BackendTable *table, KTL_SymbolEntry *symbol) {
    assert(table);
    assert(symbol);

    if (symbol->kind != KTL_SYMBOL_VAR)                  return NULL;
    if (KTL_BackendTableFindVar(table, symbol) != NULL)  return NULL;

    if (table->var_size == table->var_capacity) {
        KTL_BackendVarInfo *buffer = (KTL_BackendVarInfo *)realloc(table->funcs,
                                            sizeof(KTL_BackendVarInfo) * table->func_capacity * 2);
        if (buffer == NULL)     ExitF("NULL Calloc", NULL);
        table->vars         =  buffer;
        table->var_capacity *= 2;
    }
    table->vars[table->var_size++].origin = symbol;
    return table->vars + table->var_size - 1;
}

KTL_BackendVarInfo * KTL_BackendTableAddStaticVar(KTL_BackendTable *table, KTL_SymbolEntry *symbol) {
    assert(table);
    assert(symbol);

    KTL_BackendVarInfo *var = KTL_BackendTableAddVar(table, symbol);
    if (var == NULL)    return NULL;
    var->storage = KTL_BACKEND_STORAGE_GLOBAL;

    return var;
}

KTL_BackendVarInfo * KTL_BackendTableAddParamVar(KTL_Ba)

KTL_Error KTL_BackendTableUninit(KTL_BackendTable *table) {
    assert(table);

    free(table->funcs);
    free(table->vars);

    return KTL_OK;
}

KTL_BackendFuncInfo * KTL_BackendTableFindFunc(KTL_BackendTable *table, KTL_SymbolEntry *symbol) {
    assert(table);
    assert(symbol);

    if (symbol->kind != KTL_SYMBOL_FUNC)    return NULL;

    for (int i = 0; i < table->func_size; i++) {
        KTL_BackendFuncInfo *func = table->funcs + i;
        if (func->origin == symbol)     return func;
    }
    return NULL;
}

KTL_BackendVarInfo * KTL_BackendTableFindVar(KTL_BackendTable *table, KTL_SymbolEntry *symbol) {
    assert(table);
    assert(symbol);

    if (symbol->kind != KTL_SYMBOL_VAR)    return NULL;

    for (int i = 0; i < table->var_size; i++) {
        KTL_BackendVarInfo *var = table->vars + i;
        if (var->origin == symbol)     return var;
    }
    return NULL;
}








// KTL_Error KTL_BackendVarTableInit(KTL_BackendVarTable *table);
// KTL_Error KTL_BackendVarTableUninit(KTL_BackendVarTable *table);

/* Регистрация переменной с её размещением.
 * Дубликаты (повторное добавление того же origin) — ошибка. */
KTL_Error KTL_BackendAddStackVar(KTL_BackendVarTable *table,
                                 KTL_SymbolEntry *origin,
                                 int offset);

KTL_Error KTL_BackendAddRegVar(KTL_BackendVarTable *table,
                               KTL_SymbolEntry *origin,
                               int reg_id);

KTL_Error KTL_BackendAddStaticVar(KTL_BackendVarTable *table,
                                  KTL_SymbolEntry *origin,
                                  KTL_StrID label);

/* Поиск. NULL если не найдено — вызывающий должен это обработать. */
KTL_BackendVarInfo * KTL_BackendFindVar(KTL_BackendVarTable *table,
                                        KTL_SymbolEntry *origin);

/* Аналогично для функций */
KTL_Error KTL_BackendAddFunc(KTL_BackendVarTable *table,
                             KTL_SymbolEntry *origin,
                             KTL_StrID label,
                             int frame_size);

KTL_BackendFuncInfo * KTL_BackendFindFunc(KTL_BackendVarTable *table,
                                          KTL_SymbolEntry *origin);
