#ifndef CALLING_CONVENTION_H
#define CALLING_CONVENTION_H

/**
 * 0  - rax
 * 1  - rbx
 * 2  - rcx
 * 3  - rdx
 * 4  - rsi
 * 5  - rdi
 * 6  - rbp
 * 7  - rsp
 * 8  - r8
 * 9  - r9
 * 10 - r10
 * 11 - r11
 * 12 - r12
 * 13 - r13
 * 14 - r14
 * 15 - r15
 */

constexpr int KTL_SYSTEM_AMOUNT_REGISTERS  = 16;
constexpr int KTL_SYSTEM_PTR_SIZE          = 8;

constexpr int KTL_SYSTEM_PARAM_REGISTERS[] =
    {5, 4, 3, 2, 8, 9};
constexpr int KTL_SYSTEM_CALLER_SAVE_REGISTERS[] =
    {0, 2, 3, 4, 5, 8, 9, 10, 11};
constexpr int KTL_SYSTEM_CALLEE_SAVE_REGISTERS[] =
    {1, 6, 7, 12, 13, 14, 15};
constexpr int KTL_SYSTEM_ARITHMETIC_REG = 0;



#endif /* CALLING_CONVENTION_H */
