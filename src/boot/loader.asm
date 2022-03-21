; loader.asm
%include "constant.inc"
jmp loaderStart
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
; 数据段选择子
SELECTOR_DATA EQU 1 << 3 + 0
; 代码段选择子
SELECTOR_CODE EQU 2 << 3 + 0

loaderStart:
    ; 初始化段寄存器
    mov ax, cs
    mov ds, ax

    mov si, MSG_LOADER
    mov ah, 0x03
    mov al, 1
    call rmPrint

    mov si, MSG_DMEM
    mov ah, 0x03
    mov al, 2
    call rmPrint
    ; 查询内存信息
dectMem:
    mov ax, 0
    mov ds, ax
    ; ds:MME_CNT
    mov dword [MME_CNT], 0
    mov ax, MME_SEG
    mov es, ax
    mov di, MME_OFF
    mov ebx, 0
.loop:
    mov dword [es:di + 20], 1
    mov eax, 0xE820
    mov ecx, 24
    ;错误警告：
    ;不要把0x534D4150写成'SMAP'，编译后顺序会被改变，导致CF置位报错，还是老老实实写成16进制形式
    ;错误用法：mov edx, 'SMAP'
    mov edx, 0x534D4150
    int 0x15
    jc .err
    inc dword [MME_CNT]
    cmp ebx, 0
    jz .end
    add di, 24
    jmp .loop
.err:
    mov bx, cs
    mov ds, bx

    mov ah, 0_000__0_100B   ; 红色
    mov al, 2
    mov si, MSG_DMEM_ERR
    call rmPrint
    jmp $
.end:
    ; 进入保护模式
    mov ax, cs
    mov ds, ax
    mov ah, 0b0_000_0_011 ;蓝色
    mov al, 3
    mov si, MSG_PROTECT
    call rmPrint
    ; 1、打开A20地址线
    in al, 0x92
    or al, 0000_0010B
    out 0x92, al

    ; 2、加载GDT
    cli	;关中断
    ; dx被重新初始化了，不用再加0x8600
    lgdt [GDT_PTR]

    ; 3、置CR0.PM位为1
    mov eax, cr0
    or eax, 0000_0001B
    mov cr0, eax

    ; 初始化段寄存器
    mov ax, SELECTOR_DATA
    mov ds, ax
    mov es, ax
    mov gs, ax
    mov fs, ax
    mov ss, ax

    jmp dword SELECTOR_CODE: 0x8600 + protect
[BITS 32]
protect:
    mov ax, KERNEL_SELECTOR
    mov fs, ax
    mov ebx, KERNEL_OFFSET
    mov eax, KERNEL_LBA28
    mov ecx, KERNEL_CNT
    call readSectors 
    jmp SELECTOR_CODE: KERNEL_OFFSET
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
[BITS 32]
; 函数名：readSectors
; 参数：eax=LBA28, ecx=读取的扇区数（要小于255），fs:ebx=数据的位置
; 返回： 无
readSectors:
    push edx
    push eax
    xor edx, edx
    xor eax, eax
    ; 1、写要读取的扇区数
    mov dx, 0x1F2
    mov al, cl      ; 读取的扇区数
    out dx, al
    ; 2、写LBA28
    pop eax
    ; LBA0~7
    inc dx
    out dx, al
    ; LBA8~15
    shr eax, 8
    inc dx
    out dx, al
    ; LBA16~24
    shr eax, 8
    inc dx
    out dx, al
    ; 3、写device
    shr eax, 8
    or al, 0b1110_0000
    inc dx
    out dx, al
    xor eax, eax
    mov al, 0x20
    inc dx
    out dx, al
.notReady:
    nop
    in al, dx
    ; 0b1000_1000 0x88
    and al, 0x88
    cmp al, 0x08
    jne .notReady
    xor eax, eax
    xor edx, edx

    mov ax, cx
    mov dx, 512 / 2
    mul dx
    push ecx
    xor ecx, ecx
    mov cx, ax
    xor edx, edx
    xor eax, eax
    mov dx, 0x1F0
.copyData:
    in ax, dx
    mov [fs:ebx], ax
    add ebx, 2
    loop .copyData
    pop eax
    pop edx
    ret
MSG_LOADER:     db "[02] Loader start",0
MSG_DMEM:       db "[03] Detect memory info",0
MSG_PROTECT:    db "[04] Protect mode",0
MSG_DMEM_ERR: db "ERROR: int 0x15 not support!",0