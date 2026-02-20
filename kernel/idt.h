#ifndef IDT_H
#define IDT_H

#include <stdint.h>

// ─── IDT Gate descriptor (8 bytes) ───────────────────────────
typedef struct {
    uint16_t offset_low;    // bits 0-15 de la dirección del handler
    uint16_t selector;      // selector de segmento de código (GDT)
    uint8_t  zero;          // siempre 0
    uint8_t  type_attr;     // tipo + atributos del gate
    uint16_t offset_high;   // bits 16-31 de la dirección del handler
} __attribute__((packed)) idt_entry_t;

// ─── IDT Pointer para LIDT ───────────────────────────────────
typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) idt_ptr_t;

// ─── type_attr flags ─────────────────────────────────────────
#define IDT_INTERRUPT_GATE  0x8E  // Present + Ring0 + 32bit Interrupt Gate
#define IDT_TRAP_GATE       0x8F  // Present + Ring0 + 32bit Trap Gate

// ─── Número de entradas en la IDT ────────────────────────────
#define IDT_MAX_ENTRIES 256

// ─── API ──────────────────────────────────────────────────────
void idt_init(void);
void idt_set_gate(uint8_t num, uint32_t handler,
                  uint16_t selector, uint8_t type_attr);

// Definido en boot.asm
extern void idt_load(idt_ptr_t *ptr);

// ─── Handlers de ISR (definidos en boot.asm) ─────────────────
extern void isr_keyboard(void);
extern void isr_mouse(void);

#endif
