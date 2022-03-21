; 段地址：偏移移地址 ==> 地址 = 段地址 << 8 + 偏移地址

; 简单的磁盘配置
; Loader开始存放的逻辑扇区数
LOADER_SEC          EQU 1
LOADER_LBA_LOW16    EQU 0x0000_0001
LOADER_LBA_HIGH12   EQU 0x0000_0000
; Loader占用的扇区数
LOADER_CNT EQU 2

; 栈的位置
; 错误警告：设置成0x7BFF = 0x07BF:0x000F时导致大量用到压栈操作时压栈错误，会导致 int 0x10, 03不能正确执行

STACK_SEG EQU 0x0000
STACK_OFF EQU 0x7C00

; 内存信息的位置
MME_CNT EQU 0x7E00
MME_SEG EQU 0x07E0
MME_OFF EQU 0x0004

; Loader位置
LOADER_SEG EQU 0x0860
LOADER_OFF EQU 0x0000

; 内核的数据
KERNEL_LBA28    EQU 3
KERNEL_CNT      EQU 2
KERNEL_SELECTOR EQU 1 << 3 + 0
KERNEL_OFFSET   EQU 0x0010_0000 