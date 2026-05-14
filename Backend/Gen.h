#ifndef GEN_H
#define GEN_H

#include "GenType.h"

KTL_Error KTL_Backend_GenerateNasm(KTL_BackIR_Buffer *text,
                                   KTL_BackIR_Buffer *data,
                                   KTL_BackIR_Buffer *rodata,
                                   FILE              *stream);

void KTL_GenProcess(KTL_GenContext *cont);
void KTL_GenDumpFlat(KTL_GenFlat *flat);
void KTL_GenByte(KTL_BackIR_Buffer *text,
                 KTL_BackIR_Buffer *data,
                 KTL_BackIR_Buffer *rodata);

void KTL_ElfEmit(KTL_GenContext *cont);
void KTL_GenUninit(KTL_GenContext *cont);


#endif /* GEN_H */
