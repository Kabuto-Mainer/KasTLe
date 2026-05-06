#ifndef OPCODE_INFO_H
#define OPCODE_INFO_H

#include <stdint.h>

struct KTL_Opcode {
    int8_t  len;
    uint8_t data[8];
};

#define COUNT_ARGS(...) COUNT_ARGS_HELPER(__VA_ARGS__, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1)
#define COUNT_ARGS_HELPER(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, N, ...) N
#define XXX( ...) { COUNT_ARGS(__VA_ARGS__), { __VA_ARGS__ } }

const KTL_Opcode OPCODES_NO_ARG[] = {
XXX(0x48, 0x98),    // cdqe
XXX(0x48, 0x99),    // cqo
XXX(0xF3, 0xAA),    // rep stosb
XXX(0xF3, 0xA4),    // rep movsb
XXX(0xC3),
XXX(0x90),
XXX(0x0F, 0x05)
};



#endif /* OPCODE_INFO_H */
