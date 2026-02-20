#include "taskbar.h"
#include "theme.h"
#include "wm.h"
#include "../framebuffer.h"
#include "../font.h"
#include "../string.h"

int taskbar_start_open = 0;

// ─── Y de la taskbar ──────────────────────────────────────────
static inline int tb_y(void) { return fb.height - THEME_TASKBAR_H; }

// ─── Dibujar el botón Start ───────────────────────────────────
static void draw_start_button(void) {
    int x = 2, y = tb_y() + 2;
    int w = THEME_START_W, h = THEME_TASKBAR_H - 4;

    // Gradiente verde XP
    fb_fill_gradient_h(x, y, w, h, THEME_START_BG_L, THEME_START_BG_R);

    // Borde
    fb_draw_rect(x, y, w, h, 0x1A5010);

    // Highlight superior
    fb_draw_hline(x+1, y+1, w-2, 0xAAE860);

    // Texto "Start" centrado
    int tx = x + (w - font_str_width("  Start")) / 2;
    int ty = y + (h - FONT_H) / 2;
    font_draw_str(tx, ty, "  Start", THEME_START_TXT, 0, 1);

    // ─── Ícono Windows (círculos de colores XP) ───────────────
    int ix = x + 6, iy = y + (h - 10) / 2;
    // Cuadrante rojo
    fb_fill_rect(ix,   iy,   5, 4, 0xFF4040);
    // Cuadrante verde
    fb_fill_rect(ix+6, iy,   5, 4, 0x40CC40);
    // Cuadrante azul
    fb_fill_rect(ix,   iy+5, 5, 4, 0x4040FF);
    // Cuadrante amarillo
    fb_fill_rect(ix+6, iy+5, 5, 4, 0xFFCC00);
}

// ─── Dibujar botones de ventanas en la taskbar ───────────────
static void draw_window_buttons(void) {
    int bx = THEME_START_W + 8;
    int by = tb_y() + 3;
    int bh = THEME_TASKBAR_H - 6;
    int bw = 110;

    for (int i = 0; i < wm.count && bx + bw < (int)fb.width - 80; i++) {
        window_t *win = &wm.windows[i];
        if (!(win->flags & WIN_FLAG_VISIBLE)) continue;

        int active = (i == wm.focused);
        uint32_t bg1 = active ? 0x4080D0 : 0x2858A8;
        uint32_t bg2 = active ? 0x2458A0 : 0x1840808;

        fb_fill_gradient_v(bx, by, bw, bh, bg1, bg2);
        fb_draw_rect(bx, by, bw, bh,
                     active ? 0x90C0FF : 0x2848888);

        // Texto del título (truncado)
        char tmp[20];
        k_strncpy(tmp, win->title, 16);
        tmp[16] = '\0';
        if (win->flags & WIN_FLAG_MINIMIZED) {
            k_strcat(tmp, " _");
        }
        int ty2 = by + (bh - FONT_H) / 2;
        font_draw_str(bx + 4, ty2, tmp,
                      active ? 0xFFFFFF : 0xC0D0FF, 0, 1);

        bx += bw + 2;
    }
}

// ─── Dibujar reloj (área system tray) ────────────────────────
static void draw_clock(void) {
    // Por ahora mostramos texto estático
    // Stage 4+ tendrá un RTC driver
    const char *time_str = "XP7 OS";
    int tw = font_str_width(time_str);
    int tx = fb.width - tw - 10;
    int ty2 = tb_y() + (THEME_TASKBAR_H - FONT_H) / 2;

    // Fondo del tray
    fb_fill_gradient_v(fb.width - tw - 18, tb_y() + 2,
                        tw + 16, THEME_TASKBAR_H - 4,
                        0x1848A8, 0x0A2878);
    fb_draw_rect(fb.width - tw - 19, tb_y() + 1,
                  tw + 18, THEME_TASKBAR_H - 2, 0x2060CC);

    font_draw_str(tx, ty2, time_str, 0xFFFFFF, 0, 1);
}

// ─── Dibujar Start Menu (popup) ──────────────────────────────
static void draw_start_menu(void) {
    int mx = 2;
    int mw = 200;
    int mh = 160;
    int my = tb_y() - mh;

    // Barra lateral azul (estilo XP)
    fb_fill_gradient_v(mx, my, 28, mh, 0x1A4E8C, 0x0A2A5C);

    // Área principal
    fb_fill_rect(mx+28, my, mw-28, mh, 0xF0F0F0);

    // Header con nombre usuario
    fb_fill_gradient_h(mx, my, mw, 28, 0x1A4E8C, 0x2878CC);
    font_draw_str(mx+32, my+8, "Usuario XP7", 0xFFFFFF, 0, 1);

    // Bordes
    fb_draw_rect(mx, my, mw, mh, 0x0A246A);

    // Opciones del menú
    const char *items[] = {
        "Terminal",
        "Explorador",
        "---",
        "Apagar..."
    };
    int iy = my + 34;
    for (int i = 0; i < 4; i++) {
        if (items[i][0] == '-') {
            fb_draw_hline(mx+32, iy+4, mw-36, 0xC0C0C0);
            iy += 12;
        } else {
            // Hover effect (por ahora sin hover real)
            font_draw_str(mx+36, iy, items[i], 0x000000, 0, 1);
            iy += FONT_H + 6;
        }
    }
}

// ─── Init ─────────────────────────────────────────────────────
void taskbar_init(void) {
    taskbar_start_open = 0;
}

// ─── Dibujar toda la taskbar ──────────────────────────────────
void taskbar_draw(void) {
    int y = tb_y();
    int w = fb.width;
    int h = THEME_TASKBAR_H;

    // Gradiente de la barra
    fb_fill_gradient_v(0, y, w, h,
                        THEME_TASKBAR_BG_TOP, THEME_TASKBAR_BG_BOT);

    // Línea superior brillante (W7-style glass edge)
    fb_draw_hline(0, y, w, 0x6090D8);
    fb_draw_hline(0, y+1, w, 0x4070C0);

    draw_start_button();
    draw_window_buttons();
    draw_clock();

    // Start menu si está abierto
    if (taskbar_start_open)
        draw_start_menu();
}

// ─── Click en taskbar ─────────────────────────────────────────
void taskbar_on_click(int x, int y) {
    int tby = tb_y();
    if (y < tby) {
        // Click en el start menu
        if (taskbar_start_open) {
            // Área del menú: mx=2, mw=200
            if (x > 2 && x < 202) {
                // Calcular qué item
                int mh = 160;
                int my = tby - mh;
                int iy = my + 34;
                int item_y[4];
                for (int i = 0; i < 4; i++) {
                    item_y[i] = iy;
                    iy += FONT_H + 6;
                }
                if (y >= item_y[0] && y < item_y[0]+FONT_H) {
                    // Terminal → ya existe, traerla al frente
                    if (wm.count > 0) wm_set_focus(0);
                    taskbar_start_open = 0;
                } else if (y >= item_y[3] && y < item_y[3]+FONT_H) {
                    // Apagar
                    __asm__ volatile("cli; hlt");
                }
            }
            taskbar_start_open = 0;
        }
        return;
    }

    // Click en la barra de tareas
    // ¿Start button?
    if (x >= 2 && x <= 2 + THEME_START_W &&
        y >= tby + 2 && y <= tby + THEME_TASKBAR_H - 2) {
        taskbar_start_open = !taskbar_start_open;
        return;
    }

    // ¿Botón de ventana?
    int bx = THEME_START_W + 8;
    int bw = 110;
    for (int i = 0; i < wm.count; i++) {
        if (x >= bx && x < bx + bw) {
            if (wm.focused == i)
                wm_minimize_window(i);
            else
                wm_set_focus(i);
            return;
        }
        bx += bw + 2;
    }
}
