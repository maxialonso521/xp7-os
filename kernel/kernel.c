#include "vga.h"
#include "string.h"
#include "gdt.h"
#include "idt.h"
#include "pic.h"
#include "keyboard.h"
#include "mouse.h"
#include "framebuffer.h"
#include "font.h"
#include "mm/mm.h"
#include "fs/fat32.h"
#include "win32/win32_stubs.h"
#include "exec/pe_loader.h"
#include "vbox/vbox.h"
#include "gui/desktop.h"
#include <stdint.h>

/*
 *  XP7 OS — Kernel Principal
 *  Stage 3: MM + FAT32 + PE Loader + Win32 Stubs + VBoxSF + DOOM WAD
 */

typedef struct {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint8_t  syms[16];
    uint32_t mmap_length;
    uint32_t mmap_addr;
    uint32_t drives_length;
    uint32_t drives_addr;
    uint32_t config_table;
    uint32_t boot_loader_name;
    uint32_t apm_table;
    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint16_t vbe_mode;
    uint16_t vbe_interface_seg;
    uint16_t vbe_interface_off;
    uint16_t vbe_interface_len;
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t  framebuffer_bpp;
    uint8_t  framebuffer_type;
} __attribute__((packed)) multiboot_info_t;

#define MB_FLAG_MEMORY      (1 << 0)
#define MB_FLAG_FRAMEBUFFER (1 << 12)

void kernel_panic(const char *msg) {
    __asm__ volatile("cli");
    if (fb.ready) {
        fb_fill_rect(0, 0, fb.width, fb.height, RGB(180, 0, 0));
        font_draw_str(20, 20, "!! KERNEL PANIC !!", 0xFFFFFF, 0, 1);
        font_draw_str(20, 40, msg,                   0xFFFFFF, 0, 1);
    } else {
        vga_set_color(VGA_WHITE, VGA_RED);
        vga_clear();
        vga_print("\n  !! KERNEL PANIC !!\n\n  ");
        vga_print(msg);
    }
    while (1) __asm__ volatile("hlt");
}

void kernel_main(uint32_t magic, multiboot_info_t *mbi) {
    vga_init();
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_print("XP7 OS Kernel v0.3 - Iniciando Stage 3...\n");

    if (magic != 0x2BADB002)
        kernel_panic("Bootloader Multiboot invalido!");

    // ── Subsistemas base ──────────────────────────────────────
    gdt_init();      vga_print("[OK] GDT\n");
    idt_init();      vga_print("[OK] IDT\n");
    pic_init();      vga_print("[OK] PIC\n");
    keyboard_init(); vga_print("[OK] Teclado\n");

    // ── Stage 3: Memory, Filesystem, VirtualBox, Ejecutables ──
    mm_init();
    vga_print("[OK] Memory Manager (8MB heap)\n");

    // ── VirtualBox Guest Driver ─────────────────────────────
    // Detecta VMMDev PCI, conecta HGCM, monta SharedFolder "xp7"
    // (configurar en VBox: Settings → Shared Folders → nombre="xp7")
    vbox_init();

    if (fat32_init() < 0)
        vga_print("[!] FAT32 no detectado (sin disco o formato incorrecto)\n");

    win32_stubs_init();

    pe_loader_init("/syswow64", "/system32");

    // ── Modo grafico ──────────────────────────────────────────
    int has_fb = (mbi->flags & MB_FLAG_FRAMEBUFFER) &&
                 mbi->framebuffer_type == 1 &&
                 mbi->framebuffer_bpp  == 32 &&
                 mbi->framebuffer_addr != 0;

    if (has_fb) {
        vga_print("[OK] Framebuffer 800x600x32\n");
        fb_init((uint32_t)mbi->framebuffer_addr,
                mbi->framebuffer_pitch,
                mbi->framebuffer_width,
                mbi->framebuffer_height,
                mbi->framebuffer_bpp);
        mouse_init();
        vga_print("[OK] Mouse\n");
        vga_print("Iniciando GUI...\n");
        desktop_init();
        desktop_run();
    } else {
        vga_set_color(VGA_LIGHT_BROWN, VGA_BLACK);
        vga_print("[!] Sin framebuffer - modo texto\n");
        vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
        extern void shell_run(void);
        shell_run();
    }

    kernel_panic("Fin inesperado del kernel");
}
