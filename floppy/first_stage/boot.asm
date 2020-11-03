use16
org 7c00h
; http://www.osdever.net/tutorials/view/the-world-of-protected-mode
_start:
    mov ax, 0
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ds, ax
    mov ss, ax
    jmp 0:start
start: 
    mov byte [boot_drive], dl
    mov ebp, stack
    mov esp, ebp

    call a20
    call double_toggle

    call read_all
    ;call _change_video_mode
    call enter_pmode
    use32
    mov ebp, new_stack
    mov esp, ebp
    ;mov esi, enter_rmode
    ;mov edi, enter_pmode
    mov esi, change_video_mode
    jmp 08h:jmp_addr
    use16

jmp_addr equ 100000h+200h
new_stack equ 0a000h

a20:
    ; https://wiki.osdev.org/A20_Line
    in al, 0x92
    test al, 2
    jnz a20_done
    or al, 2
    and al, 0xFE
    out 0x92, al
a20_done:
    ret

cvm_ebp: dd 0
cvm_esp: dd 0

change_video_mode:
    use32
    pushad
    ;push ebp
    mov dword [cvm_ebp], ebp
    mov dword [cvm_esp], esp
    mov byte [video_mode], al
    mov esp, stack
    mov ebp, esp
    call enter_rmode
    use16
    mov al, byte [video_mode]
    
    call _change_video_mode
    call enter_pmode
    use32
    ;pop ebp
    mov ebp, dword [cvm_ebp]
    mov esp, dword [cvm_esp]
    popad
    ret
    use16

_change_video_mode:
    xor ah, ah
    int 10h
    ret

double_toggle:
    call enter_pmode
    use32
    call enter_rmode
    use16
    ret

; set dh before calling
read_all:
    xor ch, ch
_read_all:
    xor dh, dh
    push cx
    call read_cylinder
    pop cx
    mov dh, 1
    push cx
    call read_cylinder
    pop cx
    inc ch
    add dword [current_lba], sectors_per*2
    cmp dword [current_lba], max_lba
    jne _read_all
    ret

; set ch and dh before calling
read_cylinder:
    mov dl, byte [boot_drive]
    mov cl, 1
    xor ax, ax
    mov es, ax
    mov ah, 2
    mov al, sectors_per
    mov bx, scratch_start
    int 13h
    jc read_cylinder
    xor ax, ax
    mov es, ax
    pushad
    call enter_pmode
    use32
    popad
    call copy_scratch
    pushad
    call enter_rmode
    use16
    popad
    ret

copy_scratch:
    use32
    cld
    mov ecx, scratch_len
    mov esi, scratch_start
    mov edi, dword [counter]
    add dword [counter], ecx
    shr ecx, 2
    rep movsd
    ret
    use16

enter_pmode:
    xor eax, eax
    pop ax
    push eax
    mov dword [esp_tmp], esp
    cli
    xor ax, ax
    mov ds, ax
    lgdt [gdt_desc]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp 08h:clear_pipe

back2:
    ret


clear_pipe2:
    mov ax, 10h
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ebp, stack
    ;mov esp, ebp

    
    mov eax, cr0
    and eax, 0fffffffeh
    mov cr0, eax

    lidt [idt_real]
back:
    mov ax, 0
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ds, ax
    mov ss, ax
    mov ebp, stack
    mov esp, dword [esp_tmp]
    
    jmp 00h:back2

use32

clear_pipe:
    mov ax, 10h
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ebp, stack
    mov esp, dword [esp_tmp]
    ret

enter_rmode:
    pop eax
    push word ax
    mov dword [esp_tmp], esp
    cli
    lgdt [gdt_real_desc]
    ;use16
    jmp 08h:clear_pipe2
    ;use32

counter: dd 0100000h

esp_tmp: dd 0

stack equ 09000h

scratch_start equ 0A000h
sectors_per equ 18
max_lba equ 80 * 18 * 2
scratch_len equ 512*sectors_per

gdt_desc:
    dw gdt_end - gdt
    dd gdt

gdt_real_desc:
    dw gdt_real_end - gdt_real
    dd gdt_real

idt_real:
    dw 03ffh
    dd 0

gdt:
    gdt_null:
        dq 0
    gdt_code:
        dw 0FFFFh
        dw 0
        db 0
        db 10011010b
        db 11001111b
        db 0
    gdt_data:
        dw 0FFFFh
        dw 0
        db 0
        db 10010010b
        db 11001111b
        db 0
    gdt_end:

gdt_real:
    gdt_real_null:
        dq 0
    gdt_real_code:
        dw 0FFFFh
        dw 0
        db 0
        db 10011010b
        db 10001111b
        db 0
    gdt_real_data:
        dw 0FFFFh
        dw 0
        db 0
        db 10010010b
        db 10001111b
        db 0
    gdt_real_end:

current_lba: dd 0

boot_drive: db 0

video_mode: db 0

times 510-($-$$) db 0
db 0x55
db 0xAA

