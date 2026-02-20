#include "wm.h"
#include "../framebuffer.h"
#include "../font.h"
#include "../string.h"
#include "theme.h"

wm_t wm = {0};

// ─── Coordenadas de los botones de ventana ────────────────────
static inline int btn_close_x(window_t *w) {
    return w->x + w->w - THEME_BORDER_W - THEME_WINCAP_PAD - THEME_WINCAP_BTN_W;
}
static inline int btn_close_y(window_t *w) {
    return w->y + THEME_BORDER_W + (THEME_TITLEBAR_H - THEME_WINCAP_BTN_H) / 2;
}
static inline int btn_min_x(window_t *w) {
    return btn_close_x(w) - THEME_WINCAP_PAD - THEME_WINCAP_BTN_W;
}

// ─── Hit test: ¿está (px,py) dentro del rect? ────────────────
static int in_rect(int px, int py, int rx, int ry, int rw, int rh) {
    return px >= rx && px < rx+rw && py >= ry && py < ry+rh;
}

// ─── Dibujar borde 3D ─────────────────────────────────────────
static void draw_3d_border(int x, int y, int w, int h, int active) {
    uint32_t c = active ? THEME_WIN_BORDER_ACT : THEME_WIN_BORDER_INACT;
    for (int i = 0; i < THEME_BORDER_W; i++) {
        fb_draw_hline(x+i, y+i, w-2*i, c);
        fb_draw_hline(x+i, y+h-1-i, w-2*i, c);
        fb_draw_vline(x+i, y+i, h-2*i, c);
        fb_draw_vline(x+w-1-i, y+i, h-2*i, c);
    }
}

// ─── Dibujar botón (close/min) ────────────────────────────────
static void draw_win_btn(int x, int y, int type, int active) {
    // type: 0=close(X), 1=min(-)
    int w = THEME_WINCAP_BTN_W, h = THEME_WINCAP_BTN_H;
    uint32_t bg = (type == 0) ? THEME_BTN_CLOSE_BG : THEME_BTN_MIN_BG;
    (void)active;

    // Fondo del botón con gradiente sutil
    fb_fill_gradient_v(x, y, w, h, bg,
        fb_blend(bg, 0x000000, 180));

    // Borde
    fb_draw_rect(x, y, w, h, fb_blend(bg, 0x000000, 150));

    // Icono
    uint32_t ic = THEME_BTN_ICON_CLR;
    int cx = x + w/2, cy = y + h/2;
    if (type == 0) {
        // X de cierre
        fb_put_pixel(cx-3,cy-3,ic); fb_put_pixel(cx-2,cy-2,ic);
        fb_put_pixel(cx-1,cy-1,ic); fb_put_pixel(cx,  cy,  ic);
        fb_put_pixel(cx+1,cy+1,ic); fb_put_pixel(cx+2,cy+2,ic);
        fb_put_pixel(cx+3,cy+3,ic);
        fb_put_pixel(cx+3,cy-3,ic); fb_put_pixel(cx+2,cy-2,ic);
        fb_put_pixel(cx+1,cy-1,ic); fb_put_pixel(cx-1,cy+1,ic);
        fb_put_pixel(cx-2,cy+2,ic); fb_put_pixel(cx-3,cy+3,ic);
        // Engrosar
        fb_put_pixel(cx-3,cy-2,ic); fb_put_pixel(cx-2,cy-3,ic);
        fb_put_pixel(cx+3,cy-2,ic); fb_put_pixel(cx+2,cy-3,ic);
        fb_put_pixel(cx-3,cy+2,ic); fb_put_pixel(cx-2,cy+3,ic);
        fb_put_pixel(cx+3,cy+2,ic); fb_put_pixel(cx+2,cy+3,ic);
    } else {
        // Barra de minimizar
        fb_draw_hline(cx-3, cy+2, 7, ic);
        fb_draw_hline(cx-3, cy+3, 7, ic);
    }
}

// ─── Dibujar decoraciones de una ventana ─────────────────────
void wm_draw_decorations(window_t *win) {
    int active = (win->flags & WIN_FLAG_ACTIVE) != 0;

    // ── Borde exterior ────────────────────────────────────────
    draw_3d_border(win->x, win->y, win->w, win->h, active);

    // ── Barra de título ───────────────────────────────────────
    int tx = win->x + THEME_BORDER_W;
    int ty = win->y + THEME_BORDER_W;
    int tw = win->w - 2*THEME_BORDER_W;
    int th = THEME_TITLEBAR_H;

    if (active)
        fb_fill_gradient_h(tx, ty, tw, th,
                           THEME_TITLE_ACT_L, THEME_TITLE_ACT_R);
    else
        fb_fill_gradient_h(tx, ty, tw, th,
                           THEME_TITLE_INACT_L, THEME_TITLE_INACT_R);

    // Título (texto centrado verticalmente en titlebar)
    int txt_y = ty + (th - FONT_H) / 2;
    uint32_t txt_color = active ? THEME_TITLE_ACT_TXT : THEME_TITLE_INACT_TXT;
    font_draw_str(tx + 6, txt_y, win->title, txt_color, 0, 1);

    // ── Botones close / minimize ─────────────────────────────
    if (win->flags & WIN_FLAG_CLOSEABLE) {
        draw_win_btn(btn_close_x(win), btn_close_y(win), 0, active);
        draw_win_btn(btn_min_x(win),   btn_close_y(win), 1, active);
    }

    // ── Línea separadora bajo titlebar ────────────────────────
    fb_draw_hline(tx, ty + th, tw, active ? THEME_WIN_BORDER_ACT
                                          : THEME_WIN_BORDER_INACT);
}

// ─── Dibujar contenido de la ventana ─────────────────────────
static void draw_window_content(window_t *win) {
    if (win->flags & WIN_FLAG_MINIMIZED) return;

    int cx = win->cx, cy = win->cy;
    int cw = win->cw, ch = win->ch;

    if (win->is_terminal) {
        // Fondo de terminal
        fb_fill_rect(cx, cy, cw, ch, THEME_TERM_BG);

        // Dibujar texto del buffer
        int y = cy + 2;
        int visible_lines = ch / FONT_H;
        int start = 0;
        if (win->term_row >= visible_lines)
            start = win->term_row - visible_lines + 1;

        for (int line = start; line <= win->term_row && y < cy+ch; line++) {
            if (line < WIN_TERM_LINES) {
                font_draw_str(cx + 4, y, win->term_buf[line],
                              THEME_TERM_TEXT, THEME_TERM_BG, 0);
            }
            y += FONT_H;
        }

        // Cursor parpadeante (siempre dibujado)
        int cur_y = cy + 2 + (win->term_row - start) * FONT_H;
        int cur_x = cx + 4 + win->term_col * FONT_W;
        if (cur_y >= cy && cur_y + FONT_H <= cy + ch)
            fb_fill_rect(cur_x, cur_y, FONT_W, FONT_H, THEME_TERM_PROMPT);

    } else {
        // Ventana genérica: fondo blanco
        fb_fill_rect(cx, cy, cw, ch, THEME_WIN_CONTENT_BG);
        if (win->on_paint) win->on_paint(win);
    }
}

// ─── Dibujar una ventana completa ────────────────────────────
void wm_redraw_window(int id) {
    if (id < 0 || id >= wm.count) return;
    window_t *win = &wm.windows[id];
    if (!(win->flags & WIN_FLAG_VISIBLE)) return;
    if (win->flags & WIN_FLAG_MINIMIZED) return;

    wm_draw_decorations(win);
    draw_window_content(win);
}

// ─── Redibujar todo ───────────────────────────────────────────
void wm_redraw_all(void) {
    for (int i = 0; i < wm.count; i++)
        wm_redraw_window(i);
    wm.needs_redraw = 0;
}

// ─── Inicializar WM ───────────────────────────────────────────
void wm_init(void) {
    k_memset(&wm, 0, sizeof(wm));
    wm.focused = -1;
}

// ─── Actualizar coordenadas de contenido ─────────────────────
static void win_recalc_content(window_t *win) {
    win->cx = win->x + THEME_BORDER_W;
    win->cy = win->y + THEME_BORDER_W + THEME_TITLEBAR_H + 1;
    win->cw = win->w - 2*THEME_BORDER_W;
    win->ch = win->h - 2*THEME_BORDER_W - THEME_TITLEBAR_H - 1;
}

// ─── Crear ventana ────────────────────────────────────────────
int wm_create_window(int x, int y, int w, int h,
                      const char *title, int terminal) {
    if (wm.count >= WM_MAX_WINDOWS) return -1;
    int id = wm.count++;
    window_t *win = &wm.windows[id];
    k_memset(win, 0, sizeof(window_t));

    win->id = id;
    win->x = x; win->y = y;
    win->w = w; win->h = h;
    win->flags = WIN_FLAG_VISIBLE | WIN_FLAG_CLOSEABLE |
                 WIN_FLAG_MOVEABLE | WIN_FLAG_ACTIVE;
    k_strncpy(win->title, title, WM_TITLE_MAX-1);

    win->is_terminal = terminal;
    win->term_bg     = THEME_TERM_BG;
    win->term_fg     = THEME_TERM_TEXT;
    win->term_row    = 0;
    win->term_col    = 0;
    k_memset(win->term_buf, 0, sizeof(win->term_buf));

    win_recalc_content(win);

    // Quitar foco de ventanas anteriores
    for (int i = 0; i < id; i++)
        wm.windows[i].flags &= ~WIN_FLAG_ACTIVE;
    wm.focused = id;
    wm.needs_redraw = 1;
    return id;
}

// ─── Destruir ventana ─────────────────────────────────────────
void wm_destroy_window(int id) {
    if (id < 0 || id >= wm.count) return;
    // Shift hacia abajo
    for (int i = id; i < wm.count-1; i++)
        wm.windows[i] = wm.windows[i+1];
    wm.count--;
    wm.needs_redraw = 1;
}

// ─── Cambiar foco ─────────────────────────────────────────────
void wm_set_focus(int id) {
    for (int i = 0; i < wm.count; i++)
        wm.windows[i].flags &= ~WIN_FLAG_ACTIVE;
    if (id >= 0 && id < wm.count) {
        wm.windows[id].flags |= WIN_FLAG_ACTIVE;
        wm.focused = id;
    }
    wm.needs_redraw = 1;
}

// ─── Mover ventana ────────────────────────────────────────────
void wm_move_window(int id, int x, int y) {
    if (id < 0 || id >= wm.count) return;
    window_t *win = &wm.windows[id];
    win->x = x; win->y = y;
    win_recalc_content(win);
    wm.needs_redraw = 1;
}

// ─── Minimizar ────────────────────────────────────────────────
void wm_minimize_window(int id) {
    if (id < 0 || id >= wm.count) return;
    wm.windows[id].flags ^= WIN_FLAG_MINIMIZED;
    wm.needs_redraw = 1;
}

// ─── Mouse Move ───────────────────────────────────────────────
void wm_on_mouse_move(int x, int y) {
    // Drag de ventana activa
    if (wm.focused >= 0 && wm.focused < wm.count) {
        window_t *win = &wm.windows[wm.focused];
        if (win->flags & WIN_FLAG_DRAGGING) {
            int nx = x - win->drag_off_x;
            int ny = y - win->drag_off_y;
            // Clamp: no salir de pantalla
            if (nx < 0) nx = 0;
            if (ny < 0) ny = 0;
            if (nx + win->w > (int)fb.width)  nx = fb.width  - win->w;
            if (ny + win->h > (int)fb.height - THEME_TASKBAR_H)
                ny = fb.height - THEME_TASKBAR_H - win->h;
            wm_move_window(wm.focused, nx, ny);
        }
    }
}

// ─── Mouse Click ─────────────────────────────────────────────
void wm_on_mouse_click(int x, int y, int btn) {
    (void)btn;
    // Buscar ventana clickeada (de la más nueva a la más vieja)
    for (int i = wm.count-1; i >= 0; i--) {
        window_t *win = &wm.windows[i];
        if (!(win->flags & WIN_FLAG_VISIBLE)) continue;
        if (win->flags & WIN_FLAG_MINIMIZED) continue;
        if (!in_rect(x, y, win->x, win->y, win->w, win->h)) continue;

        // Traer al frente (cambiar foco)
        if (wm.focused != i) {
            wm_set_focus(i);
            return;
        }

        // ¿Clic en botón close?
        if ((win->flags & WIN_FLAG_CLOSEABLE) &&
            in_rect(x, y, btn_close_x(win), btn_close_y(win),
                    THEME_WINCAP_BTN_W, THEME_WINCAP_BTN_H)) {
            wm_destroy_window(i);
            return;
        }

        // ¿Clic en botón minimizar?
        if ((win->flags & WIN_FLAG_CLOSEABLE) &&
            in_rect(x, y, btn_min_x(win), btn_close_y(win),
                    THEME_WINCAP_BTN_W, THEME_WINCAP_BTN_H)) {
            wm_minimize_window(i);
            return;
        }

        // ¿Clic en titlebar → drag?
        int ty = win->y + THEME_BORDER_W;
        int th = THEME_TITLEBAR_H;
        if ((win->flags & WIN_FLAG_MOVEABLE) &&
            in_rect(x, y, win->x, ty, win->w, th)) {
            win->flags |= WIN_FLAG_DRAGGING;
            win->drag_off_x = x - win->x;
            win->drag_off_y = y - win->y;
        }
        return;
    }
}

// ─── Mouse Release ───────────────────────────────────────────
void wm_on_mouse_release(int x, int y, int btn) {
    (void)x; (void)y; (void)btn;
    if (wm.focused >= 0 && wm.focused < wm.count)
        wm.windows[wm.focused].flags &= ~WIN_FLAG_DRAGGING;
}

// ─── Key Event → ventana con foco ────────────────────────────
void wm_on_key(char key) {
    if (wm.focused < 0 || wm.focused >= wm.count) return;
    window_t *win = &wm.windows[wm.focused];
    if (win->on_key) win->on_key(win, key);
}

// ═══════════════════════════════════════════════════════════════
//   API Terminal dentro de ventana
// ═══════════════════════════════════════════════════════════════

void win_term_newline(window_t *win) {
    win->term_col = 0;
    if (win->term_row < WIN_TERM_LINES - 1) {
        win->term_row++;
    } else {
        // Scroll: mover todo hacia arriba
        for (int i = 0; i < WIN_TERM_LINES - 1; i++)
            k_memcpy(win->term_buf[i], win->term_buf[i+1], WIN_TERM_COLS);
        k_memset(win->term_buf[WIN_TERM_LINES-1], 0, WIN_TERM_COLS);
    }
}

void win_term_print(window_t *win, const char *s, uint32_t color) {
    (void)color;  // TODO: soporte colores por char (Stage 3)
    while (*s) {
        char c = *s++;
        if (c == '\n') {
            win_term_newline(win);
        } else if (c == '\b') {
            if (win->term_col > 0) {
                win->term_col--;
                win->term_buf[win->term_row][win->term_col] = ' ';
            }
        } else if (c == '\r') {
            win->term_col = 0;
        } else {
            if (win->term_col < WIN_TERM_COLS - 1) {
                win->term_buf[win->term_row][win->term_col++] = c;
            } else {
                win_term_newline(win);
                win->term_buf[win->term_row][win->term_col++] = c;
            }
        }
    }
    wm.needs_redraw = 1;
}

void win_term_clear(window_t *win) {
    k_memset(win->term_buf, 0, sizeof(win->term_buf));
    win->term_row = 0;
    win->term_col = 0;
    wm.needs_redraw = 1;
}
