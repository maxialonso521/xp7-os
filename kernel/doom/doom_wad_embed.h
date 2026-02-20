#ifndef DOOM_WAD_EMBED_H
#define DOOM_WAD_EMBED_H

/*
 * ╔══════════════════════════════════════════════════════════════╗
 * ║  XP7 OS — WAD embebido en el kernel                        ║
 * ║                                                            ║
 * ║  Con WAD real (objcopy):                                   ║
 * ║    _binary_doom_wad_start → primer byte                    ║
 * ║    _binary_doom_wad_end   → byte tras el último            ║
 * ║    size = end - start                                      ║
 * ║                                                            ║
 * ║  Sin WAD (doom_wad_stub.asm):                              ║
 * ║    start == end → size == 0 → wad_present() == false       ║
 * ╚══════════════════════════════════════════════════════════════╝
 */

#include <stdint.h>
#include <stddef.h>

extern uint8_t _binary_doom_wad_start[];
extern uint8_t _binary_doom_wad_end[];

static inline uint8_t *wad_data(void) {
    return _binary_doom_wad_start;
}
static inline size_t wad_size(void) {
    return (size_t)(_binary_doom_wad_end - _binary_doom_wad_start);
}
static inline int wad_present(void) {
    return (_binary_doom_wad_end > _binary_doom_wad_start);
}

#endif
