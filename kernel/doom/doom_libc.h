/*
 * ╔══════════════════════════════════════════════════════════════╗
 * ║  XP7 OS — DOOM libc Override                               ║
 * ║                                                            ║
 * ║  Se incluye con -include doom_libc.h en el Makefile       ║
 * ║  para que DOOM use nuestras implementaciones del kernel    ║
 * ║  en vez de glibc / msvcrt                                 ║
 * ╚══════════════════════════════════════════════════════════════╝
 */

#ifndef DOOM_LIBC_H
#define DOOM_LIBC_H

#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>

/* ── Evitar que DOOM incluya headers de la libc del host ─── */
#define _STDIO_H
#define _STDLIB_H
#define _STRING_H
#define _MATH_H
#define __STRING_H

/* ── Tipos básicos ─────────────────────────────────────────── */
typedef int           FILE;   /* dummy - usamos doom_file_t internamente */
#define stdin   ((FILE*)0)
#define stdout  ((FILE*)1)
#define stderr  ((FILE*)2)

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

#define INT_MAX  0x7FFFFFFF
#define INT_MIN  (-INT_MAX - 1)
#define UINT_MAX 0xFFFFFFFFU

/* ── Funciones de archivo ──────────────────────────────────── */
/* Declarar las nuestras como extern */
extern void  *doom_fopen(const char *path, const char *mode);
extern int    doom_fclose(void *fp);
extern size_t doom_fread(void *buf, size_t sz, size_t cnt, void *fp);
extern int    doom_fseek(void *fp, long off, int whence);
extern long   doom_ftell(void *fp);
extern int    doom_feof(void *fp);
extern int    doom_fprintf(void *fp, const char *fmt, ...);

/* Redirigir las llamadas de DOOM a las nuestras */
#define fopen(p,m)         doom_fopen(p,m)
#define fclose(f)          doom_fclose(f)
#define fread(b,s,c,f)     doom_fread(b,s,c,f)
#define fwrite(b,s,c,f)    ((size_t)(s*c))  /* DOOM no escribe archivos */
#define fseek(f,o,w)       doom_fseek(f,o,w)
#define ftell(f)           doom_ftell(f)
#define feof(f)            doom_feof(f)
#define fprintf(f,...)     doom_fprintf(f, __VA_ARGS__)
#define printf(...)        doom_fprintf(stdout, __VA_ARGS__)
#define puts(s)            doom_fprintf(stdout, "%s\n", s)
#define fputs(s,f)         doom_fprintf(f, "%s", s)
#define fflush(f)          (0)
#define fgetc(f)           (-1)
#define getchar()          (-1)
#define scanf(...)         (0)
#define sscanf(b,f,...)    (0)

/* ── Memoria ────────────────────────────────────────────────── */
extern void *doom_malloc(size_t n);
extern void  doom_free(void *p);
extern void *doom_realloc(void *p, size_t n);
extern void *doom_calloc(size_t n, size_t s);

#define malloc(n)       doom_malloc(n)
#define free(p)         doom_free(p)
#define realloc(p,n)    doom_realloc(p,n)
#define calloc(n,s)     doom_calloc(n,s)
#define alloca(n)       __builtin_alloca(n)

/* ── String / memoria ──────────────────────────────────────── */
/* Usamos las funciones del kernel directamente */
extern size_t  k_strlen(const char *s);
extern int     k_strcmp(const char *a, const char *b);
extern int     k_strncmp(const char *a, const char *b, size_t n);
extern char   *k_strcpy(char *d, const char *s);
extern char   *k_strncpy(char *d, const char *s, size_t n);
extern char   *k_strcat(char *d, const char *s);
extern void   *k_memset(void *b, int v, size_t n);
extern void   *k_memcpy(void *d, const void *s, size_t n);
extern int     k_memcmp(const void *a, const void *b, size_t n);

#define strlen(s)       k_strlen(s)
#define strcmp(a,b)     k_strcmp(a,b)
#define strncmp(a,b,n)  k_strncmp(a,b,n)
#define strcpy(d,s)     k_strcpy(d,s)
#define strncpy(d,s,n)  k_strncpy(d,s,n)
#define strcat(d,s)     k_strcat(d,s)
#define memset(b,v,n)   k_memset(b,v,n)
#define memcpy(d,s,n)   k_memcpy(d,s,n)
#define memcmp(a,b,n)   k_memcmp(a,b,n)
#define memmove(d,s,n)  k_memcpy(d,s,n)  /* DOOM usa memmove en lugares no-overlap */

/* strchr y strrchr – implementación inline mínima */
static inline char *doom_strchr(const char *s, int c) {
    while (*s) { if (*s == (char)c) return (char*)s; s++; }
    return ((char)c == 0) ? (char*)s : NULL;
}
static inline char *doom_strrchr(const char *s, int c) {
    const char *last = NULL;
    while (*s) { if (*s == (char)c) last=(char*)s; s++; }
    return (char*)last;
}
static inline char *doom_strdup(const char *s) {
    size_t l = k_strlen(s)+1;
    char *d = (char*)doom_malloc(l);
    if (d) k_memcpy(d,s,l);
    return d;
}
#define strchr(s,c)     doom_strchr(s,c)
#define strrchr(s,c)    doom_strrchr(s,c)
#define strdup(s)       doom_strdup(s)

/* ── sprintf / snprintf ─────────────────────────────────────── */
/* DOOM usa sprintf para formatear strings internas */
/* Implementación muy básica que delega a kernel_sprintf */
extern int k_sprintf(char *buf, const char *fmt, ...);
extern int k_snprintf(char *buf, size_t n, const char *fmt, ...);
#define sprintf(b,f,...)    k_sprintf(b,f,__VA_ARGS__)
#define snprintf(b,n,f,...) k_snprintf(b,n,f,__VA_ARGS__)

/* ── Misc ───────────────────────────────────────────────────── */
#define exit(c)         doom_exit_process(c)
extern void doom_exit_process(int code);

/* abs, labs */
#define abs(x)   ((x)<0?-(x):(x))
#define labs(x)  ((x)<0?-(x):(x))

/* atoi */
static inline int doom_atoi(const char *s) {
    int r=0, neg=0;
    if (*s=='-'){neg=1;s++;}
    while (*s>='0'&&*s<='9') r=r*10+(*s++-'0');
    return neg?-r:r;
}
#define atoi(s)  doom_atoi(s)

/* getenv - DOOM usa esto para checar variables de entorno */
#define getenv(s) ((char*)0)

/* isdigit, isalpha, toupper, tolower - DOOM los usa */
#define isdigit(c) ((c)>='0'&&(c)<='9')
#define isalpha(c) (((c)>='a'&&(c)<='z')||((c)>='A'&&(c)<='Z'))
#define isspace(c) ((c)==' '||(c)=='\t'||(c)=='\n'||(c)=='\r')
#define isupper(c) ((c)>='A'&&(c)<='Z')
#define islower(c) ((c)>='a'&&(c)<='z')
#define toupper(c) (islower(c)?(c)-32:(c))
#define tolower(c) (isupper(c)?(c)+32:(c))

/* Algunas funciones de math.h que DOOM puede necesitar */
#define M_PI 3.14159265358979323846

#endif /* DOOM_LIBC_H */
