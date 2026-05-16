section .data
    msg db "Hello, World!", 0x0A
    txt db "Poltorashka", 0x0A
    len dq txt - msg

section .bss
    buffer resb 64                 ; Reserve 64 bytes

section .text
    global _start                  ; Entry point for linker

_start:
    mov rax, 1                    ; sys_write
    mov rdi, 1                    ; file descriptor (stdout)
    mov rsi, msg                  ; message address
    mov rdx, len                  ; message length
    syscall

    ; Example: Syscall to exit
    mov rax, 60                   ; sys_exit
    xor rdi, rdi                  ; exit code 0
    syscall
->
