#include "idt.h"
#include "string.h"

static idt_entry_t idt_entries[IDT_MAX_ENTRIES];
static idt_ptr_t   idt_ptr;

void idt_set_gate(uint8_t num, uint32_t handler,
                  uint16_t selector, uint8_t type_attr) {
    idt_entries[num].offset_low  = (uint16_t)(handler & 0xFFFF);
    idt_entries[num].offset_high = (uint16_t)((handler >> 16) & 0xFFFF);
    idt_entries[num].selector    = selector;
    idt_entries[num].zero        = 0;
    idt_entries[num].type_attr   = type_attr;
}

void idt_init(void) {
    idt_ptr.limit = sizeof(idt_entries) - 1;
    idt_ptr.base  = (uint32_t)&idt_entries;
    k_memset(&idt_entries, 0, sizeof(idt_entries));

    // INT 0x21 = IRQ1 = Teclado
    idt_set_gate(0x21, (uint32_t)isr_keyboard, 0x08, IDT_INTERRUPT_GATE);

    // INT 0x2C = IRQ12 = Mouse PS/2
    idt_set_gate(0x2C, (uint32_t)isr_mouse, 0x08, IDT_INTERRUPT_GATE);

    idt_load(&idt_ptr);
}
