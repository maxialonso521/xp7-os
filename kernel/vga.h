#ifndef VGA_H
#define VGA_H

#include <stdint.h>
#include <stddef.h>

// ─── Colores VGA (16 colores clásicos) ───────────────────────
typedef enum {
    VGA_BLACK         = 0,
    VGA_BLUE          = 1,
    VGA_GREEN         = 2,
    VGA_CYAN          = 3,
    VGA_RED           = 4,
    VGA_MAGENTA       = 5,
    VGA_BROWN         = 6,
    VGA_LIGHT_GREY    = 7,
    VGA_DARK_GREY     = 8,
    VGA_LIGHT_BLUE    = 9,
    VGA_LIGHT_GREEN   = 10,
    VGA_LIGHT_CYAN    = 11,
    VGA_LIGHT_RED     = 12,
    VGA_LIGHT_MAGENTA = 13,
    VGA_LIGHT_BROWN   = 14,  // Yellow
    VGA_WHITE         = 15,
} vga_color_t;

#define VGA_WIDTH  80
#define VGA_HEIGHT 25
#define VGA_MEMORY 0xB8000

// ─── API Pública ──────────────────────────────────────────────
void     vga_init(void);
void     vga_clear(void);
void     vga_set_color(vga_color_t fg, vga_color_t bg);
void     vga_put_char(char c);
void     vga_print(const char *str);
void     vga_print_at(const char *str, uint8_t x, uint8_t y);
void     vga_print_hex(uint32_t val);
void     vga_print_dec(uint32_t val);
void     vga_scroll(void);
void     vga_set_cursor(uint8_t x, uint8_t y);
uint8_t  vga_get_col(void);
uint8_t  vga_get_row(void);
void     vga_newline(void);
void     vga_delete_char(void);

#endif
