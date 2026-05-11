#ifndef GEN_H
#define GEN_H

#include "GenType.h"

KTL_Error KTL_Backend_GenerateNasm(KTL_BackIR_Buffer *text,
                                   KTL_BackIR_Buffer *data,
                                   KTL_BackIR_Buffer *rodata,
                                   FILE              *stream);

#endif /* GEN_H */
