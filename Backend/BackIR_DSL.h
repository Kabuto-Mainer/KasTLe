#ifndef BACK_IR_DLL_H
#define BACK_IR_DLL_H

#include "BackendType.h"
#include "BackIR.h"

// =======================================================================
// MACROS FOR SIMPLE USE
// =======================================================================

/* Operands */
#define _REG_64(_reg_)               op_reg(_reg_, 8)
#define _REG(_reg_,_size_)           op_reg(_reg_,_size_)
#define _IMM_64(_imm_)               op_imm(_imm_, 8)
#define _MEM_IDX(_base_,_off_,_size_)op_mem(_base_, KTL_REG_INVALID, 0, _off_, _size_)
#define _MEM_RIP_VAR(_name_, _size_) op_mem_rip(_name_, KTL_BACK_IR_SYM_LOCAL_VAR, _size_)
#define _LBL(_name_)                 op_label(_name_)
#define _NR(_name_)                  KTL_REG_##_name_

/* Instraction */
#define _MOV(_dst_,_src_)            ir_txt(cont, KTL_ASM_MOV, _dst_, _src_)
#define _MOVSX(_dst_,_src_)          ir_txt(cont, KTL_ASM_MOVSX, _dst_, _src_)
#define _MOVZX(_dst_,_src_)          ir_txt(cont, KTL_ASM_MOVZX, _dst_, _src_)
#define _LEA(_dst_,_src_)            ir_txt(cont, KTL_ASM_LEA, _dst_,_src_)

#define _PUSH(_src_)                 ir_txt(cont, KTL_ASM_PUSH,_src_)
#define _POP(_src_)                  ir_txt(cont, KTL_ASM_POP, _src_)

#define _ADD(_dst_,_src_)            ir_txt(cont, KTL_ASM_ADD, _dst_, _src_)
#define _SUB(_dst_,_src_)            ir_txt(cont, KTL_ASM_SUB, _dst_, _src_)
#define _IMUL(_dst_,_src_)           ir_txt(cont, KTL_ASM_IMUL, _dst_, _src_)
#define _IDIV1(_src_)                ir_txt(cont, KTL_ASM_IDIV1, _src_)  /* 1-op */

#define _CMP(_dst_,_src_)            ir_txt(cont, KTL_ASM_CMP, _dst_, _src_)
#define _TEST(_dst_,_src_)           ir_txt(cont, KTL_ASM_TEST, _dst_, _src_)
#define _AND(_dst_,_src_)            ir_txt(cont, KTL_ASM_AND, _dst_, _src_)
#define _XOR(_dst_,_src_)            ir_txt(cont, KTL_ASM_XOR, _dst_, _src_)
#define _NEG(_src_)                  ir_txt(cont, KTL_ASM_NEG, _src_)

#define _JMP(_label_)                ir_txt(cont, KTL_ASM_JMP, _label_)
#define _JZ(_label_)                 ir_txt(cont, KTL_ASM_JZ, _label_)
#define _JNZ(_label_)                ir_txt(cont, KTL_ASM_JNZ, _label_)

#define _REP_MOVSB                   ir_txt(cont, KTL_ASM_REP_MOVSB)
#define _REP_STOSB                   ir_txt(cont, KTL_ASM_REP_STOSB)

#define _SYSCALL                     ir_txt(cont, KTL_ASM_SYSCALL)
#define _CALL(_op_)                  ir_txt(cont, KTL_ASM_CALL, _op_)
#define _CALL_PLT(_op_)              ir_txt(cont, KTL_ASM_CALL_PLT, _op_)
#define _RET                         ir_txt(cont, KTL_ASM_RET)

#define _CDQE                        ir_txt(cont, KTL_ASM_CDQE)
#define _CQO                         ir_txt(cont, KTL_ASM_CQO)

#define _SET(_cc_,_dst_)             ir_txt(cont, KTL_ASM_SET##_cc_, _dst_)

/* Data & Rodata Init */
#define _INT(_val_,_size_)           ir_data_int(cont, _val_, _size_)
#define _ZERO(_size_)                ir_data_zero(cont, _size_)
#define _BYTE(_name_)                ir_data_bytes(cont, _name_, strlen(_name_) + 1)
#define _LBYTE(_name_,_len_)         ir_data_bytes(cont, _name_, _len_)
#define _SYM_FUNC(_sym_)             op_sym(_sym_, KTL_BACK_IR_SYM_LOCAL_FUNC, 0)
#define _SYM_FUNC_GOT(_sym_)         op_sym(_sym_, KTL_BACK_IR_SYM_GOT_FUNC, 0)


/* Support */
#define _COMMENT(_text_)             ir_comment(cont, _text_)
#define _TEXT(_text_)                KTL_StrMapFind(cont->str_map, _text_)
#define _LABEL(_name_)               ir_label(cont, _name_, false)
#define _GLABEL(_name_)              ir_label(cont, _name_, true)
#define _SWITCH_DATA                 ir_section_data(cont)
#define _SWITCH_RODATA               ir_section_rodata(cont)
#define _SWITCH_TEXT                 ir_section_text(cont)
#define _ALIGN(_size_)               ir_align(cont, _size_)


// =======================================================================
// DECLARATION
// =======================================================================

static KTL_BackIR_InstrOperand op_reg    (KTL_RegID reg, int size);
static KTL_BackIR_InstrOperand op_imm    (int64_t value, int size);
static KTL_BackIR_InstrOperand op_mem    (KTL_RegID base, KTL_RegID idx,
                                          int scale, int offset, int size);
static KTL_BackIR_InstrOperand op_mem_rip(KTL_StrID sym,
                                          KTL_BackIR_SymbolKind kind, int size);
static KTL_BackIR_InstrOperand op_sym    (KTL_StrID sym,
                                          KTL_BackIR_SymbolKind kind, int size);
static KTL_BackIR_InstrOperand op_label  (KTL_StrID name);

static void ir_txt           (KTL_BackendContext *cont, KTL_AsmInstr instr);
static void ir_txt           (KTL_BackendContext *cont, KTL_AsmInstr instr,
                              KTL_BackIR_InstrOperand op);
static void ir_txt           (KTL_BackendContext *cont, KTL_AsmInstr instr,
                              KTL_BackIR_InstrOperand dst, KTL_BackIR_InstrOperand src);
static void ir_label         (KTL_BackendContext *cont, KTL_StrID name, bool is_global);
static void ir_comment       (KTL_BackendContext *cont, KTL_StrID text);
static void ir_align         (KTL_BackendContext *cont, int align);
static void ir_data_zero     (KTL_BackendContext *cont, int count);
static void ir_data_int      (KTL_BackendContext *cont, int64_t value, int size);
static void ir_data_bytes    (KTL_BackendContext *cont, KTL_StrID bytes, int len);
static void ir_data_symbol   (KTL_BackendContext *cont, KTL_StrID sym,
                              KTL_BackIR_SymbolKind kind, int64_t addend, int size);
static void ir_section_text  (KTL_BackendContext *cont);
static void ir_section_data  (KTL_BackendContext *cont);
static void ir_section_rodata(KTL_BackendContext *cont);

// =======================================================================
// BODY
// =======================================================================

static KTL_BackIR_InstrOperand op_reg(KTL_RegID reg, int size) {
    return (KTL_BackIR_InstrOperand){.kind     = KTL_BACK_IR_OP_REG,
                                     .reg.reg  = reg,
                                     .reg.size = (uint8_t) size};
}

static KTL_BackIR_InstrOperand op_imm(int64_t value, int size) {
    return (KTL_BackIR_InstrOperand){.kind     = KTL_BACK_IR_OP_IMM,
                                     .imm.imm  = value,
                                     .imm.size = (uint8_t) size};
}

static KTL_BackIR_InstrOperand op_mem(KTL_RegID base, KTL_RegID idx,
                                      int scale, int offset, int size) {
    return (KTL_BackIR_InstrOperand){.kind       = KTL_BACK_IR_OP_MEM,
                                     .mem.base   = base,
                                     .mem.idx    = idx,
                                     .mem.offset = offset,
                                     .mem.scale  = (uint8_t) scale,
                                     .mem.size   = size};
}

static KTL_BackIR_InstrOperand op_mem_rip(KTL_StrID sym,
                                          KTL_BackIR_SymbolKind kind, int size) {
    return (KTL_BackIR_InstrOperand){.kind         = KTL_BACK_IR_OP_MEM_RIP,
                                     .mem_rip.kind = kind,
                                     .mem_rip.size = (uint8_t) size,
                                     .mem_rip.sym  = sym};
}

static KTL_BackIR_InstrOperand op_sym(KTL_StrID sym,
                                      KTL_BackIR_SymbolKind kind, int size) {
    return (KTL_BackIR_InstrOperand){.kind     = KTL_BACK_IR_OP_SYMBOL,
                                     .sym.sym  = sym,
                                     .sym.kind = kind,
                                     .sym.size = size};
}

static KTL_BackIR_InstrOperand op_label(KTL_StrID name) {
    return (KTL_BackIR_InstrOperand){.kind       = KTL_BACK_IR_OP_LABEL,
                                     .label.name = name};
}

static void ir_txt(KTL_BackendContext *cont, KTL_AsmInstr instr) {
    assert(cont);
    KTL_BackIR_AddInstr(cont->cur_buf, instr);
}

static void ir_txt(KTL_BackendContext *cont, KTL_AsmInstr instr,
                   KTL_BackIR_InstrOperand op) {
    assert(cont);
    KTL_BackIR_AddInstr(cont->cur_buf, instr, &op);
}

static void ir_txt(KTL_BackendContext *cont, KTL_AsmInstr instr,
                   KTL_BackIR_InstrOperand dst, KTL_BackIR_InstrOperand src) {
    assert(cont);
    KTL_BackIR_AddInstr(cont->cur_buf, instr, &dst, &src);
}

static void ir_label(KTL_BackendContext *cont, KTL_StrID name, bool is_global) {
    assert(cont);
    assert(StrIDCheck(name));

    KTL_BackIR_Item item      = {};
    item.kind                 = KTL_BACK_IR_ITEM_LABEL;
    item.label_decl.name      = name;
    item.label_decl.is_global = is_global;

    add_item(cont->cur_buf, &item);
}

static void ir_comment(KTL_BackendContext *cont, KTL_StrID text) {
    assert(cont);
    assert(StrIDCheck(text));

    KTL_BackIR_AddComment(cont->cur_buf, text);
}

static void ir_align(KTL_BackendContext *cont, int align) {
    assert(cont);
    KTL_BackIR_AddAlign(cont->cur_buf, align);
}

static void ir_data_zero(KTL_BackendContext *cont, int count) {
    assert(cont);
    KTL_BackIR_AddZeroData(cont->cur_buf, count);
}

static void ir_data_int(KTL_BackendContext *cont, int64_t value, int size) {
    assert(cont);
    KTL_BackIR_AddIntData(cont->cur_buf, value, size);
}

static void ir_data_bytes(KTL_BackendContext *cont, KTL_StrID bytes, int len) {
    assert(cont);
    KTL_BackIR_AddByteData(cont->cur_buf, bytes, len);
}

static void ir_data_symbol(KTL_BackendContext *cont, KTL_StrID sym,
                           KTL_BackIR_SymbolKind kind, int64_t addend, int size) {
    assert(cont);
    KTL_BackIR_AddSymbolData(cont->cur_buf, sym, kind, addend, size);
}

static void ir_section_text(KTL_BackendContext *cont) {
    assert(cont);
    cont->cur_buf = cont->output.text;
}

static void ir_section_data(KTL_BackendContext *cont) {
    assert(cont);
    cont->cur_buf = cont->output.data;
}

static void ir_section_rodata(KTL_BackendContext *cont) {
    assert(cont);
    cont->cur_buf = cont->output.rodata;
}


#endif /* BACK_IR_DLL_H */
