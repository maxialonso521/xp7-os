#ifndef GDT_H
#define GDT_H

#include <stdint.h>

// ─── GDT Entry (8 bytes cada una) ────────────────────────────
typedef struct {
    uint16_t limit_low;     // bits 0-15 del límite
    uint16_t base_low;      // bits 0-15 de la base
    uint8_t  base_mid;      // bits 16-23 de la base
    uint8_t  access;        // flags de acceso + tipo de segmento
    uint8_t  granularity;   // bits 16-19 del límite + flags
    uint8_t  base_high;     // bits 24-31 de la base
} __attribute__((packed)) gdt_entry_t;

// ─── GDT Descriptor para LGDT ────────────────────────────────
typedef struct {
    uint16_t limit;         // tamaño de la GDT - 1
    uint32_t base;          // dirección de la GDT
} __attribute__((packed)) gdt_ptr_t;

// ─── Valores de access byte ──────────────────────────────────
#define GDT_PRESENT    0x80  // Segmento presente en memoria
#define GDT_RING0      0x00  // Privilege level 0 (kernel)
#define GDT_RING3      0x60  // Privilege level 3 (usuario)
#define GDT_CODE_SEG   0x18  // Segmento de código
#define GDT_DATA_SEG   0x10  // Segmento de datos
#define GDT_READABLE   0x02  // Código: readable; Data: writable
#define GDT_EXECUTABLE 0x08  // Segmento ejecutable

// ─── API ──────────────────────────────────────────────────────
void gdt_init(void);

// Definido en boot.asm — hace far jump para recargar CS
extern void gdt_flush(gdt_ptr_t *ptr);

#endif
