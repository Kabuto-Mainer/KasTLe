#ifndef DIAGNOSTIC_TYPE_H
#define DIAGNOSTIC_TYPE_H

#include "Common.h"
#include "StrMapType.h"
#include "ParseConfig.h"

enum KTL_DiagError {
    KTL_DIAG_NONE = 0,

    /* Error on Lexer */
    KTL_DIAG_LEX_UNKNOWN_CHAR,
    KTL_DIAG_LEX_UNTERMINATED_STR,
    KTL_DIAG_LEX_BAD_NUMBER,
    KTL_DIAG_LEX_BAD_FILE,

    /* Parser */
    KTL_DIAG_PARSE_EXPECTED_TOKEN,
    KTL_DIAG_PARSE_EXPECTED_NAME,
    KTL_DIAG_PARSE_UNKNOWN_STD_FUNC,
    KTL_DIAG_PARSE_EXPECTED_TYPE,
    KTL_DIAG_PARSE_EXPECTED_EXPR,
    KTL_DIAG_PARSE_EXPECTED_STRING,
    KTL_DIAG_PARSE_UNEXPECTED_EOF,
    KTL_DIAG_PARSE_UNEXPECTED_TOKEN,
    KTL_DIAG_PARSE_UNKNOWN_TYPE,
    KTL_DIAG_PARSE_BAD_ARRAY_SIZE,
    KTL_DIAG_PARSE_EXPECTED_NUMBER,
    KTL_DIAG_PARSE_BAD_MODIFIERS,
    KTL_DIAG_PARSE_ONLY_ONE_ADDR,

    /* Semantic */
    KTL_DIAG_SEM_UNDECLARED_NAME,
    KTL_DIAG_SEM_REDECLARATION,
    KTL_DIAG_SEM_TYPE_MISMATCH,
    KTL_DIAG_SEM_NOT_A_FUNCTION,
    KTL_DIAG_SEM_NOT_A_VARIABLE,
    KTL_DIAG_SEM_BAD_ARG_COUNT,
    KTL_DIAG_SEM_ASSIGN_TO_CONST,
    KTL_DIAG_SEM_UNSUPPORTED_FUNCTION_ARG,
    KTL_DIAG_SEM_ASSIGN_CONST_TO_NCONST,
    KTL_DIAG_SEM_ADDR_PARAM,
    KTL_DIAG_SEM_BAD_FIELD,
    KTL_DIAG_SEM_NOT_INDEXABLE,
    KTL_DIAG_SEM_RETURN_TYPE,
    KTL_DIAG_SEM_UNSUPPORTED_TYPE_OPER,
    KTL_DIAG_SEM_BREAK_OUTSIDE_LOOP,
    KTL_DIAG_SEM_CONTINUE_OUTSIDE_LOOP,
    KTL_DIAG_SEM_NO_MAIN,
    KTL_DIAG_SEM_DUPLICATE_MAIN,
    KTL_DIAG_SEM_RETURN_OUTSIDE_FUNC,

    KTL_DIAG_PARSE_NON_CONST_EXPR,
};

enum KTL_DiagSeverity {
    KTL_DIAG_SEV_NOTE,
    KTL_DIAG_SEV_WARNING,
    KTL_DIAG_SEV_ERROR,
    KTL_DIAG_SEV_FATAL,
};

enum KTL_DiagPayload {
    KTL_DIAG_PL_NONE,
    KTL_DIAG_PL_NAME,
    KTL_DIAG_PL_TWO_NAMES,
    KTL_DIAG_PL_NUMBER,
    KTL_DIAG_PL_TWO_INTS,
    KTL_DIAG_PL_TOKEN,
};

struct KTL_DiagEntry {
    KTL_SourcePos     pos;
    KTL_DiagError     error;
    KTL_DiagSeverity  severity;

    union {
        struct { KTL_StrID name; }                       name;
        struct { KTL_StrID expected; KTL_StrID got; }    two_names;
        struct { int64_t value; }                        number;
        struct { int expected; int got; }                two_ints;
        struct { KTL_ParseTokenRef expected; }           token;
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
