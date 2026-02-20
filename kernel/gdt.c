#include "gdt.h"

// ─── Tabla GDT (null + kernel code + kernel data) ────────────
// Máximo 6 entradas por ahora: null, kcode, kdata, ucode, udata, tss
static gdt_entry_t gdt_entries[6];
static gdt_ptr_t   gdt_ptr;

// ─── Configurar una entrada de la GDT ────────────────────────
static void gdt_set_gate(int num, uint32_t base, uint32_t limit,
                         uint8_t access, uint8_t gran) {
    gdt_entries[num].base_low   = (uint16_t)(base & 0xFFFF);
    gdt_entries[num].base_mid   = (uint8_t)((base >> 16) & 0xFF);
    gdt_entries[num].base_high  = (uint8_t)((base >> 24) & 0xFF);

    gdt_entries[num].limit_low  = (uint16_t)(limit & 0xFFFF);
    gdt_entries[num].granularity = (uint8_t)((limit >> 16) & 0x0F);
    gdt_entries[num].granularity |= (gran & 0xF0);

    gdt_entries[num].access = access;
}

// ─── Inicializar GDT ─────────────────────────────────────────
void gdt_init(void) {
    gdt_ptr.limit = sizeof(gdt_entries) - 1;
    gdt_ptr.base  = (uint32_t)&gdt_entries;

    // Segmento 0: NULL descriptor (requerido por x86)
    gdt_set_gate(0, 0, 0, 0, 0);

    // Segmento 1: Kernel Code (ring 0, ejecutable, readable)
    // base=0, limit=4GB (0xFFFFF con granularity=4KB → 4GB)
    gdt_set_gate(1, 0x00000000, 0xFFFFFFFF,
                 GDT_PRESENT | GDT_RING0 | GDT_CODE_SEG | GDT_READABLE,
                 0xCF);  // 0xC = 4KB gran + 32bit; 0xF = limit high nibble

    // Segmento 2: Kernel Data (ring 0, writable)
    gdt_set_gate(2, 0x00000000, 0xFFFFFFFF,
                 GDT_PRESENT | GDT_RING0 | GDT_DATA_SEG | GDT_READABLE,
                 0xCF);

    // Segmento 3: User Code (ring 3) — para cuando implementemos userspace
    gdt_set_gate(3, 0x00000000, 0xFFFFFFFF,
                 GDT_PRESENT | GDT_RING3 | GDT_CODE_SEG | GDT_READABLE,
                 0xCF);

    // Segmento 4: User Data (ring 3)
    gdt_set_gate(4, 0x00000000, 0xFFFFFFFF,
                 GDT_PRESENT | GDT_RING3 | GDT_DATA_SEG | GDT_READABLE,
                 0xCF);

    // Cargar GDT + far jump para recargar segmentos (en boot.asm)
    gdt_flush(&gdt_ptr);
}
