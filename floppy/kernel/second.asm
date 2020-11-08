;[org 100200h]
[BITS 32]
section .text
global _start
extern kernel_main
extern timer_event
extern keyboard_event
global _debug_msg
global get_esp
global do_cli
global do_sti
global change_video_mode
global _halt

_start:
    mov ax, 10h
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov dword [change_video_mode_ptr], esi
    mov ebp, 7ffffh
    mov esp, ebp

    lidt [idt_descr]
    call start_pit
    
    cli

    mov ebp, 7ffffh
    mov esp, ebp

    call kernel_main

    mov ebp, 7ffffh
    mov esp, ebp

    push leave_warning_msg
    call _debug

    jmp hang

get_esp:
    mov eax, esp
    ret

_halt:
    pushf
    sti
    hlt
    popf
    ret

change_video_mode:
    pop eax
    pop eax
    sub esp, 8

    push ebp
    mov ebp, esp

    pushad
    push eax
    call pic_disable
    pop eax

    pushf
    cli
    call dword [change_video_mode_ptr]
    popf

    lidt [idt_descr]

    call pic_enable
    popad

    mov esp, ebp
    pop ebp

    ret

do_sti:
    sti
    call send_eoi
    ret

do_cli:
    cli
    ret

_debug_msg:
    pop eax
    pop eax
    sub esp, 8

    push ebp
    mov ebp, esp

    pushad
    push eax
    call _debug
    popad

    mov esp, ebp
    pop ebp
    ret

pic_mask equ 0b11111100

pic_disable:
    mov al, 0xFF
    out 0x21, al
    ret

pic_enable:
    in al, 0x21
    and al, pic_mask
    out 0x21, al
    ret

start_pit:
    cli
    ; http://www.brokenthorn.com/Resources/OSDevPic.html
    mov al, 0x11
    out 0x20, al
    out 0xA0, al

    mov al, 0x20
    out 0x21, al
    mov al, 0x28
    out 0xA1, al

    mov al, 0x04
    out 0x21, al
    mov al, 0x02
    out 0xA1, al

    mov al, 1
    out 0x21, al
    out 0xA1, al

    call pic_enable

    sti

    mov al, 0b00110000
    out 0x43, al

    call reset_timer
    call send_eoi

    ret

send_eoi:
    mov al, 0x20
    out 0x20, al
    ret

reset_timer:
    ; 44191.93Hz
    mov al, 27
    out 0x40, al
    xor al, al
    out 0x40, al
    
    ret

start_msg:
    db 'Hello, world!'
    db 0

leave_warning_msg:
    db 'Warning: left kernel_main! This could indicate something is broken.'
    db 0

dummy_err:
    add esp, 4
dummy:
    iretd

timer_msg:
    db 'IRQ0 was called.'
    db 0

timer:
    pushad
    call reset_timer
    call send_eoi
    call timer_event
    popad
    iretd

irq13_msg:
    db 'A general protection fault has occurred.'
    db 0

irq13:
    add esp, 4
    pushad
    push irq13_msg
    call _debug_msg
    pop eax
_die:
    mov ebx, 0x80386F00
    mov ecx, 0x31415926
    mov dx, 0xB004
    mov ax, 0x2000
    ; kill the CPU
    out dx, ax
    ; kill the CPU again
    mov dx, 0x604
    out dx, ax
    mov dx, 0x4004
    mov ax, 0x3400
    ; kill the CPU one more time for good measure
    out dx, ax
    ; don't let it wake up if it survived
    cli
    ; put it to rest if it isn't dead already
    hlt
    ; if it survives kill it again
    jmp _die
    ; if it got this far it deserves to live
    popad
    iretd

debug:
    pusha
    push eax
    call _debug
    popa
    iretd

_debug:
    pop eax
    pop ebx
    push eax
    xor eax, eax
_debug2:
    mov al, byte [ebx]
    cmp al, 0
    je _debug3
    push ax
    call _sendchar
    inc ebx
    jmp _debug2
_debug3:
    push word `\n`
    call _sendchar
    ret

_sendchar:
    pop ecx
    pop ax
    push ecx
    out 0xE9, al
    ret

keyboard:
    pushad
    in ax, 60h
    cmp ax, 80h
    jge _keyboard_end
    call keyboard_event
_keyboard_end:
    call send_eoi
    popad
    iretd

section .data

change_video_mode_ptr: dd 0

align 32
idt:
    %rep 8
    ; irq 0-7
    dw (dummy-_start)+200h
    dw 08h
    db 0
    db 8eh
    dw 0010h
    %endrep

    ; irq 8
    dw (dummy_err-_start)+200h
    dw 08h
    db 0
    db 8eh
    dw 0010h

    ; irq 9
    dw (dummy-_start)+200h
    dw 08h
    db 0
    db 8eh
    dw 0010h

    %rep 3
    ; irq 10-12 (0xA-0xC)
    dw (dummy_err-_start)+200h
    dw 08h
    db 0
    db 8eh
    dw 0010h
    %endrep

    ; irq 13/0eh (GPE)
    dw (irq13-_start)+200h
    dw 08h
    db 0
    db 8eh
    dw 0010h

    %rep 2+16
    ; more unused irqs
    dw 0
    dw 08h
    db 0
    db 0eh
    dw 0010h
    %endrep

    ; irq 32/20h
    dw (timer-_start)+200h
    dw 08h
    db 0
    db 8eh
    dw 0010h

    ; irq 33/21h
    dw (keyboard-_start)+200h
    dw 08h
    db 0
    db 8eh
    dw 0010h

    %rep 5
    ; more unused irqs
    dw 0
    dw 08h
    db 0
    db 0eh
    dw 0010h
    %endrep

    ; irq 39/27h (https://forum.osdev.org/viewtopic.php?t=11379)
    dw (dummy-_start)+200h
    dw 08h
    db 0
    db 8eh
    dw 0010h

idt_end:

idt_descr:
    dw idt_end - idt
    dd idt

hang:
    cli
    hlt
    jmp hang