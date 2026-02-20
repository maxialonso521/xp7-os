#include "mouse.h"
#include "framebuffer.h"
#include "pic.h"

mouse_state_t mouse = {400, 300, 0, 0, 0, 0, 0};

// ─── I/O helpers ─────────────────────────────────────────────
static inline void outb_m(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0,%1"::"a"(val),"Nd"(port));
}
static inline uint8_t inb_m(uint16_t port) {
    uint8_t r; __asm__ volatile("inb %1,%0":"=a"(r):"Nd"(port)); return r;
}
static inline void io_wait_m(void) { outb_m(0x80, 0x00); }

// ─── Esperar que el controlador 8042 esté listo ───────────────
static void kbd_wait_write(void) {
    while (inb_m(0x64) & 0x02);
}
static void kbd_wait_read(void) {
    while (!(inb_m(0x64) & 0x01));
}

// ─── Buffer de paquetes del mouse (3 bytes por paquete) ───────
static volatile uint8_t  mouse_cycle = 0;
static volatile uint8_t  mouse_packet[3];

// ─── Backup del área bajo el cursor ──────────────────────────
#define CURSOR_W 12
#define CURSOR_H 19
static uint32_t cursor_bg[CURSOR_W * CURSOR_H];
static int cursor_bg_x = -1, cursor_bg_y = -1;

// ─── Sprite del cursor (flecha) ──────────────────────────────
// 1 = blanco, 2 = negro, 0 = transparente
static const uint8_t cursor_sprite[CURSOR_H][CURSOR_W] = {
    {1,0,0,0,0,0,0,0,0,0,0,0},
    {1,1,0,0,0,0,0,0,0,0,0,0},
    {1,2,1,0,0,0,0,0,0,0,0,0},
    {1,2,2,1,0,0,0,0,0,0,0,0},
    {1,2,2,2,1,0,0,0,0,0,0,0},
    {1,2,2,2,2,1,0,0,0,0,0,0},
    {1,2,2,2,2,2,1,0,0,0,0,0},
    {1,2,2,2,2,2,2,1,0,0,0,0},
    {1,2,2,2,2,2,2,2,1,0,0,0},
    {1,2,2,2,2,2,2,2,2,1,0,0},
    {1,2,2,2,2,2,2,2,2,2,1,0},
    {1,2,2,2,2,2,2,1,1,1,1,1},
    {1,2,2,2,1,2,2,1,0,0,0,0},
    {1,2,2,1,0,1,2,2,1,0,0,0},
    {1,2,1,0,0,1,2,2,1,0,0,0},
    {1,1,0,0,0,0,1,2,2,1,0,0},
    {0,0,0,0,0,0,1,2,2,1,0,0},
    {0,0,0,0,0,0,0,1,2,1,0,0},
    {0,0,0,0,0,0,0,1,1,0,0,0},
};

// ─── Borrar cursor (restaurar fondo) ─────────────────────────
void mouse_erase_cursor(void) {
    if (cursor_bg_x < 0) return;
    for (int row = 0; row < CURSOR_H; row++)
        for (int col = 0; col < CURSOR_W; col++) {
            int sx = cursor_bg_x + col;
            int sy = cursor_bg_y + row;
            if (sx >= 0 && sx < (int)fb.width &&
                sy >= 0 && sy < (int)fb.height)
                fb_put_pixel(sx, sy, cursor_bg[row*CURSOR_W+col]);
        }
}

// ─── Dibujar cursor ───────────────────────────────────────────
void mouse_draw_cursor(void) {
    cursor_bg_x = mouse.x;
    cursor_bg_y = mouse.y;
    // Guardar fondo
    for (int row = 0; row < CURSOR_H; row++)
        for (int col = 0; col < CURSOR_W; col++) {
            int sx = mouse.x + col, sy = mouse.y + row;
            if (sx >= 0 && sx < (int)fb.width &&
                sy >= 0 && sy < (int)fb.height) {
                uint32_t *p = (uint32_t*)((uint8_t*)fb.addr+sy*fb.pitch)+sx;
                cursor_bg[row*CURSOR_W+col] = *p;
            }
        }
    // Dibujar sprite
    for (int row = 0; row < CURSOR_H; row++)
        for (int col = 0; col < CURSOR_W; col++) {
            int sx = mouse.x + col, sy = mouse.y + row;
            if (sx >= 0 && sx < (int)fb.width &&
                sy >= 0 && sy < (int)fb.height) {
                uint8_t px = cursor_sprite[row][col];
                if (px == 1) fb_put_pixel(sx, sy, 0xFFFFFF); // blanco
                if (px == 2) fb_put_pixel(sx, sy, 0x000000); // negro
            }
        }
}

// ─── ISR — llamado cada vez que el mouse envía datos ─────────
void mouse_handler(void) {
    uint8_t status = inb_m(0x64);
    if (!(status & 0x01)) { pic_send_eoi(12); return; }

    uint8_t data = inb_m(0x60);

    switch (mouse_cycle) {
    case 0:
        // Validar byte de estado: bit 3 siempre 1
        if (!(data & 0x08)) break;
        mouse_packet[0] = data;
        mouse_cycle = 1;
        break;
    case 1:
        mouse_packet[1] = data;
        mouse_cycle = 2;
        break;
    case 2:
        mouse_packet[2] = data;
        mouse_cycle = 0;

        // ── Procesar paquete ──────────────────────────────────
        uint8_t flags = mouse_packet[0];
        int dx = (int)mouse_packet[1];
        int dy = (int)mouse_packet[2];

        // Aplicar signo
        if (flags & MOUSE_SIGN_X) dx -= 256;
        if (flags & MOUSE_SIGN_Y) dy -= 256;

        // Ignorar overflow
        if (flags & MOUSE_OVERFLOW_X || flags & MOUSE_OVERFLOW_Y) break;

        // Borrar cursor viejo
        mouse_erase_cursor();

        // Actualizar posición (Y invertida: mouse+Y = screen-Y)
        mouse.x += dx;
        mouse.y -= dy;

        // Clamp al borde de pantalla
        if (mouse.x < 0) mouse.x = 0;
        if (mouse.y < 0) mouse.y = 0;
        if (mouse.x >= (int)fb.width  - 1) mouse.x = fb.width  - 1;
        if (mouse.y >= (int)fb.height - 1) mouse.y = fb.height - 1;

        // Botones
        uint8_t prev_buttons = mouse.buttons;
        mouse.buttons = flags & 0x07;
        mouse.left_click  = (mouse.buttons & MOUSE_BTN_LEFT)  &&
                            !(prev_buttons & MOUSE_BTN_LEFT);
        mouse.right_click = (mouse.buttons & MOUSE_BTN_RIGHT) &&
                            !(prev_buttons & MOUSE_BTN_RIGHT);

        // Dibujar cursor nuevo
        mouse_draw_cursor();
        break;
    }
    pic_send_eoi(12);
}

// ─── Inicializar el mouse PS/2 ────────────────────────────────
void mouse_init(void) {
    // 1. Habilitar el dispositivo auxiliar (mouse) en el 8042
    kbd_wait_write();
    outb_m(0x64, 0xA8);        // Enable auxiliary device
    io_wait_m();

    // 2. Habilitar interrupciones del mouse (bit 1 del CMB)
    kbd_wait_write();
    outb_m(0x64, 0x20);        // Read Controller Command Byte
    io_wait_m();
    kbd_wait_read();
    uint8_t cmb = inb_m(0x60);
    cmb |= 0x02;               // Bit 1: Enable IRQ12
    cmb &= ~0x20;              // Bit 5: Disable mouse clock disable
    kbd_wait_write();
    outb_m(0x64, 0x60);        // Write Controller Command Byte
    io_wait_m();
    kbd_wait_write();
    outb_m(0x60, cmb);
    io_wait_m();

    // 3. Enviar comando al mouse via 0xD4
    // Helper: escribir al mouse
    #define MOUSE_WRITE(cmd) do { \
        kbd_wait_write(); outb_m(0x64,0xD4); \
        io_wait_m(); kbd_wait_write(); outb_m(0x60,cmd); \
        io_wait_m(); kbd_wait_read(); inb_m(0x60); \
    } while(0)

    MOUSE_WRITE(0xFF);   // Reset
    MOUSE_WRITE(0xF6);   // Set defaults
    MOUSE_WRITE(0xF4);   // Enable data reporting

    // 4. Habilitar IRQ12 en el PIC
    pic_clear_mask(12);

    mouse.x = fb.width  / 2;
    mouse.y = fb.height / 2;
    mouse_cycle = 0;

    // Dibujar cursor inicial
    mouse_draw_cursor();
}
