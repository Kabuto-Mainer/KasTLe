#ifndef TOKEN_CONFIG_H
#define TOKEN_CONFIG_H

#include "TokenEnum.h"
#include "TokenType.h"

// =======================================================================
// Keys
// =======================================================================

#define XXX(__string__,__number__) {__string__, ktl_gnu_hash(__string__), __number__}

const KTL_KeyConstBlock KTL_KEY_WORDS[] = {
    XXX("main",     KTL_KEY_MAIN),

    XXX("if",       KTL_KEY_IF),
    XXX("elif",     KTL_KEY_ELIF),
    XXX("else",     KTL_KEY_ELSE),

    XXX("while",    KTL_KEY_WHILE),
    XXX("for",      KTL_KEY_FOR),
    XXX("break",    KTL_KEY_BREAK),
    XXX("continue", KTL_KEY_CONTINUE),

    XXX("return",   KTL_KEY_RETURN),
    XXX("in",       KTL_KEY_IN),
    XXX("out",      KTL_KEY_OUT),

    XXX("typedef",  KTL_KEY_TYPEDEF),
    XXX("block",    KTL_KEY_STRUCT),

    XXX("var",      KTL_KEY_VAR_DECL),
    XXX("func",     KTL_KEY_FUNC_DECL),

    XXX("cast",     KTL_KEY_CAST),
    XXX("exit",     KTL_KEY_EXIT),

    XXX("and",      KTL_KEY_AND),
    XXX("or",       KTL_KEY_OR),

    XXX("equal",            KTL_KEY_EQ),
    XXX("not_equal",        KTL_KEY_NEQ),
    XXX("less",             KTL_KEY_LT),
    XXX("greater",          KTL_KEY_GT),
    XXX("less_or_equal",    KTL_KEY_LE),
    XXX("greater_or_equal", KTL_KEY_GE),

    XXX("const",    KTL_KEY_CONST),
    XXX("mutable",  KTL_KEY_MUTABLE),
    XXX("resister", KTL_KEY_REGISTER),
    XXX("stack",    KTL_KEY_STACK),
};

#undef XXX

// =======================================================================
// One Symbol
// =======================================================================

#define XXX(__sym__,__number__) {__sym__, __number__}

const KTL_PunctConstBLock KTL_PUNCTS[] = {
    XXX(',',  KTL_PUNCT_COMMA),
    XXX('.',  KTL_PUNCT_DOT),
    XXX(';',  KTL_PUNCT_SEMICOLON),

    XXX('(',  KTL_PUNCT_LEFT_ROUND),
    XXX(')',  KTL_PUNCT_RIGHT_ROUND),
    XXX('{',  KTL_PUNCT_LEFT_FIGURE),
    XXX('}',  KTL_PUNCT_RIGHT_FIGURE),
    XXX('<',  KTL_PUNCT_LEFT_TRIG),
    XXX('>',  KTL_PUNCT_RIGHT_TRIG),
    XXX('[',  KTL_PUNCT_LEFT_SQUARE),
    XXX(']',  KTL_PUNCT_RIGHT_SQUARE),

    XXX('+',  KTL_PUNCT_PLUS),
    XXX('-',  KTL_PUNCT_MINUS),
    XXX('*',  KTL_PUNCT_MUL),
    XXX('/',  KTL_PUNCT_DEL),
    XXX('&',  KTL_PUNCT_AMP),
    XXX('!',  KTL_PUNCT_EXCL),

    XXX('#',  KTL_PUNCT_GRID),
    XXX('$',  KTL_PUNCT_DOLLAR),
    XXX('\n', KTL_PUNCT_DOLLAR),
    XXX('%',  KTL_PUNCT_PROCENT),
    XXX('=',  KTL_PUNCT_ASSIGN),
    XXX('\"', KTL_PUNCT_QUOT),
};

#undef XXX

// =======================================================================
// Two symbols
// =======================================================================

#define XXX(__s0__,__s1__,__number__) {{__s0__, __s1__}, __number__}

const KTL_PunctConst2Block KTL_PUNCTS_2[] = {
    XXX('=', '=', KTL_PUNCT_EQ),
    XXX('!', '=', KTL_PUNCT_NEQ),
    XXX('<', '=', KTL_PUNCT_LE),
    XXX('>', '=', KTL_PUNCT_GE),
    XXX('-', '>', KTL_PUNCT_ARROW),
};

#undef XXX

#endif /* TOKEN_CONFIG_H */
