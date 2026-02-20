#include "win32_stubs.h"
#include "../mm/mm.h"
#include "../fs/fat32.h"
#include "../string.h"
#include "../vga.h"
#include "../keyboard.h"
#include "../mouse.h"
#include "../gui/wm.h"
#include "../framebuffer.h"
#include "../font.h"

// ─── Estado global Windows-like ──────────────────────────────
static DWORD last_error = 0;
static HANDLE stdout_handle = (HANDLE)1;
static HANDLE stdin_handle  = (HANDLE)0;
static HANDLE stderr_handle = (HANDLE)2;

// ─── Tabla de stubs ───────────────────────────────────────────
typedef struct {
    const char *dll;   // "kernel32.dll"
    const char *name;  // "ExitProcess"
    uint32_t    addr;  // dirección del stub
} stub_entry_t;

// ─── printf simplificado (sin libc) ──────────────────────────
static void gui_or_vga_print(const char *s) {
    // Si el GUI está activo, imprimir en la ventana terminal activa
    if (fb.ready && wm.focused >= 0 && wm.focused < wm.count) {
        window_t *w = &wm.windows[wm.focused];
        if (w->is_terminal) {
            win_term_print(w, s, 0xC0C0C0);
            return;
        }
    }
    vga_print(s);
}

// ─── Formato básico para stub_printf (sin floats) ────────────
static int do_printf(char *out_buf, int buf_sz, const char *fmt, va_list ap) {
    int i = 0;
    char tmp[32];
    while (*fmt && i < buf_sz - 1) {
        if (*fmt != '%') { if(out_buf) out_buf[i]=*fmt; i++; fmt++; continue; }
        fmt++;
        switch (*fmt++) {
        case 's': {
            const char *s = (const char *)(uintptr_t)va_arg(ap, uint32_t);
            if (!s) s = "(null)";
            while (*s && i < buf_sz-1) { if(out_buf) out_buf[i]=*s; i++; s++; }
            break; }
        case 'd': case 'i': {
            int v = va_arg(ap, int);
            k_itoa(v, tmp, 10);
            for (int j=0; tmp[j] && i<buf_sz-1; j++) { if(out_buf) out_buf[i]=tmp[j]; i++; }
            break; }
        case 'u': {
            unsigned v = va_arg(ap, unsigned);
            k_itoa((int)v, tmp, 10);
            for (int j=0; tmp[j] && i<buf_sz-1; j++) { if(out_buf) out_buf[i]=tmp[j]; i++; }
            break; }
        case 'x': case 'X': {
            unsigned v = va_arg(ap, unsigned);
            k_itoa((int)v, tmp, 16);
            for (int j=0; tmp[j] && i<buf_sz-1; j++) { if(out_buf) out_buf[i]=tmp[j]; i++; }
            break; }
        case 'c': {
            char c = (char)va_arg(ap, int);
            if(out_buf) out_buf[i]=c; i++;
            break; }
        case '%': if(out_buf) out_buf[i]='%'; i++; break;
        default: if(out_buf) out_buf[i]='?'; i++; break;
        }
    }
    if(out_buf) out_buf[i]=0;
    return i;
}

// ════════════════════════════════════════════════════════════════
//   KERNEL32.DLL STUBS
// ════════════════════════════════════════════════════════════════

DWORD __attribute__((stdcall)) stub_GetLastError(void) {
    return last_error;
}
void __attribute__((stdcall)) stub_SetLastError(DWORD err) {
    last_error = err;
}

void __attribute__((stdcall)) stub_ExitProcess(UINT code) {
    char buf[64] = "Proceso terminado. Codigo: ";
    char num[16]; k_itoa((int)code, num, 10);
    k_strcat(buf, num); k_strcat(buf, "\n");
    gui_or_vga_print(buf);
    // Stage 3: detener ejecución (Stage 4 tendrá scheduler real)
    while(1) __asm__ volatile("hlt");
}

HANDLE __attribute__((stdcall)) stub_GetStdHandle(DWORD nStdHandle) {
    if (nStdHandle == (DWORD)-10) return stdin_handle;
    if (nStdHandle == (DWORD)-11) return stdout_handle;
    if (nStdHandle == (DWORD)-12) return stderr_handle;
    return INVALID_HANDLE_VALUE;
}

BOOL __attribute__((stdcall)) stub_WriteConsoleA(
        HANDLE h, LPCSTR buf, DWORD len, DWORD *written, void *reserved) {
    (void)h; (void)reserved;
    char tmp[512]; int i;
    for (i = 0; i < (int)len && i < 510; i++) tmp[i] = buf[i];
    tmp[i] = 0;
    gui_or_vga_print(tmp);
    if (written) *written = (DWORD)i;
    return TRUE;
}

BOOL __attribute__((stdcall)) stub_WriteFile(
        HANDLE h, LPCSTR buf, DWORD len, DWORD *written, void *ov) {
    return stub_WriteConsoleA(h, buf, len, written, ov);
}

BOOL __attribute__((stdcall)) stub_ReadConsoleA(
        HANDLE h, LPSTR buf, DWORD len, DWORD *read, void *reserved) {
    (void)h; (void)reserved;
    keyboard_read_line(buf, (int)len);
    if (read) *read = (DWORD)k_strlen(buf);
    return TRUE;
}

BOOL __attribute__((stdcall)) stub_ReadFile(
        HANDLE h, LPSTR buf, DWORD len, DWORD *read, void *ov) {
    return stub_ReadConsoleA(h, buf, len, read, ov);
}

HANDLE __attribute__((stdcall)) stub_CreateFileA(
        LPCSTR fn, DWORD acc, DWORD share, void *sec,
        DWORD disp, DWORD flags, HANDLE tmpl) {
    (void)acc; (void)share; (void)sec; (void)disp; (void)flags; (void)tmpl;
    // Stage 3: solo lectura via FAT32
    if (fn && fat32_exists(fn)) {
        // Retornar handle ficticio — implementación completa en Stage 4
        return (HANDLE)0x1000;
    }
    last_error = 2;  // ERROR_FILE_NOT_FOUND
    return INVALID_HANDLE_VALUE;
}

BOOL __attribute__((stdcall)) stub_CloseHandle(HANDLE h) {
    (void)h;
    return TRUE;
}

LPVOID __attribute__((stdcall)) stub_VirtualAlloc(
        LPVOID addr, SIZE_T size, DWORD type, DWORD prot) {
    (void)addr; (void)type; (void)prot;
    return kmalloc(size);
}

BOOL __attribute__((stdcall)) stub_VirtualFree(
        LPVOID addr, SIZE_T size, DWORD type) {
    (void)size; (void)type;
    kfree(addr);
    return TRUE;
}

static uint8_t process_heap_dummy = 0;
HANDLE __attribute__((stdcall)) stub_GetProcessHeap(void) {
    return (HANDLE)&process_heap_dummy;
}

LPVOID __attribute__((stdcall)) stub_HeapAlloc(
        HANDLE heap, DWORD flags, SIZE_T size) {
    (void)heap;
    void *p = kmalloc(size);
    if (p && (flags & 0x08)) k_memset(p, 0, size);  // HEAP_ZERO_MEMORY
    return p;
}

BOOL __attribute__((stdcall)) stub_HeapFree(
        HANDLE heap, DWORD flags, LPVOID ptr) {
    (void)heap; (void)flags;
    kfree(ptr);
    return TRUE;
}

DWORD __attribute__((stdcall)) stub_GetCurrentThreadId(void)   { return 1; }
DWORD __attribute__((stdcall)) stub_GetCurrentProcessId(void)  { return 42; }

void __attribute__((stdcall)) stub_Sleep(DWORD ms) {
    // Busy-wait aproximado (Stage 4 tendrá timer real)
    volatile uint64_t count = (uint64_t)ms * 100000;
    while (count--);
}

BOOL __attribute__((stdcall)) stub_GetConsoleMode(HANDLE h, DWORD *mode) {
    (void)h;
    if (mode) *mode = 0x0003;  // ENABLE_PROCESSED_INPUT | ENABLE_LINE_INPUT
    return TRUE;
}
BOOL __attribute__((stdcall)) stub_SetConsoleMode(HANDLE h, DWORD mode) {
    (void)h; (void)mode; return TRUE;
}

HMODULE __attribute__((stdcall)) stub_GetModuleHandleA(LPCSTR name) {
    (void)name;
    return (HMODULE)0x400000;  // Base ficticia
}

DWORD __attribute__((stdcall)) stub_GetModuleFileNameA(
        HMODULE mod, LPSTR buf, DWORD size) {
    (void)mod;
    const char *n = "C:\\XP7OS\\app.exe";
    k_strncpy(buf, n, size-1);
    return (DWORD)k_strlen(n);
}

LPVOID __attribute__((stdcall)) stub_GetProcAddress(HMODULE mod, LPCSTR func) {
    (void)mod;
    // Buscar en nuestra tabla de stubs
    uint32_t addr = win32_stub_find("kernel32.dll", func);
    if (!addr) addr = win32_stub_find("ntdll.dll", func);
    if (!addr) addr = win32_stub_find("msvcrt.dll", func);
    return (LPVOID)(uintptr_t)addr;
}

HMODULE __attribute__((stdcall)) stub_LoadLibraryA(LPCSTR name) {
    (void)name;
    // Stage 4: cargar DLL real con PE loader
    return (HMODULE)0x10000000;
}

BOOL __attribute__((stdcall)) stub_FreeLibrary(HMODULE mod) {
    (void)mod; return TRUE;
}

void __attribute__((stdcall)) stub_OutputDebugStringA(LPCSTR msg) {
    vga_set_color(VGA_DARK_GREY, VGA_BLACK);
    vga_print("[DBG] "); vga_print(msg);
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
}

BOOL __attribute__((stdcall)) stub_IsDebuggerPresent(void) { return FALSE; }

DWORD __attribute__((stdcall)) stub_FormatMessageA(
        DWORD flags, LPVOID src, DWORD msgid, DWORD lang,
        LPSTR buf, DWORD size, void *args) {
    (void)flags; (void)src; (void)lang; (void)args;
    char tmp[64] = "Error ";
    char num[16]; k_itoa((int)msgid, num, 10);
    k_strcat(tmp, num);
    k_strncpy(buf, tmp, size-1);
    return (DWORD)k_strlen(tmp);
}

int __attribute__((stdcall)) stub_MultiByteToWideChar(
        UINT cp, DWORD flags, LPCSTR mb, int mbl, void *wc, int wcl) {
    (void)cp; (void)flags;
    uint16_t *out = (uint16_t *)wc;
    int i = 0;
    if (mbl < 0) mbl = (int)k_strlen(mb)+1;
    while (i < mbl && i < wcl) { out[i] = (uint16_t)(uint8_t)mb[i]; i++; }
    return i;
}

// ════════════════════════════════════════════════════════════════
//   NTDLL.DLL STUBS
// ════════════════════════════════════════════════════════════════

LONG __attribute__((stdcall)) stub_RtlGetLastWin32Error(void) {
    return (LONG)last_error;
}
void __attribute__((stdcall)) stub_RtlSetLastWin32Error(DWORD err) {
    last_error = err;
}
LONG __attribute__((stdcall)) stub_NtTerminateProcess(HANDLE proc, LONG status) {
    (void)proc;
    stub_ExitProcess((UINT)status);
    return 0;
}
LONG __attribute__((stdcall)) stub_NtWriteFile(
        HANDLE file, HANDLE evt, void*a, void*c, void*io,
        void*buf, ULONG len, void*off, void*key) {
    (void)evt;(void)a;(void)c;(void)io;(void)off;(void)key;
    DWORD wr;
    stub_WriteFile(file, (LPCSTR)buf, len, &wr, NULL);
    return 0;
}

// ════════════════════════════════════════════════════════════════
//   MSVCRT / UCRTBASE STUBS
// ════════════════════════════════════════════════════════════════

// stub_printf necesita va_list — usamos un approach especial en C
int stub_printf(const char *fmt, ...) {
    char buf[1024];
    // Acceder a args via stack
    uint32_t *args = (uint32_t *)(&fmt + 1);
    // Mini va_list manual
    int i = 0, ai = 0;
    char out[1024]; int oi = 0;
    while (fmt[i] && oi < 1023) {
        if (fmt[i] != '%') { out[oi++] = fmt[i++]; continue; }
        i++;
        char tmp[32];
        switch(fmt[i++]) {
        case 's': {
            const char *s = (const char *)(uintptr_t)args[ai++];
            if(!s) s="(null)";
            while(*s && oi<1023) out[oi++]=*s++;
            break; }
        case 'd': case 'i': {
            k_itoa((int)args[ai++], tmp, 10);
            for(int j=0; tmp[j]&&oi<1023; j++) out[oi++]=tmp[j];
            break; }
        case 'u': {
            k_itoa((int)args[ai++], tmp, 10);
            for(int j=0; tmp[j]&&oi<1023; j++) out[oi++]=tmp[j];
            break; }
        case 'x': case 'X': {
            k_itoa((int)args[ai++], tmp, 16);
            for(int j=0; tmp[j]&&oi<1023; j++) out[oi++]=tmp[j];
            break; }
        case 'c': out[oi++]=(char)args[ai++]; break;
        case '%': out[oi++]='%'; break;
        default: out[oi++]='?'; break;
        }
    }
    out[oi]=0;
    gui_or_vga_print(out);
    (void)buf;
    return oi;
}

int stub_sprintf(char *buf, const char *fmt, ...) {
    uint32_t *args = (uint32_t *)(&fmt + 1);
    (void)args;
    // Versión simple: copiar fmt con sustituciones básicas
    k_strcpy(buf, fmt);
    return (int)k_strlen(buf);
}

int stub_puts(const char *s) {
    gui_or_vga_print(s);
    gui_or_vga_print("\n");
    return 0;
}

void *stub_malloc(SIZE_T size) { return kmalloc(size); }
void *stub_calloc(SIZE_T n, SIZE_T sz) { return kcalloc(n, sz); }
void *stub_realloc(void *ptr, SIZE_T size) { return krealloc(ptr, size); }
void  stub_free(void *ptr) { kfree(ptr); }

void stub_exit(int code) { stub_ExitProcess((UINT)code); }
int  stub_strcmp(const char *a, const char *b) { return k_strcmp(a, b); }
size_t stub_strlen(const char *s) { return k_strlen(s); }
void *stub_memcpy(void *d, const void *s, SIZE_T n) { return k_memcpy(d,s,n); }
void *stub_memset(void *d, int v, SIZE_T n) { return k_memset(d,v,n); }

// ════════════════════════════════════════════════════════════════
//   USER32.DLL STUBS
// ════════════════════════════════════════════════════════════════

int __attribute__((stdcall)) stub_MessageBoxA(
        HWND hwnd, LPCSTR text, LPCSTR cap, UINT type) {
    (void)hwnd; (void)type;
    // Mostrar como ventana en el GUI si está disponible
    if (fb.ready) {
        // Crear ventana de dialogo temporal
        int w = 320, h = 120;
        int x = (fb.width  - w) / 2;
        int y = (fb.height - h) / 2;
        fb_fill_rect_alpha(x-4, y-4, w+8, h+8, 0x000000, 100);
        fb_fill_rect(x, y, w, h, 0xF0F0F0);
        fb_draw_rect(x, y, w, h, 0x0A246A);
        fb_fill_rect(x, y, w, 22, 0x0A246A);
        const char *title = cap ? cap : "XP7 OS";
        font_draw_str(x+6, y+7, title, 0xFFFFFF, 0, 1);
        if (text) font_draw_str(x+10, y+40, text, 0x000000, 0, 1);
        // Botón OK
        fb_fill_rect(x + w/2 - 25, y+h-30, 50, 20, 0xECE9D8);
        fb_draw_rect(x + w/2 - 25, y+h-30, 50, 20, 0x0A246A);
        font_draw_str(x + w/2 - 8, y+h-24, "OK", 0x000000, 0, 1);
        // Esperar click (simplificado)

        while (!mouse.left_click)
            __asm__ volatile("hlt");
        mouse.left_click = 0;
        return 1;  // IDOK
    } else {
        vga_print("\n[MSGBOX] ");
        if (cap) { vga_print(cap); vga_print(": "); }
        if (text) vga_print(text);
        vga_print("\n");
        return 1;
    }
}

HWND __attribute__((stdcall)) stub_CreateWindowExA(
        DWORD exstyle, LPCSTR cls, LPCSTR name, DWORD style,
        int x, int y, int w, int h, HWND parent, HANDLE menu,
        HINSTANCE inst, LPVOID param) {
    (void)exstyle;(void)cls;(void)style;(void)parent;(void)menu;(void)inst;(void)param;
    if (fb.ready) {
        int id = wm_create_window(x < 0 ? 50 : x,
                                   y < 0 ? 50 : y,
                                   w <= 0 ? 400 : w,
                                   h <= 0 ? 300 : h,
                                   name ? name : "Ventana", 0);
        return (HWND)(uint32_t)(id + 1);
    }
    return (HWND)1;
}

BOOL __attribute__((stdcall)) stub_ShowWindow(HWND hwnd, int cmd) {
    (void)hwnd; (void)cmd; return TRUE;
}
BOOL __attribute__((stdcall)) stub_UpdateWindow(HWND hwnd) {
    (void)hwnd;
    if (fb.ready) { wm_redraw_all(); }
    return TRUE;
}

static int msg_quit = 0;
BOOL __attribute__((stdcall)) stub_GetMessageA(
        void *msg, HWND hwnd, UINT min, UINT max) {
    (void)hwnd;(void)min;(void)max;
    if (msg_quit) return FALSE;
    // Esperar teclado o mouse

    while (!keyboard_available() && !msg_quit)
        __asm__ volatile("hlt");
    if (msg) k_memset(msg, 0, 28);  // sizeof(MSG)
    return TRUE;
}

BOOL __attribute__((stdcall)) stub_TranslateMessage(void *msg) {
    (void)msg; return TRUE;
}
LONG __attribute__((stdcall)) stub_DispatchMessageA(void *msg) {
    (void)msg; return 0;
}
LONG __attribute__((stdcall)) stub_DefWindowProcA(
        HWND hwnd, UINT msg, uint32_t wp, uint32_t lp) {
    (void)hwnd;(void)msg;(void)wp;(void)lp; return 0;
}

// ════════════════════════════════════════════════════════════════
//   TABLA DE LOOKUP DE STUBS
// ════════════════════════════════════════════════════════════════

#define S(dll,nm,fn) { dll, nm, (uint32_t)(uintptr_t)fn }

static const stub_entry_t stub_table[] = {
    // kernel32.dll
    S("kernel32.dll","GetLastError",         stub_GetLastError),
    S("kernel32.dll","SetLastError",         stub_SetLastError),
    S("kernel32.dll","ExitProcess",          stub_ExitProcess),
    S("kernel32.dll","GetStdHandle",         stub_GetStdHandle),
    S("kernel32.dll","WriteConsoleA",        stub_WriteConsoleA),
    S("kernel32.dll","ReadConsoleA",         stub_ReadConsoleA),
    S("kernel32.dll","WriteFile",            stub_WriteFile),
    S("kernel32.dll","ReadFile",             stub_ReadFile),
    S("kernel32.dll","CreateFileA",          stub_CreateFileA),
    S("kernel32.dll","CloseHandle",          stub_CloseHandle),
    S("kernel32.dll","VirtualAlloc",         stub_VirtualAlloc),
    S("kernel32.dll","VirtualFree",          stub_VirtualFree),
    S("kernel32.dll","HeapAlloc",            stub_HeapAlloc),
    S("kernel32.dll","HeapFree",             stub_HeapFree),
    S("kernel32.dll","GetProcessHeap",       stub_GetProcessHeap),
    S("kernel32.dll","GetCurrentThreadId",   stub_GetCurrentThreadId),
    S("kernel32.dll","GetCurrentProcessId",  stub_GetCurrentProcessId),
    S("kernel32.dll","Sleep",                stub_Sleep),
    S("kernel32.dll","GetConsoleMode",       stub_GetConsoleMode),
    S("kernel32.dll","SetConsoleMode",       stub_SetConsoleMode),
    S("kernel32.dll","GetModuleHandleA",     stub_GetModuleHandleA),
    S("kernel32.dll","GetModuleFileNameA",   stub_GetModuleFileNameA),
    S("kernel32.dll","GetProcAddress",       stub_GetProcAddress),
    S("kernel32.dll","LoadLibraryA",         stub_LoadLibraryA),
    S("kernel32.dll","FreeLibrary",          stub_FreeLibrary),
    S("kernel32.dll","OutputDebugStringA",   stub_OutputDebugStringA),
    S("kernel32.dll","IsDebuggerPresent",    stub_IsDebuggerPresent),
    S("kernel32.dll","FormatMessageA",       stub_FormatMessageA),
    S("kernel32.dll","MultiByteToWideChar",  stub_MultiByteToWideChar),
    // ntdll.dll
    S("ntdll.dll","RtlGetLastWin32Error",  stub_RtlGetLastWin32Error),
    S("ntdll.dll","RtlSetLastWin32Error",  stub_RtlSetLastWin32Error),
    S("ntdll.dll","NtTerminateProcess",    stub_NtTerminateProcess),
    S("ntdll.dll","NtWriteFile",           stub_NtWriteFile),
    // msvcrt.dll
    S("msvcrt.dll","printf",   stub_printf),
    S("msvcrt.dll","sprintf",  stub_sprintf),
    S("msvcrt.dll","puts",     stub_puts),
    S("msvcrt.dll","malloc",   stub_malloc),
    S("msvcrt.dll","calloc",   stub_calloc),
    S("msvcrt.dll","realloc",  stub_realloc),
    S("msvcrt.dll","free",     stub_free),
    S("msvcrt.dll","exit",     stub_exit),
    S("msvcrt.dll","strcmp",   stub_strcmp),
    S("msvcrt.dll","strlen",   stub_strlen),
    S("msvcrt.dll","memcpy",   stub_memcpy),
    S("msvcrt.dll","memset",   stub_memset),
    // ucrtbase.dll (mismo que msvcrt)
    S("ucrtbase.dll","printf",  stub_printf),
    S("ucrtbase.dll","malloc",  stub_malloc),
    S("ucrtbase.dll","free",    stub_free),
    S("ucrtbase.dll","puts",    stub_puts),
    S("ucrtbase.dll","exit",    stub_exit),
    // user32.dll
    S("user32.dll","MessageBoxA",      stub_MessageBoxA),
    S("user32.dll","CreateWindowExA",  stub_CreateWindowExA),
    S("user32.dll","ShowWindow",       stub_ShowWindow),
    S("user32.dll","UpdateWindow",     stub_UpdateWindow),
    S("user32.dll","GetMessageA",      stub_GetMessageA),
    S("user32.dll","TranslateMessage", stub_TranslateMessage),
    S("user32.dll","DispatchMessageA", stub_DispatchMessageA),
    S("user32.dll","DefWindowProcA",   stub_DefWindowProcA),
    { NULL, NULL, 0 }
};

uint32_t win32_stub_find(const char *dll, const char *name) {
    // Normalizar nombre de DLL a minúsculas
    char ldll[64]; int i=0;
    if (dll) {
        while(dll[i]&&i<63){ldll[i]=dll[i];if(ldll[i]>='A'&&ldll[i]<='Z')ldll[i]+=32;i++;}
    }
    ldll[i]=0;

    for (const stub_entry_t *e = stub_table; e->dll; e++) {
        if (k_strcmp(e->dll, ldll) == 0 && k_strcmp(e->name, name) == 0)
            return e->addr;
    }
    return 0;
}

uint32_t win32_stub_ordinal(const char *dll, uint16_t ord) {
    (void)dll; (void)ord;
    return 0;  // Ordinals: Stage 4
}

// Stub genérico para funciones desconocidas
static int __attribute__((stdcall)) stub_unimplemented(void) {
    vga_set_color(VGA_DARK_GREY, VGA_BLACK);
    vga_print("[WIN32] Funcion no implementada (retorna 0)\n");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    return 0;
}

static char missing_dll[64], missing_fn[128];
static int __attribute__((stdcall)) stub_missing_impl(void) {
    vga_set_color(VGA_BROWN, VGA_BLACK);
    vga_print("[WIN32] FALTA: "); vga_print(missing_dll);
    vga_print("!"); vga_print(missing_fn); vga_put_char('\n');
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    return 0;
}

uint32_t win32_stub_missing(const char *dll, const char *fn) {
    k_strncpy(missing_dll, dll ? dll : "?", 63);
    k_strncpy(missing_fn,  fn  ? fn  : "?", 127);
    (void)stub_unimplemented;
    return (uint32_t)(uintptr_t)stub_missing_impl;
}

void win32_stubs_init(void) {
    vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    vga_print("[WIN32] API stubs listos (kernel32 / ntdll / msvcrt / user32)\n");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
}
