#ifndef GENERATION_CODE_H
#define GENERATION_CODE_H

#include <stdint.h>

#define BYTE constexpr uint8_t

BYTE MOV_RM_64_F8_REG_PREFIX = 0x48;
BYTE MOV_RM_8_L8_REG_PREFIX  = 0x44;
BYTE MOV_RM_16_L8_REG_PREFIX = 0x44;
BYTE MOV_RM_32_L8_REG_PREFIX = 0x44;
BYTE MOV_RM_64_L8_REG_PREFIX = 0x4c;


BYTE MOV_RM_64_OP          = 0x8b;
BYTE MOV_RM_32_OP          = 0x8b;
BYTE MOV_RM_16_OP          = 0x8b;
BYTE MOV_RM_8_OP           = 0x8a;


#include <elf.h>
#endif /* GENERATION_CODE_H */
