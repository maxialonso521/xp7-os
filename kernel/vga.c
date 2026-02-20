#include "vga.h"
#include <stdint.h>

// ─── Estado interno del driver ───────────────────────────────
static uint16_t *vga_buf   = (uint16_t *)VGA_MEMORY;
static uint8_t   cur_color = 0;
static uint8_t   col       = 0;
static uint8_t   row       = 0;

// ─── I/O port helpers ────────────────────────────────────────
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// ─── Construye entrada VGA (carácter + atributo de color) ─────
static inline uint16_t vga_entry(char c, uint8_t color) {
    return (uint16_t)c | ((uint16_t)color << 8);
}

static inline uint8_t make_color(vga_color_t fg, vga_color_t bg) {
    return (uint8_t)fg | ((uint8_t)bg << 4);
}

// ─── Cursor hardware via CRTC ────────────────────────────────
void vga_set_cursor(uint8_t x, uint8_t y) {
    uint16_t pos = (uint16_t)(y * VGA_WIDTH + x);
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
    col = x;
    row = y;
}

uint8_t vga_get_col(void) { return col; }
uint8_t vga_get_row(void) { return row; }

// ─── Inicializar pantalla ─────────────────────────────────────
void vga_init(void) {
    cur_color = make_color(VGA_LIGHT_GREY, VGA_BLACK);
    col = 0;
    row = 0;
    // Habilitar cursor VGA (scanlines 14-15 = cursor pequeño al fondo)
    outb(0x3D4, 0x0A);
    outb(0x3D5, (inb(0x3D5) & 0xC0) | 14);
    outb(0x3D4, 0x0B);
    outb(0x3D5, (inb(0x3D5) & 0xE0) | 15);
    vga_clear();
}

// ─── Limpiar pantalla ─────────────────────────────────────────
void vga_clear(void) {
    for (int y = 0; y < VGA_HEIGHT; y++)
        for (int x = 0; x < VGA_WIDTH; x++)
            vga_buf[y * VGA_WIDTH + x] = vga_entry(' ', cur_color);
    col = 0;
    row = 0;
    vga_set_cursor(0, 0);
}

// ─── Cambiar color actual ─────────────────────────────────────
void vga_set_color(vga_color_t fg, vga_color_t bg) {
    cur_color = make_color(fg, bg);
}

// ─── Scroll de 1 línea ───────────────────────────────────────
void vga_scroll(void) {
    // Mover todo una fila hacia arriba
    for (int y = 0; y < VGA_HEIGHT - 1; y++)
        for (int x = 0; x < VGA_WIDTH; x++)
            vga_buf[y * VGA_WIDTH + x] = vga_buf[(y + 1) * VGA_WIDTH + x];
    // Limpiar la última fila
    for (int x = 0; x < VGA_WIDTH; x++)
        vga_buf[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = vga_entry(' ', cur_color);
    row = VGA_HEIGHT - 1;
}

// ─── Nueva línea ─────────────────────────────────────────────
void vga_newline(void) {
    col = 0;
    if (++row >= VGA_HEIGHT)
        vga_scroll();
    vga_set_cursor(col, row);
}

// ─── Borrar un carácter (backspace) ──────────────────────────
void vga_delete_char(void) {
    if (col > 0) {
        col--;
        vga_buf[row * VGA_WIDTH + col] = vga_entry(' ', cur_color);
        vga_set_cursor(col, row);
    }
}

// ─── Escribir un carácter ─────────────────────────────────────
void vga_put_char(char c) {
    if (c == '\n') {
        vga_newline();
        return;
    }
    if (c == '\r') {
        col = 0;
        vga_set_cursor(col, row);
        return;
    }
    if (c == '\t') {
        int spaces = 4 - (col % 4);
        for (int i = 0; i < spaces; i++) vga_put_char(' ');
        return;
    }
    if (c == '\b') {
        vga_delete_char();
        return;
    }

    vga_buf[row * VGA_WIDTH + col] = vga_entry(c, cur_color);
    if (++col >= VGA_WIDTH) {
        col = 0;
        if (++row >= VGA_HEIGHT)
            vga_scroll();
    }
    vga_set_cursor(col, row);
}

// ─── Imprimir string ──────────────────────────────────────────
void vga_print(const char *str) {
    while (*str)
        vga_put_char(*str++);
}

// ─── Imprimir en posición absoluta ───────────────────────────
void vga_print_at(const char *str, uint8_t x, uint8_t y) {
    uint8_t save_col   = col;
    uint8_t save_row   = row;
    uint8_t save_color = cur_color;
    col = x; row = y;
    vga_set_cursor(x, y);
    vga_print(str);
    col = save_col; row = save_row; cur_color = save_color;
    vga_set_cursor(save_col, save_row);
}

// ─── Imprimir número hexadecimal ─────────────────────────────
void vga_print_hex(uint32_t val) {
    static const char hex[] = "0123456789ABCDEF";
    char buf[10] = "0x";
    for (int i = 7; i >= 2; i--) {
        buf[i] = hex[val & 0xF];
        val >>= 4;
    }
    buf[8] = '\0';
    vga_print(buf);
}

// ─── Imprimir número decimal ──────────────────────────────────
void vga_print_dec(uint32_t val) {
    char buf[12];
    int i = 10;
    buf[11] = '\0';
    if (val == 0) { vga_put_char('0'); return; }
    while (val > 0 && i >= 0) {
        buf[i--] = '0' + (val % 10);
        val /= 10;
    }
    vga_print(&buf[i + 1]);
}
