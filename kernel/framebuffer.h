#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <stdint.h>
#include <stddef.h>

// ─── Resolución objetivo ─────────────────────────────────────
#define FB_WIDTH   800
#define FB_HEIGHT  600
#define FB_BPP     32

// ─── Estado del framebuffer ───────────────────────────────────
typedef struct {
    uint32_t *addr;     // dirección física del framebuffer
    uint32_t  pitch;    // bytes por fila (puede ser > width*4)
    uint32_t  width;
    uint32_t  height;
    uint8_t   bpp;
    int       ready;    // 1 si inicializado correctamente
} framebuffer_t;

extern framebuffer_t fb;

// ─── Helpers de color ─────────────────────────────────────────
// VBE 32bpp: 0x00RRGGBB en memoria little-endian = B,G,R,0
#define RGB(r,g,b)  (((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (uint32_t)(b))
#define COLOR_BLACK   RGB(0,0,0)
#define COLOR_WHITE   RGB(255,255,255)
#define COLOR_RED     RGB(255,0,0)
#define COLOR_GREEN   RGB(0,200,0)
#define COLOR_BLUE    RGB(0,0,200)

// ─── API ──────────────────────────────────────────────────────
void fb_init(uint32_t addr, uint32_t pitch,
             uint32_t w, uint32_t h, uint8_t bpp);
void fb_clear(uint32_t color);

// Primitivas básicas
static inline void fb_put_pixel(int x, int y, uint32_t color) {
    extern framebuffer_t fb;
    if ((unsigned)x >= fb.width || (unsigned)y >= fb.height) return;
    uint8_t *row = (uint8_t *)fb.addr + y * fb.pitch;
    ((uint32_t *)row)[x] = color;
}

void fb_fill_rect(int x, int y, int w, int h, uint32_t color);
void fb_draw_rect(int x, int y, int w, int h, uint32_t color);
void fb_draw_hline(int x, int y, int len, uint32_t color);
void fb_draw_vline(int x, int y, int len, uint32_t color);

// Gradiente horizontal (de color1 a color2)
void fb_fill_gradient_h(int x, int y, int w, int h,
                         uint32_t c1, uint32_t c2);
// Gradiente vertical
void fb_fill_gradient_v(int x, int y, int w, int h,
                         uint32_t c1, uint32_t c2);

// Copia una region a otra posicion
void fb_copy_rect(int dx, int dy, int sx, int sy, int w, int h);

// Blend simple (alpha 0-255 donde 255=opaco c1)
uint32_t fb_blend(uint32_t c1, uint32_t c2, uint8_t alpha);
void     fb_fill_rect_alpha(int x, int y, int w, int h,
                              uint32_t color, uint8_t alpha);

#endif
