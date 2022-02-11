; boot.asm
; 用于测试开发环境的简单代码
; 加载的扇区数
LOADER_CNT EQU 1
; 开始的逻辑扇区数
LOADER_SEC EQU 1
;0x7E00:0x0000
; loader段位置
LOADER_SEG EQU 0x7E00
; loader位移
LOADER_OFF EQU 0x0000
org 0x7C00

boot_start:
    ; 初始化段寄存器
    mov ax, cs
    mov ds, ax
    mov es, ax
    ; 初始化栈
    mov ax, 0x7C00
    mov ss, ax
    mov sp, 0x0000
    ; 设置视屏段位置
    mov ax, 0xB800
    mov gs, ax

    ; 10H号中断，设置视屏模式：Text(文字模式) 80*25 16色
    mov ax, 3H
    int 10H
    mov si, MSG_BOOT
    mov ax, 0
    call print_rm

    mov cx, LOADER_CNT
    mov ax, LOADER_SEG
    mov bx, LOADER_OFF
    mov es, ax
    mov ax, LOADER_SEC

    call load_bin    
    jmp LOADER_SEG: LOADER_OFF
; 函数名： load_bin
; 功能：加载loader到内存中
; 参数： (ax)=开始的逻辑扇区号， (cx)=加载的扇区数(0~255)，es:bx加载到的位置
; 返回：无
load_bin:
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
MSG_BOOT: db "[01] Hello ForgeOS!",0
times 510-($-$$) db 0
dw 0xAA55