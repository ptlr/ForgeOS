extern cPutChar
extern putHex
extern putStr

extern intrHandlerTable

global intrEntryTable
%define ERROR_CODE  nop
%define ERROR_ZERO  push 0
; bug勘误：
; bug表现：无法进入中断，提示地址无效或者代码无效
; 原因：
; 在连接的时候连接器会把代码段和数据段分开存放
; 我没有定义“.text”段，导致代码和数据被存放到了一起，构建IDT门描述符的时候，无法找到对应中断的入口
%macro VECTOR 2
; 宏内部的“.text”必须被定义
[SECTION .text]
[BITS 32]
intr%1entry:
    %2
    ; 保存寄存器环境
    push ds
    push es
    push fs
    push gs
    pushad

    ;向8259A发送EOI信号
    mov al, 0x20
    out 0xA0, al            ; 从片
    out 0x20, al            ; 主片

    ;调用中断处理函数
    push %1 ; 压入中断号
    call [intrHandlerTable + %1 * 4]
    ; 平衡栈空间，跳过中断处理函数的参数
    add esp, 4

    jmp intrExit
; 这里的“.data”部分必须被定义
; 这里面保存了中断的入口数据
[SECTION .data]
    dd intr%1entry
%endmacro
[SECTION .text]
[BITS 32]
intrExit:
    popad
    pop gs
    pop fs
    pop es
    pop ds
    ; 跳过错误码
    add esp, 4
    iretd
[SECTION .data]
INTR_COUNT  dd 0
INTR_MSG_PRE    db "Interrupt occur! INTR=",0
INTR_MSG_COUNT  db ", COUNT = ",0
intrEntryTable:
VECTOR 0x00, ERROR_ZERO
VECTOR 0x01, ERROR_ZERO
VECTOR 0x02, ERROR_ZERO
VECTOR 0x03, ERROR_ZERO
VECTOR 0x04, ERROR_ZERO
VECTOR 0x05, ERROR_ZERO
VECTOR 0x06, ERROR_ZERO
VECTOR 0x07, ERROR_ZERO
VECTOR 0x08, ERROR_ZERO
VECTOR 0x09, ERROR_ZERO
VECTOR 0x0A, ERROR_ZERO
VECTOR 0x0B, ERROR_ZERO
VECTOR 0x0C, ERROR_ZERO
VECTOR 0x0D, ERROR_ZERO
VECTOR 0x0E, ERROR_ZERO
VECTOR 0x0F, ERROR_ZERO
VECTOR 0x10, ERROR_ZERO
VECTOR 0x11, ERROR_ZERO
VECTOR 0x12, ERROR_ZERO
VECTOR 0x13, ERROR_ZERO
VECTOR 0x14, ERROR_ZERO
VECTOR 0x15, ERROR_ZERO
VECTOR 0x16, ERROR_ZERO
VECTOR 0x17, ERROR_ZERO
VECTOR 0x18, ERROR_ZERO
VECTOR 0x19, ERROR_ZERO
VECTOR 0x1A, ERROR_ZERO
VECTOR 0x1B, ERROR_ZERO
VECTOR 0x1C, ERROR_ZERO
VECTOR 0x1D, ERROR_ZERO
VECTOR 0x1E, ERROR_CODE
VECTOR 0x1F, ERROR_ZERO
VECTOR 0x20, ERROR_ZERO    ;时钟中断处理程序
VECTOR 0x21, ERROR_ZERO    ;键盘中断对应的入口
VECTOR 0x22, ERROR_ZERO    ;级联用的
VECTOR 0x23, ERROR_ZERO    ;串口2对应的入口
VECTOR 0x24, ERROR_ZERO    ;串口1对应的入口
VECTOR 0x25, ERROR_ZERO    ;并口2对应的入口
VECTOR 0x26, ERROR_ZERO    ;软盘对应的入口
VECTOR 0x27, ERROR_ZERO    ;并口1对应的入口
VECTOR 0x28, ERROR_ZERO    ;实时时钟对应的入口
VECTOR 0x29, ERROR_ZERO    ;重定向
VECTOR 0x2A, ERROR_ZERO    ;保留
VECTOR 0x2B, ERROR_ZERO    ;保留
VECTOR 0x2C, ERROR_ZERO    ;ps/2鼠标
VECTOR 0x2d, ERROR_ZERO    ;fpu浮点单元异常
VECTOR 0x2E, ERROR_ZERO    ;硬盘
VECTOR 0x2F, ERROR_ZERO    ;保留

