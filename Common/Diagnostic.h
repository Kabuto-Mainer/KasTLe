#ifndef DIAGNOSTIC_H
#define DIAGNOSTIC_H

#include "Common.h"
#include "StrMapType.h"
#include "DiagnosticType.h"


KTL_Error KTL_DiagCreate (KTL_Diagnostic *diag, int initial_capacity);
KTL_Error KTL_DiagDestroy(KTL_Diagnostic *diag);

KTL_Error KTL_DiagEmit(KTL_Diagnostic *diag, KTL_SourcePos pos,
                       KTL_DiagError error, KTL_DiagSeverity sev);

KTL_Error KTL_DiagEmit(KTL_Diagnostic *diag, KTL_SourcePos pos,
                       KTL_DiagError error, KTL_DiagSeverity sev,
                       KTL_StrID arg1);

KTL_Error KTL_DiagEmit(KTL_Diagnostic *diag, KTL_SourcePos pos,
                       KTL_DiagError error, KTL_DiagSeverity sev,
                       KTL_StrID arg1, KTL_StrID arg2);

KTL_Error KTL_DiagEmit(KTL_Diagnostic *diag, KTL_SourcePos pos,
                       KTL_DiagError error, KTL_DiagSeverity sev,
                       int64_t arg1);

KTL_Error KTL_DiagEmit(KTL_Diagnostic *diag, KTL_SourcePos pos,
                       KTL_DiagError error, KTL_DiagSeverity sev,
                       int arg1, int arg2);

KTL_Error KTL_DiagEmit(KTL_Diagnostic *diag, KTL_SourcePos pos,
                       KTL_DiagError error, KTL_DiagSeverity sev,
                       KTL_ParseTokenRef expected);

void KTL_DiagFlush(const KTL_Diagnostic *diag, const char *file_name);

#endif /* DIAGNOSTIC_H */
