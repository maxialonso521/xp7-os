#include "framebuffer.h"
#include "string.h"

framebuffer_t fb = {0};

void fb_init(uint32_t addr, uint32_t pitch,
             uint32_t w, uint32_t h, uint8_t bpp) {
    fb.addr  = (uint32_t *)addr;
    fb.pitch = pitch;
    fb.width = w;
    fb.height = h;
    fb.bpp   = bpp;
    fb.ready = 1;
}

void fb_clear(uint32_t color) {
    for (uint32_t y = 0; y < fb.height; y++) {
        uint32_t *row = (uint32_t *)((uint8_t *)fb.addr + y * fb.pitch);
        for (uint32_t x = 0; x < fb.width; x++)
            row[x] = color;
    }
}

void fb_fill_rect(int x, int y, int w, int h, uint32_t color) {
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > (int)fb.width)  w = fb.width  - x;
    if (y + h > (int)fb.height) h = fb.height - y;
    if (w <= 0 || h <= 0) return;
    for (int row = y; row < y + h; row++) {
        uint32_t *p = (uint32_t *)((uint8_t *)fb.addr + row * fb.pitch) + x;
        for (int col = 0; col < w; col++)
            p[col] = color;
    }
}

void fb_draw_rect(int x, int y, int w, int h, uint32_t color) {
    fb_fill_rect(x,     y,     w, 1, color);  // top
    fb_fill_rect(x,     y+h-1, w, 1, color);  // bottom
    fb_fill_rect(x,     y,     1, h, color);  // left
    fb_fill_rect(x+w-1, y,     1, h, color);  // right
}

void fb_draw_hline(int x, int y, int len, uint32_t color) {
    fb_fill_rect(x, y, len, 1, color);
}

void fb_draw_vline(int x, int y, int len, uint32_t color) {
    fb_fill_rect(x, y, 1, len, color);
}

// ─── Gradiente horizontal ─────────────────────────────────────
void fb_fill_gradient_h(int x, int y, int w, int h,
                         uint32_t c1, uint32_t c2) {
    if (w <= 0 || h <= 0) return;
    uint8_t r1 = (c1 >> 16) & 0xFF, g1 = (c1 >> 8) & 0xFF, b1 = c1 & 0xFF;
    uint8_t r2 = (c2 >> 16) & 0xFF, g2 = (c2 >> 8) & 0xFF, b2 = c2 & 0xFF;
    for (int col = 0; col < w; col++) {
        // Interpolación lineal
        uint8_t r = (uint8_t)(r1 + (int)(r2-r1) * col / w);
        uint8_t g = (uint8_t)(g1 + (int)(g2-g1) * col / w);
        uint8_t b = (uint8_t)(b1 + (int)(b2-b1) * col / w);
        uint32_t color = RGB(r, g, b);
        fb_fill_rect(x + col, y, 1, h, color);
    }
}

// ─── Gradiente vertical ───────────────────────────────────────
void fb_fill_gradient_v(int x, int y, int w, int h,
                         uint32_t c1, uint32_t c2) {
    if (w <= 0 || h <= 0) return;
    uint8_t r1 = (c1 >> 16) & 0xFF, g1 = (c1 >> 8) & 0xFF, b1 = c1 & 0xFF;
    uint8_t r2 = (c2 >> 16) & 0xFF, g2 = (c2 >> 8) & 0xFF, b2 = c2 & 0xFF;
    for (int row = 0; row < h; row++) {
        uint8_t r = (uint8_t)(r1 + (int)(r2-r1) * row / h);
        uint8_t g = (uint8_t)(g1 + (int)(g2-g1) * row / h);
        uint8_t b = (uint8_t)(b1 + (int)(b2-b1) * row / h);
        uint32_t color = RGB(r, g, b);
        fb_fill_rect(x, y + row, w, 1, color);
    }
}

// ─── Blend ────────────────────────────────────────────────────
uint32_t fb_blend(uint32_t c1, uint32_t c2, uint8_t alpha) {
    uint8_t r1 = (c1>>16)&0xFF, g1 = (c1>>8)&0xFF, b1 = c1&0xFF;
    uint8_t r2 = (c2>>16)&0xFF, g2 = (c2>>8)&0xFF, b2 = c2&0xFF;
    uint8_t r = (uint8_t)((r1*alpha + r2*(255-alpha)) / 255);
    uint8_t g = (uint8_t)((g1*alpha + g2*(255-alpha)) / 255);
    uint8_t b = (uint8_t)((b1*alpha + b2*(255-alpha)) / 255);
    return RGB(r,g,b);
}

void fb_fill_rect_alpha(int x, int y, int w, int h,
                         uint32_t color, uint8_t alpha) {
    if (x < 0){w+=x;x=0;} if(y<0){h+=y;y=0;}
    if(x+w>(int)fb.width) w=fb.width-x;
    if(y+h>(int)fb.height) h=fb.height-y;
    if(w<=0||h<=0) return;
    for(int row=y; row<y+h; row++) {
        uint32_t *p=(uint32_t*)((uint8_t*)fb.addr+row*fb.pitch)+x;
        for(int col=0; col<w; col++)
            p[col] = fb_blend(color, p[col], alpha);
    }
}

// ─── Copy region ─────────────────────────────────────────────
void fb_copy_rect(int dx, int dy, int sx, int sy, int w, int h) {
    for(int row=0; row<h; row++) {
        uint32_t *src = (uint32_t*)((uint8_t*)fb.addr+(sy+row)*fb.pitch)+sx;
        uint32_t *dst = (uint32_t*)((uint8_t*)fb.addr+(dy+row)*fb.pitch)+dx;
        for(int col=0; col<w; col++) dst[col]=src[col];
    }
}
