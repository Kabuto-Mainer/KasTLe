#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/stat.h>
#include <unistd.h>

#include "Common.h"

constexpr int KTL_CHAR_BUFFER_INIT_SIZE = 8;
constexpr int KTL_CHAR_BUFFER_GROW_MOD  = 2;


char * ktl_sup_create_file_buffer(const char *file) {
    assert(file);

    FILE *stream = fopen(file, "rb");
    if (stream == NULL)     ExitF("NULL File", NULL);

    int size = ktl_sup_get_file_size(file);

    char *buffer = (char *)calloc((size_t)size + 1, sizeof(char));
    if (buffer == NULL) {
        fclose(stream);
        ExitF("NULL Calloc", NULL);
    }

    size_t read = fread(buffer, sizeof(char), (size_t)size, stream);
    buffer[read] = '\0';
    fclose(stream);

    return buffer;
}

int ktl_sup_get_file_size(const char *file) {
    assert(file);

    struct stat file_stat = {};
    if (stat(file, &file_stat) == -1)   ExitF("Bad Stat", 0);

    return (int) file_stat.st_size;
}

KTL_Error KTL_CharBufferInit(KTL_CharBuffer *buf) {
    assert(buf);

    buf->data = (char *)calloc(KTL_CHAR_BUFFER_INIT_SIZE, sizeof(char));
    if (buf->data == NULL)  ExitF("NULL Calloc", KTL_MEMORY_ERR);

    buf->capacity = KTL_CHAR_BUFFER_INIT_SIZE;
    buf->pos      = 0;

    return KTL_OK;
}

KTL_Error KTL_CharBuffer
