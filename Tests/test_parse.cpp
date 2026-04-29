#include "Parse.h"
#include "Token.h"
#include "DumpAst.h"

constexpr char *NAME_FILE = "DataTests/3.txt";

int main() {
    KTL_StrMap str_map = {};
    KTL_StrMapCreate(&str_map, 5000);

    KTL_Diagnostic diag = {};
    KTL_DiagCreate(&diag, 10);

    KTL_TokenContext token_cont = {};
    KTL_TokenInit     (&token_cont, NAME_FILE);
    KTL_TokenAddStrMap(&token_cont, &str_map);
    KTL_TokenAddDiag  (&token_cont, &diag);

    KTL_TokenProcess(&token_cont);

    if (diag.error_count > 0) {
        KTL_DiagFlush(&diag, NAME_FILE);
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
    KTL_AstDumpRaw(parse_cont.root, &str_map, parse_cont.global_map, &type_map, "dump.html");

    if (diag.error_count > 0) {
        KTL_DiagFlush(&diag, NAME_FILE);
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
