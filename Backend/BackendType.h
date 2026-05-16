#ifndef BACKEND_TYPE_H
#define BACKEND_TYPE_H

#include <stdio.h>

#include "Common.h"
#include "StrMapType.h"
#include "TypeMapType.h"
#include "SymMapType.h"
#include "BackMapType.h"
#include "BackIRType.h"

extern const KTL_RegID KTL_PARAM_REGS[6];
constexpr int KTL_PARAM_REGS_COUNT = 6;
constexpr int KTL_SYSTEM_PTR_SIZE  = 8;

// #define EMIT_DEBUG

struct KTL_BackendContext {
    KTL_TypeMap   *type_map;
    KTL_StrMap    *str_map;
    KTL_SymbolMap *global_scope;

    KTL_BackendTable table;

    struct {
        KTL_BackIR_Buffer *text;
        KTL_BackIR_Buffer *data;
        KTL_BackIR_Buffer *rodata;
    } output;

    KTL_BackIR_Buffer *cur_buf;

    KTL_BackendFuncInfo *current_func;

    int loop_label_break;
    int loop_label_continue;
    int label_counter;
    int stack_depth;

    int frame_offset;
    int main_frame_size;

    const char *symbol_prefix;  /* for MacOS users */

#ifdef EMIT_DEBUG
    FILE *debug_emit;
#endif

};

#define print_asm(_fmt_, ...)     fprintf(cont->output, _fmt_,  ##__VA_ARGS__)


#endif /* BACKEND_TYPE_H */
