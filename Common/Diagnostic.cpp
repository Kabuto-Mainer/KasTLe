#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "Diagnostic.h"

constexpr static int KTL_DIAG_GROW_MOD = 2;

// =======================================================================
// PAYLOAD MAPPING
// =======================================================================

static KTL_DiagPayload ktl_diag_payload(KTL_DiagError error) {
    switch (error) {
        case KTL_DIAG_PARSE_UNKNOWN_TYPE:
        case KTL_DIAG_SEM_UNDECLARED_NAME:
        case KTL_DIAG_SEM_REDECLARATION:
        case KTL_DIAG_SEM_NOT_A_FUNCTION:
        case KTL_DIAG_SEM_NOT_A_VARIABLE:
        case KTL_DIAG_SEM_BAD_FIELD:
            return KTL_DIAG_PL_NAME;

        case KTL_DIAG_SEM_TYPE_MISMATCH:
        case KTL_DIAG_SEM_RETURN_TYPE:
            return KTL_DIAG_PL_TWO_NAMES;

        case KTL_DIAG_PARSE_BAD_ARRAY_SIZE:
        case KTL_DIAG_PARSE_BAD_MODIFIERS:
            return KTL_DIAG_PL_NUMBER;

        case KTL_DIAG_SEM_BAD_ARG_COUNT:
            return KTL_DIAG_PL_TWO_INTS;

        case KTL_DIAG_PARSE_EXPECTED_TOKEN:
            return KTL_DIAG_PL_TOKEN;

        default:
            return KTL_DIAG_PL_NONE;
    }
}

// =======================================================================
// HELPERS
// =======================================================================

static KTL_Error ktl_diag_grow(KTL_Diagnostic *diag) {
    int new_cap = diag->capacity * KTL_DIAG_GROW_MOD;
    KTL_DiagEntry *buf = (KTL_DiagEntry *)realloc(diag->data,
                            (size_t) new_cap * sizeof(KTL_DiagEntry));
    if (buf == NULL)  ExitF("NULL Realloc", KTL_MEMORY_ERR);

    diag->data     = buf;
    diag->capacity = new_cap;

    return KTL_OK;
}

static KTL_DiagEntry * ktl_diag_alloc(KTL_Diagnostic *diag) {
    if (diag->size == diag->capacity) {
        if (ktl_diag_grow(diag) != KTL_OK)  return NULL;
    }

    return &diag->data[diag->size++];
}

static void ktl_diag_count(KTL_Diagnostic *diag, KTL_DiagSeverity sev) {
    if (sev >= KTL_DIAG_SEV_ERROR)  diag->error_count++;
    if (sev == KTL_DIAG_SEV_FATAL)  diag->fatal_count++;
}

// =======================================================================
// API
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

KTL_Error KTL_DiagEmit(KTL_Diagnostic *diag, KTL_SourcePos pos,
                       KTL_DiagError error, KTL_DiagSeverity sev) {
    assert(diag);
    assert(ktl_diag_payload(error) == KTL_DIAG_PL_NONE);

    KTL_DiagEntry *entry = ktl_diag_alloc(diag);
    if (entry == NULL)  return KTL_MEMORY_ERR;

    entry->pos      = pos;
    entry->error    = error;
    entry->severity = sev;

    ktl_diag_count(diag, sev);

    return KTL_OK;
}

KTL_Error KTL_DiagEmit(KTL_Diagnostic *diag, KTL_SourcePos pos,
                       KTL_DiagError error, KTL_DiagSeverity sev,
                       KTL_StrID arg1) {
    assert(diag);
    assert(ktl_diag_payload(error) == KTL_DIAG_PL_NAME);

    KTL_DiagEntry *entry = ktl_diag_alloc(diag);
    if (entry == NULL)  return KTL_MEMORY_ERR;

    entry->pos           = pos;
    entry->error         = error;
    entry->severity      = sev;
    entry->val.name.name = arg1;

    ktl_diag_count(diag, sev);

    return KTL_OK;
}

KTL_Error KTL_DiagEmit(KTL_Diagnostic *diag, KTL_SourcePos pos,
                       KTL_DiagError error, KTL_DiagSeverity sev,
                       KTL_StrID arg1, KTL_StrID arg2) {
    assert(diag);
    assert(ktl_diag_payload(error) == KTL_DIAG_PL_TWO_NAMES);

    KTL_DiagEntry *entry = ktl_diag_alloc(diag);
    if (entry == NULL)  return KTL_MEMORY_ERR;

    entry->pos                    = pos;
    entry->error                  = error;
    entry->severity               = sev;
    entry->val.two_names.expected = arg1;
    entry->val.two_names.got      = arg2;

    ktl_diag_count(diag, sev);

    return KTL_OK;
}

KTL_Error KTL_DiagEmit(KTL_Diagnostic *diag, KTL_SourcePos pos,
                       KTL_DiagError error, KTL_DiagSeverity sev,
                       int64_t arg1) {
    assert(diag);
    assert(ktl_diag_payload(error) == KTL_DIAG_PL_NUMBER);

    KTL_DiagEntry *entry = ktl_diag_alloc(diag);
    if (entry == NULL)  return KTL_MEMORY_ERR;

    entry->pos             = pos;
    entry->error           = error;
    entry->severity        = sev;
    entry->val.number.value = arg1;

    ktl_diag_count(diag, sev);

    return KTL_OK;
}

KTL_Error KTL_DiagEmit(KTL_Diagnostic *diag, KTL_SourcePos pos,
                       KTL_DiagError error, KTL_DiagSeverity sev,
                       int arg1, int arg2) {
    assert(diag);
    assert(ktl_diag_payload(error) == KTL_DIAG_PL_TWO_INTS);

    KTL_DiagEntry *entry = ktl_diag_alloc(diag);
    if (entry == NULL)  return KTL_MEMORY_ERR;

    entry->pos                   = pos;
    entry->error                 = error;
    entry->severity              = sev;
    entry->val.two_ints.expected = arg1;
    entry->val.two_ints.got      = arg2;

    ktl_diag_count(diag, sev);

    return KTL_OK;
}

KTL_Error KTL_DiagEmit(KTL_Diagnostic *diag, KTL_SourcePos pos,
                       KTL_DiagError error, KTL_DiagSeverity sev,
                       KTL_ParseTokenRef expected) {
    assert(diag);
    assert(ktl_diag_payload(error) == KTL_DIAG_PL_TOKEN);

    KTL_DiagEntry *entry = ktl_diag_alloc(diag);
    if (entry == NULL)  return KTL_MEMORY_ERR;

    entry->pos                = pos;
    entry->error              = error;
    entry->severity           = sev;
    entry->val.token.expected = expected;

    ktl_diag_count(diag, sev);

    return KTL_OK;
}


// =======================================================================
// FLUSH
// =======================================================================

static const char * ktl_sev_str(KTL_DiagSeverity sev) {
    switch (sev) {
        case KTL_DIAG_SEV_NOTE:    return "note";
        case KTL_DIAG_SEV_WARNING: return "warning";
        case KTL_DIAG_SEV_ERROR:   return "error";
        case KTL_DIAG_SEV_FATAL:   return "fatal";
        default:                   return "?";
    }
}

static const char * ktl_err_str(KTL_DiagError err) {
    switch (err) {
        case KTL_DIAG_NONE:                       return "none";

        case KTL_DIAG_LEX_UNKNOWN_CHAR:           return "unknown character";
        case KTL_DIAG_LEX_UNTERMINATED_STR:       return "unterminated string literal";
        case KTL_DIAG_LEX_BAD_NUMBER:             return "bad number literal";

        case KTL_DIAG_PARSE_EXPECTED_TOKEN:       return "expected token";
        case KTL_DIAG_PARSE_EXPECTED_NAME:        return "expected name";
        case KTL_DIAG_PARSE_EXPECTED_TYPE:        return "expected type";
        case KTL_DIAG_PARSE_EXPECTED_EXPR:        return "expected expression";
        case KTL_DIAG_PARSE_EXPECTED_NUMBER:      return "expected number";
        case KTL_DIAG_PARSE_UNEXPECTED_EOF:       return "unexpected end of file";
        case KTL_DIAG_PARSE_UNEXPECTED_TOKEN:     return "unexpected token";
        case KTL_DIAG_PARSE_UNKNOWN_TYPE:         return "unknown type";
        case KTL_DIAG_PARSE_BAD_ARRAY_SIZE:       return "bad array size";
        case KTL_DIAG_PARSE_BAD_MODIFIERS:        return "conflicting modifiers";
        case KTL_DIAG_PARSE_NON_CONST_EXPR:       return "expression is not constant";

        case KTL_DIAG_SEM_UNDECLARED_NAME:        return "undeclared name";
        case KTL_DIAG_SEM_REDECLARATION:          return "redeclaration";
        case KTL_DIAG_SEM_TYPE_MISMATCH:          return "type mismatch";
        case KTL_DIAG_SEM_NOT_A_FUNCTION:         return "not a function";
        case KTL_DIAG_SEM_NOT_A_VARIABLE:         return "not a variable";
        case KTL_DIAG_SEM_BAD_ARG_COUNT:          return "wrong number of arguments";
        case KTL_DIAG_SEM_ASSIGN_TO_CONST:        return "assignment to const";
        case KTL_DIAG_SEM_BAD_FIELD:              return "bad field";
        case KTL_DIAG_SEM_NOT_INDEXABLE:          return "not indexable";
        case KTL_DIAG_SEM_RETURN_TYPE:            return "return type mismatch";
        case KTL_DIAG_SEM_BREAK_OUTSIDE_LOOP:     return "break outside loop";
        case KTL_DIAG_SEM_CONTINUE_OUTSIDE_LOOP:  return "continue outside loop";
        case KTL_DIAG_SEM_NO_MAIN:                return "no main";
        case KTL_DIAG_SEM_RETURN_OUTSIDE_FUNC:    return "return outside function";
        case KTL_DIAG_SEM_DUPLICATE_MAIN:         return "duplicate main";

        default:                                  return "?";
    }
}

void KTL_DiagFlush(const KTL_Diagnostic *diag, const char *file_name) {
    assert(diag);
    if (file_name == NULL)  file_name = "<input>";

    for (int i = 0; i < diag->size; i++) {
        const KTL_DiagEntry *e = &diag->data[i];

        fprintf(stderr, "%s:%d:%d: %s: %s",
                file_name,
                e->pos.line + 1,
                e->pos.column + 1,
                ktl_sev_str(e->severity),
                ktl_err_str(e->error));

        switch (ktl_diag_payload(e->error)) {
            case KTL_DIAG_PL_NAME:
                fprintf(stderr, " '%s'", e->val.name.name);
                break;
            case KTL_DIAG_PL_TWO_NAMES:
                fprintf(stderr, " (expected '%s', got '%s')",
                        e->val.two_names.expected,
                        e->val.two_names.got);
                break;
            case KTL_DIAG_PL_NUMBER:
                fprintf(stderr, " (%lld)", (long long) e->val.number.value);
                break;
            case KTL_DIAG_PL_TWO_INTS:
                fprintf(stderr, " (expected %d, got %d)",
                        e->val.two_ints.expected,
                        e->val.two_ints.got);
                break;
            case KTL_DIAG_PL_NONE:
            default:
                break;
        }
        fputc('\n', stderr);
    }
}
