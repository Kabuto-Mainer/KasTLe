#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
typedef uint64_t KTL_Hash;

enum KTL_Error {
    KTL_OK = 0,
    KTL_MEMORY_ERR = -1,
    KTL_BAD_ARG_ERR = -2,
    KTL_LOGICAL_ERR = -3,
};

#define ExitF(__text__,__val__) \
 do {printf("ERROR[%s:%d]: %s\n", __FILE__, __LINE__, __text__); \
    return __val__;} while (0)


char * ktl_sup_create_file_buffer(const char *file);
int ktl_sup_get_file_size(const char *file);
constexpr KTL_Hash ktl_gnu_hash(const char *string);


#endif /* COMMON_H */
