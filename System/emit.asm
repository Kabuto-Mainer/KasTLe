section .data
section .rodata
	__string__0x7bc758be0130:
	0x250x640x0
	__string__0x7bd758be0070:
	0x460x690x620x6f0x6e0x610x630x630x690x200x580x200x3d0x200x250x640xa0x0
	__string__0x7bc758be0170:
	0x500x6f0x6c0x740x6f0x720x610x730x680x6b0x610xa0x0
section .txt
	_start:
	push  rbp  
	mov  rbp  rsp  
	sub  rsp  b_ 0x18  
	mov  rax  b_ 0xa  
	mov  word [rbp + 0xfffffff0]eax  
	lea  rax  byte [rbp + 0xfffffff0]
	push  rax  
	lea  rax  byte [rel (null)]  
	push  rax  
	pop  rdi  
	pop  rsi  
	xor  rax  rax  
	call  scanf  WRT ..plt 
	lea  rax  byte [rbp + 0xfffffff0]
	push  rax  
	lea  rax  byte [rbp + 0xfffffff0]
	mov  eax  word [rax + 0x0]
	cdqe  
	push  rax  
	pop  rdi  
	sub  rsp  b_ 0x8  
	xor  rax  rax  
	call  __func__number_fibonacci  
	add  rsp  b_ 0x8  
	pop  rdi  
	mov  word [rdi + 0x0]eax  
	lea  rax  byte [rbp + 0xfffffff0]
	mov  eax  word [rax + 0x0]
	push  rax  
	lea  rax  byte [rel (null)]  
	push  rax  
	pop  rdi  
	pop  rsi  
	xor  rax  rax  
	call  printf  WRT ..plt 
	lea  rax  byte [rel (null)]  
	push  rax  
	pop  rdi  
	xor  rax  rax  
	call  printf  WRT ..plt 
	mov  rax  b_ 0x3c  
	xor  rdi  rdi  
	syscall  
	__func__fact:
	push  rbp  
	mov  rbp  rsp  
	sub  rsp  b_ 0x10  
	mov  byte [rbp + 0xfffffff8]rdi  
	lea  rax  byte [rbp + 0xfffffff8]
	mov  eax  word [rax + 0x0]
	push  rax  
	mov  rax  b_ 0x1  
	pop  rdi  
	cmp  rdi  rax  
	sete  al  
	movzx  rax  al  
	test  rax  rax  
	jz  .L0:
	mov  rax  b_ 0x1  
	mov  rsp  rbp  
	pop  rbp  
	ret  
	jmp  .L0:
	.L0:
	.L0:
	lea  rax  byte [rbp + 0xfffffff8]
	mov  eax  word [rax + 0x0]
	push  rax  
	lea  rax  byte [rbp + 0xfffffff8]
	mov  eax  word [rax + 0x0]
	push  rax  
	mov  rax  b_ 0x1  
	pop  rdi  
	sub  rdi  rax  
	mov  rax  rdi  
	push  rax  
	pop  rdi  
	sub  rsp  b_ 0x8  
	xor  rax  rax  
	call  __func__fact  
	add  rsp  b_ 0x8  
	pop  rdi  
	add  rax  rdi  
	mov  rsp  rbp  
	pop  rbp  
	ret  
	mov  rsp  rbp  
	pop  rbp  
	ret  
	__func__number_fibonacci:
	push  rbp  
	mov  rbp  rsp  
	sub  rsp  b_ 0x10  
	mov  byte [rbp + 0xfffffff8]rdi  
	lea  rax  byte [rbp + 0xfffffff8]
	mov  rax  byte [rax + 0x0]
	push  rax  
	mov  rax  b_ 0x2  
	pop  rdi  
	cmp  rdi  rax  
	setle  al  
	movzx  rax  al  
	test  rax  rax  
	jz  .L1:
	mov  rax  b_ 0x1  
	mov  rsp  rbp  
	pop  rbp  
	ret  
	jmp  .L1:
	.L1:
	.L1:
	lea  rax  byte [rbp + 0xfffffff8]
	mov  rax  byte [rax + 0x0]
	push  rax  
	mov  rax  b_ 0x1  
	pop  rdi  
	sub  rdi  rax  
	mov  rax  rdi  
	push  rax  
	pop  rdi  
	xor  rax  rax  
	call  __func__number_fibonacci  
	push  rax  
	lea  rax  byte [rbp + 0xfffffff8]
	mov  rax  byte [rax + 0x0]
	push  rax  
	mov  rax  b_ 0x2  
	pop  rdi  
	sub  rdi  rax  
	mov  rax  rdi  
	push  rax  
	pop  rdi  
	sub  rsp  b_ 0x8  
	xor  rax  rax  
	call  __func__number_fibonacci  
	add  rsp  b_ 0x8  
	pop  rdi  
	add  rax  rdi  
	mov  rsp  rbp  
	pop  rbp  
	ret  
	mov  rsp  rbp  
	pop  rbp  
	ret  
