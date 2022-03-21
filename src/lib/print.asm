; ！分页模式下可用
global cPutChar         ; 到处标号供外部使用

[BITS 32]
[SECTION .text]
;函数名：cPutChar
;参数：color(1B), char(1B)，按cdecl调用协议，把参数保存在栈中，从右往左开始压入参数，由调用者清理。
;返回：无
cPutChar:
    pushad       ; 入栈顺序是EAX、ECX、EDX、EBX、ESP、EBP、ESI、EDI
    ; 初始化数据位置
    ;mov ax, 3 << 3 + 0
    ;mov gs, ax
    mov ebp, esp
.getCursor:
    ; 清空，后面需要使用
    ; 只使用BX时会被编译器优化
    xor ebx, ebx
    ; 光标数据保存到bx
    ; 高8位
    mov al, 0x0E
    mov dx, 0x03D4
    out dx,al

    mov dx, 0x03D5
    in al, dx
    mov bh, al
    ; 低8位
    mov al, 0x0F
    mov dx, 0x03D4
    out dx, al

    mov dx, 0x03D5
    in al, dx
    mov bl, al
.getChar:
    ; 调用的时候会压入4字节地址
    ; 保存寄存器环境
    ; 获取颜色
    mov edx, [ebp + 36]      ; 所有寄存器的值,4*8=32,加上主调函数的返回地址4字节,故esp+36
    ; 获取字符
    mov eax, [ebp + 40]      ; 在颜色后压入字符的值
    mov ah, dl               ; ax中保存带颜色的字符
    cmp al, 0x0A
    jz .newLine
    cmp al, 0x0D
    jz .newLine
    cmp al, 0x08
    jz .backspace
    cmp al, 0x20
    jb .putEnd
    cmp al, 0x7E
    ja .putEnd
    jmp .normalChar
.backspace:
    dec bx
    shl bx, 1   ; 按位向左移1位，相当于乘2
    mov word [gs:ebx], 0x0720   ; 黑底白色空格
    shr bx, 1   ; 向右移1位
    jmp .setCursor
.normalChar:
    ; 在光标处写入带属性的字符
    shl bx, 1
    mov [gs:ebx], ax
    shr bx, 1
    inc bx
    cmp bx, 2000    ; 判断是否在第一页的末尾
    jl .setCursor   ; 小于是直接设置光标,否则进入新的一行
.newLine:
    xor dx, dx      ; 16位除法
    mov ax, bx      ; ax存被除数低16位， dx存被除数的高16位 
    mov si, 80      ; 每行80个字符
    div si          ; 除数保存在任意寄存器或内存中，ax保存商，dx保存余数
    sub bx, dx      ; 定位到当前行首
    add bx, 80      ; 将光标指向下一行
    cmp bx, 2000    ; 
    jl .setCursor   ; 如果下一行在第一屏内则设置光标，否则向上滚屏以清出新的空行
.rollScreen:
    cld                 ; 设置自增方向， 每执行一次自增对应的数目，比如movsb时加1， movsw时加2, movs加4
    mov ecx,960         ; 第1~25行共80 * 24 * 2  / 4= 1920 / 2 = 960， 需要移动960次
    mov esi,0x00A0  ; 第1行的行首   ds:esi指向起始位置
    mov edi,0x0000  ; 第0行的行首   es:edi指向数据的目的地
    rep movsd           ; 每次移动4字节
    ;清空最后一行
    mov bx,3840
    mov ecx,80
.cls:
    mov word [gs:ebx], 0x0720    ;黑底白字的空格
    add ebx,2
    loop .cls
    mov bx,1920
.setCursor:
    ; 设置光标高8位
    mov al, 0x0E
    mov dx, 0x03D4
    out dx, al
    mov dx, 0x03D5
    mov al, bh
    out dx, al
    ; 设置光标低8位
    mov al, 0x0F
    mov dx, 0x03D4
    out dx, al
    mov dx, 0x03D5
    mov al, bl
    out dx, al
.putEnd:
    popad
    ret
