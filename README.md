# 🎮 XP7 OS — Sistema Operativo Multi-Bit con Roblox

[![GitHub](https://img.shields.io/badge/GitHub-maxialonso521%2Fxp7--os-blue?logo=github)](https://github.com/maxialonso521/xp7-os)
[![Build Status](https://img.shields.io/badge/build-passing-brightgreen)]()
[![License](https://img.shields.io/badge/license-MIT-green)](LICENSE)
[![Stage](https://img.shields.io/badge/stage-3-orange)](https://github.com/maxialonso521/xp7-os/releases)
[![Arch](https://img.shields.io/badge/arch-32bit%20%2B%2064bit-blue)]()

**🎯 Meta:** El primer OS hecho desde cero que ejecute apps de 16, 32 Y 64-bit + Roblox

---

## 🌟 Demo

![XP7 OS Screenshot](docs/screenshot.png)

> Sistema operativo educativo con GUI, Window Manager, y DOOM jugable

---

## 🚀 Quick Start

### Compilar en 3 comandos (WSL2)

```bash
wsl
cd /mnt/c/tu-carpeta/xp7-os
bash build-wsl2.sh
```

**Resultado:** `xp7os.iso` listo en 2-3 minutos ⚡

### Probar en VirtualBox

```
1. VirtualBox → New → XP7-OS (32-bit)
2. Settings → Display → VBoxVGA
3. Settings → Storage → xp7os.iso
4. Start! 🚀
```

**Guía completa:** [HOW_TO_START.md](HOW_TO_START.md)

---

## 📊 Estado del Proyecto

### ✅ Stage 3 — Sistema base de 32-bit (COMPLETADO)

```
✅ Bootloader (Multiboot 1)
✅ Kernel en C con GUI
✅ Memory Manager (8 MB heap)
✅ Window Manager + Desktop
✅ PE Loader (ejecuta .exe)
✅ Win32 API stubs (50+ funciones)
✅ FAT32 + VirtualBox Shared Folders
✅ DOOM jugable 🎮
```

### 🚧 Stage 4 — Migración a 64-bit (EN PROGRESO)

```
✅ Bootloader de 64-bit (Long Mode)
✅ Paging de 4 niveles
⏳ Kernel adaptado (6-9 semanas)
⏳ PE32+ loader
⏳ x64 calling convention
```

### ⏳ Stage 5-9 — Sistema Multi-Bit + Roblox (PLANEADO)

```
⏳ WOW64 Layer (32-bit en 64)
⏳ NTVDM Layer (16-bit)
⏳ GPU Stack (Vulkan)
⏳ Wine + Sober integration
⏳ 🎯 ROBLOX PLAYABLE 🎯
```

**Timeline:** 12 meses | **Progreso:** ~20% ████░░░░░░░░░░░░░░░░

---

## 🏗️ Arquitectura

```
XP7 OS Kernel (64-bit)
├── Native 64-bit Layer
│   ├── System32/ (DLLs 64-bit)
│   └── PE32+ loader
│
├── WOW64 Layer (32 en 64)
│   ├── SysWOW64/ (DLLs 32-bit)
│   └── Thunking 32↔64
│
├── NTVDM Layer (16-bit)
│   ├── V86 emulator
│   └── DOS compatibility
│
└── Wine/Proton
    ├── Sober (Roblox)
    ├── DXVK (DX → Vulkan)
    └── VKD3D (D3D12)
```

---

## 📦 Estructura del Proyecto

```
xp7-os/
├── boot/
│   ├── boot.asm              # Bootloader 32-bit
│   └── boot64.asm            # Bootloader 64-bit ✨
│
├── kernel/
│   ├── kernel.c              # Entry point
│   ├── gdt.c / idt.c         # Descriptor tables
│   ├── mm/mm.c               # Memory manager
│   ├── exec/pe_loader.c      # .exe loader
│   ├── win32/win32_stubs.c   # Windows API
│   ├── vbox/vbox.c           # VirtualBox integration
│   ├── gui/wm.c              # Window Manager
│   └── doom/doom_xp7.c       # DOOM integration 🎮
│
├── docs/                     # 22 archivos de documentación
│   ├── README_WSL2.md        # ← START HERE
│   ├── MULTIBIT_ROBLOX_ROADMAP.md  # Roadmap completo
│   └── SOBER_INTEGRATION.md  # Roblox integration
│
├── Makefile                  # Build 32-bit
├── Makefile.x64              # Build 64-bit ✨
└── build-wsl2.sh             # Script automático
```

**Total:** 87 archivos | 27 headers + 22 sources + 4 assembly

---

## 🎯 Roadmap a Roblox

| Fase | Duración | Features |
|------|----------|----------|
| **Stage 1-3** | ✅ 3 meses | Kernel 32-bit + GUI + DOOM |
| **Stage 4** | 🚧 2 meses | Kernel 64-bit estable |
| **Stage 5** | ⏳ 2 meses | PE32+ + Apps nativas 64-bit |
| **Stage 6** | ⏳ 4 meses | WOW64 Layer (32 en 64) |
| **Stage 7** | ⏳ 2 meses | NTVDM (16-bit) |
| **Stage 8** | ⏳ 2 meses | GPU Stack + Vulkan |
| **Stage 9** | ⏳ 1 mes | Wine + Sober → **ROBLOX** 🎮 |

**Timeline total:** 12 meses (1 año)

[Ver roadmap detallado →](docs/MULTIBIT_ROBLOX_ROADMAP.md)

---

## 🛠️ Build Instructions

### Opción A: WSL2 (Recomendado ⭐)

```bash
# 1. Instalar WSL2 (solo primera vez)
wsl --install

# 2. Clonar repo
git clone https://github.com/maxialonso521/xp7-os.git
cd xp7-os

# 3. Build automático
bash build-wsl2.sh

# Outputs:
#   xp7os.kernel  → Kernel ELF
#   xp7os.iso     → ISO booteable
```

**Tiempo:** 2-3 minutos | **Requiere:** WSL2 (Ubuntu)

### Opción B: Docker

```bash
# Build
docker build -t xp7os-builder .
docker run --rm -v "${PWD}/output:/output" xp7os-builder

# Si Docker falla con EOF:
docker build -f Dockerfile.minimal -t xp7os-builder .
```

**Tiempo:** 10-15 minutos | **Requiere:** Docker Desktop

### Opción C: Linux nativo

```bash
# Instalar dependencias
sudo apt install gcc-i686-linux-gnu nasm make grub-pc-bin xorriso

# Build
make clean
make all
make iso
```

---

## 🎮 Features

### Actualmente funcional (32-bit)

#### 💻 Sistema
- [x] Bootloader Multiboot
- [x] Kernel en C (2000+ líneas)
- [x] Memory Manager (heap de 8 MB)
- [x] GDT, IDT, PIC configurados
- [x] Drivers: keyboard, mouse, VGA, framebuffer

#### 🖼️ GUI
- [x] Window Manager con drag & drop
- [x] Taskbar con Start menu
- [x] Desktop con iconos
- [x] Múltiples ventanas simultáneas
- [x] Tema XP-style

#### 💾 Filesystem
- [x] FAT32 read/write
- [x] VirtualBox Shared Folders
- [x] Acceso a archivos de Windows
- [x] Directory listing

#### 🚀 Executables
- [x] PE32 loader (.exe de 32-bit)
- [x] Win32 API stubs (kernel32, ntdll, msvcrt, user32)
- [x] DLL loading desde SysWOW64
- [x] Apps funcionando: Notepad, Calc

#### 🎮 Gaming
- [x] DOOM integrado (doomgeneric)
- [x] Framebuffer de 32-bit (800x600)
- [x] Mouse + Keyboard input
- [x] WAD loading desde VBoxSF

### En desarrollo (64-bit)

- [x] Boot de 64-bit (Long Mode) ✨
- [x] Paging de 4 niveles
- [ ] Kernel adaptado
- [ ] PE32+ loader
- [ ] WOW64 layer

---

## 📚 Documentación

### Getting Started
- [README_WSL2.md](docs/README_WSL2.md) — **START HERE** (compilar sin Docker)
- [HOW_TO_START.md](docs/HOW_TO_START.md) — Cómo arrancar el OS
- [QUICKSTART.md](docs/QUICKSTART.md) — Setup en 2 minutos

### Arquitectura
- [MULTIBIT_ROBLOX_ROADMAP.md](docs/MULTIBIT_ROBLOX_ROADMAP.md) — **Roadmap completo**
- [SOBER_INTEGRATION.md](docs/SOBER_INTEGRATION.md) — **Roblox integration**
- [32BIT_TO_64BIT_GUIDE.md](docs/32BIT_TO_64BIT_GUIDE.md) — Migración a 64-bit
- [DUAL_ARCH_GUIDE.md](docs/DUAL_ARCH_GUIDE.md) — Build 32 + 64 simultáneo

### Troubleshooting
- [ERROR_EOF_FIX.md](docs/ERROR_EOF_FIX.md) — Solución error EOF de Docker
- [DOCKER_TROUBLESHOOTING.md](docs/DOCKER_TROUBLESHOOTING.md) — Problemas comunes
- [COMPILATION_FIXES.md](docs/COMPILATION_FIXES.md) — Bugs corregidos

### VirtualBox
- [VIRTUALBOX_SETUP.md](docs/VIRTUALBOX_SETUP.md) — Configuración completa
- [DOOM_VBOXSF.md](docs/DOOM_VBOXSF.md) — DOOM + Shared Folders

[Ver toda la documentación →](docs/)

---

## 🤝 Contribuir

¡Las contribuciones son bienvenidas! Este es un proyecto educativo.

### Áreas de contribución

1. **Código**
   - Implementar syscalls faltantes
   - Optimizar memory manager
   - Añadir drivers (red, USB, etc.)
   - Migración a 64-bit

2. **Documentación**
   - Tutoriales paso a paso
   - Videos explicativos
   - Traducciones

3. **Testing**
   - Probar en hardware real
   - Report de bugs
   - Benchmarks

### Cómo contribuir

```bash
# 1. Fork del repo
git clone https://github.com/tu-usuario/xp7-os.git

# 2. Crear branch
git checkout -b feature/mi-feature

# 3. Commit
git commit -am "Add: mi feature"

# 4. Push
git push origin feature/mi-feature

# 5. Pull Request en GitHub
```

[Ver guía completa de contribución →](CONTRIBUTING.md)

---

## 🐛 Reportar Bugs

¿Encontraste un bug? [Abre un issue →](https://github.com/maxialonso521/xp7-os/issues/new)

Por favor incluye:
- Sistema operativo host (Windows/Linux/macOS)
- Método de compilación (WSL2/Docker/Linux)
- Logs completos del error
- Pasos para reproducir

---

## 📊 Comparación con otros OS educativos

| Proyecto | 16-bit | 32-bit | 64-bit | WOW64 | GUI | Gaming | Roblox |
|----------|--------|--------|--------|-------|-----|--------|--------|
| **XP7 OS** | ⏳ | ✅ | 🚧 | ⏳ | ✅ | ✅ | ⏳ |
| **SerenityOS** | ❌ | ❌ | ✅ | ❌ | ✅ | ❌ | ❌ |
| **ToaruOS** | ❌ | ❌ | ✅ | ❌ | ✅ | ❌ | ❌ |
| **Kolibri OS** | ❌ | ✅ | ❌ | ❌ | ✅ | ⚠️ | ❌ |
| **ReactOS** | ❌ | ✅ | 🚧 | ⏳ | ✅ | ⚠️ | ❌ |

**Único:** XP7 OS busca ser el primero en ejecutar Roblox desde cero 🎯

---

## 🌟 Características únicas

- 🎮 **Gaming-first:** Diseñado para ejecutar juegos (DOOM, Roblox)
- 🔀 **Multi-bit:** Soporte para 16, 32 Y 64-bit simultáneamente
- 🍷 **Wine integration:** Wine desde cero en el kernel
- 🎨 **GUI moderna:** Window Manager estilo XP
- 📦 **VirtualBox integration:** Shared Folders nativo
- 🚀 **Build rápido:** 2-3 minutos en WSL2

---

## 📜 Licencia

MIT License - Libre para usar, modificar y distribuir.

Ver [LICENSE](LICENSE) para más detalles.

---

## 🙏 Agradecimientos

- [OSDev.org](https://wiki.osdev.org/) — Recursos de desarrollo de OS
- [doomgeneric](https://github.com/oxyroneth/doomgeneric) — DOOM portable
- [Sober](https://github.com/MirayXS/Sober) — Roblox para Linux
- [Wine](https://www.winehq.org/) — Windows compatibility layer
- [DXVK](https://github.com/doitsujin/dxvk) — DirectX → Vulkan

---

## 📞 Links

- **GitHub:** https://github.com/maxialonso521/xp7-os
- **Issues:** https://github.com/maxialonso521/xp7-os/issues
- **Releases:** https://github.com/maxialonso521/xp7-os/releases
- **Wiki:** https://github.com/maxialonso521/xp7-os/wiki

---

## 📈 Roadmap Visual

```
├── Stage 1-3: Sistema base 32-bit           ████████████████████ 100% ✅
├── Stage 4:   Kernel 64-bit                 ███████░░░░░░░░░░░░░  35% 🚧
├── Stage 5:   PE32+ loader                  ░░░░░░░░░░░░░░░░░░░░   0% ⏳
├── Stage 6:   WOW64 Layer                   ░░░░░░░░░░░░░░░░░░░░   0% ⏳
├── Stage 7:   NTVDM (16-bit)                ░░░░░░░░░░░░░░░░░░░░   0% ⏳
├── Stage 8:   GPU Stack                     ░░░░░░░░░░░░░░░░░░░░   0% ⏳
└── Stage 9:   Wine + Sober → ROBLOX 🎯     ░░░░░░░░░░░░░░░░░░░░   0% ⏳

Progreso total: ████░░░░░░░░░░░░ 20%
Tiempo estimado: 10 meses restantes
```

---

## 🎯 Siguiente Hito

**Stage 4 — Kernel de 64-bit estable**

- [ ] Adaptar kernel.c para 64-bit
- [ ] IDT de 64-bit funcionando
- [ ] Memory manager con punteros de 64-bit
- [ ] GUI arrancando en 64-bit

**Fecha estimada:** Mayo 2026

---

## ⭐ Apoya el Proyecto

Si te gusta XP7 OS:
- ⭐ Dale una estrella al repo
- 🐛 Reporta bugs
- 💬 Comparte el proyecto
- 🤝 Contribuye código
- 📖 Mejora la documentación

---

```
   ___  _______    ____  _____
  | \ \/ /  __ \  / __ \/ ____|
   \ \  /| |__) || |  | | (___
    > < |  ___/ | |  | |\___ \
   / . \| |     | |__| |____) |
  /_/ \_\_|      \____/|_____/

  Sistema Operativo Multi-Bit
  16 | 32 | 64 → ROBLOX 🎮
  
  Made with ❤️ by @maxialonso521
```

---

**De cero a Roblox — ¡Únete al viaje!** 🚀
