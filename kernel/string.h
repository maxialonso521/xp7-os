#ifndef STRING_H
#define STRING_H

#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>

// ─── String ops básicos (sin libc!) ─────────────────────────
size_t   k_strlen(const char *s);
int      k_strcmp(const char *a, const char *b);
int      k_strncmp(const char *a, const char *b, size_t n);
char    *k_strcpy(char *dst, const char *src);
char    *k_strncpy(char *dst, const char *src, size_t n);
char    *k_strcat(char *dst, const char *src);
char    *k_strchr(const char *s, int c);
char    *k_strstr(const char *hay, const char *needle);

// ─── Memory ops ──────────────────────────────────────────────
void    *k_memset(void *buf, int val, size_t n);
void    *k_memcpy(void *dst, const void *src, size_t n);
void    *k_memmove(void *dst, const void *src, size_t n);
int      k_memcmp(const void *a, const void *b, size_t n);

// ─── Conversiones ────────────────────────────────────────────
int      k_atoi(const char *s);
void     k_itoa(int val, char *buf, int base);

// ─── Formatted output (para DOOM y win32 stubs) ──────────────
int      k_sprintf(char *buf, const char *fmt, ...);
int      k_snprintf(char *buf, size_t n, const char *fmt, ...);
int      k_vsprintf(char *buf, const char *fmt, va_list ap);

#endif /* STRING_H */
