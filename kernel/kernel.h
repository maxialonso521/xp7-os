#ifndef KERNEL_H
#define KERNEL_H

#include <stdint.h>

// ─── Funciones de memoria dinámica ────────────────────────────
void *k_malloc(uint32_t size);
void *k_calloc(uint32_t nmemb, uint32_t size);
void *k_realloc(void *ptr, uint32_t size);
void k_free(void *ptr);

// ─── Información del sistema ──────────────────────────────────
typedef struct {
    uint32_t mem_total;      // KB
    uint32_t mem_used;       // KB
    uint32_t mem_free;       // KB
} system_info_t;

system_info_t *kernel_get_info(void);

// ─── Estructura Multiboot Info (simplificada) ─────────────────
typedef struct {
    uint32_t flags;
    uint32_t mem_lower;     // KB de RAM por debajo de 1 MB
    uint32_t mem_upper;     // KB de RAM por encima de 1 MB
} __attribute__((packed)) multiboot_info_t;

// ─── Kernel main entry point ──────────────────────────────────
void kernel_main(uint32_t magic, multiboot_info_t *mbi);

#endif
