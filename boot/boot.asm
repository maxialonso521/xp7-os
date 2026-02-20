; ============================================================
;  XP7 OS — Boot Entry (Multiboot + Framebuffer VBE)
;  Stage 2: Solicita modo grafico 800x600x32 via GRUB
; ============================================================

MBOOT_PAGE_ALIGN    equ 1 << 0
MBOOT_MEM_INFO      equ 1 << 1
MBOOT_VIDEO_MODE    equ 1 << 2
MBOOT_HEADER_MAGIC  equ 0x1BADB002
MBOOT_HEADER_FLAGS  equ MBOOT_PAGE_ALIGN | MBOOT_MEM_INFO | MBOOT_VIDEO_MODE
MBOOT_CHECKSUM      equ -(MBOOT_HEADER_MAGIC + MBOOT_HEADER_FLAGS)

section .multiboot
align 4
    dd MBOOT_HEADER_MAGIC
    dd MBOOT_HEADER_FLAGS
    dd MBOOT_CHECKSUM
    dd 0    ; header_addr  (ignorado con ELF)
    dd 0    ; load_addr
    dd 0    ; load_end_addr
    dd 0    ; bss_end_addr
    dd 0    ; entry_addr
    dd 0    ; mode_type: 0 = framebuffer lineal
    dd 800  ; width
    dd 600  ; height
    dd 32   ; depth (bpp)

section .bss
align 16
stack_bottom:
    resb 16384
stack_top:

section .text

global gdt_flush
gdt_flush:
    mov eax, [esp+4]
    lgdt [eax]
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    jmp 0x08:.flush
.flush:
    ret

global idt_load
idt_load:
    mov eax, [esp+4]
    lidt [eax]
    ret

global isr_keyboard
extern keyboard_handler
isr_keyboard:
    pusha
    call keyboard_handler
    popa
    iret

global isr_mouse
extern mouse_handler
isr_mouse:
    pusha
    call mouse_handler
    popa
    iret

global _start
extern kernel_main
_start:
    mov esp, stack_top
    push ebx
    push eax
    call kernel_main
    cli
.hang:
    hlt
    jmp .hang
