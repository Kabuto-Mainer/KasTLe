#ifndef DIAGNOSTIC_H
#define DIAGNOSTIC_H

#include "StrMapType.h"

enum KTL_DiagError {
    // KTL_DIAG_UNKN
};

struct KTL_SourcePos {
    int line;
    int column;
};

struct KTL_DiagEntry {
    KTL_SourcePos pos;
    KTL_DiagError error;
    union {

    } val;
};


struct KTL_Diagnostic {

};

#endif /* DIAGNOSTIC_H */
