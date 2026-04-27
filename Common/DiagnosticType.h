#ifndef DIAGNOSTIC_TYPE_H
#define DIAGNOSTIC_TYPE_H

#include "Common.h"
#include "StrMapType.h"

enum KTL_DiagError {
    KTL_DIAG_NONE = 0,

    /* Error on Lexer */
    KTL_DIAG_LEX_UNKNOWN_CHAR,
    KTL_DIAG_LEX_UNTERMINATED_STR,
    KTL_DIAG_LEX_BAD_NUMBER,

    /* Parser */
    KTL_DIAG_PARSE_EXPECTED_TOKEN,
    KTL_DIAG_PARSE_EXPECTED_NAME,
    KTL_DIAG_PARSE_EXPECTED_TYPE,
    KTL_DIAG_PARSE_EXPECTED_EXPR,
    KTL_DIAG_PARSE_UNEXPECTED_EOF,
    KTL_DIAG_PARSE_UNEXPECTED_TOKEN,

    /* Semantic */
    KTL_DIAG_SEM_UNDECLARED_NAME,
    KTL_DIAG_SEM_REDECLARATION,
    KTL_DIAG_SEM_TYPE_MISMATCH,
    KTL_DIAG_SEM_NOT_A_FUNCTION,
    KTL_DIAG_SEM_NOT_A_VARIABLE,
    KTL_DIAG_SEM_BAD_ARG_COUNT,
    KTL_DIAG_SEM_ASSIGN_TO_CONST,
    KTL_DIAG_SEM_BAD_FIELD,
    KTL_DIAG_SEM_NOT_INDEXABLE,
    KTL_DIAG_SEM_RETURN_TYPE,
    KTL_DIAG_SEM_BREAK_OUTSIDE_LOOP,
    KTL_DIAG_SEM_CONTINUE_OUTSIDE_LOOP,
    KTL_DIAG_SEM_NO_MAIN,
    KTL_DIAG_SEM_DUPLICATE_MAIN,
};

enum KTL_DiagSeverity {
    KTL_DIAG_SEV_NOTE,
    KTL_DIAG_SEV_WARNING,
    KTL_DIAG_SEV_ERROR,
    KTL_DIAG_SEV_FATAL,
};

struct KTL_SourcePos {
    int line;
    int column;
};

struct KTL_DiagEntry {
    KTL_SourcePos     pos;
    KTL_DiagError     error;
    KTL_DiagSeverity  severity;

    union {
        struct { KTL_StrID expected; KTL_StrID got; } token;
        struct { KTL_StrID name; }                    name;
        struct { KTL_StrID expected; KTL_StrID got; } type;
        struct { int expected; int got; }             count;
    } val;
};

struct KTL_Diagnostic {
    KTL_DiagEntry *data;
    int size;
    int capacity;

    int error_count;
    int fatal_count;
};

#endif /* DIAGNOSTIC_TYPE_H */
