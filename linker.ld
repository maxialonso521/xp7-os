/*
 * XP7 OS — Linker Script para x86 Bare Metal
 * El kernel se carga en la dirección 1MB (0x00100000)
 * porque GRUB y convención Multiboot lo ponen ahí.
 */

ENTRY(_start)

SECTIONS {
    /* ─── Cargamos todo desde 1 MB ───────────────────────── */
    . = 1M;

    /* ─── .text: Código ejecutable ───────────────────────── */
    .text BLOCK(4K) : ALIGN(4K) {
        *(.multiboot)   /* PRIMERO el header multiboot — GRUB lo busca en los primeros 8KB */
        *(.text)
        *(.text.*)
    }

    /* ─── .rodata: Constantes y strings de solo lectura ──── */
    .rodata BLOCK(4K) : ALIGN(4K) {
        *(.rodata)
        *(.rodata.*)
    }

    /* ─── .data: Datos inicializados ─────────────────────── */
    .data BLOCK(4K) : ALIGN(4K) {
        *(.data)
        *(.data.*)
    }

    /* ─── .bss: Datos no inicializados (stack, buffers) ──── */
    .bss BLOCK(4K) : ALIGN(4K) {
        *(COMMON)
        *(.bss)
        *(.bss.*)
    }

    /* ─── Símbolo de fin del kernel (útil para el allocator) */
    kernel_end = .;
}
