#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/stat.h>
#include <unistd.h>

#include "Common.h"

static KTL_Process CURRENT_STAGE = {0};
static const char* STAGE_TEXT[] = {
    "[  0%] START TOKEN PROCESS\n",
    "[ 20%] START PARSING\n",
    "[ 40%] START ANALYZE\n",
    "[ 60%] START CODE GENERATION\n",
    "[ 80%] START GENERATION ELF\n",
    "[100%] END COMPILATION\n"
};


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

void KTL_NextStage() {
    CURRENT_STAGE.stage++;
}

void KTL_PrintStage() {
    printf("%s", STAGE_TEXT[CURRENT_STAGE.stage]);
}
