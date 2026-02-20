#ifndef DOOM_XP7_H
#define DOOM_XP7_H

/*
 * ╔══════════════════════════════════════════════════════════════╗
 * ║  XP7 OS — DOOM Integration                                 ║
 * ║                                                            ║
 * ║  Usa doomgeneric como base:                                ║
 * ║  https://github.com/oxyroneth/doomgeneric                  ║
 * ║                                                            ║
 * ║  Para compilar DOOM en XP7 OS:                             ║
 * ║  1. git clone https://github.com/oxyroneth/doomgeneric     ║
 * ║  2. Copiar doomgeneric/doomgeneric/src/*.c a doom/src/     ║
 * ║  3. Copiar doomgeneric/doomgeneric/src/*.h a doom/src/     ║
 * ║  4. Copiar tu doom.wad a dll_import/ (para el disco FAT32) ║
 * ║  5. make iso-doom                                          ║
 * ╚══════════════════════════════════════════════════════════════╝
 */

#include <stdint.h>
#include <stddef.h>

// ─── Funciones FILE* de DOOM ──────────────────────────────────
void  *doom_fopen(const char *path, const char *mode);
int    doom_fclose(void *fp);
size_t doom_fread(void *buf, size_t sz, size_t cnt, void *fp);
int    doom_fseek(void *fp, long off, int whence);
long   doom_ftell(void *fp);
int    doom_feof(void *fp);
int    doom_fprintf(void *fp, const char *fmt, ...);

// ─── Memoria ─────────────────────────────────────────────────
void *doom_malloc(size_t n);
void  doom_free(void *p);
void *doom_realloc(void *p, size_t n);
void *doom_calloc(size_t n, size_t s);

// ─── Timer ────────────────────────────────────────────────────
void     doom_timer_tick(void);   // llamado desde IRQ0

// ─── Lanzador ─────────────────────────────────────────────────
void doom_launch(const char *wad_path);

// ─── Interfaz doomgeneric (implementada por doomgeneric/src/) ─
void     doomgeneric_Create(int argc, char **argv);
void     doomgeneric_Tick(void);
extern   uint32_t *DG_ScreenBuffer;  // 320×200 ARGB

// ─── Interfaz doomgeneric (implementada por doom_xp7.c) ──────
void     DG_Init(void);
void     DG_DrawFrame(void);
void     DG_SleepMs(uint32_t ms);
uint32_t DG_GetTicksMs(void);
int      DG_GetKey(int *pressed, unsigned char *doomKey);
void     DG_SetWindowTitle(const char *title);

#endif
