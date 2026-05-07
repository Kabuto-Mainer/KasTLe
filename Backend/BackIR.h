#ifndef BACK_IR_H
#define BACK_IR_H

#include "BackIRType.h"

KTL_Error KTL_BackIR_Init         (KTL_BackIR_Buffer *buf);
KTL_Error KTL_BackIR_Uninit       (KTL_BackIR_Buffer *buf);
KTL_Error KTL_BackIR_AddInstr     (KTL_BackIR_Buffer *buf,
                                   KTL_AsmInstr       instr);
KTL_Error KTL_BackIR_AddInstr     (KTL_BackIR_Buffer       *buf,
                                   KTL_AsmInstr             instr,
                                   KTL_BackIR_InstrOperand *op);
KTL_Error KTL_BackIR_AddInstr     (KTL_BackIR_Buffer       *buf,
                                   KTL_AsmInstr             instr,
                                   KTL_BackIR_InstrOperand *dst,
                                   KTL_BackIR_InstrOperand *src);
KTL_Error KTL_BackIR_AddLabel     (KTL_BackIR_Buffer *buf,
                                   KTL_StrID          name);
KTL_Error KTL_BackIR_AddAlign     (KTL_BackIR_Buffer *buf,
                                   int                size);
KTL_Error KTL_BackIR_AddComment   (KTL_BackIR_Buffer *buf,
                                   KTL_StrID          comment);
KTL_Error KTL_BackIR_AddZeroData  (KTL_BackIR_Buffer *buf,
                                   int                size);
KTL_Error KTL_BackIR_AddIntData   (KTL_BackIR_Buffer *buf,
                                   int64_t            value,
                                   uint8_t            size);
KTL_Error KTL_BackIR_AddByteData  (KTL_BackIR_Buffer *buf,
                                   KTL_StrID          bytes,
                                   int                len);
KTL_Error KTL_BackIR_AddSymbolData(KTL_BackIR_Buffer    *buf,
                                   KTL_StrID             sym,
                                   KTL_BackIR_SymbolKind kind,
                                   int64_t               addend,
                                   uint8_t               size);

/* This function can't be static, but you mustn't call it
   Only for system */
KTL_Error add_item(KTL_BackIR_Buffer *buf,
                   KTL_BackIR_Item   *item);

#endif /* BACK_IR_H */
