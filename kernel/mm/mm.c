#include "mm.h"
#include "../string.h"
#include "../vga.h"

/*
 * Layout del heap:
 *  [ block_header | ... datos ... ] [ block_header | ... ] ...
 *
 * block_header:
 *   - size: tamaño del bloque de datos (sin el header)
 *   - used: 1 si está ocupado
 *   - next: puntero al siguiente bloque (NULL = último)
 */

typedef struct block_s {
    size_t         size;    // tamaño del payload
    uint32_t       magic;   // 0xDEADBEEF = válido
    uint8_t        used;
    uint8_t        _pad[3];
    struct block_s *next;
    struct block_s *prev;
} __attribute__((packed)) block_t;

#define BLOCK_MAGIC   0xDEADBEEF
#define BLOCK_HDR_SZ  sizeof(block_t)

// ─── El heap vive aquí — extern kernel_end viene del linker script
extern uint8_t kernel_end;  // símbolo del linker (fin del kernel)

static uint8_t  *heap_start = NULL;
static uint8_t  *heap_end   = NULL;
static block_t  *free_list  = NULL;
static size_t    total_free  = 0;
static size_t    total_used  = 0;

// ─── Alinear tamaño ──────────────────────────────────────────
static inline size_t align_up(size_t n) {
    return (n + MM_ALIGN - 1) & ~(MM_ALIGN - 1);
}

// ─── Inicializar el heap ──────────────────────────────────────
void mm_init(void) {
    // El heap empieza después del kernel, alineado a 4KB
    uintptr_t start = (uintptr_t)&kernel_end;
    start = (start + 0xFFF) & ~0xFFF;  // align 4KB

    heap_start = (uint8_t *)start;
    heap_end   = heap_start + MM_HEAP_SIZE;

    // Un único bloque libre que abarca todo el heap
    free_list = (block_t *)heap_start;
    free_list->size  = MM_HEAP_SIZE - BLOCK_HDR_SZ;
    free_list->magic = BLOCK_MAGIC;
    free_list->used  = 0;
    free_list->next  = NULL;
    free_list->prev  = NULL;

    total_free = free_list->size;
    total_used = 0;
}

// ─── malloc del kernel ────────────────────────────────────────
void *kmalloc(size_t size) {
    if (size == 0) return NULL;
    size = align_up(size);

    // Buscar primer bloque libre que quepa (First Fit)
    block_t *blk = free_list;
    while (blk) {
        if (!blk->used && blk->size >= size) {
            // ¿Hay espacio para dividirlo?
            if (blk->size >= size + BLOCK_HDR_SZ + MM_ALIGN) {
                // Dividir: crear nuevo bloque libre después
                block_t *next = (block_t *)((uint8_t *)blk
                                + BLOCK_HDR_SZ + size);
                next->size  = blk->size - size - BLOCK_HDR_SZ;
                next->magic = BLOCK_MAGIC;
                next->used  = 0;
                next->next  = blk->next;
                next->prev  = blk;
                if (blk->next) blk->next->prev = next;
                blk->next  = next;
                blk->size  = size;
            }
            blk->used  = 1;
            total_free -= blk->size;
            total_used += blk->size;
            return (void *)((uint8_t *)blk + BLOCK_HDR_SZ);
        }
        blk = blk->next;
    }

    // Sin memoria
    vga_set_color(VGA_LIGHT_RED, VGA_BLACK);
    vga_print("[MM] PANIC: kmalloc sin memoria!\n");
    return NULL;
}

// ─── free del kernel ──────────────────────────────────────────
void kfree(void *ptr) {
    if (!ptr) return;
    block_t *blk = (block_t *)((uint8_t *)ptr - BLOCK_HDR_SZ);

    // Validar magic
    if (blk->magic != BLOCK_MAGIC) {
        vga_set_color(VGA_LIGHT_RED, VGA_BLACK);
        vga_print("[MM] kfree: puntero invalido o corruption!\n");
        return;
    }
    if (!blk->used) {
        vga_print("[MM] kfree: doble free detectado!\n");
        return;
    }

    blk->used  = 0;
    total_used -= blk->size;
    total_free += blk->size;

    // ── Coalescencia hacia adelante ───────────────────────────
    if (blk->next && !blk->next->used) {
        blk->size += BLOCK_HDR_SZ + blk->next->size;
        blk->next  = blk->next->next;
        if (blk->next) blk->next->prev = blk;
    }
    // ── Coalescencia hacia atrás ──────────────────────────────
    if (blk->prev && !blk->prev->used) {
        blk->prev->size += BLOCK_HDR_SZ + blk->size;
        blk->prev->next  = blk->next;
        if (blk->next) blk->next->prev = blk->prev;
    }
}

// ─── calloc (alloc + zero) ────────────────────────────────────
void *kcalloc(size_t n, size_t sz) {
    void *ptr = kmalloc(n * sz);
    if (ptr) k_memset(ptr, 0, n * sz);
    return ptr;
}

// ─── realloc ─────────────────────────────────────────────────
void *krealloc(void *ptr, size_t new_size) {
    if (!ptr) return kmalloc(new_size);
    if (new_size == 0) { kfree(ptr); return NULL; }

    block_t *blk = (block_t *)((uint8_t *)ptr - BLOCK_HDR_SZ);
    if (blk->size >= new_size) return ptr;  // ya tiene espacio

    void *new_ptr = kmalloc(new_size);
    if (!new_ptr) return NULL;
    k_memcpy(new_ptr, ptr, blk->size);
    kfree(ptr);
    return new_ptr;
}

// ─── umalloc / ufree (alias para Stage 3, Stage 4 usará paging) ──
void *umalloc(size_t size) { return kmalloc(size); }
void  ufree(void *ptr)      { kfree(ptr); }

// ─── Estadísticas ─────────────────────────────────────────────
size_t mm_free_bytes(void) { return total_free; }
size_t mm_used_bytes(void) { return total_used; }

void mm_dump_stats(void) {
    vga_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
    vga_print("[MM] Heap: libre=");
    vga_print_dec((uint32_t)(total_free / 1024));
    vga_print(" KB | usado=");
    vga_print_dec((uint32_t)(total_used / 1024));
    vga_print(" KB | total=");
    vga_print_dec((uint32_t)(MM_HEAP_SIZE / 1024));
    vga_print(" KB\n");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
}
