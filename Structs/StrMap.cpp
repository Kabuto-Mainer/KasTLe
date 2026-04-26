#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "StrMapType.h"
#include "TypeMapType.h"

// =======================================================================
// HELPER FUNCTIONS DECLARATION
// =======================================================================

static KTL_StrList *ktl_str_allocate_list(void);
static void ktl_str_destroy_list(KTL_StrList *list);

// =======================================================================
// API FUNCTIONS
// =======================================================================

KTL_Error KTL_StrMapCreate(KTL_StrMap *map, int size) {
    assert(map);
    if (size <= 0)  ExitF("Bad Size of Map", KTL_BAD_ARG_ERR);

    map->data = (KTL_StrList *)calloc((size_t) size, sizeof(KTL_StrList));
    if (map->data == NULL)    ExitF("NULL Calloc", KTL_MEMORY_ERR);
    map->size = size;

    return KTL_OK;
}

KTL_StrID KTL_StrMapFind(KTL_StrMap *map, const char *string) {
    assert(map);
    if (string == NULL)     ExitF("NULL string", NULL);

    KTL_Hash hash_list = map->get_hash_list(string);
    KTL_Hash hash_cell = map->get_hash_cell(string);

    KTL_StrList *list = &(map->data[hash_cell % (KTL_Hash) map->size]);

    if (list->string == NULL) {
        list->string = strdup(string);
        list->hash_list = hash_list;
        list->next = NULL;
        return list->string;
    }

    while (true) {
        if (list->hash_list == hash_list && strcmp(list->string, string) == 0) {
            return list->string;
        }
        if (list->next != NULL) {
            list = list->next;
            continue;
        }
        list->next = ktl_str_allocate_list();
        list = list->next;
        if (list == NULL) {
            return NULL;
        };

        list->hash_list = hash_list;
        list->string = strdup(string);

        if (list->string == NULL)   ExitF("NULL strdup", NULL);
        list->next = NULL;
        return list->string;
    }
}

KTL_Error KTL_StrMapDestroy(KTL_StrMap *map) {
    assert(map);

    for (int i = 0; i < map->size; i++) {
        KTL_StrList *list = &(map->data[i]);
        if (list->next != NULL) {
            ktl_str_destroy_list(list->next);
        }
        free(list->string);
    }
    return KTL_OK;
}



// =======================================================================
// HELPER FUNCTIONS
// =======================================================================

static void ktl_str_destroy_list(KTL_StrList *list) {
    assert(list);

    if (list->next != NULL) {
        ktl_str_destroy_list(list->next);
    }
    free(list->string);
    free(list);

    return ;
}

static KTL_StrList *ktl_str_allocate_list(void) {
    KTL_StrList *list = (KTL_StrList *)calloc(1, sizeof(KTL_StrList));
    if (list == NULL)   ExitF("NULL Calloc", NULL);
    return list;
}
