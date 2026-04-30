#include "Token.h"
#include "StrMap.h"
#include "Diagnostic.h"

int main() {
    KTL_TokenContext cont = {};
    KTL_StrMap str_map = {};

    KTL_TokenInit(&cont, "DataTests/1.txt");
    KTL_StrMapCreate(&str_map, 10);

    KTL_TokenAddStrMap(&cont, &str_map);

    KTL_Diagnostic diag = {};
    KTL_DiagCreate(&diag, 10);
    KTL_TokenAddDiag(&cont, &diag);

    KTL_TokenProcess(&cont);
    KTL_TokenDump(&cont);

    KTL_TokenUninit(&cont);
    KTL_StrMapDestroy(&str_map);
    KTL_DiagDestroy(&diag);

    return 0;
}
