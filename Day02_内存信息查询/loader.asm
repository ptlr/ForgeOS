; loader.asm
%include "bootloader.inc"
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
    mov dword [MME_CNT], 0
    mov ax, MME_SEG
    mov es, ax
    mov di, MME_OFF
    mov ebx, 0
.dmem_loop:
    mov eax, 0xE820
    mov ecx, 24
    ;错误警告：
    ;不要把0x534D4150写成'SMAP'，编译后顺序会被改变，导致CF置位报错，还是老老实实写成16进制形式
    mov edx, 0x534D4150
    int 0x15
    jc .dmem_err
    inc dword [MME_CNT]
    cmp ebx, 0
    jz .dmem_end
    add di, 24
    jmp .dmem_loop
.dmem_err:
    mov ah, 0_000__0_100B   ; 红色
    mov al, 2
    mov si, MSG_DMEM_ERR
    call print_rm
    jmp $
.dmem_end:
    jmp $
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