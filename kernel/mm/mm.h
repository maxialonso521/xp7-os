#ifndef MM_H
#define MM_H

/*
 * ╔══════════════════════════════════════════════════════════════╗
 * ║  XP7 OS — Memory Manager                                   ║
 * ║  Implementa malloc/free sin libc                           ║
 * ║  Algoritmo: Free-list con coalescencia                     ║
 * ║  Heap: desde kernel_end hasta 16 MB (configurable)         ║
 * ╚══════════════════════════════════════════════════════════════╝
 */

#include <stdint.h>
#include <stddef.h>

// ─── Tamaño del heap del kernel ───────────────────────────────
#define MM_HEAP_SIZE   (8 * 1024 * 1024)   // 8 MB para el kernel
#define MM_USER_SIZE   (16 * 1024 * 1024)  // 16 MB para procesos

// ─── Alineación ───────────────────────────────────────────────
#define MM_ALIGN       16  // Alinear a 16 bytes (SSE-safe)

// ─── API pública ──────────────────────────────────────────────
void  mm_init(void);

void *kmalloc(size_t size);          // Alloc kernel
void  kfree(void *ptr);              // Free kernel
void *krealloc(void *ptr, size_t n); // Realloc kernel
void *kcalloc(size_t n, size_t sz);  // Alloc + zero

// Para procesos de usuario (Stage 4+)
void *umalloc(size_t size);
void  ufree(void *ptr);

// Debug
void  mm_dump_stats(void);
size_t mm_free_bytes(void);
size_t mm_used_bytes(void);

// ─── Región de memoria para cargar ejecutables ───────────────
// Se llama "user space" aunque en Stage 3 sigue siendo ring 0
// Stage 4 lo moverá a ring 3 con paging real
#define USER_LOAD_BASE  0x01000000  // 16 MB — aquí van los .exe
#define USER_STACK_TOP  0x02000000  // 32 MB — stack de procesos

#endif
