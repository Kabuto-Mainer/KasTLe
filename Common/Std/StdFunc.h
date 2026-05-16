#ifndef STD_FUNC_H
#define STD_FUNC_H

#include "Token.h"
#include "StdType.h"

const KTL_StandardFunc_Gen KTL_STANDARD_FUNC_INFO[] = {
    #include "StdFuncData.h"
};

const char *KTL_STANDARD_NAME_PARAM[KTL_STANDARD_MAX_PARAM] = {
    "p1", "p2", "p3", "p4", "p5", "p6"
};

#endif /* STD_FUNC_H */
