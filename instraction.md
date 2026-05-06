# Файл с инструкциями ассемблера и их op-кодами

## Теория

Каждая инструкция состоит из нескольких частей:
`[Prefix]` + `[Opcode]` + `[ModR/M]` + `[SIB]` + `[Displacement]` + `[Immediate]`.

У некоторых инструкций может не быть некоторых блоков, быть больше не может.

## mov



### Тесты

* `mov al, [0x7F'FF'FF'FF]`  -> `8a 04 25 ff ff ff 7f`
* `mov ax, [0x7F'FF'FF'FF]`  -> `66 8b 04 25 ff ff ff 7f`
* `mov eax, [0x7F'FF'FF'FF]` -> `8b 04 25 ff ff ff 7f`
* `mov rax, [0x7F'FF'FF'FF]` -> `48 8b 04 25 ff ff ff 7f`

* `mov cl, [0x7F'FF'FF'FF]`  -> `8a 0c 25 ff ff ff 7f`
* `mov cx, [0x7F'FF'FF'FF]`  -> `66 8b 0c 25 ff ff ff 7f`
* `mov eax, [0x7F'FF'FF'FF]` -> `8b 0c 25 ff ff ff 7f`
* `mov rcx, [0x7F'FF'FF'FF]` -> `48 8b 0c 25 ff ff ff 7f`

* `mov r15b, [0x7F'FF'FF'FF]`  -> `8a 0c 25 ff ff ff 7f`
* `mov cx, [0x7F'FF'FF'FF]`  -> `66 8b 0c 25 ff ff ff 7f`
* `mov eax, [0x7F'FF'FF'FF]` -> `8b 0c 25 ff ff ff 7f`
* `mov rcx, [0x7F'FF'FF'FF]` -> `48 8b 04 25 ff ff ff 7f`



* `mov rax, rax` -> `48 89 c0` : `0100 1000 1000 1001 1100 0000`
* `mov rax, rcx` -> `48 89 c8` : `0100 1000 1000 1001 1100 1000`
* `mov rax, rdx` -> `48 89 d0` : `0100 1000 1000 1001 1101 0000`
* `mov rax, rbx` -> `48 89 d8` : `0100 1000 1000 1001 1101 1000`

* `mov rcx, rax` -> `48 89 c1` : `0100 1000 1000 1001 1100 0001`
* `mov rcx, rcx` -> `48 89 c9` : `0100 1000 1000 1001 1100 1001`


* `mov rax, [rax]` -> `48 8b 00` : `0100 1000 1000 1101 0000 0000`
* `mov rax, [rcx]` -> `48 8b 01` : `0100 1000 1000 1101 0000 0001`

* `mov rcx, [rax]` -> `48 8b 08` : `0100 1000 1000 1101 0000 1000`
* `mov rcx, [rcx]` -> `48 8b 09` : `0100 1000 1000 1101 0000 1001`


* `mov rax, [rax + 0x'0F'FF'FF]` -> `48 8b 80 ff ff ff 0f` : `0100 1000 1000 1101 1000 0000 1111 1111 1111 1111 0000 1111`
* `mov rax, [rax + 0x'0F'FF'FF]` -> `48 8b 81 ff ff ff 0f` : `0100 1000 1000 1101 1000 0001 1111 1111 1111 1111 0000 1111`

* `mov rcx, [rax + 0x'0F'FF'FF]` -> `48 8b 88 ff ff ff 0f` : `0100 1000 1000 1101 1000 1000 1111 1111 1111 1111 0000 1111`
* `mov rax, [rax + 0x'0F'FF'FF]` -> `48 8b 89 ff ff ff 0f` : `0100 1000 1000 1101 1000 1001 1111 1111 1111 1111 0000 1111`

* `mov rax, [0x7F'FF'FF'FF]` -> `48 8b 04 25 FF FF FF 7F` : `0100 1000 1000 1011 0000 0100 0010 0101 1111 1111 1111 1111 1111 0111 1111`

* `mov ax, 0xFF'FF` -> `66 b8 FF FF` : `0110 0110 1011 1000 1111 1111 1111 1111`
* `mov cx, 0xFF'FF` -> `66 b9 FF FF` : `0110 0110 1011 1001 1111 1111 1111 1111`

* `mov cx, [0x7F'FF'FF'FF]` -> '66 8b 0C 25 FF FF FF 7F `

### Вывод


Каждая
  Формат инструкции:
    [REX (0x48)] [Opcode (0x89 или 0x8B)] [ModR/M] [Disp8 или Disp32]

  Коды 64-битных регистров (поле reg / r/m):
    RAX=0  RCX=1  RDX=2  RBX=3  RSP=4  RBP=5  RSI=6  RDI=7

  Байт ModR/M = [mod:2][reg:3][r/m:3]
    mod=11              – регистровый режим (r/m – регистр)
    mod=00 (r/m≠100,101)– память [r/m] (без смещения)
    mod=01              – память [r/m + disp8]
    mod=10              – память [r/m + disp32]

  Opcode 0x89:  MOV r/m64, r64    (запись из регистра в r/m)
    reg   = регистр-источник
    r/m   = регистр или память-приёмник

  Opcode 0x8B:  MOV r64, r/m64    (чтение из r/m в регистр)
    reg   = регистр-приёмник
    r/m   = регистр или память-источник

  Смещение (displacement):
    при mod=01 – 1 байт со знаком
    при mod=10 – 4 байта (little-endian)

  Примеры:
    mov rax, rcx          -> 48 89 c8   (mod=11, reg=1(RCX), r/m=0(RAX))
    mov rax, [rcx]        -> 48 8b 01   (mod=00, reg=0(RAX), r/m=1(RCX))
    mov rax, [rbp+10]     -> 48 8b 45 0a(disp8)   (mod=01, reg=0, r/m=5, disp=10)
    mov rax, [rcx+0xFF]   -> 48 8b 81 ff 00 00 00 (mod=10, reg=0, r/m=1, disp32=0xFF)
    mov [rax], rcx        -> 48 89 08   (mod=00, reg=1(RCX), r/m=0(RAX))

  Примечание: адреса с RSP (100) или RBP (101) при mod=00 требуют
  специальной обработки (SIB или disp32) – здесь не рассматриваются.
