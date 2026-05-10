#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "BackIR.h"
#include "Common.h"

static constexpr int KTL_START_BUFFER_SIZE = 128;
static constexpr int KTL_BUFFER_GROW_MOD   = 2;

// =====================================================================
// HELPER FUNCTION'S DECLARATION
// =====================================================================

// =====================================================================
// API
// =====================================================================

KTL_Error KTL_BackIR_Init(KTL_BackIR_Buffer *buf) {
    assert(buf);

    buf->data = (KTL_BackIR_Item *)calloc(KTL_START_BUFFER_SIZE,
                                        sizeof(KTL_BackIR_Item));
    if (buf->data == NULL)  ExitF("NULL Calloc", KTL_MEMORY_ERR);

    buf->capacity = KTL_START_BUFFER_SIZE;
    buf->size     = 0;

    return KTL_OK;
}

KTL_Error KTL_BackIR_Uninit(KTL_BackIR_Buffer *buf) {
    assert(buf);

    if (buf->data != NULL)  free(buf->data);
    buf->data     = NULL;
    buf->capacity = 0;
    buf->size     = 0;

    return KTL_OK;
}


KTL_Error KTL_BackIR_AddInstr(KTL_BackIR_Buffer *buf,
                              KTL_AsmInstr       instr) {
    assert(buf);
    assert(get_amount_operand(instr) == 0);

    KTL_BackIR_Item item   = {};
    item.kind              = KTL_BACK_IR_ITEM_INSTR;
    item.instr.cmd         = instr;
    item.instr.amount_args = 0;

    return add_item(buf, &item);
}


KTL_Error KTL_BackIR_AddInstr(KTL_BackIR_Buffer       *buf,
                              KTL_AsmInstr             instr,
                              KTL_BackIR_InstrOperand *op) {
    assert(buf);
    assert(op);
    assert(get_amount_operand(instr) == 1);

    KTL_BackIR_Item item   = {};
    item.kind              = KTL_BACK_IR_ITEM_INSTR;
    item.instr.cmd         = instr;
    item.instr.amount_args = 1;
    item.instr.one         = *op;

    return add_item(buf, &item);
}


KTL_Error KTL_BackIR_AddInstr(KTL_BackIR_Buffer       *buf,
                              KTL_AsmInstr             instr,
                              KTL_BackIR_InstrOperand *dst,
                              KTL_BackIR_InstrOperand *src) {
    assert(buf);
    assert(src);
    assert(dst);
    assert(get_amount_operand(instr) == 2);

    KTL_BackIR_Item item   = {};
    item.kind              = KTL_BACK_IR_ITEM_INSTR;
    item.instr.cmd         = instr;
    item.instr.amount_args = 2;
    item.instr.two.dst     = *dst;
    item.instr.two.src     = *src;

    return add_item(buf, &item);
}

KTL_Error KTL_BackIR_AddLabel(KTL_BackIR_Buffer *buf,
                              KTL_StrID          name) {
    assert(buf);
    assert(StrIDCheck(name));

    KTL_BackIR_Item item = {};
    item.kind            = KTL_BACK_IR_ITEM_LABEL;
    item.label_decl.name = name;

    return add_item(buf, &item);
}

KTL_Error KTL_BackIR_AddAlign(KTL_BackIR_Buffer *buf,
                              int                size) {
    assert(buf);
    assert(size >= 0);

    KTL_BackIR_Item item   = {};
    item.kind              = KTL_BACK_IR_ITEM_DIRECTIVE;
    item.direct.kind       = KTL_BACK_IR_DIR_ALIGN;
    item.direct.align.size = size;

    return add_item(buf, &item);
}

KTL_Error KTL_BackIR_AddComment(KTL_BackIR_Buffer *buf,
                                KTL_StrID          comment) {
    assert(buf);
    assert(StrIDCheck(comment));

    KTL_BackIR_Item item = {};
    item.kind            = KTL_BACK_IR_ITEM_COMMENT;
    item.comment.text    = comment;

    return add_item(buf, &item);
}

KTL_Error KTL_BackIR_AddZeroData(KTL_BackIR_Buffer *buf,
                                 int                size) {
    assert(buf);
    assert(size >= 0);

    KTL_BackIR_Item item = {};
    item.kind            = KTL_BACK_IR_ITEM_DATA;
    item.data.kind       = KTL_BACK_IR_DATA_ZERO;
    item.data.zero.count = size;

    return add_item(buf, &item);
}

KTL_Error KTL_BackIR_AddIntData(KTL_BackIR_Buffer *buf,
                                int64_t            value,
                                uint8_t            size) {
    assert(buf);
    assert(size == 1 || size == 2 ||
           size == 4 || size == 8);

    KTL_BackIR_Item item    = {};
    item.kind               = KTL_BACK_IR_ITEM_DATA;
    item.data.kind          = KTL_BACK_IR_DATA_INT;
    item.data.int_val.value = value;
    item.data.int_val.size  = size;

    return add_item(buf, &item);
}

KTL_Error KTL_BackIR_AddByteData(KTL_BackIR_Buffer *buf,
                                 KTL_StrID          bytes,
                                 int                len) {
    assert(buf);
    assert(StrIDCheck(bytes));
    assert(len >= 0);

    KTL_BackIR_Item item  = {};
    item.kind             = KTL_BACK_IR_ITEM_DATA;
    item.data.kind        = KTL_BACK_IR_DATA_BYTES;
    item.data.bytes.bytes = bytes;
    item.data.bytes.len   = len;

    return add_item(buf, &item);
}

KTL_Error KTL_BackIR_AddByte15(KTL_BackIR_Buffer *buf,
                               const uint8_t     *byte,
                               const uint8_t      len) {
    assert(buf);
    assert(byte);

    KTL_BackIR_Item item  = {};
    item.kind             = KTL_BACK_IR_ITEM_BYTE_15;
    item.byte_15.len      = len;

    memcpy(item.byte_15.byte, byte, len);

    return add_item(buf, &item);
}

KTL_Error KTL_BackIR_AddSymbolData(KTL_BackIR_Buffer *buf,
                                   KTL_StrID          sym,
                                   KTL_BackIR_SymbolKind kind,
                                   int64_t               addend,
                                   uint8_t               size) {
    assert(buf);
    assert(StrIDCheck(sym));
    assert(size == 1 || size == 2 ||
           size == 4 || size == 8);

    KTL_BackIR_Item item      = {};
    item.kind                 = KTL_BACK_IR_ITEM_DATA;
    item.data.kind            = KTL_BACK_IR_DATA_SYMBOL;
    item.data.symbol.size     = size;
    item.data.symbol.sym      = sym;
    item.data.symbol.addend   = addend;
    item.data.symbol.sym_kind = kind;

    return add_item(buf, &item);
}


// =====================================================================
// HELPERS
// =====================================================================

KTL_Error add_item(KTL_BackIR_Buffer *buf,
                   KTL_BackIR_Item   *item) {
    assert(buf);
    assert(item);

    if (buf->size == buf->capacity) {
        KTL_BackIR_Item *buffer = (KTL_BackIR_Item *)realloc(buf->data,
                                    buf->capacity * sizeof(KTL_BackIR_Item) * KTL_BUFFER_GROW_MOD);
        if (buffer == NULL)     ExitF("NULL Realloc", KTL_MEMORY_ERR);

        buf->data      = buffer;
        buf->capacity *= KTL_BUFFER_GROW_MOD;
    }
    buf->data[buf->size++] = *item;
    return KTL_OK;
}

int get_amount_operand(KTL_AsmInstr instr) {
    switch (instr) {

    case KTL_ASM_MOV:
    case KTL_ASM_MOVZX:
    case KTL_ASM_MOVSX:
    case KTL_ASM_ADD:
    case KTL_ASM_SUB:
    case KTL_ASM_IMUL:
    case KTL_ASM_IDIV:
    case KTL_ASM_XOR:
    case KTL_ASM_AND:
    case KTL_ASM_TEST:
    case KTL_ASM_CMP:
    case KTL_ASM_LEA:
        return 2;

    case KTL_ASM_PUSH:
    case KTL_ASM_POP:
    case KTL_ASM_IDIV1:
    case KTL_ASM_SETNE:
    case KTL_ASM_SETGE:
    case KTL_ASM_SETG:
    case KTL_ASM_SETLE:
    case KTL_ASM_SETL:
    case KTL_ASM_SETE:
    case KTL_ASM_NEG:
    case KTL_ASM_JMP:
    case KTL_ASM_JZ:
    case KTL_ASM_JNZ:
    case KTL_ASM_CALL:
    case KTL_ASM_CALL_PLT:
        return 1;

    case KTL_ASM_REP_STOSB:
    case KTL_ASM_REP_MOVSB:
    case KTL_ASM_RET:
    case KTL_ASM_SYSCALL:
    case KTL_ASM_CDQE:
    case KTL_ASM_CQO:
        return 0;

    default:
        assert(0 && "Unknown instraction");
        return -1;
    }
}
