#ifndef _FONT_H
#define _FONT_H

#include <stdint.h>

#define FONT_W 8
#define FONT_H 8

// Datos de la fuente: 96 chars (ASCII 32-127), 8 bytes cada uno
extern const uint8_t font8x8[96][8];

// ─── API ──────────────────────────────────────────────────────
// Dibuja un carácter en el framebuffer en pos (x,y)
// fg = color del texto, bg = color de fondo (0 = transparente)
void font_draw_char(int x, int y, char c,
                    uint32_t fg, uint32_t bg, int transparent);
void font_draw_str(int x, int y, const char *s,
                   uint32_t fg, uint32_t bg, int transparent);
int  font_str_width(const char *s);  // ancho en píxeles

#endif
