#include "pic.h"

/*
 * ╔══════════════════════════════════════════════════════════╗
 * ║  PIC 8259A — Remapeo de IRQs                            ║
 * ║                                                         ║
 * ║  Por defecto el PIC mapea IRQ0-7 a INT 0x08-0x0F       ║
 * ║  Eso COLISIONA con las excepciones del CPU (0x00-0x1F) ║
 * ║  Solución: remapear IRQ0-7 → 0x20-0x27                ║
 * ║                          IRQ8-15 → 0x28-0x2F          ║
 * ╚══════════════════════════════════════════════════════════╝
 */

void pic_init(void) {
    // Guardar masks actuales (para restaurarlas después del remap)
    uint8_t mask1 = inb(PIC1_DATA);
    uint8_t mask2 = inb(PIC2_DATA);

    // ── Inicialización cascada (ICW1) ─────────────────────────
    outb(PIC1_CMD,  PIC_ICW1_INIT | PIC_ICW1_ICW4);
    io_wait();
    outb(PIC2_CMD,  PIC_ICW1_INIT | PIC_ICW1_ICW4);
    io_wait();

    // ── Vector base (ICW2) ────────────────────────────────────
    outb(PIC1_DATA, 0x20);   // Master: IRQ0 → INT 0x20
    io_wait();
    outb(PIC2_DATA, 0x28);   // Slave:  IRQ8 → INT 0x28
    io_wait();

    // ── Configurar cascada (ICW3) ─────────────────────────────
    outb(PIC1_DATA, 0x04);   // Master: Slave conectado en IRQ2 (bit 2 = 0b100)
    io_wait();
    outb(PIC2_DATA, 0x02);   // Slave:  ID cascada = 2
    io_wait();

    // ── Modo 8086 (ICW4) ──────────────────────────────────────
    outb(PIC1_DATA, PIC_ICW4_8086);
    io_wait();
    outb(PIC2_DATA, PIC_ICW4_8086);
    io_wait();

    // ── Restaurar máscaras ────────────────────────────────────
    // Habilitamos solo IRQ1 (teclado) en el master; el resto enmascarado
    outb(PIC1_DATA, mask1 & ~(1 << 1));   // IRQ1 = teclado = unmask
    outb(PIC2_DATA, mask2);

    // ── Habilitar interrupciones (STI) ────────────────────────
    __asm__ volatile("sti");
}

// ─── Señal EOI al PIC ────────────────────────────────────────
void pic_send_eoi(uint8_t irq) {
    if (irq >= 8)
        outb(PIC2_CMD, PIC_EOI);  // Si vino del slave, le avisamos también
    outb(PIC1_CMD, PIC_EOI);
}

// ─── Enmascarar (deshabilitar) un IRQ ─────────────────────────
void pic_set_mask(uint8_t irq) {
    uint16_t port = (irq < 8) ? PIC1_DATA : PIC2_DATA;
    if (irq >= 8) irq -= 8;
    outb(port, inb(port) | (1 << irq));
}

// ─── Desenmascarar (habilitar) un IRQ ────────────────────────
void pic_clear_mask(uint8_t irq) {
    uint16_t port = (irq < 8) ? PIC1_DATA : PIC2_DATA;
    if (irq >= 8) irq -= 8;
    outb(port, inb(port) & ~(1 << irq));
}
