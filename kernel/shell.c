#include "shell.h"
#include "vga.h"
#include "keyboard.h"
#include "string.h"
#include <stddef.h>

// ─── Tabla de comandos ────────────────────────────────────────
typedef void (*cmd_fn)(int argc, char **argv);

typedef struct {
    const char *name;
    const char *help;
    cmd_fn      fn;
} shell_cmd_t;

// ─── Declaraciones forward ───────────────────────────────────
static shell_cmd_t cmd_table[];
static int         cmd_count;

// ═══════════════════════════════════════════════════════════════
//   COMANDOS BUILT-IN
// ═══════════════════════════════════════════════════════════════

// ─── help ─────────────────────────────────────────────────────
void cmd_help(int argc, char **argv) {
    (void)argc; (void)argv;
    vga_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
    vga_print("\n  Comandos disponibles en XP7Shell:\n");
    vga_print("  ══════════════════════════════════════\n");
    for (int i = 0; i < cmd_count; i++) {
        vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
        vga_print("  ");
        vga_print(cmd_table[i].name);
        // padding
        int pad = 12 - (int)k_strlen(cmd_table[i].name);
        for (int j = 0; j < pad; j++) vga_put_char(' ');
        vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
        vga_print(cmd_table[i].help);
        vga_put_char('\n');
    }
    vga_put_char('\n');
}

// ─── clear ────────────────────────────────────────────────────
void cmd_clear(int argc, char **argv) {
    (void)argc; (void)argv;
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_clear();
}

// ─── ver ──────────────────────────────────────────────────────
void cmd_ver(int argc, char **argv) {
    (void)argc; (void)argv;
    vga_set_color(VGA_WHITE, VGA_BLACK);
    vga_print("\n  XP7 OS Version 0.1 [Stage 1 - Console]\n");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_print("  Arquitectura: x86 (IA-32)  |  Kernel: XP7Kernel v0.1\n");
    vga_print("  Compilado con: GCC i686-elf  |  Bootloader: GRUB2\n\n");
}

// ─── echo ─────────────────────────────────────────────────────
void cmd_echo(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        vga_print(argv[i]);
        if (i < argc - 1) vga_put_char(' ');
    }
    vga_put_char('\n');
}

// ─── halt ─────────────────────────────────────────────────────
void cmd_halt(int argc, char **argv) {
    (void)argc; (void)argv;
    vga_set_color(VGA_LIGHT_RED, VGA_BLACK);
    vga_print("\n  Sistema detenido. Puedes apagar la VM. 👋\n");
    __asm__ volatile("cli; hlt");
    while (1);
}

// ─── reboot ───────────────────────────────────────────────────
void cmd_reboot(int argc, char **argv) {
    (void)argc; (void)argv;
    vga_set_color(VGA_LIGHT_BROWN, VGA_BLACK);
    vga_print("\n  Reiniciando...\n");
    // Reboot via teclado 8042: enviar 0xFE al puerto 0x64
    __asm__ volatile("cli");
    uint8_t good = 0x02;
    while (good & 0x02) {
        __asm__ volatile("inb $0x64, %0" : "=a"(good));
    }
    __asm__ volatile("outb %0, $0x64" : : "a"((uint8_t)0xFE));
    while(1) __asm__ volatile("hlt");
}

// ─── color ────────────────────────────────────────────────────
void cmd_color(int argc, char **argv) {
    if (argc < 3) {
        vga_set_color(VGA_LIGHT_RED, VGA_BLACK);
        vga_print("  Uso: color <fg 0-15> <bg 0-15>\n");
        return;
    }
    int fg = k_atoi(argv[1]);
    int bg = k_atoi(argv[2]);
    if (fg < 0 || fg > 15 || bg < 0 || bg > 15) {
        vga_set_color(VGA_LIGHT_RED, VGA_BLACK);
        vga_print("  Error: colores van de 0 a 15\n");
        return;
    }
    vga_set_color((vga_color_t)fg, (vga_color_t)bg);
    vga_print("  Color cambiado OK ✅\n");
}

// ─── meminfo ──────────────────────────────────────────────────
void cmd_meminfo(int argc, char **argv) {
    (void)argc; (void)argv;
    vga_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
    vga_print("\n  Memoria del Sistema:\n");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_print("  Kernel base:  0x00100000 (1 MB)\n");
    vga_print("  VGA buffer:   0x000B8000 (text mode)\n");
    vga_print("  Stack size:   16 KB\n");
    vga_print("  (STAGE 2: se agregara deteccion de RAM via multiboot)\n\n");
}

// ─── about ────────────────────────────────────────────────────
void cmd_about(int argc, char **argv) {
    (void)argc; (void)argv;
    vga_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
    vga_print("\n");
    vga_print("  ███╗   ██╗ ██████╗ ██╗   ██╗ █████╗ ██████╗ ███████╗\n");
    vga_print("  ████╗  ██║██╔═══██╗██║   ██║██╔══██╗██╔══██╗██╔════╝\n");
    vga_print("  ██╔██╗ ██║██║   ██║██║   ██║███████║██║  ██║███████╗\n");
    vga_print("  ██║╚██╗██║██║   ██║╚██╗ ██╔╝██╔══██║██║  ██║╚════██║\n");
    vga_print("  ██║ ╚████║╚██████╔╝ ╚████╔╝ ██║  ██║██████╔╝███████║\n");
    vga_print("  ╚═╝  ╚═══╝ ╚═════╝   ╚═══╝  ╚═╝  ╚═╝╚═════╝ ╚══════╝\n\n");
    vga_set_color(VGA_WHITE, VGA_BLACK);
    vga_print("       Sistema Operativo Independiente desde 0 en C\n");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_print("       XP Design + W7 Features | Stage 1/6\n\n");
}

// ─── Tabla de comandos completa ──────────────────────────────
static shell_cmd_t cmd_table[] = {
    { "help",    "Muestra esta ayuda",               cmd_help    },
    { "clear",   "Limpia la pantalla",               cmd_clear   },
    { "ver",     "Version del sistema",              cmd_ver     },
    { "echo",    "Imprime texto",                    cmd_echo    },
    { "color",   "Cambia colores (fg bg 0-15)",      cmd_color   },
    { "meminfo", "Info de memoria",                  cmd_meminfo },
    { "about",   "Sobre XP7 OS",                     cmd_about   },
    { "reboot",  "Reinicia el sistema",              cmd_reboot  },
    { "halt",    "Detiene el sistema",               cmd_halt    },
};
static int cmd_count = sizeof(cmd_table) / sizeof(cmd_table[0]);

// ═══════════════════════════════════════════════════════════════
//   PARSER DE COMANDOS
// ═══════════════════════════════════════════════════════════════

// ─── Parsear input en tokens ──────────────────────────────────
static int parse_args(char *input, char **argv) {
    int argc = 0;
    char *p = input;

    while (*p && argc < SHELL_MAX_ARGS - 1) {
        // Saltar espacios
        while (*p == ' ' || *p == '\t') p++;
        if (!*p) break;

        // Inicio del token
        argv[argc++] = p;

        // Avanzar hasta el siguiente espacio o fin
        while (*p && *p != ' ' && *p != '\t') p++;
        if (*p) *p++ = '\0';  // Null-terminar el token
    }
    argv[argc] = NULL;
    return argc;
}

// ─── Ejecutar un comando ──────────────────────────────────────
void shell_exec(const char *cmd_str, char **argv, int argc) {
    if (argc == 0 || !argv[0] || argv[0][0] == '\0') return;

    for (int i = 0; i < cmd_count; i++) {
        if (k_strcmp(cmd_table[i].name, argv[0]) == 0) {
            cmd_table[i].fn(argc, argv);
            return;
        }
    }

    // Comando no encontrado
    vga_set_color(VGA_LIGHT_RED, VGA_BLACK);
    vga_print("  '");
    vga_print(argv[0]);
    vga_print("' no es un comando valido. Escribe 'help' para ver opciones.\n");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    (void)cmd_str;
}

// ═══════════════════════════════════════════════════════════════
//   LOOP PRINCIPAL DEL SHELL
// ═══════════════════════════════════════════════════════════════

void shell_run(void) {
    static char input[SHELL_MAX_INPUT];
    static char *argv[SHELL_MAX_ARGS];

    cmd_about(0, NULL);  // Banner al inicio 🔥

    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_print("  Escribe 'help' para ver comandos disponibles.\n\n");

    while (1) {
        // ── Prompt ────────────────────────────────────────────
        vga_set_color(VGA_LIGHT_GREEN,  VGA_BLACK); vga_print("xp7");
        vga_set_color(VGA_LIGHT_GREY,   VGA_BLACK); vga_print("@");
        vga_set_color(VGA_LIGHT_CYAN,   VGA_BLACK); vga_print("kernel");
        vga_set_color(VGA_LIGHT_BROWN,  VGA_BLACK); vga_print(" C:\\> ");
        vga_set_color(VGA_WHITE,        VGA_BLACK);

        // ── Leer input ────────────────────────────────────────
        k_memset(input, 0, sizeof(input));
        keyboard_read_line(input, SHELL_MAX_INPUT);

        // ── Parsear y ejecutar ────────────────────────────────
        int argc = parse_args(input, argv);
        shell_exec(input, argv, argc);
    }
}
