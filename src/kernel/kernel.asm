; 功能：开启分页模式、更新寄存器

%include "constant.inc"

global kernelStart
extern kernelMain

[SECTION .data]
GDT:
DESC_BASE:   dq 0x00_0_0_00_000000_0000
DESC_CODE:   dq 0x00_C_F_9A_000000_FFFF     ; 可读的执行代码段,0~4GB,界限粒度4KB,32位操作数32位代码段
DESC_DATA:   dq 0x00_C_F_93_000000_FFFF     ; 可读写的数据段,映射至0～4GB,界限粒度4KB
DESC_TEXT:   dq 0xC0_0_0_93_0B8000_7FFF     ; 可读写的数据段，粒度1字节，物理地址区间0xB8000~0xB8FFF, 虚拟地址区间0xC00B_8000~0xC00B_8FFF

GDT_SIZE EQU $-GDT

GDT_PTR:
GDT_LEN:    dw  GDT_SIZE - 1
GDT_BASE:   dd  DESC_BASE

; 选择子
SELECTOR_CODE   EQU 1 << 3 + 0
SELECTOR_DATA   EQU 2 << 3 + 0
SELECTOR_TEXT   EQU 3 << 3 + 0

[SECTION .text]
[BITS 32]
kernelStart:
    call initPage
    add esp, 0xC000_0000
    mov eax, PAGE_DIR_TABLE_PHY_POS
    mov cr3, eax
    mov eax, cr0
    or eax, 0x8000_0000
    mov cr0, eax
    lgdt [GDT_PTR]
    ; 重新初始化选择子
    mov ax, SELECTOR_DATA
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov ax, SELECTOR_TEXT
    mov gs, ax
jmp SELECTOR_CODE : kernelMain

; 函数：initPage
; 功能：建立并初始化一个页目录表，建立并初始化一个内核页表。
; 参数：无
; 返回：无
initPage:
; 清空页目录表数据
.clearPd:
    mov ecx, 4096
    mov esi, 0
.clearPdLoop:
    mov byte [PAGE_DIR_TABLE_PHY_POS + esi], 0
    inc esi
    loop .clearPdLoop
.initPde:
    mov eax, PAGE_DIR_TABLE_PHY_POS
    mov edx, eax
    or eax, PAGE_US_U | PAGE_RW_W | PAGE_P
    add eax, 0x1000
    ; 虚拟地址0~2MB也映射至物理地址0~2MB
    mov [PAGE_DIR_TABLE_PHY_POS + 0x0000], eax
    ; 第768项指向内核的第一个页表，该页表存放在物理地址0x0020_1000处
    mov [PAGE_DIR_TABLE_PHY_POS + 0x0C00], eax
    ; 页目录表最后一个PDE指向页目录表本身
    sub eax, 0x1000
    mov [PAGE_DIR_TABLE_PHY_POS + 4092], eax
    ; 创建剩余的内核PDE
    ; 此处创建的原因是便于后面用户进程共享内核
    add eax, 0x2000
    mov ebx, PAGE_DIR_TABLE_PHY_POS
    mov esi, 769    ; 从769个开始
    mov ecx, 254    ; 内核占用的页目录项共256个，减去页目录表和初始化的第一个页表，余254个
.kernelPdeLoop:
    mov [ebx + esi * 4], eax
    add eax, PAGE_SIZE_4KB   ; 每个页为4096字节
    inc esi
    loop .kernelPdeLoop
; 共4096个，但目前只需要初始化内核的第一个页表
; 映射0~2MB内存的PTE,只需要512个页表项
.initPt:
    mov ebx, PAGE_DIR_TABLE_PHY_POS
    add ebx, 0x1000
    mov ecx, 512        ; 需要映射2MB的内容，2MB / 4KB = 512
    xor edx, edx        ; 清空edx,0x0000_0000~0x0020_0000
    or edx, PAGE_US_U | PAGE_RW_W | PAGE_P
    mov esi, 0
.createPteLoop:
    mov [ebx + esi * 4], edx
    add edx, PAGE_SIZE_4KB
    inc esi
    loop .createPteLoop
    ret