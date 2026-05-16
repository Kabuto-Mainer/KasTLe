section .data
section .rodata
	__string__0x7b93ddbe00d0:
	0x2a0x0
	__string__0x7b93ddbe00f0:
	0x5f0x0
	__string__0x7b93ddbe0110:
	0xa0x0
section .txt
	_start:
	push  rbp
	mov  rbp  rsp
	sub  rsp  b_ 0x28
	mov  rax  b_ 0x0
	cdqe
	mov  byte [rbp + 0xffffffe8]rax
	.L1:
	lea  rax  byte [rbp + 0xffffffe8]
	mov  rax  byte [rax + 0x0]
	push  rax
	mov  rax  b_ 0xb
	pop  rdi
	cmp  rdi  rax
	setl  al
	movzx  rax  al
	test  rax  rax
	jz  .L0:
	mov  rax  b_ 0x0
	cdqe
	mov  byte [rbp + 0xffffffe0]rax
	.L3:
	lea  rax  byte [rbp + 0xffffffe0]
	mov  rax  byte [rax + 0x0]
	push  rax
	mov  rax  b_ 0xb
	pop  rdi
	cmp  rdi  rax
	setl  al
	movzx  rax  al
	test  rax  rax
	jz  .L2:
	lea  rax  byte [rbp + 0xffffffe8]
	mov  rax  byte [rax + 0x0]
	push  rax
	mov  rax  b_ 0x5
	pop  rdi
	sub  rdi  rax
	mov  rax  rdi
	push  rax
	lea  rax  byte [rbp + 0xffffffe8]
	mov  rax  byte [rax + 0x0]
	push  rax
	mov  rax  b_ 0x5
	pop  rdi
	sub  rdi  rax
	mov  rax  rdi
	pop  rdi
	imul  rax  rdi
	push  rax
	lea  rax  byte [rbp + 0xffffffe0]
	mov  rax  byte [rax + 0x0]
	push  rax
	mov  rax  b_ 0x5
	pop  rdi
	sub  rdi  rax
	mov  rax  rdi
	push  rax
	lea  rax  byte [rbp + 0xffffffe0]
	mov  rax  byte [rax + 0x0]
	push  rax
	mov  rax  b_ 0x5
	pop  rdi
	sub  rdi  rax
	mov  rax  rdi
	pop  rdi
	imul  rax  rdi
	pop  rdi
	add  rax  rdi
	push  rax
	mov  rax  b_ 0x10
	pop  rdi
	cmp  rdi  rax
	setl  al
	movzx  rax  al
	test  rax  rax
	jz  .L4:
	lea  rax  byte [rel (null)]
	push  rax
	pop  rdi
	xor  rax  rax
	call  printf  WRT ..plt
	jmp  .L5:
	.L4:
	lea  rax  byte [rel (null)]
	push  rax
	pop  rdi
	xor  rax  rax
	call  printf  WRT ..plt
	.L5:
	lea  rax  byte [rbp + 0xffffffe0]
	push  rax
	lea  rax  byte [rbp + 0xffffffe0]
	mov  rax  byte [rax + 0x0]
	push  rax
	mov  rax  b_ 0x1
	pop  rdi
	add  rax  rdi
	pop  rdi
	mov  byte [rdi + 0x0]rax
	jmp  .L3:
	.L2:
	lea  rax  byte [rel (null)]
	push  rax
	pop  rdi
	xor  rax  rax
	call  printf  WRT ..plt
	lea  rax  byte [rbp + 0xffffffe8]
	push  rax
	lea  rax  byte [rbp + 0xffffffe8]
	mov  rax  byte [rax + 0x0]
	push  rax
	mov  rax  b_ 0x1
	pop  rdi
	add  rax  rdi
	pop  rdi
	mov  byte [rdi + 0x0]rax
	jmp  .L1:
	.L0:
	mov  rax  b_ 0x0
	mov  word [rbp + 0xfffffff0]eax
	mov  rax  b_ 0x3c
	xor  rdi  rdi
	syscall
