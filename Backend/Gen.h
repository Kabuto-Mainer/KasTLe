#ifndef GEN_H
#define GEN_H

#include "GenType.h"

KTL_Error KTL_GenerateNasm(KTL_BackIR_Buffer *text,
                           KTL_BackIR_Buffer *data,
                           KTL_BackIR_Buffer *rodata,
                           FILE              *stream);

void KTL_ElfEmit          (KTL_GenContext *cont,
                           FILE           *stream);

void KTL_GenUninit        (KTL_GenContext *cont);
void KTL_GenProcess       (KTL_GenContext *cont);
void KTL_GenDumpFlat      (KTL_GenFlat *flat);
void KTL_GenByte          (KTL_GenContext    *cont,
                           KTL_BackIR_Buffer *text,
                           KTL_BackIR_Buffer *data,
                           KTL_BackIR_Buffer *rodata);
void KTL_GenElf           (KTL_BackIR_Buffer *text,
                           KTL_BackIR_Buffer *data,
                           KTL_BackIR_Buffer *rodata,
                           FILE *stream);


#endif /* GEN_H */
