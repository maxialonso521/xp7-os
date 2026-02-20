# 🖥️ XP7 OS — Guía de Build y Configuración

> Un OS desde cero en C puro. Sin copiar proyectos ajenos. Sin magia. Solo **código limpio y determinación**. 💪

---

## 📦 Requisitos del Sistema

| Tool | Versión mínima | Para qué sirve |
|------|---------------|----------------|
| `i686-elf-gcc` | ≥ 10.0 | Cross-compiler C para x86 |
| `nasm` | ≥ 2.14 | Ensamblar el boot.asm |
| `i686-elf-ld` | incluido en binutils | Linkear el kernel |
| `grub-mkrescue` | ≥ 2.04 | Crear el .iso booteable |
| `qemu-system-i386` | ≥ 5.0 | Testing rápido sin VM |
| `VirtualBox` | ≥ 6.1 | Correr el .iso final |

---

## 🛠️ Instalar el Cross-Compiler (i686-elf-gcc)

> El cross-compiler es **lo más importante**. Sin él no compilas.
> El GCC de tu sistema no sirve (genera código para tu OS actual, no para bare metal).

### 🐧 Linux (Ubuntu/Debian)

```bash
# Instalar dependencias para compilar GCC desde fuente
sudo apt-get install -y \
    build-essential \
    bison flex \
    libgmp-dev libmpc-dev libmpfr-dev \
    texinfo libisl-dev

# O usar el paquete pre-compilado (más fácil) 🚀
# Para Ubuntu 22.04+:
sudo apt-get install -y gcc-i686-linux-gnu
# ⚠️ Nota: cambia "i686-elf" por "i686-linux-gnu" en el Makefile

# Para tener el i686-elf genuino, compilar GCC desde fuente:
# Ver: https://wiki.osdev.org/GCC_Cross-Compiler
```

### 🍎 macOS (Homebrew)

```bash
brew tap nativeos/i686-elf-toolchain
brew install i686-elf-toolchain
brew install nasm
brew install grub
brew install qemu
```

### 🪟 Windows

Usar **WSL2** con Ubuntu 22.04 y seguir las instrucciones de Linux. ✅

---

## 🏗️ Compilar el Kernel

```bash
# 1. Clonar / Descomprimir el proyecto
cd XP7 OS/

# 2. Compilar el kernel
make all

# Output: xp7os.kernel 🎉

# 3. Verificar que es Multiboot válido
make verify

# 4. Crear el .iso booteable
make iso

# Output: xp7os.iso — listo para VirtualBox 🚀
```

---

## 🎮 Correr en QEMU (Recomendado para Testing)

QEMU es **mucho más rápido** que VirtualBox para el ciclo de desarrollo:

```bash
make run
# Abre QEMU con tu kernel directamente 🖥️
```

Para debug con GDB:
```bash
# Terminal 1:
make run-debug

# Terminal 2 (conectar GDB):
gdb xp7os.kernel
(gdb) target remote localhost:1234
(gdb) continue
```

---

## 📀 Correr en VirtualBox

1. Ejecutar `make iso` → genera `xp7os.iso`
2. Abrir VirtualBox → **Nueva VM**
3. Configuración:
   - **Tipo:** Other
   - **Versión:** Other/Unknown (32-bit)
   - **RAM:** 128 MB mínimo
   - **Sin disco duro** (boot desde ISO)
4. Configuración → Almacenamiento → Añadir `xp7os.iso`
5. ¡Iniciar! 🚀

---

## 📂 Estructura del Proyecto

```
XP7 OS/
├── boot/
│   └── boot.asm           ← Entry point, multiboot header, ISR stubs
├── kernel/
│   ├── kernel.c           ← Kernel main (inicializa todo)
│   ├── vga.c / vga.h      ← Driver VGA text mode 80x25
│   ├── string.c / .h      ← String/memory ops sin libc
│   ├── gdt.c / .h         ← Global Descriptor Table
│   ├── idt.c / .h         ← Interrupt Descriptor Table
│   ├── pic.c / .h         ← PIC 8259A (IRQ remapping)
│   ├── keyboard.c / .h    ← Driver PS/2 teclado
│   └── shell.c / .h       ← Shell interactivo XP7Shell
├── formats/
│   ├── pak.h              ← Spec formato .pak (apps)
│   └── act.h              ← Spec formato .act (updates)
├── linker.ld              ← Script de linkeo (carga en 1MB)
├── Makefile               ← Build system completo
└── BUILD.md               ← Esta guía
```

---

## 🗺️ Roadmap de Stages

| Stage | Estado | Descripción |
|-------|--------|-------------|
| **Stage 1** | ✅ **DONE** | Boot + GDT/IDT + PIC + Teclado + Shell |
| **Stage 2** | 🔜 Próximo | GUI con framebuffer (XP + W7 style) |
| **Stage 3** | 📋 Planeado | Filesystem + loader .exe / .pak / .act |
| **Stage 4** | 📋 Planeado | Memory Manager (malloc/free) + procesos |
| **Stage 5** | 📋 Planeado | Motor DOM / JS engine embebido |
| **Stage 6** | 🔮 Futuro | Chromium portado + apps completas |

### Stage 2 — GUI Plan 🎨
- **Framebuffer VBE** (modo gráfico 1024x768 o 800x600 via VESA)
- **Window Manager** básico (ventanas draggable, minimizar)
- **Barra de tareas** estilo Windows XP + efectos W7 (aero-lite)
- **Sistema de widgets**: botones, textbox, listas
- **Tema**: azul XP luna + translucencia W7

### Stage 3 — Ejecutables 🏃
- **FAT32 filesystem** para leer del disco
- **PE Loader** para archivos .exe (como Wine pero en kernel)
- **PAK Loader** para cargar apps .pak
- **ACT System** para aplicar parches .act

### Stage 5 — DOM 🌐
- Parsear HTML + CSS básico con un engine propio en C
- Renderizar en framebuffer
- No depende de proyectos externos

### Stage 6 — Chromium 🚀
- Requiere userspace completo + POSIX parcial
- Compilar Chromium con toolchain propio apuntando a XP7 OS
- **Es el boss final del proyecto** 😤

---

## 🐛 Troubleshooting

**Error: `i686-elf-gcc: command not found`**
→ El cross-compiler no está instalado. Seguir las instrucciones arriba.

**Error: `multiboot header not found`**
→ Verificar que `*(.multiboot)` está PRIMERO en el linker script.
→ GRUB busca el header en los primeros 8KB del binario.

**El kernel bootea pero no hay texto**
→ El driver VGA usa `0xB8000`. Verificar que la VM tiene VGA estándar.
→ En VirtualBox: Configuración → Pantalla → Controlador: VBoxVGA.

**Teclado no responde**
→ Verificar que el PIC hizo EOI correctamente (keyboard_handler → pic_send_eoi).
→ En QEMU probar con `-device ps2-kbd`.

---

## 💡 Recursos Útiles

- [OSDev Wiki](https://wiki.osdev.org) — La biblia del OS dev 📖
- [Intel SDM Vol. 3](https://software.intel.com/content/www/us/en/develop/articles/intel-sdm.html) — Manual de arquitectura x86
- [NASM Docs](https://nasm.us/doc/) — Para el assembly

---

> **"El kernel no se escribe, se moldea. Línea por línea."** 🔥
>
> — XP7 OS Dev Team
