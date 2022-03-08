; boot.asm
; 用于测试开发环境的简单代码
%include "bootloader.inc"
org 0x7C00

bootStart:
    ; 初始化段寄存器
    mov ax, cs
    mov ds, ax
    mov es, ax
    ; 初始化栈
    mov ax, STACK_SEG
    mov ss, ax
    mov sp, STACK_OFF
    ; 设置视屏段位置
    mov ax, 0xB800
    mov gs, ax

    ; 10H号中断，设置视屏模式：Text(文字模式) 80*25 16色
    mov ax, 0x003   ; AH=0x00, AL=0x03
    int 10H
    mov si, MSG_BOOT
    mov ah, 0x03
    mov al, 0
    call rmPrint

    mov cx, LOADER_CNT
    mov ax, LOADER_SEG
    mov bx, LOADER_OFF
    mov es, ax
    mov ax, LOADER_SEC

    call loadLoader    
    jmp LOADER_SEG: LOADER_OFF
; 函数名： loadLoader
; 功能：加载loader到内存中
; 参数： (ax)=开始的逻辑扇区号， (cx)=加载的扇区数(0~255)，es:bx加载到的位置
; 返回：无
loadLoader:
    push dx
    push cx
    mov dx, 18
    div dl      ; 8位除法， ax存被除数， AL存商， AH存余数
    mov cl, ah
    inc cl      ; 扇区号
    mov ah, 0
    mov dl, 2
    div dl
    mov ch, al  ; 得到磁道号
    mov dh, ah  ; 磁头号

    pop ax          ; al中存放要读取的扇区数
    mov ah, 0x02    ; 子功能号
    mov dl, 0x00    ; 驱动器号

    int 0x13
    pop dx
    ret
;函数名：rmPrint
;功能：在是模式下显示以0结尾的字符串
;参数：AH=颜色（AL）=行数（0~24）（ds）=字符串的起始位置段地址，（si）=偏移地址
;返回：无
rmPrint:
    push bx
    push ax
    ; 80 * 2 * 25 = 4000 < 65535, 8位乘法
    mov bx, 160 ; bh=0x00, bl=160H
    mul bl
    mov bx, ax
    pop ax
.putChar:
    mov al, [si]
    cmp al,0
    jz .endPut
    mov [gs:bx],ax
    inc si
    add bx, 2
    jmp .putChar
.endPut:
    pop bx
    ret
MSG_BOOT: db "[01] boot start",0
times 510-($-$$) db 0
dw 0xAA55