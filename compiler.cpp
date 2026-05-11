#include "Parse.h"
#include "Token.h"
#include "DumpAst.h"
#include "Analysis.h"
#include "Backend.h"
#include "BackIR.h"
#include "GenByte.h"
#include "Gen.h"

constexpr const char *SOURCE = "DataTests/3.txt";
constexpr const char *DEST   = "Bin/1.asm";


int main() {
    KTL_StrMap str_map = {};
    KTL_StrMapCreate(&str_map, 5000);

    KTL_Diagnostic diag = {};
    KTL_DiagCreate(&diag, 10);

    KTL_TokenContext token_cont = {};
    KTL_TokenInit     (&token_cont, SOURCE);
    KTL_TokenAddStrMap(&token_cont, &str_map);
    KTL_TokenAddDiag  (&token_cont, &diag);

    KTL_TokenProcess(&token_cont);

    if (diag.error_count > 0) {
        KTL_DiagFlush(&diag, SOURCE);
        KTL_StrMapDestroy (&str_map);
        KTL_DiagDestroy   (&diag);
        KTL_TokenUninit   (&token_cont);
        return 0;
    }

    // KTL_TokenDump(&token_cont);

    KTL_TypeMap type_map = {};
    KTL_TypeMapCreate(&type_map, 10);

    KTL_ParseContext parse_cont = {};
    KTL_ParseInit(&parse_cont,
                   token_cont.tokens, token_cont.token_pos,
                  &str_map,          &type_map,
                  &diag);

    KTL_ParseProcess(&parse_cont);
    if (diag.error_count > 0) {
        KTL_DiagFlush(&diag, SOURCE);
        KTL_StrMapDestroy (&str_map);
        KTL_DiagDestroy   (&diag);
        KTL_TokenUninit   (&token_cont);
        KTL_TypeMapDestroy(&type_map);
        KTL_ParseUninit   (&parse_cont);
        return 0;
    }


    KTL_AnalysisContext an_cont = {};
    KTL_AnalysisInit(&an_cont, &str_map,
                     &type_map, parse_cont.global_map,
                     &diag, parse_cont.root);

    KTL_AnalysisProcess(&an_cont);

    if (diag.error_count > 0) {
        KTL_DiagFlush(&diag, SOURCE);
        KTL_StrMapDestroy (&str_map);
        KTL_DiagDestroy   (&diag);
        KTL_TokenUninit   (&token_cont);
        KTL_TypeMapDestroy(&type_map);
        KTL_ParseUninit   (&parse_cont);
        return 0;
    }

    KTL_BackendContext back_cont = {};
    KTL_BackIR_Buffer  text      = {};
    KTL_BackIR_Buffer  data      = {};
    KTL_BackIR_Buffer  rodata    = {};

    KTL_BackIR_Init(&text);
    KTL_BackIR_Init(&data);
    KTL_BackIR_Init(&rodata);

    KTL_BackendInit(&back_cont, &type_map, &str_map, an_cont.global_scope, &text, &data, &rodata);
    KTL_BackendRun(&back_cont, an_cont.root);

    FILE *output = fopen(DEST, "wb");
    KTL_Backend_GenerateNasm(&text, &data, &rodata, output);
    fclose(output);

    // KTL_GenByte(&text, &data, &rodata);

    KTL_BackendUninit(&back_cont);

    KTL_BackIR_Uninit(&text);
    KTL_BackIR_Uninit(&data);
    KTL_BackIR_Uninit(&rodata);


    // KTL_DiagFlush(&diag, NAME_FILE);

    KTL_AstDumpRaw(parse_cont.root, &str_map, parse_cont.global_map, &type_map, "dump.html");

    if (diag.error_count > 0) {
        KTL_DiagFlush(&diag, SOURCE);
        KTL_StrMapDestroy (&str_map);
        KTL_DiagDestroy   (&diag);
        KTL_TokenUninit   (&token_cont);
        KTL_TypeMapDestroy(&type_map);
        KTL_ParseUninit   (&parse_cont);
        return 0;
    }


    KTL_StrMapDestroy (&str_map);
    KTL_DiagDestroy   (&diag);
    KTL_TokenUninit   (&token_cont);
    KTL_TypeMapDestroy(&type_map);
    KTL_ParseUninit   (&parse_cont);
    return 0;
}
