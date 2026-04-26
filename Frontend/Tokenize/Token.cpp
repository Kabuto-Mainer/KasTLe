#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "TokenType.h"
#include "TokenConfig.h"
#include "StrMap.h"
#include "Common.h"

constexpr static int KTL_START_TOKEN_SIZE = 10;
constexpr static int KTL_MODIFIER_TOKEN = 2;
constexpr static int KTL_SIZE_TAB = 4;
constexpr static int KTL_MAX_WORD_SIZE = 1024;

static inline char get_c(KTL_TokenContext *cont) {
    return cont->buffer[cont->cur_pose];
}

static inline char get_nc(KTL_TokenContext *cont) {
    return cont->buffer[cont->cur_pose + 1];
}


static KTL_TokenStatus ktl_token_string(KTL_TokenContext *cont);
static KTL_TokenStatus ktl_token_key(KTL_TokenContext *cont);
static KTL_TokenStatus ktl_token_num(KTL_TokenContext *cont);
static KTL_TokenStatus ktl_token_punct(KTL_TokenContext *cont);
static KTL_Error ktl_skip_void(KTL_TokenContext *cont);
KTL_Error ktl_add_token(KTL_TokenContext *cont, KTL_Token *token);




KTL_Error KTL_TokenInit(KTL_TokenContext *cont, const char *file) {
    assert(cont);
    assert(file);

    char *buffer = ktl_sup_create_file_buffer(file);
    if (buffer == NULL)  {
        ExitF("NULL Calloc", KTL_MEMORY_ERR);
    }

    int size = ktl_sup_get_file_size(file);
    cont->buffer = buffer;
    cont->capacity_buf = size;

    cont->cur_pose = 0;
    cont->file_pose.line = 0;
    cont->file_pose.column = 0;

    cont->tokens = (KTL_Token *)calloc(KTL_START_TOKEN_SIZE, sizeof(KTL_Token));
    if (cont->tokens == NULL) {
        free(buffer);
        ExitF("NULL Calloc", KTL_MEMORY_ERR);
    }

    cont->capacity_token = KTL_START_TOKEN_SIZE;
    cont->cur_token = 0;

    return KTL_OK;
}

void KTL_TokenAddStrMap(KTL_TokenContext *cont, KTL_StrMap *map) {
    assert(cont);
    assert(map);

    cont->str_map = map;
}

KTL_Error KTL_TokenProcess(KTL_TokenContext *cont) {
    assert(cont);

    while (true) {
        ktl_skip_void(cont);
        if (get_c(cont) == '\0') {
            break;
        }

        if (ktl_token_num(cont) == KTL_TOKEN_THIS_OK) {
            continue;
        }

        if (ktl_token_punct(cont) == KTL_TOKEN_THIS_OK) {
            continue;
        }

        if (ktl_token_key(cont) == KTL_TOKEN_THIS_OK) {
            continue;
        }

        if (ktl_token_string(cont) == KTL_TOKEN_THIS_OK) {
            continue;
        }
        ExitF("UNKNOWN SYNTAX", KTL_LOGICAL_ERR);
    }
    return KTL_OK;
}

KTL_Error KTL_TokenUninit(KTL_TokenContext *cont) {
    assert(cont);

    free(cont->buffer);

    return KTL_OK;
}




static KTL_TokenStatus ktl_token_string(KTL_TokenContext *cont) {
    assert(cont);

    char sym = get_c(cont);
    if (!isalpha(sym)) {
        return KTL_TOKEN_NOT_THIS;
    }

    char buffer[KTL_MAX_WORD_SIZE] = "";
    int len = 0;
    sscanf(cont->buffer + cont->cur_pose, "%s%n", buffer, &len);

    KTL_Token token = {};
    token.kind = KTL_TOKEN_STRING;
    token.data.string = KTL_StrMapFind(cont->str_map, buffer);
    token.pose = cont->file_pose;

    cont->cur_pose += len;
    cont->file_pose.column += len;

    ktl_add_token(cont, &token);
    return KTL_TOKEN_THIS_OK;
}


static KTL_TokenStatus ktl_token_key(KTL_TokenContext *cont) {
    assert(cont);

    char sym = get_c(cont);
    if (!isalpha(sym)) {
        return KTL_TOKEN_NOT_THIS;
    }

    char buffer[KTL_MAX_WORD_SIZE] = "";
    int len = 0;
    sscanf(cont->buffer + cont->cur_pose, "%s%n", buffer, &len);

    KTL_Hash hash = ktl_gnu_hash(buffer);

    for (int i = 0; i < sizeof(KTL_KEY_WORDS) / sizeof(KTL_KEY_WORDS[0]); i++) {
        if (KTL_KEY_WORDS[i].hash == hash &&
            strcmp(KTL_KEY_WORDS[i].key, buffer) == 0) {

            KTL_Token token = {};
            token.kind = KLT_TOKEN_KEY;
            token.data.key = KTL_KEY_WORDS[i].value;
            token.pose = cont->file_pose;

            cont->file_pose.column += len;
            cont->cur_pose += len;

            ktl_add_token(cont, &token);

            return KTL_TOKEN_THIS_OK;
        }
    }
    return KTL_TOKEN_NOT_THIS;
}


static KTL_TokenStatus ktl_token_num(KTL_TokenContext *cont) {
    assert(cont);

    char sym = get_c(cont);
    if (!isdigit(sym) && !(sym == '-' && isdigit(get_nc(cont)))) {
        return KTL_TOKEN_NOT_THIS;
    }

    int64_t value = 0;
    int len = 0;
    sscanf(cont->buffer + cont->cur_pose, "%ld%n", &value, &len);

    cont->cur_pose += len;

    KTL_Token token = {};
    token.kind = KTL_TOKEN_VALUE;
    token.data.value = value;
    token.pose = cont->file_pose;

    cont->file_pose.column += len;
    ktl_add_token(cont, &token);

    return KTL_TOKEN_THIS_OK;
}


static KTL_TokenStatus ktl_token_punct(KTL_TokenContext *cont) {
    assert(cont);

    char sym = get_c(cont);

    for (int i = 0; i < sizeof(KTL_PUNCTS) / sizeof(KTL_PUNCTS[0]); i++) {
        if (KTL_PUNCTS[i].sym == sym) {
            KTL_Token token = {};
            token.kind = KTL_TOKEN_PUNCT;
            token.data.punct = KTL_PUNCTS[i].value;
            token.pose = cont->file_pose;

            ktl_add_token(cont, &token);
            cont->file_pose.column++;
            cont->cur_pose++;

            if (KTL_PUNCTS[i].sym == '\n') {
                cont->file_pose.column = 0;
                cont->file_pose.line += 1;
            }

            return KTL_TOKEN_THIS_OK;
        }
    }
    return KTL_TOKEN_NOT_THIS;
}


static KTL_Error ktl_skip_void(KTL_TokenContext *cont) {
    assert(cont);

    while (get_c(cont) == ' ' ||
           get_c(cont) == '\t') {
        cont->file_pose.column++;
        cont->cur_pose++;
    }

    return KTL_OK;
}


KTL_Error ktl_add_token(KTL_TokenContext *cont, KTL_Token *token) {
    assert(cont);

    if (cont->cur_token + 1 == cont->capacity_token) {
        int new_size = cont->capacity_token * KTL_MODIFIER_TOKEN;

        KTL_Token *buf = (KTL_Token *)realloc(cont->tokens, new_size * sizeof(KTL_Token));
        if (buf == NULL) {
            ExitF("NULL Calloc", KTL_MEMORY_ERR);
        }
        cont->tokens = buf;
    }

    cont->tokens[cont->cur_token++] = *token;
    return KTL_OK;
}

