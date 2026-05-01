#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdint.h>
#include <assert.h>

typedef uint64_t KTL_Hash;

enum KTL_Error {
    KTL_OK            =  0,
    KTL_MEMORY_ERR    = -1,
    KTL_BAD_ARG_ERR   = -2,
    KTL_LOGICAL_ERR   = -3,
};

// #define DEBUG

#define ExitF(__text__,__val__) \
 do { printf("ERROR[%s:%d]: %s\n", __FILE__, __LINE__, __text__); \
    return __val__; } while (0)

#ifdef DEBUG
    #define debug_out(_fmt_, ...) printf("[DEBUG] " _fmt_, ##__VA_ARGS__)
#else
    #define debug_out(fmt, ...) ((void)0)
#endif


constexpr int KTL_POINTER_SIZE = 8;

char *               ktl_sup_create_file_buffer(const char *file);
int                  ktl_sup_get_file_size     (const char *file);
constexpr KTL_Hash   ktl_gnu_hash              (const char *string) {
    assert(string);

    KTL_Hash hash = 5137;
    while (*string != '\0') {
        hash += (KTL_Hash) *string;
        hash *= 33;
        string++;
    }

    return hash;
}

#endif /* COMMON_H */
