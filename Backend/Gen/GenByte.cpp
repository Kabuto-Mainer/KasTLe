#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

#include "LabelMap.h"
#include "BackIR.h"
#include "GenType.h"
#include "Gen.h"

#define XXX static constexpr uint8_t

// =======================================================================
// REX + legacy prefix (УУУУ)

XXX REX_BASE   = 0b0100'0000;
XXX REX_W_MASK = 0b0000'1000;
XXX REX_R_MASK = 0b0000'0100;
// XXX REX_X_MASK = 0b0000'0010;
XXX REX_B_MASK = 0b0000'0001;

XXX SIZE_PREFIX_16 = 0x66;
XXX REP_PREFIX     = 0xF3;

static constexpr int ENC_MAX_LEN    = 15;


// =======================================================================
// OPCODES

XXX OP_PUSH_R     = 0x50;  /* r */
XXX OP_POP_R      = 0x58;  /* r */
XXX OP_MOV_R_IMM  = 0xB8;  /* r + imm */

XXX OP_RET        = 0xC3;
XXX OP_CDQE       = 0x98;
XXX OP_CQO        = 0x99;
XXX OP_STOSB      = 0xAA;
XXX OP_MOVSB      = 0xA4;
XXX OP_DEBUG      = 0xCC;

XXX OP_TWO_BYTE     = 0x0F;
XXX OP_SYSCALL_2    = 0x05;
XXX OP_MOVZX_RM8_2  = 0xB6;
XXX OP_MOVZX_RM16_2 = 0xB7;
XXX OP_MOVSX_RM8_2  = 0xBE;
XXX OP_MOVSX_RM16_2 = 0xBF;
XXX OP_IMUL_RM_2    = 0xAF;
XXX OP_SETCC_BASE_2 = 0x90;
XXX OP_JCC_BASE_2   = 0x80;

XXX OP_JMP_REL32  = 0xE9;
XXX OP_CALL_REL32 = 0xE8;

XXX OP_LEA        = 0x8D;
XXX OP_IMUL_IMM32 = 0x69;
XXX OP_IMUL_IMM8  = 0x6B;

XXX OP_GROUP3_8    = 0xF6; /* for 8-bit val/reg */
XXX OP_GROUP3_N8   = 0xF7; /* for 16+ bit val/reg */
XXX DIGIT_NEG      = 3;
XXX DIGIT_IDIV     = 7;

XXX OP_MOV_RM_IMM_8    = 0xC6;  /* /0 */
XXX OP_MOV_RM_IMM_WIDE = 0xC7;  /* /0 */

#undef XXX
#define XXX static const KTL_Gen_AluDesc

XXX DESC_MOV  = {0x88, 0x89, 0x8A, 0x8B, 0, true,  true,  false};
XXX DESC_ADD  = {0x00, 0x01, 0x02, 0x03, 0, true,  false, false};
XXX DESC_SUB  = {0x28, 0x29, 0x2A, 0x2B, 5, true,  false, false};
XXX DESC_XOR  = {0x30, 0x31, 0x32, 0x33, 6, true,  false, false};
XXX DESC_AND  = {0x20, 0x21, 0x22, 0x23, 4, true,  false, false};
XXX DESC_CMP  = {0x38, 0x39, 0x3A, 0x3B, 7, true,  false, false};
XXX DESC_TEST = {0x84, 0x85, 0,    0,    0, true,  false, true };

#undef XXX


// =======================================================================
// HELPER'S DECLARATIONS
// =======================================================================

static int             get_operand_size(const KTL_BackIR_InstrOperand *oper);
static bool            is_reg_extended (KTL_RegID reg);
static uint8_t         reg_low3        (KTL_RegID reg);
static bool            fits_int8       (int64_t v);
static bool            fits_int32      (int64_t v);
static KTL_BackIR_Item read_item       (KTL_GenPos *pos);
static void            advance         (KTL_GenPos *pos);
/*
static size_t          out_offset      (const KTL_GenPos *pos);*/
static void            out_write       (KTL_GenPos *pos, const uint8_t *bytes, int len);

static void    rex_init            (KTL_GenRex *rex);
static void    rex_set_w           (KTL_GenRex *rex);
static void    rex_set_r_if_needed (KTL_GenRex *rex, KTL_RegID reg);
static void    rex_set_b_if_needed (KTL_GenRex *rex, KTL_RegID reg);
/*
static void    rex_set_x_if_needed (KTL_GenRex *rex, KTL_RegID reg); */
/*
static void    rex_set_needed      (KTL_GenRex *rex); */
static bool    rex_is_present      (const KTL_GenRex *rex);
static uint8_t rex_get_byte        (const KTL_GenRex *rex);
static void    rex_emit            (const KTL_GenRex *rex, uint8_t *byte, int *len);

static void emit_imm_lend          (uint8_t *byte,  int *len,
                                    int64_t  value, int size);
static void encode_modrm_sup       (KTL_GenContext *cont,
                                    KTL_GenRex     *rex,
                                    uint8_t         reg_field_3,
                                    const KTL_BackIR_InstrOperand *rm,
                                    uint8_t *byte,
                                    int     *len,
                                    int     *rm_disp_pos);
static void encode_modrm_with_reg  (KTL_GenContext *cont,
                                    KTL_GenRex     *rex,
                                    KTL_RegID       reg_in_field,
                                    const KTL_BackIR_InstrOperand *rm,
                                    uint8_t *byte,
                                    int     *len,
                                    int     *rm_disp_pos);
static void encode_modrm_with_digit(KTL_GenContext *cont,
                                    KTL_GenRex     *rex,
                                    uint8_t         digit,
                                    const KTL_BackIR_InstrOperand *rm,
                                    uint8_t *byte,
                                    int     *len,
                                    int     *rm_disp_pos);

static void emit_zero_op     (KTL_GenContext *cont, KTL_AsmInstr cmd);
static void emit_stack_oper  (KTL_GenContext *cont, KTL_BackIR_Item *instr);
static void emit_unary_digit (KTL_GenContext  *cont,
                              KTL_BackIR_Item *instr,
                              uint8_t          digit);
static void emit_setcc       (KTL_GenContext  *cont,
                              KTL_BackIR_Item *instr,
                              KTL_Gen_CondCode cc);
static void emit_branch_rel32(KTL_GenContext *cont,
                              KTL_BackIR_Item *instr,
                              bool is_jmp,    bool is_call,
                              KTL_Gen_CondCode cc);
static void emit_lea         (KTL_GenContext  *cont,
                              KTL_BackIR_Item *instr);
static void emit_movzx_movsx (KTL_GenContext  *cont,
                              KTL_BackIR_Item *instr,
                              bool is_signed);
static void emit_mov_imm     (KTL_GenContext *cont,
                              KTL_GenRex *rex,
                              uint8_t *byte,    int *len,
                              const KTL_BackIR_InstrOperand *dst,
                              const KTL_BackIR_InstrOperand *src);
static void emit_alu_two_op  (KTL_GenContext        *cont,
                              KTL_BackIR_Item       *instr,
                              const KTL_Gen_AluDesc *desc);
static void emit_imul        (KTL_GenContext  *cont,
                              KTL_BackIR_Item *instr);
static void encode_one_instr (KTL_GenContext *cont);
static void fix_labels_inside_func(KTL_GenContext *cont);
static void fix_labels_inside_file(KTL_GenContext *cont);
static void emit_data             (KTL_GenContext *cont);
static void emit_rodata           (KTL_GenContext *cont);
static int  align_up              (int offset, int align);

static int  get_size              (KTL_BackIR_Buffer *buf);
static void flatter               (KTL_GenFlat       *flat, KTL_BackIR_Buffer *buf);
static void flatter               (KTL_GenContext    *cont);


// =======================================================================
// API
// =======================================================================

void KTL_GenElf(KTL_BackIR_Buffer *text,
                KTL_BackIR_Buffer *data,
                KTL_BackIR_Buffer *rodata,
                FILE *stream) {
    assert(text);
    assert(data);
    assert(rodata);
    assert(stream);

    KTL_GenContext cont = {};

    cont.in.data   = {data,   0, 0};
    cont.in.rodata = {rodata, 0, 0};
    cont.in.text   = {text,   0, 0};
    cont.sizes     = {0, 0, 0};

    KTL_BackIR_Buffer out_text   = {};
    KTL_BackIR_Buffer out_data   = {};
    KTL_BackIR_Buffer out_rodata = {};

    KTL_BackIR_Init(&out_text);
    KTL_BackIR_Init(&out_data);
    KTL_BackIR_Init(&out_rodata);

    cont.out = { {.buf = &out_text,   .offset = 0, .pos = 0},
                 {.buf = &out_data,   .offset = 0, .pos = 0},
                 {.buf = &out_rodata, .offset = 0, .pos = 0} };

    KTL_LabelFix_Map  func_fix_map         = {},
                      file_inside_fix_map  = {},
                      file_outside_fix_map = {},
                      data_reloc_map       = {};
    KTL_LabelDecl_Map func_decl_map        = {},
                      file_inside_decl_map = {};

    KTL_LabelDecl_Init(&func_decl_map);
    KTL_LabelDecl_Init(&file_inside_decl_map);

    KTL_LabelFix_Init(&func_fix_map);
    KTL_LabelFix_Init(&file_inside_fix_map);
    KTL_LabelFix_Init(&file_outside_fix_map);
    KTL_LabelFix_Init(&data_reloc_map);
    cont.func_decl_map        = &func_decl_map;
    cont.func_fix_map         = &func_fix_map;
    cont.file_inside_decl_map = &file_inside_decl_map;
    cont.file_inside_fix_map  = &file_inside_fix_map;
    cont.data_reloc_map       = &data_reloc_map;
    cont.file_outside_fix_map = &file_outside_fix_map;

    KTL_GenProcess(&cont);
    KTL_ElfEmit(&cont, stream);
    KTL_GenUninit(&cont);

    return ;
}

void KTL_GenByte(KTL_GenContext    *cont,
                 KTL_BackIR_Buffer *text,
                 KTL_BackIR_Buffer *data,
                 KTL_BackIR_Buffer *rodata) {
    assert(text);
    assert(data);
    assert(rodata);

    cont->in.data   = {data,   0, 0};
    cont->in.rodata = {rodata, 0, 0};
    cont->in.text   = {text,   0, 0};
    cont->sizes     = {0, 0, 0};

    KTL_BackIR_Buffer out_text   = {};
    KTL_BackIR_Buffer out_data   = {};
    KTL_BackIR_Buffer out_rodata = {};

    KTL_BackIR_Init(&out_text);
    KTL_BackIR_Init(&out_data);
    KTL_BackIR_Init(&out_rodata);

    cont->out = { {.buf = &out_text,   .offset = 0, .pos = 0},
                  {.buf = &out_data,   .offset = 0, .pos = 0},
                  {.buf = &out_rodata, .offset = 0, .pos = 0} };

    KTL_LabelFix_Map  func_fix_map         = {},
                      file_inside_fix_map  = {},
                      file_outside_fix_map = {},
                      data_reloc_map       = {};
    KTL_LabelDecl_Map func_decl_map        = {},
                      file_inside_decl_map = {};

    KTL_LabelDecl_Init(&func_decl_map);
    KTL_LabelDecl_Init(&file_inside_decl_map);

    KTL_LabelFix_Init(&func_fix_map);
    KTL_LabelFix_Init(&file_inside_fix_map);
    KTL_LabelFix_Init(&file_outside_fix_map);
    KTL_LabelFix_Init(&data_reloc_map);
    cont->func_decl_map        = &func_decl_map;
    cont->func_fix_map         = &func_fix_map;
    cont->file_inside_decl_map = &file_inside_decl_map;
    cont->file_inside_fix_map  = &file_inside_fix_map;
    cont->data_reloc_map       = &data_reloc_map;
    cont->file_outside_fix_map = &file_outside_fix_map;

    KTL_GenProcess(cont);

    return ;
}


void KTL_GenProcess(KTL_GenContext *cont) {

    assert(cont);

    emit_rodata(cont);
    emit_data(cont);

    while (cont->in.text.pos < (size_t) cont->in.text.buf->size) {
        KTL_BackIR_Item item = read_item(&cont->in.text);

        // printf("[%d]\n", cont->in.text.pos);
        if (item.kind == KTL_BACK_IR_ITEM_LABEL) {
            if (item.label_decl.is_global) {
                fix_labels_inside_func(cont);

                KTL_LabelDecl_Add(cont->file_inside_decl_map, item.label_decl.name, (int) cont->out.text.offset);
            }
            else {
                KTL_LabelDecl_Add(cont->func_decl_map, item.label_decl.name, (int) cont->out.text.offset);
            }

            advance(&cont->in.text);
            continue;
        }

        if (item.kind == KTL_BACK_IR_ITEM_INSTR) {
            encode_one_instr(cont);
            continue;
        }

        advance(&cont->in.text);
    }

    fix_labels_inside_func(cont);
    fix_labels_inside_file(cont);

    flatter(cont);

    return ;
}


void KTL_GenDumpFlat(KTL_GenFlat *flat) {
    assert(flat);

    for (int i = 0; i < flat->len; i++) {
        printf("0x%02x, ", flat->bytes[i]);
    }
    printf("\n");
    return ;
}

void KTL_GenUninit(KTL_GenContext *cont) {
    assert(cont);

    KTL_LabelFix_Uninit(cont->data_reloc_map);
    KTL_LabelFix_Uninit(cont->file_inside_fix_map);
    KTL_LabelFix_Uninit(cont->file_outside_fix_map);
    KTL_LabelFix_Uninit(cont->func_fix_map);

    KTL_LabelDecl_Uninit(cont->file_inside_decl_map);
    KTL_LabelDecl_Uninit(cont->func_decl_map);

    free(cont->out.data.buf->data);
    free(cont->out.text.buf->data);
    free(cont->out.rodata.buf->data);

    free(cont->out_flat.data.bytes);
    free(cont->out_flat.text.bytes);
    free(cont->out_flat.rodata.bytes);

    return ;
}



// =======================================================================
// HELPERS
// =======================================================================

static int get_operand_size(const KTL_BackIR_InstrOperand *oper) {
    assert(oper);
    switch (oper->kind) {
    case KTL_BACK_IR_OP_IMM:        return oper->imm.size;
    case KTL_BACK_IR_OP_LABEL:      return 4;
    case KTL_BACK_IR_OP_MEM:        return oper->mem.size;
    case KTL_BACK_IR_OP_MEM_RIP:    return oper->mem_rip.size;
    case KTL_BACK_IR_OP_REG:        return oper->reg.size;
    case KTL_BACK_IR_OP_SYMBOL:     return oper->sym.size;
    default:
        assert(0 && "bad operand");
        return -1;
    }
}

static bool is_reg_extended(KTL_RegID reg) {
    return reg >= KTL_REG_R8 && reg <= KTL_REG_R15;
}

static uint8_t reg_low3(KTL_RegID reg) {
    return (uint8_t)( ((int) reg) & 0b0000'0111);
}

static bool fits_int8 (int64_t v) {
    return v >= INT8_MIN  && v <= INT8_MAX;
}

static bool fits_int32(int64_t v) {
    return v >= INT32_MIN && v <= INT32_MAX;
}

// =======================================================================

static KTL_BackIR_Item read_item(KTL_GenPos *pos) {
    assert(pos);

    return pos->buf->data[pos->pos];
}

static void advance(KTL_GenPos *pos) {
    assert(pos);

    pos->pos++;
    return ;
}

// static size_t out_offset(const KTL_GenPos *pos) {
//     assert(pos);
//
//     return pos->offset;
// }

static void out_write(KTL_GenPos *pos, const uint8_t *bytes, int len) {
    assert(pos);
    assert(bytes);
    assert(len > 0);

    KTL_BackIR_AddByte15(pos->buf, bytes, (uint8_t)len);
    pos->offset += (size_t) len;
    pos->pos    += 1;

    return ;
}


// =======================================================================
// REX

static void rex_init(KTL_GenRex *rex) {
    assert(rex);

    rex->byte   = 0;
    rex->needed = false;

    return ;
}

static void rex_set_w(KTL_GenRex *rex) {
    assert(rex);

    rex->byte |= REX_W_MASK;

    return ;
}

static void rex_set_r_if_needed(KTL_GenRex *rex, KTL_RegID reg) {
    assert(rex);

    if (is_reg_extended(reg))  rex->byte |= REX_R_MASK;

    return ;
}

static void rex_set_b_if_needed(KTL_GenRex *rex, KTL_RegID reg) {
    assert(rex);

    if (is_reg_extended(reg))  rex->byte |= REX_B_MASK;

    return ;
}

// static void rex_set_x_if_needed(KTL_GenRex *rex, KTL_RegID reg) {
//     assert(rex);
//
//     if (is_reg_extended(reg))  rex->byte |= REX_X_MASK;
//
//     return ;
// }
//
// static void rex_set_needed(KTL_GenRex *rex) {
//     assert(rex);
//
//     rex->needed = true;
//
//     return ;
// }

static bool rex_is_present(const KTL_GenRex *rex) {
    assert(rex);

    return rex->byte != 0 || rex->needed;
}

static uint8_t rex_get_byte(const KTL_GenRex *rex) {
    assert(rex);
    assert(rex_is_present(rex));

    return (uint8_t)(REX_BASE | rex->byte);
}

static void rex_emit(const KTL_GenRex *rex, uint8_t *byte, int *len) {
    assert(rex);
    assert(byte);
    assert(len);

    if (!rex_is_present(rex))  return ;
    byte[(*len)++] = rex_get_byte(rex);

    return ;
}


// =======================================================================
// IMM

static void emit_imm_lend(uint8_t *byte, int *len, int64_t value, int size) {
    assert(byte);
    assert(len);
    assert(size == 1 || size == 2 ||
           size == 4 || size == 8);

    uint64_t v = (uint64_t)value;
    for (int i = 0; i < size; i++) {
        byte[(*len)++] = (uint8_t)(v & 0xFF);
        v >>= 8;
    }
    return ;
}

static void emit_size_prefix_if_16(uint8_t *byte, int *len, int size) {
    assert(byte);
    assert(len);

    if (size == 2)  byte[(*len)++] = SIZE_PREFIX_16;
    return ;
}


// =======================================================================
// MODR/M
static void encode_modrm_sup(KTL_GenContext *cont,
                             KTL_GenRex     *rex,
                             uint8_t         reg_field_3,
                             const KTL_BackIR_InstrOperand *rm,
                             uint8_t *byte,
                             int     *len,
                             int     *rm_disp_pos) {
    assert(cont);
    assert(rex);
    assert(rm);
    assert(byte);
    assert(len);
    assert((reg_field_3 & ~0b0000'0111) == 0);

    if (rm_disp_pos)  *rm_disp_pos = -1;

    switch (rm->kind) {
    case KTL_BACK_IR_OP_REG: {
        KTL_RegID reg = rm->reg.reg;
        rex_set_b_if_needed(rex, reg);

        uint8_t modrm = (uint8_t)((0b11 << 6)
                                 | (reg_field_3 << 3)
                                 | reg_low3(reg));
        byte[(*len)++] = modrm;
        return ;
    }

    /* [base + disp] */
    case KTL_BACK_IR_OP_MEM: {
        KTL_RegID base  = rm->mem.base;
        int       disp  = rm->mem.offset;
        uint8_t   base3 = reg_low3(base);

        assert(base3 != 0x4 && "RSP/R12 need SIB");

        rex_set_b_if_needed(rex, base);

        /* rbp & r13 need mod=01,disp8=0 if disp8 == 0 */
        bool is_rbp_like  = (base3 == reg_low3(KTL_REG_RBP));
        uint8_t mod       = 0b1111'1111;
        int     disp_size = -1;

        if (disp == 0 && is_rbp_like == false) {
            mod = 0b00;  disp_size = 0;  /* short form */
        } else if (fits_int8((int64_t)disp)) {
            mod = 0b01;  disp_size = 1;
        } else {
            mod = 0b10;  disp_size = 4;
        }

        uint8_t modrm = (uint8_t)((mod << 6)
                                 | (reg_field_3 << 3)
                                 | base3);
        byte[(*len)++] = modrm;

        if (disp_size > 0) {
            emit_imm_lend(byte, len, (int64_t)disp, disp_size);
        }
        return ;
    }

    /* [RIP + disp32] */
    case KTL_BACK_IR_OP_MEM_RIP: {
        /* mod=00, r/m=101 - RIP relative */
        uint8_t modrm = (uint8_t)((0x0 << 6)
                                 | (reg_field_3 << 3)
                                 | 0b0101);
        byte[(*len)++] = modrm;

        if (rm_disp_pos)  *rm_disp_pos = *len;

        if (rm->mem_rip.kind == KTL_BACK_IR_SYM_LOCAL_VAR) {
            KTL_LabelFix_AddGlobal(cont->file_inside_fix_map, rm->mem_rip.kind, rm->mem_rip.sym,
                        (int) cont->out.text.pos, *len, (int) cont->out.text.offset, rm->mem_rip.size);
        }
        else if (rm->mem_rip.kind == KTL_BACK_IR_SYM_GOT_VAR) {
            KTL_LabelFix_AddGlobal(cont->data_reloc_map, KTL_BACK_IR_SYM_GOT_VAR,
                rm->mem_rip.sym, (int) cont->out.text.pos, *len, (int) cont->out.text.offset, rm->mem_rip.size);
        }

        emit_imm_lend(byte, len, 0, 4);
        return ;
    }

    default:
        assert(0 && "unsupported r/m operand kind");
        return ;
    }
}

static void encode_modrm_with_reg(KTL_GenContext *cont,
                                  KTL_GenRex     *rex,
                                  KTL_RegID       reg_in_field,
                                  const KTL_BackIR_InstrOperand *rm,
                                  uint8_t *byte,
                                  int     *len,
                                  int     *rm_disp_pos) {
    assert(cont);
    assert(rex);
    assert(rm);
    assert(len);
    assert(byte);
    // assert(rm_disp_pos); don't need

    rex_set_r_if_needed(rex, reg_in_field);
    encode_modrm_sup(cont, rex, reg_low3(reg_in_field), rm, byte, len, rm_disp_pos);
}

static void encode_modrm_with_digit(KTL_GenContext *cont,
                                    KTL_GenRex     *rex,
                                    uint8_t         digit,
                                    const KTL_BackIR_InstrOperand *rm,
                                    uint8_t *byte,
                                    int     *len,
                                    int     *rm_disp_pos) {
    assert(cont);
    assert(rex);
    assert(rm);
    assert(len);
    assert(byte);
    // assert(rm_disp_pos); don't need

    assert((digit & ~0b0111) == 0);
    encode_modrm_sup(cont, rex, digit, rm, byte, len, rm_disp_pos);
}


// =======================================================================
// No operands (CDQE / CQO / SYSCALL / RET / REP STOSB / REP MOVSB)

static void emit_zero_op(KTL_GenContext *cont, KTL_AsmInstr cmd) {
    assert(cont);

    uint8_t byte[ENC_MAX_LEN] = {0};
    int     len = 0;

    switch (cmd) {
    case KTL_ASM_RET:
        byte[len++] = OP_RET;
        break;

    case KTL_ASM_SYSCALL:
        byte[len++] = OP_TWO_BYTE;
        byte[len++] = OP_SYSCALL_2;
        break;

    case KTL_ASM_CDQE:
        byte[len++] = REX_BASE | REX_W_MASK;
        byte[len++] = OP_CDQE;
        break;

    case KTL_ASM_CQO:
        byte[len++] = REX_BASE | REX_W_MASK;
        byte[len++] = OP_CQO;
        break;

    case KTL_ASM_REP_STOSB:
        byte[len++] = REP_PREFIX;
        byte[len++] = OP_STOSB;
        break;

    case KTL_ASM_REP_MOVSB:
        byte[len++] = REP_PREFIX;
        byte[len++] = OP_MOVSB;
        break;

    case KTL_ASM_DEBUG:
        byte[len++] = OP_DEBUG;
        break;

    default:
        assert(0 && "not a zero-op instruction");
        return;
    }

    out_write(&cont->out.text, byte, len);
    return ;
}


// =======================================================================
// PUSH r64 / POP r64

static void emit_stack_oper(KTL_GenContext *cont, KTL_BackIR_Item *instr) {
    assert(cont);
    assert(instr);

    KTL_AsmInstr           cmd = instr->instr.cmd;
    KTL_BackIR_InstrOperand op = instr->instr.one;

    assert(op.kind == KTL_BACK_IR_OP_REG);
    assert(op.reg.size == 8);

    uint8_t byte[ENC_MAX_LEN] = {0};
    int     len = 0;

    /* push / pop don't need REX.W */

    KTL_GenRex rex = {};
    rex_init(&rex);
    rex_set_b_if_needed(&rex, op.reg.reg);
    rex_emit(&rex, byte, &len);

    uint8_t op_base = (cmd == KTL_ASM_PUSH) ? OP_PUSH_R : OP_POP_R;
    byte[len++] = (uint8_t)(op_base + reg_low3(op.reg.reg));

    out_write(&cont->out.text, byte, len);
    return ;
}


// =======================================================================
// NEG + IDIV with digit

static void emit_unary_digit(KTL_GenContext  *cont,
                             KTL_BackIR_Item *instr,
                             uint8_t          digit) {
    assert(cont);
    assert(instr);

    KTL_BackIR_InstrOperand op = instr->instr.one;
    int size                   = get_operand_size(&op);

    assert(size == 1 || size == 2 ||
           size == 4 || size == 8);

    uint8_t byte[ENC_MAX_LEN] = {0};
    int     len = 0;

    KTL_GenRex rex = {};
    rex_init(&rex);
    if (size == 8)  rex_set_w(&rex);

    uint8_t modrm_buf[8] = {};
    int     modrm_len = 0;
    encode_modrm_with_digit(cont, &rex, digit, &op, modrm_buf, &modrm_len, NULL);

    emit_size_prefix_if_16(byte, &len, size);
    rex_emit(&rex, byte, &len);
    byte[len++] = (size == 1) ? OP_GROUP3_8 : OP_GROUP3_N8;
    memcpy(byte + len, modrm_buf, (size_t)modrm_len);
    len += modrm_len;

    out_write(&cont->out.text, byte, len);
    return ;
}


// =======================================================================
// SET cc

static void emit_setcc(KTL_GenContext  *cont,
                       KTL_BackIR_Item *instr,
                       KTL_Gen_CondCode cc) {
    assert(cont);
    assert(instr);

    KTL_BackIR_InstrOperand op = instr->instr.one;

    assert(op.kind == KTL_BACK_IR_OP_REG || op.kind == KTL_BACK_IR_OP_MEM);
    assert(get_operand_size(&op) == 1); /* only 1 byte operand */

    uint8_t byte[ENC_MAX_LEN] = {};
    int     len               = 0;

    KTL_GenRex rex = {};
    rex_init(&rex);

    /* 0F 9x + ModR/M */
    uint8_t modrm_buf[8] = {};
    int     modrm_len    = 0;
    encode_modrm_with_digit(cont, &rex, 0, &op, modrm_buf, &modrm_len, NULL);

    rex_emit(&rex, byte, &len);
    byte[len++] = OP_TWO_BYTE;
    byte[len++] = (uint8_t)(OP_SETCC_BASE_2 + (uint8_t)cc);
    memcpy(byte + len, modrm_buf, (size_t)modrm_len);
    len += modrm_len;

    out_write(&cont->out.text, byte, len);
    return ;
}


// =======================================================================
// call rel32

static void emit_branch_rel32(KTL_GenContext *cont,
                              KTL_BackIR_Item *instr,
                              bool is_jmp,    bool is_call,
                              KTL_Gen_CondCode cc) {
    assert(cont);
    assert(instr);

    KTL_BackIR_InstrOperand op = instr->instr.one;
    KTL_StrID         label_id = KTL_BAD_STR_ID;

    if (op.kind == KTL_BACK_IR_OP_LABEL) {
        label_id = op.label.name;
    }
    else if (op.kind == KTL_BACK_IR_OP_SYMBOL) {
        label_id = op.sym.sym;
    }
    else {
        assert(0 && "branch target must be label or symbol");
        return ;
    }

    uint8_t byte[ENC_MAX_LEN] = {0};
    int     len               = 0;

    if (is_jmp) {
        byte[len++] = OP_JMP_REL32;
    }
    else if (is_call) {
        byte[len++] = OP_CALL_REL32;
    }
    else {
        byte[len++] = OP_TWO_BYTE;
        byte[len++] = (uint8_t)(OP_JCC_BASE_2 + (uint8_t) cc);
    }

    int patch_local_pos = len;
    emit_imm_lend(byte, &len, 0, 4); /* buffer for address in future */

    if (op.kind == KTL_BACK_IR_OP_LABEL) {
        KTL_LabelFix_AddLocal(cont->func_fix_map, label_id, (int) cont->out.text.pos,
                            patch_local_pos, (int) cont->out.text.offset, 4);
    }
    else {  // symbol
        if (op.sym.kind == KTL_BACK_IR_SYM_LOCAL_FUNC) {
            KTL_LabelFix_AddGlobal(cont->file_inside_fix_map, KTL_BACK_IR_SYM_LOCAL_FUNC,
                                label_id, (int) cont->out.text.pos, patch_local_pos, (int) cont->out.text.offset, 4);
        }
        else {
            KTL_LabelFix_AddGlobal(cont->file_outside_fix_map, KTL_BACK_IR_SYM_GOT_FUNC,
                            label_id, (int) cont->out.text.pos, patch_local_pos, (int) cont->out.text.offset, 4);
        }
    }

    out_write(&cont->out.text, byte, len);

    return ;
}


// =======================================================================
// lea

static void emit_lea(KTL_GenContext  *cont,
                     KTL_BackIR_Item *instr) {
    assert(cont);
    assert(instr);

    KTL_BackIR_InstrOperand dst = instr->instr.two.dst;
    KTL_BackIR_InstrOperand src = instr->instr.two.src;

    assert(dst.kind == KTL_BACK_IR_OP_REG);
    assert(src.kind == KTL_BACK_IR_OP_MEM ||
           src.kind == KTL_BACK_IR_OP_MEM_RIP);

    int size = dst.reg.size;
    assert(size == 8 && "only LEA r64, m");

    uint8_t byte[ENC_MAX_LEN] = {0};
    int     len = 0;

    KTL_GenRex rex = {};
    rex_init(&rex);
    rex_set_w(&rex);

    /* dst -> reg, src -> r/m. */
    uint8_t modrm_buf[8] = {};
    int     modrm_len = 0;
    int     rip_pos   = -1;

    if (src.kind == KTL_BACK_IR_OP_MEM_RIP) {
        rex_set_r_if_needed(&rex, dst.reg.reg);
        rex_emit(&rex, byte, &len);
        byte[len++] = OP_LEA;

        uint8_t modrm = (uint8_t)((0x0 << 6)
                                 | (reg_low3(dst.reg.reg) << 3)
                                 | 0b0101);
        byte[len++] = modrm;
        int disp_pos = len;
        emit_imm_lend(byte, &len, 0, 4);

        if (src.mem_rip.kind == KTL_BACK_IR_SYM_LOCAL_VAR) {
            KTL_LabelFix_AddGlobal(cont->file_inside_fix_map, src.mem_rip.kind, src.mem_rip.sym,
                        (int) cont->out.text.pos, disp_pos, (int) cont->out.text.offset, src.mem_rip.size);
        }
        else if (src.mem_rip.kind == KTL_BACK_IR_SYM_GOT_VAR) {
            KTL_LabelFix_AddGlobal(cont->data_reloc_map, KTL_BACK_IR_SYM_GOT_VAR,
                src.mem_rip.sym, (int) cont->out.text.pos, disp_pos, (int) cont->out.text.offset, src.mem_rip.size);
        }
        else {
            assert(0 && "unsupported rip-relative symbol kind");
        }

        out_write(&cont->out.text, byte, len);
        return ;
    }
    encode_modrm_with_reg(cont, &rex, dst.reg.reg, &src, modrm_buf, &modrm_len, &rip_pos);

    rex_emit(&rex, byte, &len);
    byte[len++] = OP_LEA;

    // int modrm_local_start = len;
    memcpy(byte + len, modrm_buf, (size_t)modrm_len);
    len += modrm_len;

    // size_t out_before = out_offset(&cont->out.text);
    out_write(&cont->out.text, byte, len);

    // TODO: Add in future
//     if (src.kind == KTL_BACK_IR_OP_MEM_RIP) {
//         assert(rip_pos >= 0);
//
//         size_t patch_abs = out_before + (size_t)(modrm_local_start + (rip_pos - 0));
//
//         patch_abs = out_before + (size_t)(modrm_local_start + rip_pos);
//         size_t instr_end = out_before + (size_t)len;
//
//         KTL_LabelMapRequestRel32(cont->labels, src.mem_rip.label,
//                                  patch_abs, instr_end);
//     }
}


// =======================================================================
// movxz, movsx

static void emit_movzx_movsx(KTL_GenContext  *cont,
                             KTL_BackIR_Item *instr,
                             bool is_signed) {
    assert(cont);
    assert(instr);

    KTL_BackIR_InstrOperand dst = instr->instr.two.dst;
    KTL_BackIR_InstrOperand src = instr->instr.two.src;

    assert(dst.kind == KTL_BACK_IR_OP_REG);
    assert(src.kind == KTL_BACK_IR_OP_REG ||
           src.kind == KTL_BACK_IR_OP_MEM);

    int dst_size = dst.reg.size;
    int src_size = get_operand_size(&src);

    assert(src_size == 1 || src_size == 2);
    assert(dst_size == 2 || dst_size == 4 || dst_size == 8);
    assert(src_size < dst_size);

    uint8_t byte[ENC_MAX_LEN] = {0};
    int     len = 0;

    KTL_GenRex rex = {};
    rex_init(&rex);
    if (dst_size == 8)  rex_set_w(&rex);

    /* dst -> reg, src -> r/m. */
    uint8_t modrm_buf[8] = {};
    int     modrm_len = 0;
    encode_modrm_with_reg(cont, &rex, dst.reg.reg, &src, modrm_buf, &modrm_len, NULL);

    emit_size_prefix_if_16(byte, &len, dst_size);
    rex_emit(&rex, byte, &len);
    byte[len++] = OP_TWO_BYTE;

    if (is_signed) {
        byte[len++] = (src_size == 1) ? OP_MOVSX_RM8_2 : OP_MOVSX_RM16_2;
    } else {
        byte[len++] = (src_size == 1) ? OP_MOVZX_RM8_2 : OP_MOVZX_RM16_2;
    }

    memcpy(byte + len, modrm_buf, (size_t)modrm_len);
    len += modrm_len;

    out_write(&cont->out.text, byte, len);

    return ;
}


// =======================================================================
// MOV with IMM

static void emit_mov_imm(KTL_GenContext *cont,
                         KTL_GenRex *rex,
                         uint8_t *byte,    int *len,
                         const KTL_BackIR_InstrOperand *dst,
                         const KTL_BackIR_InstrOperand *src) {
    assert(rex);
    assert(byte);
    assert(len);
    assert(dst);
    assert(src);

    assert(src->kind == KTL_BACK_IR_OP_IMM);
    int size = get_operand_size(dst);

    if (dst->kind == KTL_BACK_IR_OP_REG) {
        rex_set_b_if_needed(rex, dst->reg.reg);
        if (size == 8)  rex_set_w(rex);

        emit_size_prefix_if_16(byte, len, size);
        rex_emit(rex, byte, len);

        uint8_t op_base = (size == 1) ? 0xB0 : OP_MOV_R_IMM;
        byte[(*len)++] = (uint8_t)(op_base + reg_low3(dst->reg.reg));
        emit_imm_lend(byte, len, src->imm.imm, size);
        return ;
    }

    assert(dst->kind == KTL_BACK_IR_OP_MEM);

    if (size == 8)  rex_set_w(rex);
    int imm_size = (size == 8) ? 4 : size;
    if (size == 8)  assert(fits_int32(src->imm.imm));

    emit_size_prefix_if_16(byte, len, size);

    uint8_t modrm_buf[8] = {};
    int     modrm_len = 0;
    encode_modrm_with_digit(cont, rex, 0, dst, modrm_buf, &modrm_len, NULL);

    rex_emit(rex, byte, len);
    byte[(*len)++] = (size == 1) ? OP_MOV_RM_IMM_8 : OP_MOV_RM_IMM_WIDE;
    memcpy(byte + *len, modrm_buf, (size_t)modrm_len);
    *len += modrm_len;
    emit_imm_lend(byte, len, src->imm.imm, imm_size);

    return ;
}


// =======================================================================
// ALU two operands : MOV / ADD / SUB / XOR / AND / CMP / TEST

static void emit_alu_two_op(KTL_GenContext        *cont,
                            KTL_BackIR_Item       *instr,
                            const KTL_Gen_AluDesc *desc) {
    assert(cont);
    assert(instr);

    KTL_BackIR_InstrOperand dst = instr->instr.two.dst;
    KTL_BackIR_InstrOperand src = instr->instr.two.src;

    int size = get_operand_size(&dst);
    if (src.kind != KTL_BACK_IR_OP_IMM) {
        assert(get_operand_size(&src) == size);
    }
    assert(size == 1 || size == 2 || size == 4 || size == 8);

    uint8_t byte[ENC_MAX_LEN] = {0};
    int     len = 0;

    KTL_GenRex rex = {};
    rex_init(&rex);
    if (size == 8)  rex_set_w(&rex);

    if (desc->is_mov && src.kind == KTL_BACK_IR_OP_IMM) {
        emit_mov_imm(cont, &rex, byte, &len, &dst, &src);
        out_write(&cont->out.text, byte, len);
        return ;
    }

    if (src.kind == KTL_BACK_IR_OP_IMM) {
        assert(desc->has_imm);
        assert(dst.kind == KTL_BACK_IR_OP_REG || dst.kind == KTL_BACK_IR_OP_MEM);
        int64_t v = src.imm.imm;

        bool use_imm8 = (size != 1) && fits_int8(v);
        int  imm_size = (size == 1) ? 1 : (use_imm8 ? 1 : 4);
        if (size == 8 && !use_imm8)  assert(fits_int32(v));

        uint8_t opcode = (uint8_t) -1;
        if (desc->is_test) {
            opcode = (size == 1) ? OP_GROUP3_8 : OP_GROUP3_N8;
        } else if (size == 1) {
            opcode = 0x80;
        } else {
            opcode = use_imm8 ? 0x83 : 0x81;
        }

        uint8_t modrm_buf[8] = {};
        int     modrm_len = 0;
        encode_modrm_with_digit(cont, &rex, desc->imm_digit, &dst,
                                modrm_buf, &modrm_len, NULL);

        emit_size_prefix_if_16(byte, &len, size);
        rex_emit(&rex, byte, &len);
        byte[len++] = opcode;
        memcpy(byte + len, modrm_buf, (size_t)modrm_len);
        len += modrm_len;

        emit_imm_lend(byte, &len, v, imm_size);
        out_write(&cont->out.text, byte, len);

        return;
    }

    bool dir_r_rm = (dst.kind == KTL_BACK_IR_OP_REG &&
                     (src.kind == KTL_BACK_IR_OP_MEM ||
                      src.kind == KTL_BACK_IR_OP_MEM_RIP));

    if (desc->is_test)  dir_r_rm = false;

    uint8_t opcode         = (uint8_t) -1;
    KTL_RegID reg_in_field = KTL_REG_INVALID;
    KTL_BackIR_InstrOperand rm_op = {};

    if (dir_r_rm) {
        opcode = (size == 1) ? desc->op_r_rm_8 : desc->op_r_rm_n8;
        reg_in_field = dst.reg.reg;
        rm_op        = src;
    } else {
        opcode = (size == 1) ? desc->op_rm_r_8 : desc->op_rm_r_n8;
        assert(src.kind == KTL_BACK_IR_OP_REG &&
               "for r/m, r form src must be a register");
        reg_in_field = src.reg.reg;
        rm_op        = dst;
    }
    assert(opcode != 0 && "form not supported by this descriptor");

    uint8_t modrm_buf[8] = {};
    int     modrm_len    = 0;
    encode_modrm_with_reg(cont, &rex, reg_in_field, &rm_op,
                          modrm_buf, &modrm_len, NULL);

    emit_size_prefix_if_16(byte, &len, size);
    rex_emit(&rex, byte, &len);
    byte[len++] = opcode;

    memcpy(byte + len, modrm_buf, (size_t)modrm_len);
    len += modrm_len;

    out_write(&cont->out.text, byte, len);
    return ;
}


// =======================================================================
// IMUL with 2 operands

static void emit_imul(KTL_GenContext  *cont,
                      KTL_BackIR_Item *instr) {
    assert(cont);
    assert(instr);

    KTL_BackIR_InstrOperand dst = instr->instr.two.dst;
    KTL_BackIR_InstrOperand src = instr->instr.two.src;

    assert(dst.kind == KTL_BACK_IR_OP_REG);

    int size = dst.reg.size;
    assert(size == 2 || size == 4 || size == 8);

    uint8_t byte[ENC_MAX_LEN] = {0};
    int     len = 0;

    KTL_GenRex rex = {};
    rex_init(&rex);
    if (size == 8)  rex_set_w(&rex);

    if (src.kind == KTL_BACK_IR_OP_IMM) {
        int64_t v = src.imm.imm;
        bool use_imm8 = fits_int8(v);
        int  imm_size = use_imm8 ? 1 : 4;
        if (!use_imm8)  assert(fits_int32(v));

        uint8_t modrm_buf[8] = {};
        int     modrm_len = 0;
        encode_modrm_with_reg(cont, &rex, dst.reg.reg, &dst,
                              modrm_buf, &modrm_len, NULL);

        emit_size_prefix_if_16(byte, &len, size);
        rex_emit(&rex, byte, &len);
        byte[len++] = use_imm8 ? OP_IMUL_IMM8 : OP_IMUL_IMM32;
        memcpy(byte + len, modrm_buf, (size_t)modrm_len);

        len += modrm_len;
        emit_imm_lend(byte, &len, v, imm_size);

        out_write(&cont->out.text, byte, len);
        return;
    }

    uint8_t modrm_buf[8] = {};
    int     modrm_len    = 0;
    encode_modrm_with_reg(cont, &rex, dst.reg.reg, &src,
                          modrm_buf, &modrm_len, NULL);

    emit_size_prefix_if_16(byte, &len, size);
    rex_emit(&rex, byte, &len);

    byte[len++] = OP_TWO_BYTE;
    byte[len++] = OP_IMUL_RM_2;

    memcpy(byte + len, modrm_buf, (size_t)modrm_len);
    len += modrm_len;

    out_write(&cont->out.text, byte, len);
    return ;
}


// =======================================================================
// distributor

static void encode_one_instr(KTL_GenContext *cont) {
    assert(cont);

    KTL_BackIR_Item item = read_item(&cont->in.text);
    if (item.kind != KTL_BACK_IR_ITEM_INSTR) {
        advance(&cont->in.text);
        return ;
    }

    KTL_BackIR_Item *instr = &item;
    KTL_AsmInstr cmd = instr->instr.cmd;

    switch (cmd) {

    case KTL_ASM_RET:
    case KTL_ASM_SYSCALL:
    case KTL_ASM_CDQE:
    case KTL_ASM_CQO:
    case KTL_ASM_REP_STOSB:
    case KTL_ASM_REP_MOVSB:
    case KTL_ASM_DEBUG:
        emit_zero_op(cont, cmd);
        break;

    case KTL_ASM_PUSH:
    case KTL_ASM_POP:
        emit_stack_oper(cont, instr);
        break;

    case KTL_ASM_NEG:
        emit_unary_digit(cont, instr, DIGIT_NEG);
        break;
    case KTL_ASM_IDIV1:
        emit_unary_digit(cont, instr, DIGIT_IDIV);
        break;

    case KTL_ASM_SETE:   emit_setcc(cont, instr, KTL_Gen_CondCode::CC_E);   break;
    case KTL_ASM_SETNE:  emit_setcc(cont, instr, KTL_Gen_CondCode::CC_NE);  break;
    case KTL_ASM_SETL:   emit_setcc(cont, instr, KTL_Gen_CondCode::CC_L);   break;
    case KTL_ASM_SETGE:  emit_setcc(cont, instr, KTL_Gen_CondCode::CC_GE);  break;
    case KTL_ASM_SETLE:  emit_setcc(cont, instr, KTL_Gen_CondCode::CC_LE);  break;
    case KTL_ASM_SETG:   emit_setcc(cont, instr, KTL_Gen_CondCode::CC_G);   break;

    case KTL_ASM_JMP:    emit_branch_rel32(cont, instr, true,  false, (KTL_Gen_CondCode) 0);    break;
    case KTL_ASM_JZ:     emit_branch_rel32(cont, instr, false, false, KTL_Gen_CondCode::CC_Z);  break;
    case KTL_ASM_JNZ:    emit_branch_rel32(cont, instr, false, false, KTL_Gen_CondCode::CC_NZ); break;
    case KTL_ASM_CALL_PLT:
    case KTL_ASM_CALL:   emit_branch_rel32(cont, instr, false, true,  (KTL_Gen_CondCode) 0);    break;

    case KTL_ASM_LEA:
        emit_lea(cont, instr);
        break;

    case KTL_ASM_MOVZX:
        emit_movzx_movsx(cont, instr, false);
        break;
    case KTL_ASM_MOVSX:
        emit_movzx_movsx(cont, instr, true);
        break;

    case KTL_ASM_MOV:    emit_alu_two_op(cont, instr, &DESC_MOV);  break;
    case KTL_ASM_ADD:    emit_alu_two_op(cont, instr, &DESC_ADD);  break;
    case KTL_ASM_SUB:    emit_alu_two_op(cont, instr, &DESC_SUB);  break;
    case KTL_ASM_XOR:    emit_alu_two_op(cont, instr, &DESC_XOR);  break;
    case KTL_ASM_AND:    emit_alu_two_op(cont, instr, &DESC_AND);  break;
    case KTL_ASM_CMP:    emit_alu_two_op(cont, instr, &DESC_CMP);  break;
    case KTL_ASM_TEST:   emit_alu_two_op(cont, instr, &DESC_TEST); break;

    /* IMUL */
    case KTL_ASM_IMUL:
        emit_imul(cont, instr);
        break;

    default:
        assert(0 && "unsupported instruction");
        break;
    }

    advance(&cont->in.text);
}

static void fix_labels_inside_func(KTL_GenContext *cont) {
    assert(cont);
    if (cont->func_decl_map == NULL)    return ;
    if (cont->func_fix_map == NULL)     return ;

    for (int i = 0; i < cont->func_fix_map->size; i++) {
        KTL_LabelFix_Entry   *fix = cont->func_fix_map->data + i;
        KTL_LabelDecl_Entry *decl = KTL_LabelDecl_Find(cont->func_decl_map, fix->target);

        assert(decl && "don't find label in table");
        assert(cont->out.text.buf->size >= fix->index);

        KTL_BackIR_Item *item = cont->out.text.buf->data + fix->index;

        assert(item->byte_15.len >= fix->inner_offset);

        int32_t disp32 = decl->offset - fix->ads_offset - fix->inner_offset - 4;

        uint8_t byte[4] = {};
        int     len     = 0;
        emit_imm_lend(byte, &len, disp32, 4);
        memcpy(item->byte_15.byte + fix->inner_offset, byte, 4);
    }
    KTL_LabelDecl_Uninit(cont->func_decl_map);
    KTL_LabelFix_Uninit(cont->func_fix_map);

    KTL_LabelDecl_Init(cont->func_decl_map);
    KTL_LabelFix_Init(cont->func_fix_map);

    return ;
}

static void fix_labels_inside_file(KTL_GenContext *cont) {
    assert(cont);
    assert(cont->file_inside_fix_map);
    assert(cont->file_inside_decl_map);

    for (int i = 0; i < cont->file_inside_fix_map->size; i++) {
        KTL_LabelFix_Entry   *fix = cont->file_inside_fix_map->data + i;
        KTL_LabelDecl_Entry *decl = KTL_LabelDecl_Find(cont->file_inside_decl_map, fix->target);

        // printf("fix target: %s\n", fix->target);
        assert(decl && "don't find label in table");
        assert(cont->out.text.buf->size >= fix->index);

        KTL_BackIR_Item *item = cont->out.text.buf->data + fix->index;

        assert(item->byte_15.len >= fix->inner_offset);

        int32_t disp32 = decl->offset - fix->ads_offset - fix->inner_offset - 4;
        // printf("disp: %d\n", disp32);
        if (fix->kind == KTL_BACK_IR_SYM_LOCAL_VAR) {
            disp32 = decl->offset;
        }

        uint8_t byte[4] = {};
        int     len     = 0;
        emit_imm_lend(byte, &len, disp32, 4);
        memcpy(item->byte_15.byte + fix->inner_offset, byte, 4);
    }
    return ;
}

static void emit_data(KTL_GenContext *cont) {
    assert(cont);

    if (cont->file_inside_decl_map == NULL) {
        KTL_LabelDecl_Init(cont->file_inside_decl_map);
    }

    int cur_offset = 0;
    for (int i = 0; i < cont->in.data.buf->size; i++) {
        KTL_BackIR_Item item = read_item(&cont->in.data);

        if (item.kind == KTL_BACK_IR_ITEM_DIRECTIVE) {
            int new_offset = align_up(cur_offset, item.direct.align.size);
            KTL_BackIR_AddZeroData(cont->out.data.buf, new_offset - cur_offset);
            cur_offset = new_offset;

            cont->out.data.pos++;
            continue;
        }
        else if (item.kind == KTL_BACK_IR_ITEM_DATA) {
            switch (item.data.kind) {

            case KTL_BACK_IR_DATA_BYTES:
                KTL_BackIR_AddByteData(cont->out.data.buf, item.data.bytes.bytes, item.data.bytes.len);
                cur_offset += item.data.bytes.len;
                break;

            case KTL_BACK_IR_DATA_INT:
                KTL_BackIR_AddIntData(cont->out.data.buf, item.data.int_val.value, item.data.int_val.size);
                cur_offset += item.data.int_val.size;
                break;

            case KTL_BACK_IR_DATA_ZERO:
                KTL_BackIR_AddZeroData(cont->out.data.buf, item.data.zero.count);
                cur_offset += item.data.zero.count;
                break;

            case KTL_BACK_IR_DATA_SYMBOL:
                KTL_BackIR_AddZeroData(cont->out.data.buf, item.data.symbol.size);
                KTL_LabelFix_AddGlobal(cont->data_reloc_map, item.data.symbol.sym_kind,
                        item.data.symbol.sym, (int) cont->out.data.pos, 0, cur_offset, item.data.symbol.size);

                cur_offset += item.data.symbol.size;
                break;

            default:
                assert(0 && "unknown kind of item_data");
                break;
            }
            cont->out.data.pos++;
            continue;
        }
        else if (item.kind == KTL_BACK_IR_ITEM_LABEL) {
            KTL_LabelDecl_Add(cont->file_inside_decl_map, item.label_decl.name, cur_offset);
            cont->out.data.pos++;
            continue;
        }
        else {
            assert(0 && "bad kind of item");
        }
    }
    cont->sizes.data_size = cur_offset;
    return ;
}

static void emit_rodata(KTL_GenContext *cont) {
    assert(cont);

    if (cont->file_inside_decl_map == NULL) {
        KTL_LabelDecl_Init(cont->file_inside_decl_map);
    }

    int cur_offset = 0;
    for (int i = 0; i < cont->in.rodata.buf->size; i++) {
        KTL_BackIR_Item item = read_item(&cont->in.rodata);
        cont->in.rodata.pos++;

        if (item.kind == KTL_BACK_IR_ITEM_LABEL) {
            KTL_LabelDecl_Add(cont->file_inside_decl_map, item.label_decl.name, cur_offset);
            continue;
        }
        else if (item.kind == KTL_BACK_IR_ITEM_DATA) {
            assert(item.data.kind == KTL_BACK_IR_DATA_BYTES);

            KTL_BackIR_AddByteData(cont->out.rodata.buf, item.data.bytes.bytes, item.data.bytes.len);
            cur_offset += item.data.bytes.len;
            continue;
        }
        else if (item.kind == KTL_BACK_IR_ITEM_COMMENT) {
            continue;
        }
        else {
            assert(0 && "bad kind of item_data");
        }
    }
    cont->sizes.rodata_size = cur_offset;
    return ;
}

static int align_up(int offset, int align) {
    assert(align > 0);

    return ((offset + align - 1) / align) * align;
}


static int get_size(KTL_BackIR_Buffer *buf) {
    assert(buf);

    int size = 0;
    for (int i = 0; i < buf->size; i++) {
        KTL_BackIR_Item *item = buf->data + i;
        switch (item->kind) {

        case KTL_BACK_IR_ITEM_BYTE_15: {
            size += item->byte_15.len;
            break;
        }
        case KTL_BACK_IR_ITEM_DATA: {
            switch (item->data.kind) {

            case KTL_BACK_IR_DATA_BYTES:
                size += item->data.bytes.len;
                break;

            case KTL_BACK_IR_DATA_INT:
                size += item->data.int_val.size;
                break;

            case KTL_BACK_IR_DATA_SYMBOL:
                size += item->data.symbol.size;
                break;

            case KTL_BACK_IR_DATA_ZERO:
                size += item->data.zero.count;
                break;
            default:
                assert(0 && "unknown kind ot item_data");
                break;
            }
            break;
        }

        default:
            break;
        }
    }
    return size;
}

static void flatter(KTL_GenFlat *flat, KTL_BackIR_Buffer *buf) {
    assert(buf);
    assert(flat);

    int size    = get_size(buf);
    // printf("SIZE: %d\n", size);
    flat->bytes = (uint8_t *)calloc((size_t) size, sizeof(uint8_t));
    if (flat->bytes == NULL)    ExitF("NULL calloc", );

    int pos = 0;

    for (int i = 0; i < buf->size; i++) {
        KTL_BackIR_Item *item = buf->data + i;
        switch (item->kind) {

        case KTL_BACK_IR_ITEM_BYTE_15: {
            memcpy(flat->bytes + pos, item->byte_15.byte, item->byte_15.len);
            pos += item->byte_15.len;
            break;
        }
        case KTL_BACK_IR_ITEM_DATA: {
            switch (item->data.kind) {

            case KTL_BACK_IR_DATA_BYTES:
                memcpy(flat->bytes + pos, item->data.bytes.bytes, (size_t) item->data.bytes.len);
                pos += item->data.bytes.len;
                break;

            case KTL_BACK_IR_DATA_INT:
                emit_imm_lend(flat->bytes + pos, &pos, item->data.int_val.value, item->data.int_val.size);
                pos += item->data.int_val.size;
                break;

            case KTL_BACK_IR_DATA_SYMBOL:
                memset(flat->bytes + pos, 0, item->data.symbol.size);
                pos += item->data.symbol.size;
                break;

            case KTL_BACK_IR_DATA_ZERO:
                memset(flat->bytes + pos, 0, (size_t) item->data.zero.count);
                size += item->data.zero.count;
                break;
            default:
                assert(0 && "unknown kind ot item_data");
                break;
            }
            break;
        }

        default:
            break;
        }
        continue;
    }
    flat->len = pos;

}

static void flatter(KTL_GenContext *cont) {
    assert(cont);

    flatter(&cont->out_flat.data,   cont->out.data.buf);
    flatter(&cont->out_flat.rodata, cont->out.rodata.buf);
    flatter(&cont->out_flat.text,   cont->out.text.buf);

    return ;
}

