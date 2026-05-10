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
    union {
        size_t offset;
        size_t pos;
    };
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
        KTL_GenPosKind kind;
    } in;

    struct {
        KTL_GenPos text;
        KTL_GenPos data;
        KTL_GenPos rodata;
        KTL_GenPosKind kind;
    } out;

    KTL_LabelFix_Map *fix_map;

};

#endif /* GEN_TYPE_H */
