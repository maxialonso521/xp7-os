; ─── DOOM IRQ0 Timer Handler ─────────────────────────────────
; Llamado cada ~1ms cuando el PIT dispara
section .text
global doom_irq0_asm
extern doom_irq0_handler

doom_irq0_asm:
    pusha
    call doom_irq0_handler
    popa
    iret
