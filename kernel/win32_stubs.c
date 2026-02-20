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
typedef struct { const char *dll; const char *name; uint32_t addr; } stub_entry_t;

// ─── Mini printf para VGA/GUI
static void gui_or_vga_print(const char *s) {
    if(fb.ready && wm.focused>=0 && wm.focused<wm.count){
        window_t *w=&wm.windows[wm.focused];
        if(w->is_terminal){ win_term_print(w,s,0xC0C0C0); return; }
    }
    vga_print(s);
}

// ═════════ STUBS KERNEL32 ════════════════════════════════════
DWORD __attribute__((stdcall)) stub_GetLastError(void){ return last_error; }
void  __attribute__((stdcall)) stub_SetLastError(DWORD err){ last_error=err; }

void __attribute__((stdcall)) stub_ExitProcess(UINT code){
    char buf[64]="Proceso terminado. Codigo: ";
    char num[16]; k_itoa((int)code,num,10);
    k_strcat(buf,num); k_strcat(buf,"\n");
    gui_or_vga_print(buf);
    while(1) __asm__ volatile("hlt");
}

HANDLE __attribute__((stdcall)) stub_GetStdHandle(DWORD nStdHandle){
    if(nStdHandle==(DWORD)-10) return stdin_handle;
    if(nStdHandle==(DWORD)-11) return stdout_handle;
    if(nStdHandle==(DWORD)-12) return stderr_handle;
    return INVALID_HANDLE_VALUE;
}

BOOL __attribute__((stdcall)) stub_WriteConsoleA(HANDLE h,LPCSTR buf,DWORD len,DWORD *written,void *reserved){
    (void)h;(void)reserved; char tmp[1024]; int i=0;
    for(;i<(int)len && i<1023;i++) tmp[i]=buf[i]; tmp[i]=0;
    gui_or_vga_print(tmp);
    if(written) *written=(DWORD)i; return TRUE;
}

BOOL __attribute__((stdcall)) stub_WriteFile(HANDLE h,LPCSTR buf,DWORD len,DWORD *written,void *ov){
    return stub_WriteConsoleA(h,buf,len,written,ov);
}

BOOL __attribute__((stdcall)) stub_ReadConsoleA(HANDLE h,LPSTR buf,DWORD len,DWORD *read,void *reserved){
    (void)h;(void)reserved; keyboard_read_line(buf,(int)len);
    if(read) *read=(DWORD)k_strlen(buf); return TRUE;
}

BOOL __attribute__((stdcall)) stub_ReadFile(HANDLE h,LPSTR buf,DWORD len,DWORD *read,void *ov){
    return stub_ReadConsoleA(h,buf,len,read,ov);
}

// Stage 3+ CreateFile/ReadFile
HANDLE __attribute__((stdcall)) stub_CreateFileA(LPCSTR fn,DWORD acc,DWORD share,void *sec,DWORD disp,DWORD flags,HANDLE tmpl){
    (void)acc;(void)share;(void)sec;(void)disp;(void)flags;(void)tmpl;
    if(fn && fat32_exists(fn)) return (HANDLE)0x1000;
    last_error=2; return INVALID_HANDLE_VALUE;
}

BOOL __attribute__((stdcall)) stub_CloseHandle(HANDLE h){ (void)h; return TRUE; }

LPVOID __attribute__((stdcall)) stub_VirtualAlloc(LPVOID addr,SIZE_T size,DWORD type,DWORD prot){ (void)addr;(void)type;(void)prot; return kmalloc(size); }
BOOL   __attribute__((stdcall)) stub_VirtualFree(LPVOID addr,SIZE_T size,DWORD type){ (void)size;(void)type; kfree(addr); return TRUE; }

static uint8_t process_heap_dummy=0;
HANDLE __attribute__((stdcall)) stub_GetProcessHeap(void){ return (HANDLE)&process_heap_dummy; }

LPVOID __attribute__((stdcall)) stub_HeapAlloc(HANDLE heap,DWORD flags,SIZE_T size){ 
    (void)heap; void *p=kmalloc(size); 
    if(p && (flags&0x08)) k_memset(p,0,size);
    return p;
}
BOOL __attribute__((stdcall)) stub_HeapFree(HANDLE heap,DWORD flags,LPVOID ptr){ (void)heap;(void)flags; kfree(ptr); return TRUE; }

DWORD __attribute__((stdcall)) stub_GetCurrentThreadId(void){ return 1; }
DWORD __attribute__((stdcall)) stub_GetCurrentProcessId(void){ return 42; }

void __attribute__((stdcall)) stub_Sleep(DWORD ms){
    // Stage 3+ timer busy-wait optimizado
    uint64_t end=timer_ticks() + ms;
    while(timer_ticks()<end) __asm__ volatile("hlt");
}

BOOL __attribute__((stdcall)) stub_GetConsoleMode(HANDLE h,DWORD *mode){ (void)h; if(mode)*mode=0x0003; return TRUE; }
BOOL __attribute__((stdcall)) stub_SetConsoleMode(HANDLE h,DWORD mode){ (void)h;(void)mode; return TRUE; }

HMODULE __attribute__((stdcall)) stub_GetModuleHandleA(LPCSTR name){ (void)name; return (HMODULE)0x400000; }
DWORD __attribute__((stdcall)) stub_GetModuleFileNameA(HMODULE mod,LPSTR buf,DWORD size){ 
    (void)mod; const char *n="C:\\XP7OS\\app.exe"; 
    k_strncpy(buf,n,size-1); return (DWORD)k_strlen(n);
}

LPVOID __attribute__((stdcall)) stub_GetProcAddress(HMODULE mod,LPCSTR func){
    (void)mod; uint32_t addr=win32_stub_find("kernel32.dll",func);
    if(!addr) addr=win32_stub_find("ntdll.dll",func);
    if(!addr) addr=win32_stub_find("msvcrt.dll",func);
    return (LPVOID)(uintptr_t)(addr ? addr : win32_stub_missing("unknown.dll",func));
}

HMODULE __attribute__((stdcall)) stub_LoadLibraryA(LPCSTR name){ (void)name; return (HMODULE)0x10000000; }
BOOL   __attribute__((stdcall)) stub_FreeLibrary(HMODULE mod){ (void)mod; return TRUE; }
void   __attribute__((stdcall)) stub_OutputDebugStringA(LPCSTR msg){ vga_print("[DBG] "); vga_print(msg); vga_print("\n"); }
BOOL   __attribute__((stdcall)) stub_IsDebuggerPresent(void){ return FALSE; }

DWORD __attribute__((stdcall)) stub_FormatMessageA(DWORD flags,LPVOID src,DWORD msgid,DWORD lang,LPSTR buf,DWORD size,void *args){
    (void)flags;(void)src;(void)lang;(void)args;
    char tmp[64]="Error "; char num[16]; k_itoa((int)msgid,num,10); k_strcat(tmp,num); k_strncpy(buf,tmp,size-1); return (DWORD)k_strlen(tmp);
}

int __attribute__((stdcall)) stub_MultiByteToWideChar(UINT cp,DWORD flags,LPCSTR mb,int mbl,void *wc,int wcl){
    (void)cp;(void)flags; uint16_t *out=(uint16_t*)wc; int i=0; if(mbl<0)mbl=(int)k_strlen(mb)+1;
    while(i<mbl && i<wcl){ out[i]=(uint16_t)(uint8_t)mb[i]; i++; } return i;
}

// ═════════ NTDLL ════════════════════════════════════════════
LONG __attribute__((stdcall)) stub_RtlGetLastWin32Error(void){ return (LONG)last_error; }
void __attribute__((stdcall)) stub_RtlSetLastWin32Error(DWORD err){ last_error=err; }
LONG __attribute__((stdcall)) stub_NtTerminateProcess(HANDLE proc,LONG status){ stub_ExitProcess((UINT)status); return 0; }
LONG __attribute__((stdcall)) stub_NtWriteFile(HANDLE file,HANDLE evt,void*a,void*c,void*io,void*buf,ULONG len,void*off,void*key){
    (void)evt;(void)a;(void)c;(void)io;(void)off;(void)key; DWORD wr;
    stub_WriteFile(file,(LPCSTR)buf,len,&wr,NULL); return 0;
}

// ═════════ MSVCRT/UCRTBASE ══════════════════════════════════
int stub_printf(const char *fmt,...){
    char out[1024]; int oi=0; uint32_t *args=(uint32_t*)(&fmt+1); int i=0,ai=0;
    char tmp[32];
    while(fmt[i] && oi<1023){
        if(fmt[i]!='%'){ out[oi++]=fmt[i++]; continue; }
        i++; switch(fmt[i++]){
            case 's':{ const char *s=(const char*)(uintptr_t)args[ai++]; if(!s)s="(null)";
                while(*s && oi<1023) out[oi++]=*s++; break; }
            case 'd': case 'i': k_itoa((int)args[ai++],tmp,10); for(int j=0;tmp[j]&&oi<1023;j++) out[oi++]=tmp[j]; break;
            case 'u': k_itoa((int)args[ai++],tmp,10); for(int j=0;tmp[j]&&oi<1023;j++) out[oi++]=tmp[j]; break;
            case 'x': case 'X': k_itoa((int)args[ai++],tmp,16); for(int j=0;tmp[j]&&oi<1023;j++) out[oi++]=tmp[j]; break;
            case 'c': out[oi++]=(char)args[ai++]; break;
            case '%': out[oi++]='%'; break;
            default: out[oi++]='?'; break;
        }
    }
    out[oi]=0; gui_or_vga_print(out); return oi;
}
int stub_sprintf(char *buf,const char *fmt,...){ k_strcpy(buf,fmt); return (int)k_strlen(buf); }
int stub_puts(const char *s){ gui_or_vga_print(s); gui_or_vga_print("\n"); return 0; }
void *stub_malloc(SIZE_T size){ return kmalloc(size); }
void *stub_calloc(SIZE_T n,SIZE_T sz){ return kcalloc(n,sz); }
void *stub_realloc(void *ptr,SIZE_T size){ return krealloc(ptr,size); }
void  stub_free(void *ptr){ kfree(ptr); }
void  stub_exit(int code){ stub_ExitProcess((UINT)code); }
int   stub_strcmp(const char *a,const char *b){ return k_strcmp(a,b); }
size_t stub_strlen(const char *s){ return k_strlen(s); }
void *stub_memcpy(void *d,const void *s,SIZE_T n){ return k_memcpy(d,s,n); }
void *stub_memset(void *d,int v,SIZE_T n){ return k_memset(d,v,n); }

// ═════════ USER32 ═══════════════════════════════════════════
int __attribute__((stdcall)) stub_MessageBoxA(HWND hwnd,LPCSTR text,LPCSTR cap,UINT type){
    (void)hwnd;(void)type;
    char buf[512]; k_strcpy(buf,cap); k_strcat(buf,": "); k_strcat(buf,text); k_strcat(buf,"\n");
    gui_or_vga_print(buf); return 1;
}

// ─── Otros user32 básicos
HWND __attribute__((stdcall)) stub_CreateWindowExA(DWORD exstyle,LPCSTR cls,LPCSTR name,DWORD style,int x,int y,int w,int h,HWND parent,HANDLE menu,HINSTANCE inst,LPVOID param){
    (void)exstyle;(void)cls;(void)name;(void)style;(void)x;(void)y;(void)w;(void)h;(void)parent;(void)menu;(void)inst;(void)param;
    return 0x2000;
}
BOOL __attribute__((stdcall)) stub_ShowWindow(HWND hwnd,int cmd){ (void)hwnd;(void)cmd; return TRUE; }
BOOL __attribute__((stdcall)) stub_UpdateWindow(HWND hwnd){ (void)hwnd; return TRUE; }
BOOL __attribute__((stdcall)) stub_GetMessageA(void *msg,HWND hwnd,UINT min,UINT max){ (void)msg;(void)hwnd;(void)min;(void)max; return FALSE; }
BOOL __attribute__((stdcall)) stub_TranslateMessage(void *msg){ (void)msg; return TRUE; }
LONG __attribute__((stdcall)) stub_DispatchMessageA(void *msg){ (void)msg; return 0; }
LONG __attribute__((stdcall)) stub_DefWindowProcA(HWND hwnd,UINT msg,uint32_t wp,uint32_t lp){ (void)hwnd;(void)msg;(void)wp;(void)lp; return 0; }

// ─── Funciones de lookup de stubs
uint32_t win32_stub_missing(const char *dll,const char *func){ 
    char buf[128]; k_strcpy(buf,"[STUB MISSING] "); k_strcat(buf,func); k_strcat(buf,"\n"); gui_or_vga_print(buf); return 0; 
}

uint32_t win32_stub_find(const char *dll,const char *func){ 
    (void)dll;(void)func; return 0;
}
uint32_t win32_stub_ordinal(const char *dll,uint16_t ord){ (void)dll;(void)ord; return 0; }

void win32_stubs_init(void){ gui_or_vga_print("[WIN32 STUBS] Iniciados Stage 3+\n"); }
