; loader.asm
loader_start:
    ; 初始化段寄存器
    mov ax, cs
    mov ds, ax

    mov si, MSG_LOADER
    mov ax, 1
    call print_rm
    jmp $
;函数名：print_rm
;功能：在是模式下显示以0结尾的字符串
;参数：（AX）=行数（0~24）（ds）=字符串的起始位置段地址，（si）=偏移地址
;返回：无
print_rm:
    push bx
    ; 80 * 2 * 25 = 4000 < 65535, 8位乘法
    mov bx, 160 ; bh=0x00, bl=160H
    mul bl
    mov bx, ax
    mov ah, 0x03
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
MSG_LOADER: db "[02] Hello Loader!",0