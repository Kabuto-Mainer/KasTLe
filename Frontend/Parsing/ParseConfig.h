#ifndef PARSE_CONFIG_H
#define PARSE_CONFIG_H

#include "TokenType.h"

#define KTL_PARSE_PUNCT(__p__) \
    KTL_ParseTokenRef{ KTL_TOKEN_PUNCT, { .punct = (__p__) } }

#define KTL_PARSE_KEY(__k__) \
    KTL_ParseTokenRef{ KTL_TOKEN_KEY, { .key = (__k__) } }

/* --- Структурные элементы  --- */
constexpr KTL_ParseTokenRef KTL_PARSE_END_LINE     = KTL_PARSE_PUNCT(KTL_PUNCT_SEMICOLON);
constexpr KTL_ParseTokenRef KTL_PARSE_BLOCK_LEFT   = KTL_PARSE_PUNCT(KTL_PUNCT_LEFT_FIGURE);
constexpr KTL_ParseTokenRef KTL_PARSE_BLOCK_RIGHT  = KTL_PARSE_PUNCT(KTL_PUNCT_RIGHT_FIGURE);
constexpr KTL_ParseTokenRef KTL_PARSE_PAREN_LEFT   = KTL_PARSE_PUNCT(KTL_PUNCT_LEFT_ROUND);
constexpr KTL_ParseTokenRef KTL_PARSE_PAREN_RIGHT  = KTL_PARSE_PUNCT(KTL_PUNCT_RIGHT_ROUND);
constexpr KTL_ParseTokenRef KTL_PARSE_INDEX_LEFT   = KTL_PARSE_PUNCT(KTL_PUNCT_LEFT_SQUARE);
constexpr KTL_ParseTokenRef KTL_PARSE_INDEX_RIGHT  = KTL_PARSE_PUNCT(KTL_PUNCT_RIGHT_SQUARE);
constexpr KTL_ParseTokenRef KTL_PARSE_ARG_SEP      = KTL_PARSE_PUNCT(KTL_PUNCT_COMMA);
constexpr KTL_ParseTokenRef KTL_PARSE_FIELD_DOT    = KTL_PARSE_PUNCT(KTL_PUNCT_DOT);
constexpr KTL_ParseTokenRef KTL_PARSE_FIELD_ARROW  = KTL_PARSE_PUNCT(KTL_PUNCT_ARROW);

/* --- Унарные операторы памяти --- */
constexpr KTL_ParseTokenRef KTL_PARSE_PTR_ADDR     = KTL_PARSE_PUNCT(KTL_PUNCT_AMP);
constexpr KTL_ParseTokenRef KTL_PARSE_PTR_DEREF    = KTL_PARSE_PUNCT(KTL_PUNCT_MUL);
constexpr KTL_ParseTokenRef KTL_PARSE_TYPE_PTR     = KTL_PARSE_PUNCT(KTL_PUNCT_MUL);

/* --- Арифметика --- */
constexpr KTL_ParseTokenRef KTL_PARSE_OP_PLUS      = KTL_PARSE_PUNCT(KTL_PUNCT_PLUS);
constexpr KTL_ParseTokenRef KTL_PARSE_OP_MINUS     = KTL_PARSE_PUNCT(KTL_PUNCT_MINUS);
constexpr KTL_ParseTokenRef KTL_PARSE_OP_MUL       = KTL_PARSE_PUNCT(KTL_PUNCT_MUL);
constexpr KTL_ParseTokenRef KTL_PARSE_OP_DIV       = KTL_PARSE_PUNCT(KTL_PUNCT_DEL);
constexpr KTL_ParseTokenRef KTL_PARSE_OP_MOD       = KTL_PARSE_PUNCT(KTL_PUNCT_PROCENT);
constexpr KTL_ParseTokenRef KTL_PARSE_OP_NEG       = KTL_PARSE_PUNCT(KTL_PUNCT_MINUS);

/* --- Сравнения --- */
constexpr KTL_ParseTokenRef KTL_PARSE_OP_EQ  = KTL_PARSE_PUNCT(KTL_PUNCT_EQ);
constexpr KTL_ParseTokenRef KTL_PARSE_OP_NEQ = KTL_PARSE_PUNCT(KTL_PUNCT_NEQ);
constexpr KTL_ParseTokenRef KTL_PARSE_OP_LT  = KTL_PARSE_PUNCT(KTL_PUNCT_LEFT_TRIG);
constexpr KTL_ParseTokenRef KTL_PARSE_OP_GT  = KTL_PARSE_PUNCT(KTL_PUNCT_RIGHT_TRIG);
constexpr KTL_ParseTokenRef KTL_PARSE_OP_LE  = KTL_PARSE_PUNCT(KTL_PUNCT_LE);
constexpr KTL_ParseTokenRef KTL_PARSE_OP_GE  = KTL_PARSE_PUNCT(KTL_PUNCT_GE);

/* --- Логика --- */
constexpr KTL_ParseTokenRef KTL_PARSE_OP_AND = KTL_PARSE_KEY(KTL_KEY_AND);
constexpr KTL_ParseTokenRef KTL_PARSE_OP_OR  = KTL_PARSE_KEY(KTL_KEY_OR);

/* --- Присваивание --- */
constexpr KTL_ParseTokenRef KTL_PARSE_ASSIGN = KTL_PARSE_PUNCT(KTL_PUNCT_ASSIGN);

/* --- Typedef --- */
constexpr KTL_ParseTokenRef KTL_PARSE_TYPEDEF_ARROW = KTL_PARSE_PUNCT(KTL_PUNCT_ARROW);

/* --- Псевдонимы по контексту --- */
constexpr KTL_ParseTokenRef KTL_PARSE_STRUCT_LEFT  = KTL_PARSE_BLOCK_LEFT;
constexpr KTL_ParseTokenRef KTL_PARSE_STRUCT_RIGHT = KTL_PARSE_BLOCK_RIGHT;

/* --- Строковые литералы --- */
constexpr KTL_ParseTokenRef KTL_PARSE_STR_LITERAL_LEFT = KTL_PARSE_PUNCT(KTL_PUNCT_QUOT);
constexpr KTL_ParseTokenRef KTL_PARSE_STR_LITERAL_RIGHT = KTL_PARSE_PUNCT(KTL_PUNCT_QUOT);


#endif /* PARSE_CONFIG_H */
