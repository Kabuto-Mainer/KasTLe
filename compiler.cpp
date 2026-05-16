#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include "Parse.h"
#include "Token.h"
#include "DumpAst.h"
#include "Analysis.h"
#include "Backend.h"
#include "BackIR.h"
#include "Gen.h"

struct KTL_Options {
    const char *source;
    const char *output_asm;
    const char *output_elf;
    const char *dump_ast_path;
    bool emit_elf;
    bool dump_ast;
    int  verbose;
};

static int  compile    (const KTL_Options *opts);
static void print_usage(const char *prog);
static int  parse_args (int argc, char **argv, KTL_Options *opts);


static void print_usage(const char *prog) {
    fprintf(stderr,
        "Usage: %s [options] <source>\n"
        "  -o, --output <file>     output .asm file (default: out.asm)\n"
        "      --emit-elf <file>   also emit ELF binary\n"
        "      --dump-ast <file>   dump AST as HTML\n"
        "  -v, --verbose           verbose output\n"
        "  -h, --help              show this help\n",
        prog);
}

static int parse_args(int argc, char **argv, KTL_Options *opts) {
    assert(argv);
    assert(opts);

    enum {
        OPT_EMIT_ELF = 256,
        OPT_DUMP_AST
    };

    static struct option long_opts[] = {
        {"output",   required_argument, 0, 'o'},
        {"emit-elf", required_argument, 0, OPT_EMIT_ELF},
        {"dump-ast", required_argument, 0, OPT_DUMP_AST},
        {"verbose",  no_argument,       0, 'v'},
        {"help",     no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };

    int c = -1;
    while ((c = getopt_long(argc, argv, "o:vh", long_opts, NULL)) != -1) {
        switch (c) {
            case 'o':
                opts->output_asm = optarg;
                break;

            case OPT_EMIT_ELF:
                opts->emit_elf = true;
                opts->output_elf = optarg;
                break;

            case OPT_DUMP_AST:
                opts->dump_ast = true;
                opts->dump_ast_path = optarg;
                break;

            case 'v':
                opts->verbose++;
                break;

            case 'h':
                print_usage(argv[0]);
                exit(0);

            default:
                return -1;
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "error: no input file\n");
        return -1;
    }
    opts->source = argv[optind];

    return 0;
}

int main(int argc, char **argv) {
    KTL_Options opts = {.output_asm = "out.asm"};

    if (parse_args(argc, argv, &opts) != 0) {
        print_usage(argv[0]);
        return 1;
    }

    return compile(&opts);
}


static int compile(const KTL_Options *opts) {
    assert(opts);

    int ret_val = 0;
    KTL_StrMap          str_map     = {};
    KTL_Diagnostic      diag        = {};
    KTL_TokenContext    token_cont  = {};
    KTL_TypeMap         type_map    = {};
    KTL_ParseContext    parse_cont  = {};
    KTL_AnalysisContext an_cont     = {};
    KTL_BackendContext  back_cont   = {};
    KTL_BackIR_Buffer   text   = {},
                        data   = {},
                        rodata = {};
    FILE *out = NULL;

    KTL_StrMapCreate(&str_map, 5000);
    KTL_DiagCreate(&diag, 10);

    KTL_TokenInit(&token_cont, opts->source);
    KTL_TokenAddStrMap(&token_cont, &str_map);
    KTL_TokenAddDiag(&token_cont, &diag);

    _PRINT_STAGE;
    KTL_TokenProcess(&token_cont);
    if (diag.error_count > 0) { ret_val = 1; goto cleanup; }

    KTL_TypeMapCreate(&type_map, 10);
    KTL_ParseInit(&parse_cont, token_cont.tokens, token_cont.token_pos,
                  &str_map, &type_map, &diag);

    _NEXT_STAGE;
    _PRINT_STAGE;
    KTL_ParseProcess(&parse_cont);
    if (diag.error_count > 0) { ret_val = 1; goto cleanup; }

    KTL_AnalysisInit(&an_cont, &str_map, &type_map,
                     parse_cont.global_map, &diag, parse_cont.root);
    _NEXT_STAGE;
    _PRINT_STAGE;
    KTL_AnalysisProcess(&an_cont);
    if (diag.error_count > 0) { ret_val = 1; goto cleanup; }

    KTL_BackIR_Init(&text);
    KTL_BackIR_Init(&data);
    KTL_BackIR_Init(&rodata);
    KTL_BackendInit(&back_cont, &type_map, &str_map,
                    an_cont.global_scope, &text, &data, &rodata);

    _NEXT_STAGE;
    _PRINT_STAGE;
    KTL_BackendRun(&back_cont, an_cont.root);

    if (opts->output_asm) {
        out = fopen(opts->output_asm, "wb");
        if (out == NULL) {
            perror(opts->output_asm);
            ret_val = 1;
            goto cleanup;
        }

        KTL_GenerateNasm(&text, &data, &rodata, out);
        fclose(out);
        out = NULL;
    }

    if (opts->emit_elf) {
        out = fopen(opts->output_elf, "wb");
        if (!out) {
            perror(opts->output_elf);
            ret_val = 1;
            goto cleanup;
        }

        KTL_GenElf(&text, &data, &rodata, out);
        fclose(out); out = NULL;
    }

    if (opts->dump_ast) {
        KTL_AstDumpRaw(parse_cont.root, &str_map, parse_cont.global_map,
                       &type_map, opts->dump_ast_path);
    }

cleanup:
    if (diag.error_count > 0) KTL_DiagFlush(&diag, opts->source);
    if (out) fclose(out);

    KTL_BackendUninit (&back_cont);
    KTL_BackIR_Uninit (&rodata);
    KTL_BackIR_Uninit (&data);
    KTL_BackIR_Uninit (&text);
    KTL_ParseUninit   (&parse_cont);
    KTL_TypeMapDestroy(&type_map);
    KTL_TokenUninit   (&token_cont);
    KTL_DiagDestroy   (&diag);
    KTL_StrMapDestroy (&str_map);

    return ret_val;
}
