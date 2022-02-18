[BITS 32]
    mov ebx, 0xB8000 + 160 * 4

    mov ah, 0x03

    mov al, '['
    mov [gs:ebx], ax
    add ebx,2

    mov al, '0'
    mov [gs:ebx], ax
    add ebx,2

    mov al,'5'
    mov [gs:ebx], ax
    add ebx, 2

    mov al, ']'
    mov [gs:ebx],ax
    add ebx, 2

    mov al, ' '
    mov [gs:ebx],ax
    add ebx, 2

    mov al, 'k'
    mov [gs:ebx],ax
    add ebx, 2

    mov al, 'e'
    mov [gs:ebx],ax
    add ebx, 2

    mov al, 'r'
    mov [gs:ebx],ax
    add ebx, 2

    mov al, 'n'
    mov [gs:ebx],ax
    add ebx, 2

    mov al, 'e'
    mov [gs:ebx],ax
    add ebx, 2

    mov al, 'l'
    mov [gs:ebx],ax
    add ebx, 2
    jmp $