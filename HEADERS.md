# 📋 XP7 OS — Lista completa de Headers

## ✅ Todos los .h del proyecto (24 archivos)

### 🔧 Subsistemas Base (10 archivos)

| Header | Path | Propósito |
|--------|------|-----------|
| **vga.h** | `kernel/vga.h` | Driver VGA texto 80×25, colores 16-bit |
| **string.h** | `kernel/string.h` | strlen, strcpy, memcpy, sprintf (sin libc!) |
| **gdt.h** | `kernel/gdt.h` | Global Descriptor Table (segmentación x86) |
| **idt.h** | `kernel/idt.h` | Interrupt Descriptor Table + ISR handlers |
| **pic.h** | `kernel/pic.h` | Programmable Interrupt Controller 8259 |
| **keyboard.h** | `kernel/keyboard.h` | Driver teclado PS/2 (IRQ1) |
| **mouse.h** | `kernel/mouse.h` | Driver ratón PS/2 (IRQ12) |
| **framebuffer.h** | `kernel/framebuffer.h` | Driver gráfico 32bpp (800×600) |
| **font.h** | `kernel/font.h` | Fuente bitmap 8×8 embebida |
| **shell.h** | `kernel/shell.h` | Shell modo texto (cuando no hay GUI) |

### 💾 Memory & Filesystem (2 archivos)

| Header | Path | Propósito |
|--------|------|-----------|
| **mm.h** | `kernel/mm/mm.h` | Memory manager: kmalloc/kfree (8MB heap) |
| **fat32.h** | `kernel/fs/fat32.h` | Driver FAT32 + ATA PIO (disco duro) |

### 🖼️ GUI (4 archivos)

| Header | Path | Propósito |
|--------|------|-----------|
| **theme.h** | `kernel/gui/theme.h` | Colores y constantes de interfaz |
| **wm.h** | `kernel/gui/wm.h` | Window Manager (ventanas, focus) |
| **taskbar.h** | `kernel/gui/taskbar.h` | Taskbar + Start menu |
| **desktop.h** | `kernel/gui/desktop.h` | Desktop + event loop principal |

### 🚀 Ejecutables & Windows PE (2 archivos)

| Header | Path | Propósito |
|--------|------|-----------|
| **pe_loader.h** | `kernel/exec/pe_loader.h` | Cargador PE (.exe + .dll) |
| **win32_stubs.h** | `kernel/win32/win32_stubs.h` | Stubs Win32 API (kernel32, ntdll, msvcrt, user32) |

### 📦 VirtualBox + Shared Folders (1 archivo)

| Header | Path | Propósito |
|--------|------|-----------|
| **vbox.h** | `kernel/vbox/vbox.h` | VBoxSF driver (leer archivos de Windows) |

### 🎮 DOOM Integration (3 archivos)

| Header | Path | Propósito |
|--------|------|-----------|
| **doom_xp7.h** | `kernel/doom/doom_xp7.h` | DOOM platform layer (doomgeneric) |
| **doom_libc.h** | `kernel/doom/doom_libc.h` | Redirect libc → kernel functions |
| **doom_wad_embed.h** | `kernel/doom/doom_wad_embed.h` | WAD embebido via objcopy |

### 📦 Formatos de Aplicaciones (2 archivos)

| Header | Path | Propósito |
|--------|------|-----------|
| **pak.h** | `formats/pak.h` | Formato .pak (packages de apps) |
| **act.h** | `formats/act.h` | Formato .act (patches y updates) |

---

## 🔍 Headers por Include Order en kernel.c

```c
#include "vga.h"              // ← texto 80×25
#include "string.h"           // ← strlen, sprintf, memcpy
#include "gdt.h"              // ← segmentación x86
#include "idt.h"              // ← interrupts + ISRs
#include "pic.h"              // ← 8259 PIC
#include "keyboard.h"         // ← IRQ1 keyboard
#include "mouse.h"            // ← IRQ12 mouse
#include "framebuffer.h"      // ← gráficos 32bpp
#include "font.h"             // ← fuente 8×8
#include "mm/mm.h"            // ← malloc/free
#include "fs/fat32.h"         // ← disco FAT32
#include "win32/win32_stubs.h"  // ← Win32 API stubs
#include "exec/pe_loader.h"   // ← .exe loader
#include "vbox/vbox.h"        // ← VBoxSF
#include "gui/desktop.h"      // ← desktop GUI
#include <stdint.h>           // ← GCC builtin (uint32_t, etc)
```

---

## 🐛 Headers que NO existen en libc (son nuestros)

Estos headers tienen **nombres que colisionan con libc**, pero son los nuestros:

```c
"string.h"    // ← kernel/string.h (NO /usr/include/string.h)
"gdt.h"       // ← nuestro (GDT no existe en libc)
"idt.h"       // ← nuestro (IDT no existe en libc)
```

El **Makefile** usa `-nostdinc -iquote kernel` para que el compilador:
1. NO busque en `/usr/include/`
2. Busque en `kernel/` PRIMERO cuando ve `"..."`

---

## 📦 Cómo usarlos en tu proyecto

### Opción A: Descargar el ZIP completo
```
XP7OS_Stage3_complete.zip (101 KB)
├── todos los .h
├── todos los .c
├── Makefile configurado
└── Dockerfile
```

### Opción B: Descargar headers individuales
Haz clic en cada `.h` arriba para descargarlo individualmente.

### Opción C: Estructura de carpetas
```
XP7OS/
├── kernel/
│   ├── vga.h
│   ├── string.h
│   ├── gdt.h
│   ├── idt.h
│   ├── pic.h
│   ├── keyboard.h
│   ├── mouse.h
│   ├── framebuffer.h
│   ├── font.h
│   ├── shell.h
│   ├── mm/
│   │   └── mm.h
│   ├── fs/
│   │   └── fat32.h
│   ├── exec/
│   │   └── pe_loader.h
│   ├── vbox/
│   │   └── vbox.h
│   ├── win32/
│   │   └── win32_stubs.h
│   ├── gui/
│   │   ├── theme.h
│   │   ├── wm.h
│   │   ├── taskbar.h
│   │   └── desktop.h
│   └── doom/
│       ├── doom_xp7.h
│       ├── doom_libc.h
│       └── doom_wad_embed.h
└── formats/
    ├── pak.h
    └── act.h
```

---

## 🛠️ Include Guards — Todos verificados ✓

Todos los headers tienen guards correctos:

```c
#ifndef NOMBRE_H
#define NOMBRE_H

// contenido...

#endif /* NOMBRE_H */
```

Sin includes circulares. Sin declaraciones duplicadas.

---

*XP7 OS — Hecho con 💪 y 24 headers limpios*
