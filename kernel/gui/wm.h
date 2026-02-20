#ifndef WM_H
#define WM_H

#include <stdint.h>
#include "../framebuffer.h"
#include "../font.h"
#include "theme.h"

// ─── Límites ──────────────────────────────────────────────────
#define WM_MAX_WINDOWS   8
#define WM_TITLE_MAX     64

// ─── Flags de ventana ─────────────────────────────────────────
#define WIN_FLAG_VISIBLE    (1<<0)
#define WIN_FLAG_ACTIVE     (1<<1)
#define WIN_FLAG_DRAGGING   (1<<2)
#define WIN_FLAG_MINIMIZED  (1<<3)
#define WIN_FLAG_CLOSEABLE  (1<<4)
#define WIN_FLAG_MOVEABLE   (1<<5)

// ─── Buffer de texto para ventanas con terminal ───────────────
#define WIN_TERM_LINES  30
#define WIN_TERM_COLS   72

// ─── Estructura de ventana ────────────────────────────────────
typedef struct window_s {
    int id;
    int x, y;           // posición de la ventana (borde exterior)
    int w, h;           // tamaño incluyendo decoraciones
    int flags;
    char title[WM_TITLE_MAX];

    // Posición del contenido (interior, sin title bar ni bordes)
    int cx, cy, cw, ch;

    // Drag state
    int drag_off_x, drag_off_y;

    // Terminal buffer (si es ventana de terminal)
    int is_terminal;
    char term_buf[WIN_TERM_LINES][WIN_TERM_COLS];
    int  term_row, term_col;        // fila/columna actual
    int  term_scroll;               // líneas de scroll acumulado
    uint32_t term_bg;
    uint32_t term_fg;

    // Callback de dibujo del contenido (para apps futuras)
    void (*on_paint)(struct window_s *win);
    void (*on_key)(struct window_s *win, char key);
} window_t;

// ─── Window Manager ───────────────────────────────────────────
typedef struct {
    window_t  windows[WM_MAX_WINDOWS];
    int       count;
    int       focused;          // índice de ventana con foco
    int       needs_redraw;
} wm_t;

extern wm_t wm;

// ─── API del WM ───────────────────────────────────────────────
void wm_init(void);
int  wm_create_window(int x, int y, int w, int h, const char *title, int terminal);
void wm_destroy_window(int id);
void wm_set_focus(int id);
void wm_move_window(int id, int x, int y);
void wm_minimize_window(int id);
void wm_redraw_all(void);
void wm_redraw_window(int id);

// Input routing
void wm_on_mouse_move(int x, int y);
void wm_on_mouse_click(int x, int y, int btn);
void wm_on_mouse_release(int x, int y, int btn);
void wm_on_key(char key);

// Terminal helpers
void win_term_print(window_t *win, const char *s, uint32_t color);
void win_term_clear(window_t *win);
void win_term_newline(window_t *win);

#endif
