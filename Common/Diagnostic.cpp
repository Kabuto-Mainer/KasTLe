#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "Diagnostic.h"

// =======================================================================
// USAGE CONSTATS
// =======================================================================

constexpr static int KTL_DIAG_GROW_MOD = 2;

// =======================================================================
// API FUNCTIONS
// =======================================================================

KTL_Error KTL_DiagCreate(KTL_Diagnostic *diag, int initial_capacity) {
    assert(diag);
    if (initial_capacity <= 0)  ExitF("Bad capacity", KTL_BAD_ARG_ERR);

    diag->data = (KTL_DiagEntry *)calloc((size_t) initial_capacity,
                                         sizeof(KTL_DiagEntry));
    if (diag->data == NULL)     ExitF("NULL Calloc", KTL_MEMORY_ERR);

    diag->size        = 0;
    diag->capacity    = initial_capacity;
    diag->error_count = 0;
    diag->fatal_count = 0;

    return KTL_OK;
}

KTL_Error KTL_DiagDestroy(KTL_Diagnostic *diag) {
    assert(diag);

    free(diag->data);
    diag->data        = NULL;
    diag->size        = 0;
    diag->capacity    = 0;
    diag->error_count = 0;
    diag->fatal_count = 0;

    return KTL_OK;
}

static KTL_Error ktl_diag_grow(KTL_Diagnostic *diag) {
    assert(diag);

    int new_cap = diag->capacity * KTL_DIAG_GROW_MOD;
    KTL_DiagEntry *buf = (KTL_DiagEntry *)realloc(diag->data,
                            (size_t) new_cap * sizeof(KTL_DiagEntry));
    if (buf == NULL)    ExitF("NULL Realloc", KTL_MEMORY_ERR);

    diag->data     = buf;
    diag->capacity = new_cap;

    return KTL_OK;
}

static KTL_DiagEntry * ktl_diag_alloc(KTL_Diagnostic *diag) {
    assert(diag);

    if (diag->size == diag->capacity) {
        if (ktl_diag_grow(diag) != KTL_OK)  return NULL;
    }

    return &diag->data[diag->size++];
}

static void ktl_diag_count(KTL_Diagnostic *diag, KTL_DiagSeverity sev) {
    assert(diag);

    if (sev >= KTL_DIAG_SEV_ERROR)  diag->error_count++;
    if (sev == KTL_DIAG_SEV_FATAL)  diag->fatal_count++;
}

KTL_Error KTL_DiagEmit(KTL_Diagnostic *diag, KTL_SourcePos pos,
                       KTL_DiagError err, KTL_DiagSeverity sev) {
    assert(diag);

    KTL_DiagEntry *entry = ktl_diag_alloc(diag);
    if (entry == NULL)  return KTL_MEMORY_ERR;

    entry->pos      = pos;
    entry->error    = err;
    entry->severity = sev;
    entry->val.token.expected = NULL;
    entry->val.token.got      = NULL;

    ktl_diag_count(diag, sev);

    return KTL_OK;
}

KTL_Error KTL_DiagEmitToken(KTL_Diagnostic *diag, KTL_SourcePos pos,
                            KTL_DiagError err, KTL_DiagSeverity sev,
                            KTL_StrID expected, KTL_StrID got) {
    assert(diag);

    KTL_DiagEntry *entry = ktl_diag_alloc(diag);
    if (entry == NULL)  return KTL_MEMORY_ERR;

    entry->pos                 = pos;
    entry->error               = err;
    entry->severity            = sev;
    entry->val.token.expected  = expected;
    entry->val.token.got       = got;

    ktl_diag_count(diag, sev);

    return KTL_OK;
}

KTL_Error KTL_DiagEmitName(KTL_Diagnostic *diag, KTL_SourcePos pos,
                           KTL_DiagError err, KTL_DiagSeverity sev,
                           KTL_StrID name) {
    assert(diag);

    KTL_DiagEntry *entry = ktl_diag_alloc(diag);
    if (entry == NULL)  return KTL_MEMORY_ERR;

    entry->pos             = pos;
    entry->error           = err;
    entry->severity        = sev;
    entry->val.name.name   = name;

    ktl_diag_count(diag, sev);

    return KTL_OK;
}

static const char * ktl_sev_str(KTL_DiagSeverity sev) {
    switch (sev) {
        case KTL_DIAG_SEV_NOTE:    return "note";
        case KTL_DIAG_SEV_WARNING: return "warning";
        case KTL_DIAG_SEV_ERROR:   return "error";
        case KTL_DIAG_SEV_FATAL:   return "fatal";
        default:                   return "?";
    }
}

void KTL_DiagFlush(const KTL_Diagnostic *diag, const char *file_name) {
    assert(diag);
    if (file_name == NULL)  file_name = "<input>";

    for (int i = 0; i < diag->size; i++) {
        const KTL_DiagEntry *e = &diag->data[i];
        fprintf(stderr, "%s:%d:%d: %s: code=%d\n",
                file_name, e->pos.line + 1, e->pos.column + 1,
                ktl_sev_str(e->severity), (int) e->error);
    }
}
