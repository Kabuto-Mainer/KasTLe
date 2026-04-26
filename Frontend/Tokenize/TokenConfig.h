#ifndef TOKEN_CONFIG_H
#define TOKEN_CONFIG_H

#include "TokenEnum.h"
#include "TokenType.h"


#define XXX(__string__,__number__) {__string__, ktl_gnu_hash(__string__), __number__}

const KTL_KeyConstBlock KTL_KEY_WORDS[] = {
    XXX("if", KTL_KEY_IF),
    XXX("elif", KTL_KEY_ELIF),
    XXX("else", KTL_KEY_ELSE),
    XXX("while", KTL_KEY_WHILE),
    XXX("for", KTL_KEY_FOR),
    XXX("break", KTL_KEY_BREAK),
    XXX("continue", KTL_KEY_CONTINUE),
    XXX("return", KTL_KEY_RETURN),
    XXX("=", KTL_KEY_ASSIGN),
    XXX(",in", KTL_KEY_IN),
    XXX("out", KTL_KEY_OUT),
    XXX("typedef", KTL_KEY_TYPEDEF),
    XXX("block", KTL_KEY_STRUCT),
    XXX("var", KTL_KEY_VAR_DECL),
    XXX("func", KTL_KEY_FUNC_DECL),
};

#define XXX(__sym__,__number__) {__sym__, __number__}

const KTL_PunctConstBLock KTL_PUNCTS[] = {
    XXX(',', KTL_PUNCT_COMMA),
    XXX('.', KTL_PUNCT_DOT),
    XXX('(', KTL_PUNCT_LEFT_ROUND),
    XXX(')', KTL_PUNCT_RIGHT_ROUND),
    XXX('{', KTL_PUNCT_LEFT_FIGURE),
    XXX('}', KTL_PUNCT_RIGHT_FIGURE),
    XXX('<', KTL_PUNCT_LEFT_TRIG),
    XXX('>', KTL_PUNCT_RIGHT_TRIG),
    XXX('+', KTL_PUNCT_PLUS),
    XXX('-', KTL_PUNCT_MINUS),
    XXX('*', KTL_PUNCT_MUL),
    XXX('/', KTL_PUNCT_DEL),
    XXX('#', KTL_PUNCT_GRID),
    XXX('$', KTL_PUNCT_DOLLAR),
    XXX('\n', KTL_PUNCT_NEXT_STR),
    XXX('%', KTL_PUNCT_PROCENT),
};

#undef XXX


#endif /* TOKEN_CONFIG_H */
