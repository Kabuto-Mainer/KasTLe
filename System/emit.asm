section .data
section .rodata
	__string__0x7bba187e02d0:
	0x250x780xa0x250x780xa0x0
	__string__0x7bba187e02f0:
	0x250x640xa0x0
section .txt
	_start:
	push  rbp  
	mov  rbp  rsp  
	sub  rsp  b_ 0x48  
	mov  rax  b_ 0x40  
	cdqe  
	mov  byte [rbp + 0xfffffff0]rax  
	mov  rax  b_ 0x40  
	cdqe  
	mov  byte [rbp + 0xffffffe8]rax  
	lea  rax  byte [rbp + 0xfffffff0]
	mov  rax  byte [rax + 0x0]
	push  rax  
	mov  rax  b_ 0x2  
	cdqe  
	pop  rdi  
	cqo  
	push  rax  
	mov  rax  rdi  
	pop  rdi  
	idiv  rdi  
	mov  byte [rbp + 0xffffffe0]rax  
	mov  rax  b_ 0xa  
	push  rax  
	lea  rax  byte [rbp + 0xffffffe8]
	mov  rax  byte [rax + 0x0]
	push  rax  
	mov  rax  b_ 0x1  
	pop  rdi  
	add  rax  rdi  
	push  rax  
	lea  rax  byte [rbp + 0xfffffff0]
	mov  rax  byte [rax + 0x0]
	push  rax  
	mov  rax  b_ 0x1  
	pop  rdi  
	add  rax  rdi  
	push  rax  
	pop  rdi  
	pop  rsi  
	pop  rdx  
	xor  rax  rax  
	call  gfx_init  WRT ..plt 
	mov  rax  b_ 0x0  
	push  rax  
	mov  rax  b_ 0x0  
	push  rax  
	mov  rax  b_ 0x0  
	push  rax  
	pop  rdi  
	pop  rsi  
	pop  rdx  
	xor  rax  rax  
	call  gfx_get_color  WRT ..plt 
	mov  word [rbp + 0xffffffd8]eax  
	mov  rax  b_ 0xff  
	push  rax  
	mov  rax  b_ 0x7f  
	push  rax  
	mov  rax  b_ 0xff  
	push  rax  
	pop  rdi  
	pop  rsi  
	pop  rdx  
	xor  rax  rax  
	call  gfx_get_color  WRT ..plt 
	mov  word [rbp + 0xffffffd0]eax  
	lea  rax  byte [rbp + 0xffffffd0]
	mov  eax  word [rax + 0x0]
	push  rax  
	lea  rax  byte [rbp + 0xffffffd8]
	mov  eax  word [rax + 0x0]
	push  rax  
	lea  rax  byte [rel (null)]  
	push  rax  
	pop  rdi  
	pop  rsi  
	pop  rdx  
	xor  rax  rax  
	call  printf  WRT ..plt 
	lea  rax  byte [rbp + 0xffffffe0]
	mov  rax  byte [rax + 0x0]
	push  rax  
	lea  rax  byte [rel (null)]  
	push  rax  
	pop  rdi  
	pop  rsi  
	xor  rax  rax  
	call  printf  WRT ..plt 
	mov  rax  b_ 0x0  
	cdqe  
	mov  byte [rbp + 0xffffffc8]rax  
	.L1:
	lea  rax  byte [rbp + 0xffffffc8]
	mov  rax  byte [rax + 0x0]
	push  rax  
	lea  rax  byte [rbp + 0xfffffff0]
	mov  rax  byte [rax + 0x0]
	pop  rdi  
	cmp  rdi  rax  
	setl  al  
	movzx  rax  al  
	test  rax  rax  
	jz  .L0:
	mov  rax  b_ 0x0  
	cdqe  
	mov  byte [rbp + 0xffffffc0]rax  
	.L3:
	lea  rax  byte [rbp + 0xffffffc0]
	mov  rax  byte [rax + 0x0]
	push  rax  
	lea  rax  byte [rbp + 0xffffffe8]
	mov  rax  byte [rax + 0x0]
	pop  rdi  
	cmp  rdi  rax  
	setl  al  
	movzx  rax  al  
	test  rax  rax  
	jz  .L2:
	lea  rax  byte [rbp + 0xffffffc8]
	mov  rax  byte [rax + 0x0]
	push  rax  
	lea  rax  byte [rbp + 0xffffffe0]
	mov  rax  byte [rax + 0x0]
	pop  rdi  
	sub  rdi  rax  
	mov  rax  rdi  
	push  rax  
	lea  rax  byte [rbp + 0xffffffc8]
	mov  rax  byte [rax + 0x0]
	push  rax  
	lea  rax  byte [rbp + 0xffffffe0]
	mov  rax  byte [rax + 0x0]
	pop  rdi  
	sub  rdi  rax  
	mov  rax  rdi  
	pop  rdi  
	imul  rax  rdi  
	push  rax  
	lea  rax  byte [rbp + 0xffffffc0]
	mov  rax  byte [rax + 0x0]
	push  rax  
	lea  rax  byte [rbp + 0xffffffe0]
	mov  rax  byte [rax + 0x0]
	pop  rdi  
	sub  rdi  rax  
	mov  rax  rdi  
	push  rax  
	lea  rax  byte [rbp + 0xffffffc0]
	mov  rax  byte [rax + 0x0]
	push  rax  
	lea  rax  byte [rbp + 0xffffffe0]
	mov  rax  byte [rax + 0x0]
	pop  rdi  
	sub  rdi  rax  
	mov  rax  rdi  
	pop  rdi  
	imul  rax  rdi  
	pop  rdi  
	add  rax  rdi  
	push  rax  
	mov  rax  b_ 0x378  
	pop  rdi  
	cmp  rdi  rax  
	setl  al  
	movzx  rax  al  
	test  rax  rax  
	jz  .L4:
	lea  rax  byte [rbp + 0xffffffd0]
	mov  eax  word [rax + 0x0]
	push  rax  
	lea  rax  byte [rbp + 0xffffffc0]
	mov  rax  byte [rax + 0x0]
	push  rax  
	lea  rax  byte [rbp + 0xffffffc8]
	mov  rax  byte [rax + 0x0]
	push  rax  
	pop  rdi  
	pop  rsi  
	pop  rdx  
	xor  rax  rax  
	call  gfx_pixel  WRT ..plt 
	jmp  .L5:
	.L4:
	lea  rax  byte [rbp + 0xffffffd8]
	mov  eax  word [rax + 0x0]
	push  rax  
	lea  rax  byte [rbp + 0xffffffc0]
	mov  rax  byte [rax + 0x0]
	push  rax  
	lea  rax  byte [rbp + 0xffffffc8]
	mov  rax  byte [rax + 0x0]
	push  rax  
	pop  rdi  
	pop  rsi  
	pop  rdx  
	xor  rax  rax  
	call  gfx_pixel  WRT ..plt 
	.L5:
	lea  rax  byte [rbp + 0xffffffc0]
	push  rax  
	lea  rax  byte [rbp + 0xffffffc0]
	mov  rax  byte [rax + 0x0]
	push  rax  
	mov  rax  b_ 0x1  
	pop  rdi  
	add  rax  rdi  
	pop  rdi  
	mov  byte [rdi + 0x0]rax  
	jmp  .L3:
	.L2:
	lea  rax  byte [rbp + 0xffffffc8]
	push  rax  
	lea  rax  byte [rbp + 0xffffffc8]
	mov  rax  byte [rax + 0x0]
	push  rax  
	mov  rax  b_ 0x1  
	pop  rdi  
	add  rax  rdi  
	pop  rdi  
	mov  byte [rdi + 0x0]rax  
	jmp  .L1:
	.L0:
	xor  rax  rax  
	call  gfx_present  WRT ..plt 
	.L7:
	mov  rax  b_ 0x1  
	test  rax  rax  
	jz  .L6:
	xor  rax  rax  
	call  gfx_poll_event  WRT ..plt 
	push  rax  
	mov  rax  b_ 0x1  
	pop  rdi  
	cmp  rdi  rax  
	sete  al  
	movzx  rax  al  
	test  rax  rax  
	jz  .L8:
	jmp  .L6:
	jmp  .L8:
	.L8:
	.L8:
	jmp  .L7:
	.L6:
	xor  rax  rax  
	call  gfx_close  WRT ..plt 
	mov  rax  b_ 0x3c  
	xor  rdi  rdi  
	syscall  
