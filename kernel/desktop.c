#include "desktop.h"
#include "wm.h"
#include "taskbar.h"
#include "theme.h"
#include "../framebuffer.h"
#include "../font.h"
#include "../keyboard.h"
#include "../mouse.h"
#include "../string.h"
#include "../shell.h"

// ─── Ventana de terminal principal ───────────────────────────
static int term_win_id = -1;

// ─── Buffer de línea para el shell ───────────────────────────
static char cmd_buf[256];
static int  cmd_len = 0;

// ─── Puntero a la ventana terminal ───────────────────────────
static window_t *get_term(void) {
    if (term_win_id >= 0 && term_win_id < wm.count)
        return &wm.windows[term_win_id];
    return 0;
}

// ─── Imprimir en la terminal gráfica ─────────────────────────
static void gui_print(const char *s, uint32_t color) {
    window_t *w = get_term();
    if (!w) return;
    win_term_print(w, s, color);
}

// ─── Dibujar el prompt ────────────────────────────────────────
static void gui_show_prompt(void) {
    gui_print("xp7", THEME_TERM_PROMPT);
    gui_print("@kernel C:\\> ", THEME_TERM_TEXT);
}

// ─── Ejecutar comando del shell (reusa shell.c pero redirige output) ──
// Esto es un "redirect hook" simple — Stage 3 tendrá pipes reales
extern void shell_exec(const char *cmd, char **argv, int argc);

// Parser de argumentos (igual al de shell.c)
static int parse_args_gui(char *input, char **argv) {
    int argc = 0;
    char *p = input;
    while (*p && argc < 15) {
        while (*p == ' ') p++;
        if (!*p) break;
        argv[argc++] = p;
        while (*p && *p != ' ') p++;
        if (*p) *p++ = '\0';
    }
    argv[argc] = 0;
    return argc;
}

// ─── Proceso de tecla en la terminal ────────────────────────
static void term_on_key(window_t *win, char key) {
    (void)win;

    if (key == '\n') {
        gui_print("\n", THEME_TERM_TEXT);
        cmd_buf[cmd_len] = '\0';

        if (cmd_len > 0) {
            // Ejecutar el comando — redirigimos output a la ventana
            // Por ahora manejamos comandos simples built-in aquí mismo
            if (k_strcmp(cmd_buf, "clear") == 0 ||
                k_strcmp(cmd_buf, "cls")   == 0) {
                win_term_clear(get_term());
            } else if (k_strcmp(cmd_buf, "help") == 0) {
                gui_print("Comandos: help, clear, ver, echo, about\n",
                          THEME_TERM_TEXT);
            } else if (k_strcmp(cmd_buf, "ver") == 0) {
                gui_print("XP7 OS Version 0.2 [Stage 2 - GUI]\n",
                          THEME_TERM_SUCCESS);
                gui_print("Arquitectura: x86 32-bit | Framebuffer: 800x600x32\n",
                          THEME_TERM_TEXT);
            } else if (k_strncmp(cmd_buf, "echo ", 5) == 0) {
                gui_print(cmd_buf + 5, THEME_TERM_TEXT);
                gui_print("\n", THEME_TERM_TEXT);
            } else if (k_strcmp(cmd_buf, "about") == 0) {
                gui_print("  XP7 OS — Sistema Operativo desde 0 en C\n",
                          THEME_TERM_PROMPT);
                gui_print("  XP Design + W7 Features | Stage 2/6\n",
                          THEME_TERM_TEXT);
            } else if (k_strcmp(cmd_buf, "reboot") == 0) {
                gui_print("Reiniciando...\n", THEME_TERM_ERROR);
                // Reboot via 8042
                uint8_t g = 0x02;
                while (g & 0x02)
                    __asm__ volatile("inb $0x64,%0":"=a"(g));
                __asm__ volatile("outb %0,$0x64"::"a"((uint8_t)0xFE));
            } else {
                gui_print("'", THEME_TERM_ERROR);
                gui_print(cmd_buf, THEME_TERM_ERROR);
                gui_print("' no encontrado. Escribe 'help'\n",
                          THEME_TERM_ERROR);
            }
        }

        cmd_len = 0;
        k_memset(cmd_buf, 0, sizeof(cmd_buf));
        gui_show_prompt();

    } else if (key == '\b') {
        if (cmd_len > 0) {
            cmd_len--;
            cmd_buf[cmd_len] = '\0';
            // Borrar en el buffer visual
            window_t *w = get_term();
            if (w && w->term_col > 0) {
                w->term_col--;
                w->term_buf[w->term_row][w->term_col] = ' ';
            }
        }
    } else if (key >= 32 && key < 127 && cmd_len < 255) {
        cmd_buf[cmd_len++] = key;
        cmd_buf[cmd_len]   = '\0';
        // Echo en la terminal
        char tmp[2] = {key, 0};
        window_t *w = get_term();
        if (w) win_term_print(w, tmp, THEME_TERM_TEXT);
    }

    wm.needs_redraw = 1;
}

// ─── Íconos del desktop ───────────────────────────────────────
typedef struct {
    int x, y;
    const char *label;
    uint32_t color;
} desktop_icon_t;

static const desktop_icon_t icons[] = {
    {20,  30,  "Terminal",   0x4080FF},
    {20,  100, "Mi PC",      0xFFCC40},
    {20,  170, "Papelera",   0xFF8040},
};
#define ICON_COUNT 3
#define ICON_W 48
#define ICON_H 48

// ─── Dibujar un ícono ─────────────────────────────────────────
static void draw_icon(const desktop_icon_t *ic) {
    // Sombra sutil
    fb_fill_rect_alpha(ic->x+3, ic->y+3, ICON_W, ICON_H, 0x000000, 80);
    // Fondo del ícono
    fb_fill_gradient_v(ic->x, ic->y, ICON_W, ICON_H,
                        ic->color,
                        fb_blend(ic->color, 0x000000, 180));
    // Borde
    fb_draw_rect(ic->x, ic->y, ICON_W, ICON_H,
                 fb_blend(ic->color, 0xFFFFFF, 100));
    // Etiqueta
    int lw = font_str_width(ic->label);
    int lx = ic->x + (ICON_W - lw) / 2;
    // Fondo semitransparente para la etiqueta
    fb_fill_rect_alpha(lx - 2, ic->y + ICON_H + 2, lw + 4, FONT_H + 2,
                        0x000000, 120);
    font_draw_str(lx, ic->y + ICON_H + 3, ic->label, 0xFFFFFF, 0, 1);
}

// ─── Dibujar fondo del desktop ────────────────────────────────
void desktop_draw_background(void) {
    // Gradiente vertical: teal oscuro XP
    fb_fill_gradient_v(0, 0, fb.width,
                        fb.height - THEME_TASKBAR_H,
                        THEME_DESKTOP_TOP, THEME_DESKTOP_BOT);

    // Íconos del escritorio
    for (int i = 0; i < ICON_COUNT; i++)
        draw_icon(&icons[i]);

    // Texto de versión en la esquina superior derecha
    const char *ver = "XP7 OS v0.2 | Stage 2";
    int vw = font_str_width(ver);
    font_draw_str(fb.width - vw - 8, 6, ver,
                  0xFFFFFFAA >> 8, 0, 1);  // blanco semi
}

// ─── Inicializar el desktop ───────────────────────────────────
void desktop_init(void) {
    wm_init();
    taskbar_init();

    // Dibujar fondo
    desktop_draw_background();
    taskbar_draw();

    // Crear ventana terminal principal
    term_win_id = wm_create_window(50, 40, 680, 460,
                                    "Terminal — XP7 Shell", 1);
    window_t *tw = get_term();
    if (tw) {
        tw->on_key = term_on_key;
        win_term_print(tw,
            "  XP7 OS Version 0.2 — Stage 2: GUI\n"
            "  Kernel: XP7Kernel v0.2 | Res: 800x600x32\n"
            "  ========================================\n",
            THEME_TERM_PROMPT);
        gui_show_prompt();
    }
    wm_redraw_all();
}

// ─── Loop principal del GUI ───────────────────────────────────
void desktop_run(void) {
    static int prev_mouse_x = -1;
    static int prev_mouse_y = -1;
    static int prev_buttons = 0;

    while (1) {
        // ── Leer teclado (non-blocking) ───────────────────────
        if (keyboard_available()) {
            char key = keyboard_getchar();
            // Cerrar start menu si hay tecla
            taskbar_start_open = 0;
            wm_on_key(key);
        }

        // ── Procesar mouse ────────────────────────────────────
        int mx = mouse.x, my = mouse.y;
        int mb = mouse.buttons;

        if (mx != prev_mouse_x || my != prev_mouse_y) {
            wm_on_mouse_move(mx, my);
            prev_mouse_x = mx;
            prev_mouse_y = my;
        }

        if (mouse.left_click) {
            mouse.left_click = 0;
            // ¿Clic en la taskbar?
            if (my >= (int)fb.height - THEME_TASKBAR_H)
                taskbar_on_click(mx, my);
            // ¿Clic en start menu abierto?
            else if (taskbar_start_open)
                taskbar_on_click(mx, my);
            // ¿Clic en un ícono del desktop?
            else {
                int clicked_icon = 0;
                for (int i = 0; i < ICON_COUNT; i++) {
                    if (mx >= icons[i].x && mx < icons[i].x + ICON_W &&
                        my >= icons[i].y && my < icons[i].y + ICON_H + FONT_H + 4) {
                        clicked_icon = 1;
                        if (i == 0 && term_win_id >= 0) {
                            wm_set_focus(term_win_id);
                        }
                        break;
                    }
                }
                if (!clicked_icon) wm_on_mouse_click(mx, my, 0);
            }
            prev_buttons = mb;
        }

        if (!(mb & 1) && (prev_buttons & 1)) {
            wm_on_mouse_release(mx, my, 0);
            prev_buttons = mb;
        }

        // ── Redibujar si hay cambios ──────────────────────────
        if (wm.needs_redraw) {
            desktop_draw_background();
            wm_redraw_all();
            taskbar_draw();
            // Redibujar cursor encima de todo
            mouse_draw_cursor();
        }

        // Pequeño delay (evita busy-loop a 100%)
        for (volatile int i = 0; i < 10000; i++);
    }
}
