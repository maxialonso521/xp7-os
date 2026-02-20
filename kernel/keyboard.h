#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

// ─── Estado del teclado ───────────────────────────────────────
#define KEY_BUFFER_SIZE  256

// ─── Flags de modificadores ──────────────────────────────────
#define KEY_MOD_SHIFT    (1 << 0)
#define KEY_MOD_CTRL     (1 << 1)
#define KEY_MOD_ALT      (1 << 2)
#define KEY_MOD_CAPS     (1 << 3)

// ─── Teclas especiales (scancodes Set 1) ──────────────────────
#define SC_ENTER         0x1C
#define SC_BACKSPACE     0x0E
#define SC_TAB           0x0F
#define SC_ESCAPE        0x01
#define SC_LEFT_SHIFT    0x2A
#define SC_RIGHT_SHIFT   0x36
#define SC_LEFT_CTRL     0x1D
#define SC_LEFT_ALT      0x38
#define SC_CAPS_LOCK     0x3A
#define SC_BREAK_FLAG    0x80   // bit 7 = key release

// ─── API ──────────────────────────────────────────────────────
void  keyboard_init(void);
void  keyboard_handler(void);   // llamado desde ISR en boot.asm
char  keyboard_getchar(void);   // bloqueante — espera tecla
int   keyboard_available(void); // non-bloqueante — hay datos?
void  keyboard_read_line(char *buf, int max_len);

#endif
