; ─── Stub vacío para cuando no hay doom.wad ──────────────────
; Ambos símbolos apuntan al mismo byte → end - start = 0
; wad_present() retorna false y DOOM no intenta cargar

section .rodata
global _binary_doom_wad_start
global _binary_doom_wad_end

_binary_doom_wad_start:
_binary_doom_wad_end:
    db 0    ; un byte dummy para que la sección no quede vacía
