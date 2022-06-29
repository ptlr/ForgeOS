[bits 32]
[SECTION .text]
global switch2

switch2:
    ; 栈中此处是返回地址
    ; 保护ABI约定的寄存器，下面几个寄存器需要手动维护
    push esi
    push edi
    push ebx
    push ebp

    mov eax, [esp + 20] ; 得到栈中的参数，此处是当前线程(current)的地址
    mov [eax], esp      ; 保存上一个任务的栈顶指针到当前任务的内核栈段（selfKernelStack）
                        ; selfKernelStack的偏移为0，直接保存在开头处即可
    ; 以上是备份当前线程的环境，下面是恢复下一个线程的环境
    mov eax, [esp + 24] ; 得到栈中的参数，此处是下一个线程(next)的地址
    mov esp, [eax]      ; 记录了0级栈顶指针，被换上CPU时用来恢复0级栈
                        ; 0级栈保存了进程或线程所有信息，包括3级指针
    pop ebp
    pop ebx
    pop edi
    pop esi
    ;jmp $
    ret