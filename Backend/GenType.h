#ifndef GEN_TYPE_H
#define GEN_TYPE_H

#include <stdint.h>
#include "BackIRType.h"
#include "LabelMapType.h"

enum class KTL_Gen_CondCode {
    CC_E  = 0x4,
    CC_NE = 0x5,
    CC_L  = 0xC,
    CC_GE = 0xD,
    CC_LE = 0xE,
    CC_G  = 0xF,
    CC_Z  = 0x4,
    CC_NZ = 0x5,
};

struct KTL_Gen_AluDesc {
    uint8_t op_rm_r_8;
    uint8_t op_rm_r_n8;
    uint8_t op_r_rm_8;
    uint8_t op_r_rm_n8;
    uint8_t imm_digit;
    bool    has_imm;
    bool    is_mov;
    bool    is_test;
};

struct KTL_GenRex {
    uint8_t byte;
    bool    needed;
};

struct KTL_GenPos {
    KTL_BackIR_Buffer *buf;
    size_t offset;
    size_t pos;
};

struct KTL_GenFlat {
    uint8_t *bytes;
    int      len;
};

enum class KTL_GenPosKind {
    TEXT,
    DATA,
    RODATA
};

struct KTL_GenContext {
    struct {
        KTL_GenPos text;
        KTL_GenPos data;
        KTL_GenPos rodata;
    } in;

    struct {
        KTL_GenPos text;
        KTL_GenPos data;
        KTL_GenPos rodata;
    } out;

    struct {
        int text_size;
        int data_size;
        int rodata_size;
    } sizes;

    KTL_LabelFix_Map  *func_fix_map;
    KTL_LabelDecl_Map *func_decl_map;

    KTL_LabelFix_Map  *file_inside_fix_map;
    KTL_LabelDecl_Map *file_inside_decl_map;

    KTL_LabelFix_Map  *file_outside_fix_map;
    KTL_LabelFix_Map  *data_reloc_map;

    struct {
        KTL_GenFlat text;
        KTL_GenFlat data;
        KTL_GenFlat rodata;
    } out_flat;
};

#endif /* GEN_TYPE_H */
