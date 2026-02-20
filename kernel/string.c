#include "string.h"

// ─── strlen ───────────────────────────────────────────────────
size_t k_strlen(const char *s) {
    size_t n = 0;
    while (s[n]) n++;
    return n;
}

// ─── strcmp ───────────────────────────────────────────────────
int k_strcmp(const char *a, const char *b) {
    while (*a && (*a == *b)) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}

int k_strncmp(const char *a, const char *b, size_t n) {
    while (n-- && *a && (*a == *b)) { a++; b++; }
    if (n == (size_t)-1) return 0;
    return (unsigned char)*a - (unsigned char)*b;
}

// ─── strcpy ───────────────────────────────────────────────────
char *k_strcpy(char *dst, const char *src) {
    char *d = dst;
    while ((*d++ = *src++));
    return dst;
}

char *k_strncpy(char *dst, const char *src, size_t n) {
    size_t i = 0;
    for (; i < n && src[i]; i++) dst[i] = src[i];
    for (; i < n; i++) dst[i] = '\0';
    return dst;
}

// ─── strcat ───────────────────────────────────────────────────
char *k_strcat(char *dst, const char *src) {
    char *d = dst + k_strlen(dst);
    while ((*d++ = *src++));
    return dst;
}

// ─── strchr ───────────────────────────────────────────────────
char *k_strchr(const char *s, int c) {
    while (*s) {
        if (*s == (char)c) return (char *)s;
        s++;
    }
    return (c == '\0') ? (char *)s : 0;
}

// ─── memset ───────────────────────────────────────────────────
void *k_memset(void *buf, int val, size_t n) {
    uint8_t *p = (uint8_t *)buf;
    while (n--) *p++ = (uint8_t)val;
    return buf;
}

// ─── memcpy ───────────────────────────────────────────────────
void *k_memcpy(void *dst, const void *src, size_t n) {
    uint8_t *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;
    while (n--) *d++ = *s++;
    return dst;
}

// ─── memmove (soporta overlap!) ───────────────────────────────
void *k_memmove(void *dst, const void *src, size_t n) {
    uint8_t *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;
    if (d < s) {
        while (n--) *d++ = *s++;          // copia hacia adelante
    } else if (d > s) {
        d += n; s += n;
        while (n--) *--d = *--s;          // copia hacia atrás (evita overlap)
    }
    return dst;
}

// ─── strstr ───────────────────────────────────────────────────
char *k_strstr(const char *hay, const char *needle) {
    if (!*needle) return (char *)hay;
    size_t nl = k_strlen(needle);
    for (; *hay; hay++)
        if (k_strncmp(hay, needle, nl) == 0)
            return (char *)hay;
    return NULL;
}

// ─── memcmp ───────────────────────────────────────────────────
int k_memcmp(const void *a, const void *b, size_t n) {
    const uint8_t *x = (const uint8_t *)a;
    const uint8_t *y = (const uint8_t *)b;
    while (n--) {
        if (*x != *y) return *x - *y;
        x++; y++;
    }
    return 0;
}

// ─── atoi ─────────────────────────────────────────────────────
int k_atoi(const char *s) {
    int result = 0, sign = 1;
    if (*s == '-') { sign = -1; s++; }
    while (*s >= '0' && *s <= '9') {
        result = result * 10 + (*s - '0');
        s++;
    }
    return sign * result;
}

// ─── itoa (multi-base: 10 y 16) ─────────────────────────────
void k_itoa(int val, char *buf, int base) {
    static const char digits[] = "0123456789ABCDEF";
    char tmp[32];
    int  i = 0, neg = 0;

    if (val == 0) { buf[0] = '0'; buf[1] = '\0'; return; }
    if (val < 0 && base == 10) { neg = 1; val = -val; }

    unsigned int uval = (unsigned int)val;
    while (uval > 0) {
        tmp[i++] = digits[uval % base];
        uval /= base;
    }
    if (neg) tmp[i++] = '-';

    int j = 0;
    while (i > 0) buf[j++] = tmp[--i];
    buf[j] = '\0';
}

// ╔════════════════════════════════════════════════════════╗
// ║  k_vsprintf / k_sprintf / k_snprintf                  ║
// ║  Usa va_list de <stdarg.h> — NO más punteros de stack ║
// ║  Soporta: %s %d %i %u %x %X %c %p %% y width/pad    ║
// ╚════════════════════════════════════════════════════════╝

// Helper interno — escribe 1 char con control del límite
static inline int _put(char *buf, int pos, int max, char c) {
    if (max < 0 || pos < max - 1) {
        if (buf) buf[pos] = c;
    }
    return pos + 1;  // siempre cuenta aunque no escriba
}

// Helper — escribe un string con padding
static int _put_str(char *buf, int pos, int max,
                    const char *s, int width, int zero_pad) {
    if (!s) s = "(null)";
    int len = (int)k_strlen(s);
    // Padding izquierdo
    for (int i = len; i < width; i++)
        pos = _put(buf, pos, max, zero_pad ? '0' : ' ');
    // String
    while (*s) pos = _put(buf, pos, max, *s++);
    return pos;
}

// k_vsprintf — núcleo del formatter con va_list
int k_vsprintf(char *buf, const char *fmt, va_list ap) {
    int pos = 0;
    char tmp[32];

    while (*fmt) {
        if (*fmt != '%') { pos = _put(buf, pos, -1, *fmt++); continue; }
        fmt++;  // consumir '%'

        // Flags: zero-padding y width
        int zero_pad = 0, width = 0;
        if (*fmt == '0') { zero_pad = 1; fmt++; }
        while (*fmt >= '1' && *fmt <= '9') { width = width * 10 + (*fmt++ - '0'); }
        // Prefijo 'l' (long) — lo ignoramos en 32-bit, int == long
        if (*fmt == 'l') fmt++;

        char spec = *fmt++;
        switch (spec) {
        case 's': {
            const char *s = va_arg(ap, const char *);
            pos = _put_str(buf, pos, -1, s, width, zero_pad);
            break;
        }
        case 'd': case 'i': {
            int v = va_arg(ap, int);
            k_itoa(v, tmp, 10);
            pos = _put_str(buf, pos, -1, tmp, width, zero_pad);
            break;
        }
        case 'u': {
            unsigned v = va_arg(ap, unsigned int);
            k_itoa((int)v, tmp, 10);
            pos = _put_str(buf, pos, -1, tmp, width, zero_pad);
            break;
        }
        case 'x': {
            unsigned v = va_arg(ap, unsigned int);
            k_itoa((int)v, tmp, 16);
            // lowercase
            for (char *p = tmp; *p; p++)
                if (*p >= 'A' && *p <= 'F') *p += 32;
            pos = _put_str(buf, pos, -1, tmp, width, zero_pad);
            break;
        }
        case 'X': {
            unsigned v = va_arg(ap, unsigned int);
            k_itoa((int)v, tmp, 16);
            // uppercase — ya lo es por defecto en k_itoa
            pos = _put_str(buf, pos, -1, tmp, width, zero_pad);
            break;
        }
        case 'p': {
            // Puntero — 0x + 8 dígitos hex
            unsigned v = va_arg(ap, unsigned int);
            pos = _put(buf, pos, -1, '0');
            pos = _put(buf, pos, -1, 'x');
            k_itoa((int)v, tmp, 16);
            int len = (int)k_strlen(tmp);
            for (int i = len; i < 8; i++) pos = _put(buf, pos, -1, '0');
            for (char *p = tmp; *p; p++) pos = _put(buf, pos, -1, *p);
            break;
        }
        case 'c': {
            char c = (char)va_arg(ap, int);
            pos = _put(buf, pos, -1, c);
            break;
        }
        case '%': pos = _put(buf, pos, -1, '%'); break;
        default:  pos = _put(buf, pos, -1, '?'); break;
        }
    }

    if (buf) buf[pos] = '\0';
    return pos;
}

int k_sprintf(char *buf, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int r = k_vsprintf(buf, fmt, ap);
    va_end(ap);
    return r;
}

int k_snprintf(char *buf, size_t n, const char *fmt, ...) {
    // Para snprintf: usamos k_vsprintf en buf temporal, luego truncamos
    // (simple pero correcto para nuestro uso)
    char tmp_buf[4096];
    va_list ap;
    va_start(ap, fmt);
    int r = k_vsprintf(tmp_buf, fmt, ap);
    va_end(ap);
    if (n > 0) {
        size_t to_copy = (size_t)r < n - 1 ? (size_t)r : n - 1;
        k_memcpy(buf, tmp_buf, to_copy);
        buf[to_copy] = '\0';
    }
    return r;
}

