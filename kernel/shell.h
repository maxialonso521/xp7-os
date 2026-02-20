#ifndef SHELL_H
#define SHELL_H

#include <stdint.h>

// ─── Config del shell ─────────────────────────────────────────
#define SHELL_MAX_INPUT   256
#define SHELL_MAX_ARGS    16
#define SHELL_MAX_HISTORY 20

// ─── Colores del tema XP7 OS ──────────────────────────────────
// Inspirado en Windows XP Start Button + Windows 7 taskbar
#define XP7_COLOR_PROMPT_FG    VGA_LIGHT_CYAN
#define XP7_COLOR_PROMPT_BG    VGA_BLACK
#define XP7_COLOR_TEXT_FG      VGA_LIGHT_GREY
#define XP7_COLOR_ERROR_FG     VGA_LIGHT_RED
#define XP7_COLOR_SUCCESS_FG   VGA_LIGHT_GREEN
#define XP7_COLOR_HEADER_FG    VGA_WHITE

// ─── API ──────────────────────────────────────────────────────
void shell_run(void);
void shell_exec(const char *cmd, char **argv, int argc);

// ─── Comandos built-in ────────────────────────────────────────
void cmd_help(int argc, char **argv);
void cmd_clear(int argc, char **argv);
void cmd_ver(int argc, char **argv);
void cmd_echo(int argc, char **argv);
void cmd_halt(int argc, char **argv);
void cmd_color(int argc, char **argv);
void cmd_meminfo(int argc, char **argv);
void cmd_reboot(int argc, char **argv);
void cmd_about(int argc, char **argv);

#endif
