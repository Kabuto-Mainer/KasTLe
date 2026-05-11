#ifndef STD_FUNC_H
#define STD_FUNC_H

#include "Token.h"
#include "StdType.h"

#define _VOID     {KTL_VOID_TYPE_ID,  0}


#define _I8       {KTL_I8_TYPE_ID,  0}
#define _I16      {KTL_I16_TYPE_ID, 0}
#define _I32      {KTL_I32_TYPE_ID, 0}
#define _I64      {KTL_I64_TYPE_ID, 0}

#define _I8_ptr   {KTL_I8_TYPE_ID,  1}
#define _I16_ptr  {KTL_I16_TYPE_ID, 1}
#define _I32_ptr  {KTL_I32_TYPE_ID, 1}
#define _I64_ptr  {KTL_I64_TYPE_ID, 1}


static const KTL_StandardFunc_Gen KTL_STANDARD_FUNC_INFO[] = {
    { "print", "printf", _I32,     { _I8_ptr   }, 1, true  },
    { "scan",  "scanf",  _I32,     { _I8_ptr,  }, 1, true  },
    { "alloc", "calloc", _I64_ptr, { _I64,_I64 }, 2, false },
    { "free",  "free",   _VOID,    { _I64_ptr  }, 1, false }
};

const int KTL_AMOUNT_STD_FUNCTION = sizeof(KTL_STANDARD_FUNC_INFO) / sizeof(KTL_STANDARD_FUNC_INFO[0]);

static const char *KTL_STANDARD_NAME_PARAM[KTL_STANDARD_MAX_PARAM] = {
    "p1", "p2", "p3", "p4", "p5", "p6"
};

#endif /* STD_FUNC_H */
