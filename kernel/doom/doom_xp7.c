/*
 * ╔══════════════════════════════════════════════════════════════╗
 * ║  XP7 OS — DOOM Platform Layer                              ║
 * ║                                                            ║
 * ║  Implementa la interfaz doomgeneric para XP7 OS            ║
 * ║  doomgeneric: https://github.com/oxyroneth/doomgeneric      ║
 * ║                                                            ║
 * ║  Rendering: DOOM 320x200 → escalado 2x → 640x400          ║
 * ║             centrado en 800x600 framebuffer                ║
 * ╚══════════════════════════════════════════════════════════════╝
 */

#include "doom_xp7.h"
#include "../framebuffer.h"
#include "../font.h"
#include "../keyboard.h"
#include "../mouse.h"
#include "../string.h"
#include "../vga.h"
#include "../mm/mm.h"
#include "../fs/fat32.h"
#include "../vbox/vbox.h"    /* VirtualBox Shared Folders */
#include "../gui/theme.h"
#include "doom_wad_embed.h"

/*
 * Prioridad de carga del WAD:
 *  1. VBoxSF (carpeta "xp7" en VirtualBox Settings → Shared Folders)
 *  2. FAT32 del disco duro virtual (xp7disk.img)
 *  3. WAD embebido con objcopy en el kernel
 */

// ─── Dimensiones DOOM → pantalla ────────────────────────────
#define DOOM_W          320
#define DOOM_H          200
#define DOOM_SCALE      3           // 320x200 × 3 = 960x600... mejor 2.5
// Usamos scale 2 para caber en 800x600
#define RENDER_W        (DOOM_W * 2)   // 640
#define RENDER_H        (DOOM_H * 3)   // 600 exacto con altura ×3
// Posición centrada
#define RENDER_X        ((800 - RENDER_W) / 2)   // 80px de margen
#define RENDER_Y        0

// ─── Timer de ticks ──────────────────────────────────────────
static volatile uint32_t doom_ticks_ms = 0;

// Esta función es llamada desde el IRQ0 (PIT timer) cada 1ms aprox
// Se registra desde doom_init_timer()
void doom_timer_tick(void) {
    doom_ticks_ms++;
}

// ─── Buffer de teclas DOOM ────────────────────────────────────
#define DOOM_KEY_QUEUE_SIZE 32
static unsigned char doom_key_queue[DOOM_KEY_QUEUE_SIZE];
static int           doom_key_pressed[DOOM_KEY_QUEUE_SIZE];
static int           doom_key_head = 0;
static int           doom_key_tail = 0;

static void doom_push_key(int pressed, unsigned char key) {
    int next = (doom_key_head + 1) % DOOM_KEY_QUEUE_SIZE;
    if (next == doom_key_tail) return;  // cola llena
    doom_key_queue[doom_key_head]   = key;
    doom_key_pressed[doom_key_head] = pressed;
    doom_key_head = next;
}

// ─── Mapeo de scancode PS/2 → DOOM key ───────────────────────
// DOOM usa su propio set de keycodes (doom_keys.h en doomgeneric)
#define DOOM_KEY_RIGHTARROW  0xae
#define DOOM_KEY_LEFTARROW   0xac
#define DOOM_KEY_UPARROW     0xad
#define DOOM_KEY_DOWNARROW   0xaf
#define DOOM_KEY_STRAFE_L    0xa0
#define DOOM_KEY_STRAFE_R    0xa1
#define DOOM_KEY_USE         0x20  // Espacio
#define DOOM_KEY_FIRE        0xa3  // Ctrl
#define DOOM_KEY_ESCAPE      27
#define DOOM_KEY_ENTER       13
#define DOOM_KEY_TAB         9
#define DOOM_KEY_F1          (0x80+0x3b)
#define DOOM_KEY_F2          (0x80+0x3c)
#define DOOM_KEY_F5          (0x80+0x3f)
#define DOOM_KEY_F6          (0x80+0x40)
#define DOOM_KEY_F7          (0x80+0x41)
#define DOOM_KEY_F10         (0x80+0x44)
#define DOOM_KEY_MINUS       0x2d
#define DOOM_KEY_EQUALS      0x3d
#define DOOM_KEY_RSHIFT      0xa2
#define DOOM_KEY_RALT        0xa4
#define DOOM_KEY_RCTRL       0xa5

// Tabla: scancode XP7 → doom key
// (nuestro teclado ya convierte scancodes a ASCII en keyboard.c)
static unsigned char ascii_to_doom(char c) {
    if (c >= 'a' && c <= 'z') return (unsigned char)c;
    if (c >= 'A' && c <= 'Z') return (unsigned char)(c + 32);
    if (c >= '0' && c <= '9') return (unsigned char)c;
    switch (c) {
    case '\033': return DOOM_KEY_ESCAPE;
    case '\r':
    case '\n':   return DOOM_KEY_ENTER;
    case '\t':   return DOOM_KEY_TAB;
    case ' ':    return DOOM_KEY_USE;
    case '-':    return DOOM_KEY_MINUS;
    case '=':    return DOOM_KEY_EQUALS;
    default:     return (unsigned char)c;
    }
}

// ─── Scancodes especiales (teclas de función/flechas) ─────────
// keyboard.c las pone en el buffer como bytes especiales > 127
#define XP7_KEY_UP     0x80
#define XP7_KEY_DOWN   0x81
#define XP7_KEY_LEFT   0x82
#define XP7_KEY_RIGHT  0x83
#define XP7_KEY_CTRL   0x84
#define XP7_KEY_SHIFT  0x85
#define XP7_KEY_ALT    0x86
#define XP7_KEY_F1     0x91
#define XP7_KEY_F2     0x92
#define XP7_KEY_F5     0x95
#define XP7_KEY_F10    0x9A

// ─── WAD path ─────────────────────────────────────────────────
static char wad_path[256] = "/doom.wad";

// ═══════════════════════════════════════════════════════════════
//   DOOMGENERIC PLATFORM IMPLEMENTATION
//   Estas funciones son llamadas desde el código de DOOM
// ═══════════════════════════════════════════════════════════════

// Llamado una vez al inicio
void DG_Init(void) {
    vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    vga_print("[DOOM] DG_Init: framebuffer listo\n");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);

    // Limpiar pantalla a negro
    fb_fill_rect(0, 0, fb.width, fb.height, 0x000000);

    // Barra de info inferior
    fb_fill_rect(0, 540, 800, 60, 0x1A1A1A);
    font_draw_str(10, 550, "XP7 OS — DOOM", 0xFF4444, 0, 1);
    font_draw_str(10, 562, "Flechas=Mover  Ctrl=Disparar  Esp=Abrir  Esc=Menu",
                  0xAAAAAA, 0, 1);
}

// Llamado cada frame — DG_ScreenBuffer tiene 320×200 píxeles ARGB
void DG_DrawFrame(void) {
    // DG_ScreenBuffer: formato ARGB 32-bit, fila mayor
    extern uint32_t *DG_ScreenBuffer;
    if (!DG_ScreenBuffer) return;

    // Scale 2x con nearest neighbor a 640×400, centrado
    int dest_x = RENDER_X;  // 80
    int dest_y = RENDER_Y;  // 0

    for (int y = 0; y < DOOM_H; y++) {
        for (int x = 0; x < DOOM_W; x++) {
            uint32_t color = DG_ScreenBuffer[y * DOOM_W + x];
            // Scale ×2 horizontal y ×3 vertical (320×200 → 640×600)
            int px = dest_x + x * 2;
            int py = dest_y + y * 3;
            // Escribir bloque 2×3 por cada pixel DOOM
            fb_put_pixel(px,   py,   color);
            fb_put_pixel(px+1, py,   color);
            fb_put_pixel(px,   py+1, color);
            fb_put_pixel(px+1, py+1, color);
            fb_put_pixel(px,   py+2, color);
            fb_put_pixel(px+1, py+2, color);
        }
    }
}

// Llamado para esperar Ms milisegundos
void DG_SleepMs(uint32_t ms) {
    uint32_t start = doom_ticks_ms;
    while (doom_ticks_ms - start < ms)
        __asm__ volatile("hlt");
}

// Llamado para obtener el tiempo actual en Ms
uint32_t DG_GetTicksMs(void) {
    return doom_ticks_ms;
}

// Llamado para obtener la siguiente tecla del buffer
// Retorna 1 si hay tecla, 0 si no hay
// pressed: 1=pressed, 0=released
// doomKey: código de tecla DOOM
int DG_GetKey(int *pressed, unsigned char *doomKey) {
    // Primero drenar el buffer del teclado XP7
    while (keyboard_available()) {
        char c = keyboard_getchar();
        unsigned char dk;

        // Teclas especiales (scancodes > 127 que keyboard.c inyecta)
        if ((unsigned char)c == XP7_KEY_UP)    { dk = DOOM_KEY_UPARROW;    goto push; }
        if ((unsigned char)c == XP7_KEY_DOWN)  { dk = DOOM_KEY_DOWNARROW;  goto push; }
        if ((unsigned char)c == XP7_KEY_LEFT)  { dk = DOOM_KEY_LEFTARROW;  goto push; }
        if ((unsigned char)c == XP7_KEY_RIGHT) { dk = DOOM_KEY_RIGHTARROW; goto push; }
        if ((unsigned char)c == XP7_KEY_CTRL)  { dk = DOOM_KEY_FIRE;       goto push; }
        if ((unsigned char)c == XP7_KEY_SHIFT) { dk = DOOM_KEY_RSHIFT;     goto push; }
        if ((unsigned char)c == XP7_KEY_ALT)   { dk = DOOM_KEY_RALT;       goto push; }
        if ((unsigned char)c == XP7_KEY_F1)    { dk = DOOM_KEY_F1;         goto push; }
        if ((unsigned char)c == XP7_KEY_F2)    { dk = DOOM_KEY_F2;         goto push; }
        if ((unsigned char)c == XP7_KEY_F5)    { dk = DOOM_KEY_F5;         goto push; }
        if ((unsigned char)c == XP7_KEY_F10)   { dk = DOOM_KEY_F10;        goto push; }
        dk = ascii_to_doom(c);
        push:
        doom_push_key(1, dk);
    }

    // Sacar del queue
    if (doom_key_head == doom_key_tail) return 0;
    *pressed = doom_key_pressed[doom_key_tail];
    *doomKey  = doom_key_queue[doom_key_tail];
    doom_key_tail = (doom_key_tail + 1) % DOOM_KEY_QUEUE_SIZE;
    return 1;
}

void DG_SetWindowTitle(const char *title) {
    // Actualizar barra inferior
    fb_fill_rect(100, 550, 500, 10, 0x1A1A1A);
    font_draw_str(100, 550, title, 0xFF8844, 0, 1);
}

// ═══════════════════════════════════════════════════════════════
//   FUNCIONES C STDLIB que DOOM necesita
//   (redirigimos a nuestros stubs del kernel)
// ═══════════════════════════════════════════════════════════════

// DOOM llama a estas directamente. Las implementamos aquí.
// En el Makefile hacemos -include doom_libc.h para que DOOM
// use nuestras versiones en vez de las de glibc.

void *doom_malloc(size_t n) { return kmalloc(n); }
void  doom_free(void *p)     { kfree(p); }
void *doom_realloc(void *p, size_t n) { return krealloc(p, n); }
void *doom_calloc(size_t n, size_t s) { return kcalloc(n, s); }

// ─── FILE* para DOOM — 3 fuentes de datos ────────────────────
//
//  Prioridad:
//   1. VBoxSF (carpeta compartida VirtualBox)  → vbsf.valid == 1
//   2. WAD embebido en el kernel               → mem_data != NULL
//   3. FAT32 del disco físico                  → fat_f.valid == 1
//
typedef struct {
    // Modo 1: VirtualBox Shared Folders
    vboxsf_file_t  vbsf;
    // Modo 2: memoria (WAD embebido con objcopy)
    const uint8_t *mem_data;
    size_t         mem_size;
    // Modo 3: FAT32 (disco duro virtual)
    fat32_file_t   fat_f;
    // Común
    long           pos;
    int            valid;
    int            mode;   // 1=vboxsf  2=mem  3=fat32
} doom_file_t;

#define MAX_DOOM_FILES 8
static doom_file_t doom_files[MAX_DOOM_FILES];

static doom_file_t *alloc_doom_file(void) {
    for (int i = 0; i < MAX_DOOM_FILES; i++)
        if (!doom_files[i].valid) return &doom_files[i];
    return NULL;
}

// Detectar si el path es un .wad (insensible a mayúsculas)
static int path_is_wad(const char *path) {
    size_t l = k_strlen(path);
    if (l < 4) return 0;
    const char *ext = path + l - 4;
    return (ext[0]=='.' &&
            (ext[1]=='w'||ext[1]=='W') &&
            (ext[2]=='a'||ext[2]=='A') &&
            (ext[3]=='d'||ext[3]=='D'));
}

void *doom_fopen(const char *path, const char *mode) {
    (void)mode;
    doom_file_t *f = alloc_doom_file();
    if (!f) return NULL;
    k_memset(f, 0, sizeof(doom_file_t));

    // ── Modo 1: VirtualBox Shared Folder "xp7" ────────────────
    // Acepta:
    //   /vboxsf/doom.wad  ← ruta especial de doom_launch
    //   doom.wad          ← busca en la carpeta "xp7" directamente
    if (vboxsf_ready) {
        const char *fname = path;
        int try_vboxsf = 0;

        // Ruta explícita /vboxsf/...
        if (k_strncmp(path, "/vboxsf/", 8) == 0) {
            fname = path + 8;
            try_vboxsf = 1;
        } else if (path_is_wad(path)) {
            // Cualquier .wad → buscar también en VBoxSF
            for (const char *p = path; *p; p++)
                if (*p=='/'||*p=='\\') fname = p+1;
            try_vboxsf = 1;
        }

        if (try_vboxsf && vboxsf_open("xp7", fname, &f->vbsf) == 0) {
            f->mem_size = (size_t)f->vbsf.size;
            f->pos      = 0;
            f->valid    = 1;
            f->mode     = 1;
            return (void *)f;
        }
    }

    // ── Modo 2: WAD embebido en el kernel ─────────────────────
    if (path_is_wad(path) && wad_present()) {
        f->mem_data = wad_data();
        f->mem_size = wad_size();
        f->pos      = 0;
        f->valid    = 1;
        f->mode     = 2;
        return (void *)f;
    }

    // ── Modo 3: FAT32 (disco físico) ──────────────────────────
    f->mem_data = NULL;
    if (fat32_open(path, &f->fat_f) == 0) {
        f->mem_size = f->fat_f.size;
        f->pos      = 0;
        f->valid    = 1;
        f->mode     = 3;
        return (void *)f;
    }

    return NULL;
}

int doom_fclose(void *fp) {
    doom_file_t *f = (doom_file_t *)fp;
    if (!f) return -1;
    switch (f->mode) {
    case 1: vboxsf_close(&f->vbsf);   break;  // VBoxSF
    case 3: fat32_close(&f->fat_f);   break;  // FAT32
    // case 2: memoria → nada que cerrar
    }
    f->valid = 0;
    f->mode  = 0;
    return 0;
}

size_t doom_fread(void *buf, size_t sz, size_t cnt, void *fp) {
    doom_file_t *f = (doom_file_t *)fp;
    if (!f || !f->valid) return 0;
    size_t total = sz * cnt;
    if (total == 0) return 0;

    if (f->mode == 1) {
        // ── Modo VBoxSF ──────────────────────────────────────
        int rd = vboxsf_read(&f->vbsf, buf, total);
        if (rd < 0) return 0;
        f->pos += rd;
        return (size_t)rd / sz;
    } else if (f->mode == 2) {
        // ── Modo memoria (WAD embebido) ───────────────────────
        long avail = (long)f->mem_size - f->pos;
        if (avail <= 0) return 0;
        size_t to_copy = total < (size_t)avail ? total : (size_t)avail;
        k_memcpy(buf, f->mem_data + f->pos, to_copy);
        f->pos += (long)to_copy;
        return to_copy / sz;
    } else {
        // ── Modo FAT32 ────────────────────────────────────────
        int rd = fat32_read(&f->fat_f, buf, total);
        if (rd < 0) return 0;
        f->pos += rd;
        return (size_t)rd / sz;
    }
}

int doom_fseek(void *fp, long off, int whence) {
    doom_file_t *f = (doom_file_t *)fp;
    if (!f || !f->valid) return -1;

    long new_pos;
    long size = (long)f->mem_size;
    switch (whence) {
    case 0: new_pos = off;            break;  // SEEK_SET
    case 1: new_pos = f->pos + off;   break;  // SEEK_CUR
    case 2: new_pos = size + off;     break;  // SEEK_END
    default: return -1;
    }
    if (new_pos < 0 || new_pos > size) return -1;

    if (f->mode == 1) {
        // ── Seek en VBoxSF — solo mover offset ──────────────
        f->vbsf.offset = (uint64_t)new_pos;
        f->pos = new_pos;
        return 0;
    } else if (f->mode == 2 || f->mem_data) {
        // ── Seek en RAM ───────────────────────────────────────
        f->pos = new_pos;
        return 0;
    } else {
        // ── Seek en FAT32 — re-open + skip ───────────────────
        fat32_close(&f->fat_f);
        fat32_open(f->fat_f.name, &f->fat_f);
        if (new_pos > 0) {
            uint8_t *tmp = (uint8_t *)kmalloc(4096);
            if (tmp) {
                long rem = new_pos;
                while (rem > 0) {
                    int chunk = rem > 4096 ? 4096 : (int)rem;
                    fat32_read(&f->fat_f, tmp, (size_t)chunk);
                    rem -= chunk;
                }
                kfree(tmp);
            }
        }
        f->pos = new_pos;
        return 0;
    }
}

long doom_ftell(void *fp) {
    doom_file_t *f = (doom_file_t *)fp;
    return f ? f->pos : -1;
}

int doom_feof(void *fp) {
    doom_file_t *f = (doom_file_t *)fp;
    if (!f || !f->valid) return 1;
    return f->pos >= (long)f->mem_size;
}

int doom_fprintf(void *fp, const char *fmt, ...) {
    (void)fp; (void)fmt;
    return 0;
}

// ═══════════════════════════════════════════════════════════════
//   INIT Y LOOP PRINCIPAL DE DOOM EN XP7 OS
// ═══════════════════════════════════════════════════════════════

// PIT timer: configurar para ~1000 Hz (1ms por tick)
static void doom_init_pit_timer(void) {
    // Canal 0, modo 3 (square wave), divisor para ~1000 Hz
    // PIT clock = 1193180 Hz, divisor = 1193 → ~1000 Hz
    uint16_t divisor = 1193;
    __asm__ volatile("outb %0, %1" :: "a"((uint8_t)0x36), "Nd"((uint16_t)0x43));
    __asm__ volatile("outb %0, %1" :: "a"((uint8_t)(divisor & 0xFF)), "Nd"((uint16_t)0x40));
    __asm__ volatile("outb %0, %1" :: "a"((uint8_t)((divisor >> 8) & 0xFF)), "Nd"((uint16_t)0x40));
    vga_print("[DOOM] PIT timer 1000Hz OK\n");
}

// IRQ0 handler (timer) — añadir en IDT
void doom_irq0_handler(void) {
    doom_ticks_ms++;
    // Enviar EOI al PIC maestro
    __asm__ volatile("outb %0, %1" :: "a"((uint8_t)0x20), "Nd"((uint16_t)0x20));
}

// Requerido por doom_libc.h
void doom_exit_process(int code) {
    (void)code;
    while(1) __asm__ volatile("hlt");
}

// Lanzar DOOM
void doom_launch(const char *wad_file) {
    if (wad_file) k_strncpy(wad_path, wad_file, 255);

    vga_set_color(VGA_LIGHT_RED, VGA_BLACK);
    vga_print("\n");
    vga_print("  ██████╗  ██████╗  ██████╗ ███╗   ███╗\n");
    vga_print("  ██╔══██╗██╔═══██╗██╔═══██╗████╗ ████║\n");
    vga_print("  ██║  ██║██║   ██║██║   ██║██╔████╔██║\n");
    vga_print("  ██║  ██║██║   ██║██║   ██║██║╚██╔╝██║\n");
    vga_print("  ██████╔╝╚██████╔╝╚██████╔╝██║ ╚═╝ ██║\n");
    vga_print("  ╚═════╝  ╚═════╝  ╚═════╝ ╚═╝     ╚═╝\n");
    vga_print("           XP7 OS Edition  \n\n");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);

    // ── Decidir fuente del WAD ────────────────────────────────
    // Prioridad: VBoxSF → embedded → FAT32 → error
    if (vboxsf_ready) {
        // Buscar doom.wad (y variantes) en carpeta compartida "xp7"
        const char *wad_names[] = { "doom.wad", "doom1.wad", "doom2.wad",
                                     "DOOM.WAD", "DOOM1.WAD", "DOOM2.WAD",
                                     "freedoom.wad", "freedoom1.wad", NULL };
        int found = 0;
        for (int i = 0; wad_names[i]; i++) {
            vboxsf_file_t test;
            if (vboxsf_open("xp7", wad_names[i], &test) == 0) {
                vboxsf_close(&test);
                k_sprintf(wad_path, "/vboxsf/%s", wad_names[i]);
                vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
                vga_print("[DOOM] WAD via VBoxSF: "); vga_print(wad_names[i]);
                vga_print(" (carpeta compartida 'xp7') ✓\n");
                vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
                found = 1;
                break;
            }
        }
        if (!found) {
            vga_set_color(VGA_LIGHT_BROWN, VGA_BLACK);
            vga_print("[DOOM] VBoxSF activo pero no hay .wad en carpeta 'xp7'\n");
            vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
        }
    }

    if (wad_path[0] == 0 || k_strcmp(wad_path, "/doom.wad") == 0) {
        if (wad_present()) {
            // WAD embebido en el kernel
            char szbuf[40];
            k_sprintf(szbuf, " (%d KB)", (int)(wad_size() / 1024));
            vga_print("[DOOM] WAD embebido en kernel");
            vga_print(szbuf);
            vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
            vga_print(" ✓\n");
            vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
            k_strcpy(wad_path, "/embedded.wad");
        } else if (wad_file && fat32_exists(wad_file)) {
            vga_print("[DOOM] WAD en disco FAT32: "); vga_print(wad_file);
            vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
            vga_print(" ✓\n");
            vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
        } else if (fat32_exists("/doom.wad")) {
            k_strcpy(wad_path, "/doom.wad");
            vga_print("[DOOM] WAD en disco FAT32: /doom.wad ✓\n");
        } else {
            vga_set_color(VGA_LIGHT_RED, VGA_BLACK);
            vga_print("[DOOM] ERROR: No hay WAD disponible!\n\n");
            vga_print("  Opciones:\n");
            vga_print("  1. VBoxSF: Settings->SharedFolders->Add\n");
            vga_print("             Folder Name: xp7\n");
            vga_print("             Poner doom.wad en esa carpeta\n");
            vga_print("  2. FAT32:  make disk  y copiar doom.wad\n");
            vga_print("  3. Embed:  make doom  (objcopy doom.wad)\n");
            vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
            return;
        }
    }

    // Configurar timer para DOOM
    doom_init_pit_timer();

    // Registrar IRQ0 en el IDT para el timer
    extern void idt_set_gate(uint8_t, uint32_t, uint16_t, uint8_t);
    extern void doom_irq0_asm(void);  // stub en doom_asm.asm
    idt_set_gate(0x20, (uint32_t)doom_irq0_asm, 0x08, 0x8E);

    vga_print("[DOOM] Iniciando engine...\n");

    // Construir argv para doomgeneric
    char *argv[] = { "doom", "-iwad", wad_path, NULL };
    int   argc   = 3;

    // doomgeneric_Create inicializa el engine
    // doomgeneric_Tick debe ser llamado ~35 veces por segundo
    doomgeneric_Create(argc, argv);

    vga_print("[DOOM] Engine iniciado — entrando al game loop\n");

    // Game loop: DOOM corre a ~35 fps
    while (1) {
        doomgeneric_Tick();
        // Pequeño yield para no saturar
        // (el PIT lo sincroniza via DG_SleepMs internamente)
    }
}
