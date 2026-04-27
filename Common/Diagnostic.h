#ifndef DIAGNOSTIC_H
#define DIAGNOSTIC_H

#include "Common.h"
#include "StrMapType.h"
#include "DiagnosticType.h"


KTL_Error KTL_DiagCreate (KTL_Diagnostic *diag, int initial_capacity);
KTL_Error KTL_DiagDestroy(KTL_Diagnostic *diag);

KTL_Error KTL_DiagEmit(KTL_Diagnostic *diag,    KTL_SourcePos    pos,
                       KTL_DiagError    error,  KTL_DiagSeverity sev);


KTL_Error KTL_DiagEmitToken(KTL_Diagnostic *diag, KTL_SourcePos pos,
                            KTL_DiagError err, KTL_DiagSeverity sev,
                            KTL_StrID expected, KTL_StrID got);

KTL_Error KTL_DiagEmitName (KTL_Diagnostic *diag, KTL_SourcePos pos,
                            KTL_DiagError err, KTL_DiagSeverity sev,
                            KTL_StrID name);


void KTL_DiagFlush(const KTL_Diagnostic *diag, const char *file_name);

#endif /* DIAGNOSTIC_H */
