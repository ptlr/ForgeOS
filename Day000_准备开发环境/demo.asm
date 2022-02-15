; boot.asm
; 用于测试开发环境的简单代码

org 0x7C00

boot_start:
    ; 10H号中断，设置视屏模式：Text(文字模式) 80*25 16色
    mov ax, 3H
    int 10H

    ; 输出字符串: Hello OS!
    ; 设置视屏段位置
    mov ax, 0xB800
    mov gs, ax
    ; 设置字体颜色为蓝色
    mov ah, 0x03

    mov al, 'H'
    mov [gs:0], ax

    mov al, 'e'
    mov [gs:2], ax

    mov al, 'l'
    mov [gs:4], ax

    mov [gs:6], ax

    mov al, 'o'
    mov [gs:8],ax

    mov al, ' '
    mov [gs:10], ax

    mov al,'O'
    mov [gs:12], ax

    mov al, 'S'
    mov [gs:14], ax

    mov al, '!'
    mov [gs:16], ax
    jmp $
times 510-($-$$) db 0
dw 0xAA55