#ifndef GEN_BYTE_H
#define GEN_BYTE_H

#include "GenType.h"

void KTL_GenProcess(KTL_GenContext *cont);
void KTL_GenDumpFlat(KTL_GenFlat *flat);
void KTL_GenByte(KTL_GenContext    *cont,
                 KTL_BackIR_Buffer *text,
                 KTL_BackIR_Buffer *data,
                 KTL_BackIR_Buffer *rodata);



#endif /* GEN_BYTE_H */
