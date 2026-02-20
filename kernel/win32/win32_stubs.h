#ifndef WIN32_STUBS_H
#define WIN32_STUBS_H

#include <stdint.h>

/*
 * ╔══════════════════════════════════════════════════════════════╗
 * ║  XP7 OS — Win32 API Stubs / Shimming Layer                 ║
 * ║                                                            ║
 * ║  Similar a Wine pero integrado en el kernel.               ║
 * ║  Cuando un .exe pide una función de Windows, primero       ║
 * ║  intentamos resolverla desde el DLL real (System32/WoW64)  ║
 * ║  y si no está, usamos estos stubs.                         ║
 * ╚══════════════════════════════════════════════════════════════╝
 */

// Windows Types básicos
typedef uint32_t  DWORD;
typedef uint16_t  WORD;
typedef uint8_t   BYTE;
typedef uint32_t  BOOL;
typedef void     *HANDLE;
typedef void     *LPVOID;
typedef uint32_t  UINT;
typedef uint32_t  HWND;
typedef uint32_t  HINSTANCE;
typedef uint32_t  HMODULE;
typedef int32_t   INT;
typedef int32_t   LONG;
typedef uint32_t  ULONG;
typedef char     *LPSTR;
typedef const char *LPCSTR;
typedef uint32_t  SIZE_T;
typedef uint64_t  ULONGLONG;

#define TRUE  1
#define FALSE 0
#define NULL  0
#define INVALID_HANDLE_VALUE  ((HANDLE)(uint32_t)-1)
#define MAX_PATH 260

// ─── Lookup de stubs por nombre ──────────────────────────────
uint32_t win32_stub_find(const char *dll_name, const char *func_name);
uint32_t win32_stub_ordinal(const char *dll_name, uint16_t ordinal);
uint32_t win32_stub_missing(const char *dll_name, const char *func_name);

// ─── Init ─────────────────────────────────────────────────────
void win32_stubs_init(void);

// ─── kernel32.dll stubs ───────────────────────────────────────
DWORD  __attribute__((stdcall)) stub_GetLastError(void);
void   __attribute__((stdcall)) stub_SetLastError(DWORD err);
void   __attribute__((stdcall)) stub_ExitProcess(UINT code);
HANDLE __attribute__((stdcall)) stub_GetStdHandle(DWORD nStdHandle);
BOOL   __attribute__((stdcall)) stub_WriteConsoleA(HANDLE h, LPCSTR buf, DWORD len, DWORD *written, void *reserved);
BOOL   __attribute__((stdcall)) stub_ReadConsoleA(HANDLE h, LPSTR buf, DWORD len, DWORD *read, void *reserved);
BOOL   __attribute__((stdcall)) stub_WriteFile(HANDLE h, LPCSTR buf, DWORD len, DWORD *written, void *overlap);
BOOL   __attribute__((stdcall)) stub_ReadFile(HANDLE h, LPSTR buf, DWORD len, DWORD *read, void *overlap);
HANDLE __attribute__((stdcall)) stub_CreateFileA(LPCSTR fn, DWORD acc, DWORD share, void *sec, DWORD disp, DWORD flags, HANDLE tmpl);
BOOL   __attribute__((stdcall)) stub_CloseHandle(HANDLE h);
LPVOID __attribute__((stdcall)) stub_VirtualAlloc(LPVOID addr, SIZE_T size, DWORD type, DWORD prot);
BOOL   __attribute__((stdcall)) stub_VirtualFree(LPVOID addr, SIZE_T size, DWORD type);
LPVOID __attribute__((stdcall)) stub_HeapAlloc(HANDLE heap, DWORD flags, SIZE_T size);
BOOL   __attribute__((stdcall)) stub_HeapFree(HANDLE heap, DWORD flags, LPVOID ptr);
HANDLE __attribute__((stdcall)) stub_GetProcessHeap(void);
DWORD  __attribute__((stdcall)) stub_GetCurrentThreadId(void);
DWORD  __attribute__((stdcall)) stub_GetCurrentProcessId(void);
void   __attribute__((stdcall)) stub_Sleep(DWORD ms);
BOOL   __attribute__((stdcall)) stub_GetConsoleMode(HANDLE h, DWORD *mode);
BOOL   __attribute__((stdcall)) stub_SetConsoleMode(HANDLE h, DWORD mode);
HMODULE __attribute__((stdcall)) stub_GetModuleHandleA(LPCSTR name);
DWORD  __attribute__((stdcall)) stub_GetModuleFileNameA(HMODULE mod, LPSTR buf, DWORD size);
LPVOID __attribute__((stdcall)) stub_GetProcAddress(HMODULE mod, LPCSTR func);
HMODULE __attribute__((stdcall)) stub_LoadLibraryA(LPCSTR name);
BOOL   __attribute__((stdcall)) stub_FreeLibrary(HMODULE mod);
void   __attribute__((stdcall)) stub_OutputDebugStringA(LPCSTR msg);
BOOL   __attribute__((stdcall)) stub_IsDebuggerPresent(void);
DWORD  __attribute__((stdcall)) stub_FormatMessageA(DWORD flags, LPVOID src, DWORD msgid, DWORD lang, LPSTR buf, DWORD size, void *args);
int    __attribute__((stdcall)) stub_MultiByteToWideChar(UINT cp, DWORD flags, LPCSTR mb, int mblen, void *wc, int wclen);

// ─── ntdll.dll stubs ──────────────────────────────────────────
LONG   __attribute__((stdcall)) stub_RtlGetLastWin32Error(void);
void   __attribute__((stdcall)) stub_RtlSetLastWin32Error(DWORD err);
LONG   __attribute__((stdcall)) stub_NtTerminateProcess(HANDLE proc, LONG status);
LONG   __attribute__((stdcall)) stub_NtWriteFile(HANDLE file, HANDLE evt, void*a, void*c, void*io, void*buf, ULONG len, void*off, void*key);

// ─── msvcrt.dll / ucrtbase stubs ─────────────────────────────
int    stub_printf(const char *fmt, ...);
int    stub_sprintf(char *buf, const char *fmt, ...);
int    stub_puts(const char *s);
void  *stub_malloc(SIZE_T size);
void  *stub_calloc(SIZE_T n, SIZE_T sz);
void  *stub_realloc(void *ptr, SIZE_T size);
void   stub_free(void *ptr);
void   stub_exit(int code);
int    stub_strcmp(const char *a, const char *b);
size_t stub_strlen(const char *s);
void  *stub_memcpy(void *d, const void *s, SIZE_T n);
void  *stub_memset(void *d, int v, SIZE_T n);

// ─── user32.dll stubs ─────────────────────────────────────────
int    __attribute__((stdcall)) stub_MessageBoxA(HWND hwnd, LPCSTR text, LPCSTR cap, UINT type);
HWND   __attribute__((stdcall)) stub_CreateWindowExA(DWORD exstyle, LPCSTR cls, LPCSTR name, DWORD style, int x, int y, int w, int h, HWND parent, HANDLE menu, HINSTANCE inst, LPVOID param);
BOOL   __attribute__((stdcall)) stub_ShowWindow(HWND hwnd, int cmd);
BOOL   __attribute__((stdcall)) stub_UpdateWindow(HWND hwnd);
BOOL   __attribute__((stdcall)) stub_GetMessageA(void *msg, HWND hwnd, UINT min, UINT max);
BOOL   __attribute__((stdcall)) stub_TranslateMessage(void *msg);
LONG   __attribute__((stdcall)) stub_DispatchMessageA(void *msg);
LONG   __attribute__((stdcall)) stub_DefWindowProcA(HWND hwnd, UINT msg, uint32_t wp, uint32_t lp);

#endif
