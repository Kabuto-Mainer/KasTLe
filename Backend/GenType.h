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


struct KTL_ElfImport {
    KTL_StrID             name;
    KTL_BackIR_SymbolKind kind;
    uint32_t              dynstr_offset;
    uint32_t              dynsym_offset;
    uint32_t              plt_idx;
    uint32_t              got_idx;
};

struct KTL_ElfPart {
    uint64_t file_off;
    uint64_t size;

    uint64_t vaddr;

    uint64_t align;

    KTL_GenFlat data;
};


struct KTL_ElfContext {
    KTL_GenContext *gen_cont;
    FILE           *stream;

    struct {
        KTL_ElfImport  *imps;
        int             size;
        int             func_amount;
        int             var_amount;
    } import;

    KTL_ElfPart dynstr;
    KTL_ElfPart dynsym;
    KTL_ElfPart hash;
    KTL_ElfPart rela_plt;
    KTL_ElfPart plt;
    KTL_ElfPart got_plt;
    KTL_ElfPart dynamic;
    KTL_ElfPart interp;

    KTL_ElfPart text;
    KTL_ElfPart data;
    KTL_ElfPart rodata;



    uint64_t virt_adr;
    uint32_t phdr_amount;
};



#endif /* GEN_TYPE_H */
