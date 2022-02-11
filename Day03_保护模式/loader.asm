; loader.asm
%include "bootloader.inc"
jmp loader_start
; GDT
DESC:       dq 0x00_0_0_00_000000_0000
; 数据段，0~4GB， 可读写
DESC_DATA:  dq 0x00_C_F_93_000000_FFFF
; 代码段， 0~4GB, 执行，读，已访问
DESC_CODE:  dq 0x00_C_F_9A_000000_FFFF
GDT_SIZE    equ $ - DESC

; GDT_PTR
GDT_PTR:
GDT_LIMIT:   dw GDT_SIZE - 1 
GDT_BASE:    dd 0x8600 + DESC

; 段选择子:指向GDT,
SELECTOR_DATA EQU 1 << 3 + 0
SELECTOR_CODE EQU 2 << 3 + 0

loader_start:
    ; 初始化段寄存器
    mov ax, cs
    mov ds, ax

    mov si, MSG_LOADER
    mov ah, 0x03
    mov al, 1
    call print_rm

    mov si, MSG_DMEM
    mov ah, 0x03
    mov al, 2
    call print_rm
    ; 查询内存信息
dmem:
    mov ax, 0
    mov ds, ax
    ; ds:MME_CNT
    mov dword [MME_CNT], 0
    mov ax, MME_SEG
    mov es, ax
    mov di, MME_OFF
    mov ebx, 0
.dmem_loop:
    mov dword [es:di + 20], 1
    mov eax, 0xE820
    mov ecx, 24
    ;错误警告：
    ;不要把0x534D4150写成'SMAP'，编译后顺序会被改变，导致CF置位报错，还是老老实实写成16进制形式
    ;错误用法：mov edx, 'SMAP'
    mov edx, 0x534D4150
    int 0x15
    jc .dmem_err
    inc dword [MME_CNT]
    cmp ebx, 0
    jz .dmem_end
    add di, 24
    jmp .dmem_loop
.dmem_err:
    mov bx, cs
    mov ds, bx

    mov ah, 0_000__0_100B   ; 红色
    mov al, 2
    mov si, MSG_DMEM_ERR
    call print_rm
    jmp $
.dmem_end:
    ; 进入保护模式
    ; 1、打开A20地址线
    in al, 0x92
    or al, 0000_0010B
    out 0x92, al

    ; 2、加载GDT
    cli	;关中断
    lgdt [0x8600 + GDT_PTR]

    ; 3、置CR0.PM位为1
    mov eax, cr0
    or eax, 0000_0001B
    mov cr0, eax

    mov ax, SELECTOR_DATA
    mov fs, ax

    mov ax, SELECTOR_CODE
    mov gs, ax

    jmp dword SELECTOR_CODE: 0x8600 + protect
[BITS 32]
protect:
    nop
    nop
    ;死循环阻塞
    jmp $
[BITS 16]
; 函数名：hex2char
; 功能：0~F转换成ASCII字符
; 参数：AL=16进制数字
; 返回：AL=ASCII字符
hex2char:
    cmp al, 0x0F
    ja .end
    cmp al, 0x09
    ja .A2F
.zero2nine:
    or al, 0011_0000B
    jmp .end
.A2F:
    or al, 0100_0000B
.end:
    ret
;函数名：print_rm
;功能：在是模式下显示以0结尾的字符串
;参数：AH=颜色（AL）=行数（0~24）（ds）=字符串的起始位置段地址，（si）=偏移地址
;返回：无
print_rm:
    push bx
    push ax
    ; 80 * 2 * 25 = 4000 < 65535, 8位乘法
    mov bx, 160 ; bh=0x00, bl=160H
    mul bl
    mov bx, ax
    pop ax
.put_char:
    mov al, [si]
    cmp al,0
    jz .end_put
    mov [gs:bx],ax
    inc si
    add bx, 2
    jmp .put_char
.end_put:
    pop bx
    ret
MSG_LOADER: db "Loader start",0
MSG_DMEM:   db "Detect memory info",0
MSG_DMEM_ERR: db "ERROR: int 0x15 not support!",0