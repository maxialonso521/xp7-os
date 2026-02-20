#ifndef MOUSE_H
#define MOUSE_H

#include <stdint.h>

// ─── Estado del mouse ─────────────────────────────────────────
typedef struct {
    int x, y;         // posición actual (clampada al screen)
    int dx, dy;       // delta del último movimiento
    uint8_t buttons;  // bit 0=izquierdo, bit 1=derecho, bit 2=medio
    int left_click;   // 1 si hubo click nuevo este frame
    int right_click;
} mouse_state_t;

extern mouse_state_t mouse;

// ─── Bits del byte de estado PS/2 ────────────────────────────
#define MOUSE_BTN_LEFT   (1 << 0)
#define MOUSE_BTN_RIGHT  (1 << 1)
#define MOUSE_BTN_MID    (1 << 2)
#define MOUSE_SIGN_X     (1 << 4)
#define MOUSE_SIGN_Y     (1 << 5)
#define MOUSE_OVERFLOW_X (1 << 6)
#define MOUSE_OVERFLOW_Y (1 << 7)

// ─── API ──────────────────────────────────────────────────────
void mouse_init(void);
void mouse_handler(void);   // llamado desde ISR (isr_mouse en boot.asm)
void mouse_draw_cursor(void);
void mouse_erase_cursor(void);

#endif
