section .data
section .rodata
section .txt
	_start:
	push  rbp  
	mov  rbp  rsp  
	sub  rsp  b_ 0x68  
	mov  rax  b_ 0x40  
	cdqe  
	mov  byte [rbp + 0xfffffff0]rax  
	mov  rax  b_ 0x40  
	cdqe  
	mov  byte [rbp + 0xffffffe8]rax  
	mov  rax  b_ 0xa  
	push  rax  
	lea  rax  byte [rbp + 0xffffffe8]
	mov  rax  byte [rax + 0x0]
	push  rax  
	lea  rax  byte [rbp + 0xfffffff0]
	mov  rax  byte [rax + 0x0]
	push  rax  
	pop  rdi  
	pop  rsi  
	pop  rdx  
	xor  rax  rax  
	call  gfx_init  WRT ..plt 
	mov  rax  b_ 0xff  
	push  rax  
	mov  rax  b_ 0xff  
	push  rax  
	mov  rax  b_ 0xff  
	push  rax  
	pop  rdi  
	pop  rsi  
	pop  rdx  
	xor  rax  rax  
	call  gfx_get_color  WRT ..plt 
	mov  word [rbp + 0xffffffe0]eax  
	mov  rax  b_ 0x0  
	push  rax  
	mov  rax  b_ 0x0  
	push  rax  
	mov  rax  b_ 0x0  
	push  rax  
	mov  rax  b_ 0x20  
	push  rax  
	pop  rdi  
	pop  rsi  
	pop  rdx  
	pop  rcx  
	xor  rax  rax  
	call  gfx_get_color_shadow  WRT ..plt 
	mov  word [rbp + 0xffffffd8]eax  
	mov  rax  b_ 0x0  
	cdqe  
	mov  byte [rbp + 0xffffffd0]rax  
	mov  rax  b_ 0x0  
	cdqe  
	mov  byte [rbp + 0xffffffc8]rax  
	mov  rax  b_ 0x52  
	mov  word [rbp + 0xffffffc0]eax  
	mov  rax  b_ 0x51  
	mov  word [rbp + 0xffffffb8]eax  
	mov  rax  b_ 0x50  
	mov  word [rbp + 0xffffffb0]eax  
	mov  rax  b_ 0x4f  
	mov  word [rbp + 0xffffffa8]eax  
	.L1:
	mov  rax  b_ 0x1  
	test  rax  rax  
	jz  .L0:
	mov  rax  b_ 0x1  
	mov  word [rbp + 0xffffffa0]eax  
	xor  rax  rax  
	call  gfx_poll_event  WRT ..plt 
	push  rax  
	mov  rax  b_ 0x1  
	pop  rdi  
	cmp  rdi  rax  
	sete  al  
	movzx  rax  al  
	test  rax  rax  
	jz  .L2:
	jmp  .L0:
	jmp  .L2:
	.L2:
	.L2:
	lea  rax  byte [rbp + 0xffffffc0]
	mov  eax  word [rax + 0x0]
	push  rax  
	pop  rdi  
	xor  rax  rax  
	call  gfx_key_pressed  WRT ..plt 
	test  rax  rax  
	jz  .L3:
	lea  rax  byte [rbp + 0xffffffc8]
	push  rax  
	lea  rax  byte [rbp + 0xffffffc8]
	mov  rax  byte [rax + 0x0]
	push  rax  
	mov  rax  b_ 0x1  
	pop  rdi  
	sub  rdi  rax  
	mov  rax  rdi  
	pop  rdi  
	mov  byte [rdi + 0x0]rax  
	lea  rax  byte [rbp + 0xffffffc8]
	push  rax  
	lea  rax  byte [rbp + 0xffffffc8]
	mov  rax  byte [rax + 0x0]
	push  rax  
	lea  rax  byte [rbp + 0xfffffff0]
	mov  rax  byte [rax + 0x0]
	pop  rdi  
	cqo  
	push  rax  
	mov  rax  rdi  
	pop  rdi  
	idiv  rdi  
	mov  rax  rdx  
	pop  rdi  
	mov  byte [rdi + 0x0]rax  
	jmp  .L4:
	.L3:
	lea  rax  byte [rbp + 0xffffffb8]
	mov  eax  word [rax + 0x0]
	push  rax  
	pop  rdi  
	xor  rax  rax  
	call  gfx_key_pressed  WRT ..plt 
	test  rax  rax  
	jz  .L5:
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
	lea  rax  byte [rbp + 0xffffffc8]
	push  rax  
	lea  rax  byte [rbp + 0xffffffc8]
	mov  rax  byte [rax + 0x0]
	push  rax  
	lea  rax  byte [rbp + 0xfffffff0]
	mov  rax  byte [rax + 0x0]
	pop  rdi  
	cqo  
	push  rax  
	mov  rax  rdi  
	pop  rdi  
	idiv  rdi  
	mov  rax  rdx  
	pop  rdi  
	mov  byte [rdi + 0x0]rax  
	jmp  .L4:
	.L5:
	lea  rax  byte [rbp + 0xffffffb0]
	mov  eax  word [rax + 0x0]
	push  rax  
	pop  rdi  
	xor  rax  rax  
	call  gfx_key_pressed  WRT ..plt 
	test  rax  rax  
	jz  .L6:
	lea  rax  byte [rbp + 0xffffffd0]
	push  rax  
	lea  rax  byte [rbp + 0xffffffd0]
	mov  rax  byte [rax + 0x0]
	push  rax  
	mov  rax  b_ 0x1  
	pop  rdi  
	sub  rdi  rax  
	mov  rax  rdi  
	pop  rdi  
	mov  byte [rdi + 0x0]rax  
	lea  rax  byte [rbp + 0xffffffd0]
	push  rax  
	lea  rax  byte [rbp + 0xffffffd0]
	mov  rax  byte [rax + 0x0]
	push  rax  
	lea  rax  byte [rbp + 0xffffffe8]
	mov  rax  byte [rax + 0x0]
	pop  rdi  
	cqo  
	push  rax  
	mov  rax  rdi  
	pop  rdi  
	idiv  rdi  
	mov  rax  rdx  
	pop  rdi  
	mov  byte [rdi + 0x0]rax  
	jmp  .L4:
	.L6:
	lea  rax  byte [rbp + 0xffffffa8]
	mov  eax  word [rax + 0x0]
	push  rax  
	pop  rdi  
	xor  rax  rax  
	call  gfx_key_pressed  WRT ..plt 
	test  rax  rax  
	jz  .L7:
	lea  rax  byte [rbp + 0xffffffd0]
	push  rax  
	lea  rax  byte [rbp + 0xffffffd0]
	mov  rax  byte [rax + 0x0]
	push  rax  
	mov  rax  b_ 0x1  
	pop  rdi  
	add  rax  rdi  
	pop  rdi  
	mov  byte [rdi + 0x0]rax  
	lea  rax  byte [rbp + 0xffffffd0]
	push  rax  
	lea  rax  byte [rbp + 0xffffffd0]
	mov  rax  byte [rax + 0x0]
	push  rax  
	lea  rax  byte [rbp + 0xffffffe8]
	mov  rax  byte [rax + 0x0]
	pop  rdi  
	cqo  
	push  rax  
	mov  rax  rdi  
	pop  rdi  
	idiv  rdi  
	mov  rax  rdx  
	pop  rdi  
	mov  byte [rdi + 0x0]rax  
	jmp  .L4:
	.L7:
	lea  rax  byte [rbp + 0xffffffa0]
	push  rax  
	mov  rax  b_ 0x0  
	pop  rdi  
	mov  word [rdi + 0x0]eax  
	.L4:
	lea  rax  byte [rbp + 0xffffffd8]
	mov  eax  word [rax + 0x0]
	push  rax  
	lea  rax  byte [rbp + 0xffffffe8]
	mov  rax  byte [rax + 0x0]
	push  rax  
	lea  rax  byte [rbp + 0xfffffff0]
	mov  rax  byte [rax + 0x0]
	push  rax  
	mov  rax  b_ 0x0  
	push  rax  
	mov  rax  b_ 0x0  
	push  rax  
	pop  rdi  
	pop  rsi  
	pop  rdx  
	pop  rcx  
	pop  r8  
	xor  rax  rax  
	call  gfx_rect  WRT ..plt 
	lea  rax  byte [rbp + 0xffffffa0]
	mov  eax  word [rax + 0x0]
	test  rax  rax  
	jz  .L8:
	lea  rax  byte [rbp + 0xffffffe0]
	mov  eax  word [rax + 0x0]
	push  rax  
	lea  rax  byte [rbp + 0xffffffc8]
	mov  rax  byte [rax + 0x0]
	push  rax  
	lea  rax  byte [rbp + 0xffffffd0]
	mov  rax  byte [rax + 0x0]
	push  rax  
	pop  rdi  
	pop  rsi  
	pop  rdx  
	xor  rax  rax  
	call  gfx_pixel  WRT ..plt 
	xor  rax  rax  
	call  gfx_present  WRT ..plt 
	jmp  .L8:
	.L8:
	.L8:
	mov  rax  b_ 0xa  
	push  rax  
	pop  rdi  
	xor  rax  rax  
	call  gfx_delay  WRT ..plt 
	jmp  .L1:
	jmp  .L1:
	.L0:
	xor  rax  rax  
	call  gfx_close  WRT ..plt 
	mov  rax  b_ 0x3c  
	xor  rdi  rdi  
	syscall  
