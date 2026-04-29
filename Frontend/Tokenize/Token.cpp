#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "TokenType.h"
#include "TokenConfig.h"
#include "Token.h"
#include "StrMap.h"
#include "Diagnostic.h"
#include "Common.h"

constexpr static int KTL_START_TOKEN_SIZE = 16;
constexpr static int KTL_TOKEN_GROW_MOD   = 2;
constexpr static int KTL_MAX_WORD_SIZE    = 1024;
constexpr static int KTL_MAX_STR_LITERAL  = 4096;

// =======================================================================
// HELPER FUNCTIONS DECLARATIONS
// =======================================================================

// static inline void     ktl_dump_buffer    (KTL_TokenContext *cont);
static KTL_TokenStatus ktl_token_word     (KTL_TokenContext *cont);
static KTL_TokenStatus ktl_token_number   (KTL_TokenContext *cont);
static KTL_TokenStatus ktl_token_punct    (KTL_TokenContext *cont);
static KTL_TokenStatus ktl_token_str_lit  (KTL_TokenContext *cont);

static KTL_Error       ktl_skip_trivia    (KTL_TokenContext *cont);
static KTL_Error       ktl_add_token      (KTL_TokenContext *cont,
                                           const KTL_Token  *token);

static inline bool     ktl_is_id_start    (char c);
static inline bool     ktl_is_id_cont     (char c);

static inline char     get_c              (KTL_TokenContext *cont);
static inline char     get_nc             (KTL_TokenContext *cont);
static inline void     advance            (KTL_TokenContext *cont);
static inline void     advance_n          (KTL_TokenContext *cont, int n);
static inline void     new_line           (KTL_TokenContext *cont);

// =======================================================================
// API FUNCTIONS
// =======================================================================

KTL_Error KTL_TokenInit(KTL_TokenContext *cont, const char *file) {
    assert(cont);
    assert(file);

    char *buffer = ktl_sup_create_file_buffer(file);
    if (buffer == NULL)  ExitF("NULL Calloc", KTL_MEMORY_ERR);

    int size = ktl_sup_get_file_size(file);

    cont->buffer            = buffer;
    cont->buffer_capacity   = size;
    cont->buffer_pos        = 0;

    cont->source_pos.line   = 0;
    cont->source_pos.column = 0;

    cont->tokens = (KTL_Token *)calloc(KTL_START_TOKEN_SIZE, sizeof(KTL_Token));
    if (cont->tokens == NULL) {
        free(buffer);
        ExitF("NULL Calloc", KTL_MEMORY_ERR);
    }

    cont->token_capacity    = KTL_START_TOKEN_SIZE;
    cont->token_pos         = 0;

    cont->str_map           = NULL;
    cont->diag              = NULL;

    return KTL_OK;
}

void KTL_TokenAddStrMap(KTL_TokenContext *cont, KTL_StrMap *map) {
    assert(cont);
    assert(map);

    cont->str_map = map;
}

void KTL_TokenAddDiag(KTL_TokenContext *cont, KTL_Diagnostic *diag) {
    assert(cont);
    assert(diag);

    cont->diag = diag;
}

KTL_Error KTL_TokenProcess(KTL_TokenContext *cont) {
    assert(cont);
    assert(cont->str_map);
    assert(cont->diag);

    while (true) {
        ktl_skip_trivia(cont);
        // ktl_dump_buffer(cont);
        if (get_c(cont) == '\0')  break;

        if (ktl_token_number  (cont) == KTL_TOKEN_THIS_OK)  continue;
        if (ktl_token_str_lit (cont) == KTL_TOKEN_THIS_OK)  continue;
        if (ktl_token_word    (cont) == KTL_TOKEN_THIS_OK)  continue;
        if (ktl_token_punct   (cont) == KTL_TOKEN_THIS_OK)  continue;

        KTL_DiagEmit(cont->diag, cont->source_pos,
                     KTL_DIAG_LEX_UNKNOWN_CHAR,
                     KTL_DIAG_SEV_ERROR);
        advance(cont);
    }

    /* Create buffer with eof tokens */
    KTL_Token eof_token = {};
    eof_token.pos  = cont->source_pos;
    eof_token.kind = KTL_TOKEN_EOF;

    for (int i = 0; i < 10; i++) {
        ktl_add_token(cont, &eof_token);
    }

    return KTL_OK;
}

KTL_Error KTL_TokenUninit(KTL_TokenContext *cont) {
    assert(cont);

    free(cont->buffer);
    free(cont->tokens);
    cont->buffer = NULL;
    cont->tokens = NULL;

    return KTL_OK;
}

void KTL_TokenDump(KTL_TokenContext *cont) {
    assert(cont);

    for (int i = 0; i < cont->token_pos; i++) {
        printf("[%3d]", i);

        KTL_Token *token = cont->tokens + i;
        switch (token->kind) {
            case KTL_TOKEN_KEY: {
                printf("KEY WORD|%s=[%d]\n",
                        KTL_KEY_WORDS[token->data.key].string,
                        token->data.key);
                break;
            }
            case KTL_TOKEN_STRING: {
                printf("STRING  |%s\n",
                        token->data.string);
                break;
            }
            case KTL_TOKEN_VALUE: {
                printf("VALUE   |%ld\n",
                        token->data.value);
                break;
            }
            case KTL_TOKEN_STR_LITERAL: {
                printf("STR LIT |%s\n",
                        token->data.str_literal);
                break;
            }
            case KTL_TOKEN_PUNCT: {
                if (token->data.punct >= KTL_PUNCT_FIRST_WITH_TWO_SYM) {
                    printf("PUNCT   |%c%c\n",
                            KTL_PUNCTS_2[token->data.punct - KTL_PUNCT_FIRST_WITH_TWO_SYM].sym[0],
                            KTL_PUNCTS_2[token->data.punct - KTL_PUNCT_FIRST_WITH_TWO_SYM].sym[1]);
                }
                else {
                    printf("PUNCT   |%c\n",
                            KTL_PUNCTS[token->data.punct].sym);
                }
                break;
            }
            case KTL_TOKEN_EOF: {
                printf("EOF    |\n");
                break;
            }

            default: {
                printf("ERROR   |\n");
            }
        }
    }
}

// =======================================================================
// HELPER FUNCTIONS
// =======================================================================

// static inline void ktl_dump_buffer(KTL_TokenContext *cont) {
//     assert(cont);
//
//     printf("=================\n%s\n=================\n", cont->buffer + cont->buffer_pos);
// }

static KTL_TokenStatus ktl_token_word(KTL_TokenContext *cont) {
    assert(cont);

    char sym = get_c(cont);
    if (!ktl_is_id_start(sym))  return KTL_TOKEN_NOT_THIS;

    KTL_SourcePos start_pos = cont->source_pos;
    char buffer[KTL_MAX_WORD_SIZE] = "";
    int  len = 0;

    while (ktl_is_id_cont(get_c(cont)) && len + 1 < KTL_MAX_WORD_SIZE) {
        buffer[len++] = get_c(cont);
        advance(cont);
    }
    buffer[len] = '\0';

    /* Check Key Word */
    KTL_Hash hash = ktl_gnu_hash(buffer);
    int kw_count  = sizeof(KTL_KEY_WORDS) / sizeof(KTL_KEY_WORDS[0]);

    for (int i = 0; i < kw_count; i++) {
        if (KTL_KEY_WORDS[i].hash == hash &&
            strcmp(KTL_KEY_WORDS[i].string, buffer) == 0) {

            KTL_Token tok = {};
            tok.kind        = KTL_TOKEN_KEY;
            tok.data.key    = KTL_KEY_WORDS[i].value_key;
            tok.pos         = start_pos;
            ktl_add_token(cont, &tok);
            return KTL_TOKEN_THIS_OK;
        }
    }

    /* Add String */
    KTL_Token tok = {};
    tok.kind        = KTL_TOKEN_STRING;
    tok.data.string = KTL_StrMapFind(cont->str_map, buffer);
    tok.pos         = start_pos;
    ktl_add_token(cont, &tok);

    return KTL_TOKEN_THIS_OK;
}

static KTL_TokenStatus ktl_token_number(KTL_TokenContext *cont) {
    assert(cont);

    char sym = get_c(cont);

    if (!isdigit((unsigned char) sym))  return KTL_TOKEN_NOT_THIS;

    KTL_SourcePos start_pos = cont->source_pos;
    int64_t value = 0;
    int     read  = 0;

    sscanf(cont->buffer + cont->buffer_pos, "%lld%n", (long long *)&value, &read);
    if (read <= 0) {
        KTL_DiagEmit(cont->diag, start_pos,
                     KTL_DIAG_LEX_BAD_NUMBER,
                     KTL_DIAG_SEV_ERROR);
        advance(cont);
        return KTL_TOKEN_ERROR;
    }

    advance_n(cont, read);

    KTL_Token tok  = {};
    tok.kind       = KTL_TOKEN_VALUE;
    tok.data.value = value;
    tok.pos        = start_pos;
    ktl_add_token(cont, &tok);

    return KTL_TOKEN_THIS_OK;
}

static KTL_TokenStatus ktl_token_str_lit(KTL_TokenContext *cont) {
    assert(cont);
    if (get_c(cont) != '"')  return KTL_TOKEN_NOT_THIS;

    KTL_SourcePos start_pos = cont->source_pos;
    advance(cont);  // skip '"'

    char  buffer[KTL_MAX_STR_LITERAL] = "";
    int   len               = 0;
    bool  overflow_reported = false;

    while (true) {
        char sym = get_c(cont);
        if (sym == '\0') {
            KTL_DiagEmit(cont->diag, start_pos,
                         KTL_DIAG_LEX_UNTERMINATED_STR,
                         KTL_DIAG_SEV_ERROR);
            return KTL_TOKEN_ERROR;
        }
        if (sym == '"') {
            advance(cont);
            break;
        }
        if (sym == '\n')  new_line(cont);

        if (sym == '\\') {
            advance(cont);
            char esc     = get_c(cont);
            char decoded = esc;        /* fallback: пропускаем сырой символ */
            switch (esc) {
                case 'n':  decoded = '\n'; break;
                case 't':  decoded = '\t'; break;
                case 'r':  decoded = '\r'; break;
                case '\\': decoded = '\\'; break;
                case '"':  decoded = '"';  break;
                case '0':  decoded = '\0'; break;
                default:
                    KTL_DiagEmit(cont->diag, cont->source_pos,
                                 KTL_DIAG_LEX_UNKNOWN_CHAR,
                                 KTL_DIAG_SEV_ERROR);
                    break;
            }

            if (len + 1 < KTL_MAX_STR_LITERAL) {
                buffer[len++] = decoded;
            } else if (!overflow_reported) {
                KTL_DiagEmit(cont->diag, start_pos,
                             KTL_DIAG_LEX_UNTERMINATED_STR,
                             KTL_DIAG_SEV_ERROR);
                overflow_reported = true;
            }
            advance(cont);
            continue;
        }

        if (len + 1 < KTL_MAX_STR_LITERAL) {
            buffer[len++] = sym;
        } else if (!overflow_reported) {
            KTL_DiagEmit(cont->diag, start_pos,
                         KTL_DIAG_LEX_UNTERMINATED_STR,
                         KTL_DIAG_SEV_ERROR);
            overflow_reported = true;
        }
        advance(cont);
    }
    buffer[len] = '\0';

    KTL_Token tok = {};
    tok.kind             = KTL_TOKEN_STR_LITERAL;
    tok.data.str_literal = KTL_StrMapFind(cont->str_map, buffer);
    tok.pos              = start_pos;
    ktl_add_token(cont, &tok);

    return KTL_TOKEN_THIS_OK;
}

static KTL_TokenStatus ktl_token_punct(KTL_TokenContext *cont) {
    assert(cont);

    char sym    = get_c(cont);
    char n_sym  = get_nc(cont);

    KTL_SourcePos start_pos = cont->source_pos;

    /* 2 symbols punct */
    int amount_2 = sizeof(KTL_PUNCTS_2) / sizeof(KTL_PUNCTS_2[0]);
    for (int i = 0; i < amount_2; i++) {
        if (KTL_PUNCTS_2[i].sym[0] == sym &&
            KTL_PUNCTS_2[i].sym[1] == n_sym) {

            KTL_Token tok = {};
            tok.kind       = KTL_TOKEN_PUNCT;
            tok.data.punct = KTL_PUNCTS_2[i].value_punct;
            tok.pos        = start_pos;
            ktl_add_token(cont, &tok);
            advance_n(cont, 2);

            return KTL_TOKEN_THIS_OK;
        }
    }

    /* 1 symbol punct */
    int amount_1 = sizeof(KTL_PUNCTS) / sizeof(KTL_PUNCTS[0]);
    for (int i = 0; i < amount_1; i++) {
        if (KTL_PUNCTS[i].sym == sym) {
            KTL_Token tok = {};
            tok.kind       = KTL_TOKEN_PUNCT;
            tok.data.punct = KTL_PUNCTS[i].value_punct;
            tok.pos        = start_pos;
            ktl_add_token(cont, &tok);
            advance(cont);

            return KTL_TOKEN_THIS_OK;
        }
    }
    return KTL_TOKEN_NOT_THIS;
}

static KTL_Error ktl_skip_trivia(KTL_TokenContext *cont) {
    assert(cont);

    while (true) {
        char sym    = get_c(cont);
        char n_sym  = get_nc(cont);

        if (sym == ' ' || sym == '\t' || sym == '\r') {
            advance(cont);
            continue;
        }
        if (sym == '\n') {
            new_line(cont);
            continue;
        }

        if (sym == '/' && n_sym == '/') {
            advance_n(cont, 2);
            while (get_c(cont) != '\n' && get_c(cont) != '\0') {
                advance(cont);
            }
            continue;
        }

        if (sym == '/' && n_sym == '*') {
            KTL_SourcePos start_pos = cont->source_pos;
            advance_n(cont, 2);
            while (true) {
                char cur = get_c(cont);
                if (cur == '\0') {
                    KTL_DiagEmit(cont->diag, start_pos,
                                 KTL_DIAG_LEX_UNTERMINATED_STR,
                                 KTL_DIAG_SEV_ERROR);
                    return KTL_OK;
                }
                if (cur == '*' && get_nc(cont) == '/') {
                    advance_n(cont, 2);
                    break;
                }
                if (cur == '\n')    new_line(cont);
                else                advance(cont);
            }
            continue;
        }

        break;
    }
    return KTL_OK;
}

// =======================================================================
// LITTLE HELPERS
// =======================================================================

static inline bool ktl_is_id_start(char c) {
    return (isalpha((unsigned char) c) != 0) || c == '_';
}

static inline bool ktl_is_id_cont(char c) {
    return (isalnum((unsigned char) c) != 0) || c == '_';
}

static inline char get_c(KTL_TokenContext *cont) {
    return cont->buffer[cont->buffer_pos];
}

static inline char get_nc(KTL_TokenContext *cont) {
    if (cont->buffer[cont->buffer_pos] == '\0')  return '\0';
    return cont->buffer[cont->buffer_pos + 1];
}

static inline void advance(KTL_TokenContext *cont) {
    cont->buffer_pos++;
    cont->source_pos.column++;
}

static inline void advance_n(KTL_TokenContext *cont, int n) {
    cont->buffer_pos        += n;
    cont->source_pos.column += n;
}

static inline void new_line(KTL_TokenContext *cont) {
    cont->buffer_pos++;
    cont->source_pos.line++;
    cont->source_pos.column = 0;
}

static KTL_Error ktl_add_token(KTL_TokenContext *cont, const KTL_Token *token) {
    assert(cont);
    assert(token);

    if (cont->token_pos + 1 >= cont->token_capacity) {
        int new_cap = cont->token_capacity * KTL_TOKEN_GROW_MOD;
        KTL_Token *buf = (KTL_Token *)realloc(cont->tokens,
                            (size_t) new_cap * sizeof(KTL_Token));
        if (buf == NULL)  ExitF("NULL Realloc", KTL_MEMORY_ERR);

        cont->tokens         = buf;
        cont->token_capacity = new_cap;
    }
    cont->tokens[cont->token_pos++] = *token;

    return KTL_OK;
}
