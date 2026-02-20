#ifndef PIC_H
#define PIC_H

#include <stdint.h>

// ─── Puertos I/O del PIC ─────────────────────────────────────
#define PIC1_CMD   0x20    // Master PIC - Command port
#define PIC1_DATA  0x21    // Master PIC - Data port (IMR)
#define PIC2_CMD   0xA0    // Slave PIC - Command port
#define PIC2_DATA  0xA1    // Slave PIC - Data port (IMR)

// ─── Comandos ICW1 (Initialization Command Words) ────────────
#define PIC_ICW1_INIT  0x10    // Modo inicialización
#define PIC_ICW1_ICW4  0x01    // Indica que ICW4 viene después
#define PIC_ICW4_8086  0x01    // Modo 8086/88

// ─── EOI (End of Interrupt) ───────────────────────────────────
#define PIC_EOI        0x20

// ─── API ──────────────────────────────────────────────────────
void    pic_init(void);
void    pic_send_eoi(uint8_t irq);
void    pic_set_mask(uint8_t irq);
void    pic_clear_mask(uint8_t irq);

// ─── Inline I/O helpers ──────────────────────────────────────
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}
static inline void io_wait(void) {
    outb(0x80, 0x00);  // Puerto 0x80 = delay de ~1μs (diagnóstico POST)
}

#endif
