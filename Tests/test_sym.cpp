#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "SymMap.h"
#include "StrMap.h"
#include "TypeMap.h"
#include "Common.h"


#define CHECK(cond, msg) do { \
    if (!(cond)) { \
        printf("FAIL: %s (line %d)\n", msg, __LINE__); \
        return 1; \
    } else { \
        printf("OK:   %s\n", msg); \
    } \
} while (0)

static KTL_TypeID g_type_int32 = 0;
static KTL_TypeID g_type_char = 0;

static void setup_type_map(KTL_TypeMap *type_map, KTL_StrMap *str_map) {
    assert(type_map);
    assert(str_map);

    KTL_TypeMapCreate(type_map, 8);
    g_type_int32 = KTL_TypeMapAddBase(type_map,
                       KTL_StrMapFind(str_map, "int32"), 4, 4);
    g_type_char  = KTL_TypeMapAddBase(type_map,
                       KTL_StrMapFind(str_map, "char"),  1, 1);
}

// =====================================================================
// ТЕСТЫ
// =====================================================================


static int test_insert_and_find_local(KTL_StrMap *sm) {
    KTL_SymbolMap *map = KTL_SymbolMapInit(NULL);
    CHECK(map != NULL, "create root map");

    KTL_StrID name_a = KTL_StrMapFind(sm, "a");
    KTL_StrID name_b = KTL_StrMapFind(sm, "b");

    KTL_SymbolEntry *e_a = KTL_SymbolInsertVar(map, name_a, g_type_int32,
                                               KTL_VAR_NONE);
    CHECK(e_a != NULL,         "insert var 'a'");
    CHECK(e_a->str_id == name_a, "entry name matches");
    CHECK(e_a->kind == KTL_SYMBOL_VAR, "entry kind == VAR");

    KTL_SymbolEntry *e_b = KTL_SymbolInsertVar(map, name_b, g_type_char,
                                               KTL_VAR_CONST);
    CHECK(e_b != NULL, "insert var 'b' with const");
    CHECK((e_b->var.mod & KTL_VAR_CONST) != 0, "const mod set");

    CHECK(KTL_SymbolFindLocal(map, name_a, KTL_SYMBOL_VAR) == e_a,
          "find local 'a'");
    CHECK(KTL_SymbolFindLocal(map, name_b, KTL_SYMBOL_VAR) == e_b,
          "find local 'b'");

    KTL_StrID name_c = KTL_StrMapFind(sm, "c");
    CHECK(KTL_SymbolFindLocal(map, name_c, KTL_SYMBOL_VAR) == NULL,
          "find absent 'c' returns NULL");

    KTL_SymbolMapUninit(map);
    return 0;
}

static int test_duplicate_insert(KTL_StrMap *sm) {
    KTL_SymbolMap *map = KTL_SymbolMapInit(NULL);

    KTL_StrID name = KTL_StrMapFind(sm, "dup");

    KTL_SymbolEntry *first = KTL_SymbolInsertVar(map, name, g_type_int32,
                                                 KTL_VAR_NONE);
    CHECK(first != NULL, "first insert ok");

    KTL_SymbolEntry *second = KTL_SymbolInsertVar(map, name, g_type_int32,
                                                  KTL_VAR_NONE);
    CHECK(second == NULL, "second insert (duplicate) returns NULL");

    KTL_SymbolMapUninit(map);
    return 0;
}

static int test_var_and_func_same_name(KTL_StrMap *sm) {
    KTL_SymbolMap *map = KTL_SymbolMapInit(NULL);

    KTL_StrID name = KTL_StrMapFind(sm, "foo");

    KTL_SymbolEntry *e_var = KTL_SymbolInsertVar(map, name, g_type_int32,
                                                 KTL_VAR_NONE);
    CHECK(e_var != NULL, "insert var 'foo'");

    KTL_SymbolEntry *e_func = KTL_SymbolInsertFunc(map, name, g_type_int32);
    CHECK(e_func != NULL,
          "insert func 'foo' (separate namespace)");

    CHECK(KTL_SymbolFindLocal(map, name, KTL_SYMBOL_VAR)  == e_var,
          "find 'foo' as VAR returns var entry");
    CHECK(KTL_SymbolFindLocal(map, name, KTL_SYMBOL_FUNC) == e_func,
          "find 'foo' as FUNC returns func entry");

    KTL_SymbolMapUninit(map);
    return 0;
}

static int test_hierarchical_find(KTL_StrMap *sm) {
    KTL_SymbolMap *root  = KTL_SymbolMapInit(NULL);
    KTL_SymbolMap *child = KTL_SymbolMapInit(root);
    KTL_SymbolMap *grand = KTL_SymbolMapInit(child);

    KTL_StrID name_g = KTL_StrMapFind(sm, "global_var");
    KTL_StrID name_l = KTL_StrMapFind(sm, "local_var");

    KTL_SymbolEntry *e_g = KTL_SymbolInsertVar(root, name_g,
                                               g_type_int32, KTL_VAR_NONE);
    KTL_SymbolEntry *e_l = KTL_SymbolInsertVar(child, name_l,
                                               g_type_int32, KTL_VAR_NONE);

    CHECK(KTL_SymbolFind(grand, name_g, KTL_SYMBOL_VAR) == e_g,
          "find global from grandchild");
    CHECK(KTL_SymbolFind(grand, name_l, KTL_SYMBOL_VAR) == e_l,
          "find local-of-child from grandchild");

    CHECK(KTL_SymbolFindLocal(grand, name_g, KTL_SYMBOL_VAR) == NULL,
          "FindLocal in grand returns NULL for global");

    KTL_SymbolMapUninit(grand);
    KTL_SymbolMapUninit(child);
    KTL_SymbolMapUninit(root);
    return 0;
}

static int test_shadowing(KTL_StrMap *sm) {
    KTL_SymbolMap *root  = KTL_SymbolMapInit(NULL);
    KTL_SymbolMap *child = KTL_SymbolMapInit(root);

    KTL_StrID name = KTL_StrMapFind(sm, "x");

    KTL_SymbolEntry *outer = KTL_SymbolInsertVar(root,  name,
                                                 g_type_int32, KTL_VAR_NONE);
    KTL_SymbolEntry *inner = KTL_SymbolInsertVar(child, name,
                                                 g_type_char,  KTL_VAR_NONE);

    KTL_SymbolEntry *found = KTL_SymbolFind(child, name, KTL_SYMBOL_VAR);
    CHECK(found == inner, "shadowing: inner wins");
    CHECK(found != outer, "shadowing: outer is hidden");

    CHECK(KTL_SymbolFind(root, name, KTL_SYMBOL_VAR) == outer,
          "from root see outer");

    KTL_SymbolMapUninit(child);
    KTL_SymbolMapUninit(root);
    return 0;
}

static int test_capacity_growth(KTL_StrMap *sm) {
    KTL_SymbolMap *map = KTL_SymbolMapInit(NULL);

    char buf[16];
    KTL_SymbolEntry *entries[20];

    for (int i = 0; i < 20; i++) {
        snprintf(buf, sizeof(buf), "var%d", i);
        KTL_StrID name = KTL_StrMapFind(sm, buf);
        entries[i] = KTL_SymbolInsertVar(map, name, g_type_int32,
                                         KTL_VAR_NONE);
        if (entries[i] == NULL) {
            printf("FAIL: insert at i=%d failed\n", i);
            return 1;
        }
    }

    for (int i = 0; i < 20; i++) {
        snprintf(buf, sizeof(buf), "var%d", i);
        KTL_StrID name = KTL_StrMapFind(sm, buf);
        if (KTL_SymbolFindLocal(map, name, KTL_SYMBOL_VAR) != entries[i]) {
            printf("FAIL: lookup at i=%d returned wrong entry\n", i);
            return 1;
        }
    }
    printf("OK:   all 20 entries survive capacity growth\n");

    KTL_SymbolMapUninit(map);
    return 0;
}

static int test_func_params(KTL_StrMap *sm) {
    KTL_SymbolMap *root = KTL_SymbolMapInit(NULL);
    KTL_SymbolMap *func_scope = KTL_SymbolMapInit(root);

    KTL_StrID f_name = KTL_StrMapFind(sm, "add");
    KTL_StrID a_name = KTL_StrMapFind(sm, "a");
    KTL_StrID b_name = KTL_StrMapFind(sm, "b");

    KTL_SymbolEntry *func = KTL_SymbolInsertFunc(root, f_name, g_type_int32);
    CHECK(func != NULL, "insert func 'add'");

    KTL_SymbolEntry *p_a = KTL_SymbolInsertVar(func_scope, a_name,
                                               g_type_int32, KTL_VAR_NONE);
    KTL_SymbolEntry *p_b = KTL_SymbolInsertVar(func_scope, b_name,
                                               g_type_int32, KTL_VAR_NONE);

    KTL_SymbolEntry *params[2] = { p_a, p_b };
    KTL_Error err = KTL_SymbolFuncSetParams(func, params, 2);
    CHECK(err == KTL_OK,           "set 2 params");
    CHECK(func->func.amount == 2,  "func.amount == 2");
    CHECK(func->func.params[0] == p_a, "param[0] == a");
    CHECK(func->func.params[1] == p_b, "param[1] == b");

    KTL_SymbolMapUninit(func_scope);
    KTL_SymbolMapUninit(root);
    return 0;
}

/* =====================================================================
 * MAIN
 * ===================================================================== */
int main(void) {
    KTL_StrMap str_map = {};
    KTL_StrMapCreate(&str_map, 64);

    KTL_TypeMap type_map = {};
    setup_type_map(&type_map, &str_map);

    int failures = 0;
    failures += test_insert_and_find_local  (&str_map);
    failures += test_duplicate_insert       (&str_map);
    failures += test_var_and_func_same_name (&str_map);
    failures += test_hierarchical_find      (&str_map);
    failures += test_shadowing              (&str_map);
    failures += test_capacity_growth        (&str_map);
    failures += test_func_params            (&str_map);

    KTL_TypeMapDestroy(&type_map);
    KTL_StrMapDestroy(&str_map);

    if (failures == 0) {
        printf("\n=== ALL TESTS PASSED ===\n");
        return 0;
    } else {
        printf("\n=== %d TEST(S) FAILED ===\n", failures);
        return 1;
    }
}
