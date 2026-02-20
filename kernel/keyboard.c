#include "keyboard.h"
#include "vga.h"
#include "pic.h"
#include "string.h"

// ─── Scancode Set 1 → ASCII (sin SHIFT) ─────────────────────
static const char sc_to_ascii[] = {
    0,   27,  '1','2','3','4','5','6','7','8','9','0','-','=', '\b',
    '\t','q', 'w','e','r','t','y','u','i','o','p','[',']', '\n',
    0,   'a', 's','d','f','g','h','j','k','l',';','\'','`',
    0,   '\\','z','x','c','v','b','n','m',',','.','/',
    0,   '*', 0,  ' ',
};

static const char sc_to_ascii_shift[] = {
    0,   27,  '!','@','#','$','%','^','&','*','(',')','_','+', '\b',
    '\t','Q', 'W','E','R','T','Y','U','I','O','P','{','}', '\n',
    0,   'A', 'S','D','F','G','H','J','K','L',':','"','~',
    0,   '|', 'Z','X','C','V','B','N','M','<','>','?',
    0,   '*', 0,  ' ',
};

// ─── Teclas especiales (bytes > 127 inyectados en el buffer) ─
// Coinciden con las constantes en doom_xp7.c
#define KEY_SPECIAL_UP      0x80
#define KEY_SPECIAL_DOWN    0x81
#define KEY_SPECIAL_LEFT    0x82
#define KEY_SPECIAL_RIGHT   0x83
#define KEY_SPECIAL_CTRL    0x84
#define KEY_SPECIAL_SHIFT   0x85
#define KEY_SPECIAL_ALT     0x86
#define KEY_SPECIAL_F1      0x91
#define KEY_SPECIAL_F2      0x92
#define KEY_SPECIAL_F3      0x93
#define KEY_SPECIAL_F4      0x94
#define KEY_SPECIAL_F5      0x95
#define KEY_SPECIAL_F6      0x96
#define KEY_SPECIAL_F7      0x97
#define KEY_SPECIAL_F8      0x98
#define KEY_SPECIAL_F9      0x99
#define KEY_SPECIAL_F10     0x9A

// Scancodes de teclas especiales (Set 1)
#define SC_F1          0x3B
#define SC_F2          0x3C
#define SC_F3          0x3D
#define SC_F4          0x3E
#define SC_F5          0x3F
#define SC_F6          0x40
#define SC_F7          0x41
#define SC_F8          0x42
#define SC_F9          0x43
#define SC_F10         0x44
// Teclas extendidas (con prefijo 0xE0)
#define SC_EXT_UP      0x48
#define SC_EXT_DOWN    0x50
#define SC_EXT_LEFT    0x4B
#define SC_EXT_RIGHT   0x4D
#define SC_EXT_HOME    0x47
#define SC_EXT_END     0x4F
#define SC_EXT_PGUP    0x49
#define SC_EXT_PGDN    0x51
#define SC_EXT_DEL     0x53
#define SC_EXT_RCTRL   0x1D
#define SC_EXT_RALT    0x38

// ─── Ring buffer ──────────────────────────────────────────────
static volatile char key_buf[KEY_BUFFER_SIZE];
static volatile int  buf_head = 0;
static volatile int  buf_tail = 0;
static volatile uint8_t modifiers  = 0;
static volatile uint8_t extended   = 0;  // 1 = siguiente sc es extendido

// ─── I/O ──────────────────────────────────────────────────────
static inline void outb_k(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" :: "a"(val), "Nd"(port));
}
static inline uint8_t inb_k(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// ─── Poner byte en el ring buffer ────────────────────────────
static inline void buf_push(char c) {
    int next = (buf_head + 1) % KEY_BUFFER_SIZE;
    if (next != buf_tail) {
        key_buf[buf_head] = c;
        buf_head = next;
    }
}

void keyboard_init(void) {
    buf_head = buf_tail = 0;
    modifiers = extended = 0;
    pic_clear_mask(1);
}

// ─── ISR ─────────────────────────────────────────────────────
void keyboard_handler(void) {
    uint8_t sc = inb_k(0x60);

    // Prefijo de tecla extendida
    if (sc == 0xE0) { extended = 1; pic_send_eoi(1); return; }

    int is_ext = extended;
    extended   = 0;

    // ── Release ──────────────────────────────────────────────
    if (sc & SC_BREAK_FLAG) {
        uint8_t base = sc & ~SC_BREAK_FLAG;
        if (!is_ext) {
            if (base == SC_LEFT_SHIFT || base == SC_RIGHT_SHIFT)
                modifiers &= ~KEY_MOD_SHIFT;
            else if (base == SC_LEFT_CTRL)
                modifiers &= ~KEY_MOD_CTRL;
            else if (base == SC_LEFT_ALT)
                modifiers &= ~KEY_MOD_ALT;
        }
        // Release de teclas especiales → byte con bit 6 seteado
        // (DOOM necesita saber cuándo se suelta una tecla)
        // TODO Stage 4: key-release events
        pic_send_eoi(1);
        return;
    }

    // ── Press: teclas extendidas (flechas, etc.) ─────────────
    if (is_ext) {
        char special = 0;
        switch (sc) {
        case SC_EXT_UP:     special = (char)KEY_SPECIAL_UP;    break;
        case SC_EXT_DOWN:   special = (char)KEY_SPECIAL_DOWN;  break;
        case SC_EXT_LEFT:   special = (char)KEY_SPECIAL_LEFT;  break;
        case SC_EXT_RIGHT:  special = (char)KEY_SPECIAL_RIGHT; break;
        case SC_EXT_RCTRL:  special = (char)KEY_SPECIAL_CTRL;  break;
        case SC_EXT_RALT:   special = (char)KEY_SPECIAL_ALT;   break;
        }
        if (special) buf_push(special);
        pic_send_eoi(1);
        return;
    }

    // ── Press: teclas normales ────────────────────────────────
    switch (sc) {
    case SC_LEFT_SHIFT:
    case SC_RIGHT_SHIFT: modifiers |= KEY_MOD_SHIFT;  break;
    case SC_LEFT_CTRL:   modifiers |= KEY_MOD_CTRL;
                         buf_push((char)KEY_SPECIAL_CTRL);  break;
    case SC_LEFT_ALT:    modifiers |= KEY_MOD_ALT;
                         buf_push((char)KEY_SPECIAL_ALT);   break;
    case SC_CAPS_LOCK:   modifiers ^= KEY_MOD_CAPS;         break;

    // Teclas F1-F10
    case SC_F1:  buf_push((char)KEY_SPECIAL_F1);  break;
    case SC_F2:  buf_push((char)KEY_SPECIAL_F2);  break;
    case SC_F3:  buf_push((char)KEY_SPECIAL_F3);  break;
    case SC_F4:  buf_push((char)KEY_SPECIAL_F4);  break;
    case SC_F5:  buf_push((char)KEY_SPECIAL_F5);  break;
    case SC_F6:  buf_push((char)KEY_SPECIAL_F6);  break;
    case SC_F7:  buf_push((char)KEY_SPECIAL_F7);  break;
    case SC_F8:  buf_push((char)KEY_SPECIAL_F8);  break;
    case SC_F9:  buf_push((char)KEY_SPECIAL_F9);  break;
    case SC_F10: buf_push((char)KEY_SPECIAL_F10); break;

    default: {
        char c = 0;
        if (sc < (int)sizeof(sc_to_ascii)) {
            int shifted = (modifiers & KEY_MOD_SHIFT) ? 1 : 0;
            if (modifiers & KEY_MOD_CAPS) {
                char raw = sc_to_ascii[sc];
                if (raw >= 'a' && raw <= 'z') shifted = !shifted;
            }
            c = shifted ? sc_to_ascii_shift[sc] : sc_to_ascii[sc];
        }
        if (c) buf_push(c);
    }
    }

    // Shift → inyectar para que DOOM sepa (run)
    if (sc == SC_LEFT_SHIFT || sc == SC_RIGHT_SHIFT)
        buf_push((char)KEY_SPECIAL_SHIFT);

    pic_send_eoi(1);
}

int keyboard_available(void) { return buf_head != buf_tail; }

char keyboard_getchar(void) {
    while (!keyboard_available()) __asm__ volatile("hlt");
    char c = key_buf[buf_tail];
    buf_tail = (buf_tail + 1) % KEY_BUFFER_SIZE;
    return c;
}

void keyboard_read_line(char *buf, int max_len) {
    int i = 0;
    while (i < max_len - 1) {
        char c = keyboard_getchar();
        // Ignorar teclas especiales (> 127) en input de texto
        if ((unsigned char)c > 127) continue;
        if (c == '\n') { vga_put_char('\n'); break; }
        if (c == '\b') {
            if (i > 0) { i--; vga_put_char('\b'); }
            continue;
        }
        buf[i++] = c;
        vga_put_char(c);
    }
    buf[i] = '\0';
}
