# 🎮 XP7 OS — DOOM + VirtualBox Shared Folders

## Cómo cargar doom.wad sin recompilar

El método más cómodo para probar DOOM en XP7 OS es usar las **Shared Folders de VirtualBox**. Pones el WAD en una carpeta de Windows, y XP7 OS lo lee directamente sin tener que crear imágenes de disco ni recompilar.

---

## ⚡ Configurar VirtualBox Shared Folders (5 minutos)

### Paso 1: Crear la carpeta compartida en Windows

```
C:\XP7\shared\          ← carpeta de Windows que compartirás
    ├── doom.wad         ← tu WAD aquí
    ├── doom2.wad        ← (opcional)
    └── freedoom.wad    ← (alternativa gratis)
```

### Paso 2: Configurar en VirtualBox

```
VM → Settings → Shared Folders → Icono de +

Folder Path:  C:\XP7\shared
Folder Name:  xp7          ← OBLIGATORIO este nombre exacto
[ ] Read-only:              → sin marca (deja en blanco)
[x] Auto-mount:             → marcar esto
[ ] Make Permanent:         → opcional
```

> ⚠️ **El nombre DEBE ser `xp7`** (en minúsculas). El driver VBoxSF busca exactamente ese nombre.

### Paso 3: Arrancar XP7 OS

Al arrancar verás en la consola:

```
[VBOX] VirtualBox detectado ✓
[VBOX] VMMDev PCI encontrado @ bus:0 dev:4
[VBOXSF] Conectado a VBoxSharedFolders ✓
[VBOXSF] Carpeta 'xp7' mapeada OK
```

### Paso 4: Lanzar DOOM desde el terminal GUI

Abrir el Terminal en el escritorio de XP7 OS y escribir:

```
doom
```

O con ruta explícita:

```
doom doom2.wad
doom /vboxsf/doom2.wad
doom /vboxsf/freedoom.wad
```

---

## 🔍 Prioridad de búsqueda del WAD

XP7 OS busca el WAD en este orden:

| Prioridad | Fuente | Cómo usar |
|-----------|--------|-----------|
| **1ª** | VBoxSF (carpeta `xp7`) | Copiar .wad a la carpeta compartida |
| **2ª** | WAD embebido en kernel | `make doom WAD=doom.wad` al compilar |
| **3ª** | FAT32 del disco virtual | `make disk` + copiar .wad al disco |

---

## 🆓 Conseguir un WAD gratuito (Freedoom)

Si no tienes el DOOM original puedes usar **Freedoom**, totalmente gratis y compatible con el engine:

```
https://freedoom.github.io/download.html
```

Descarga `freedoom-0.13.0.zip` y usa `freedoom1.wad` o `freedoom2.wad`.

```
C:\XP7\shared\
    └── freedoom1.wad   ← equivale a DOOM 1
```

---

## 🎮 Controles de DOOM en XP7 OS

| Tecla | Acción |
|-------|--------|
| `↑` / `W` | Avanzar |
| `↓` / `S` | Retroceder |
| `←` | Girar izquierda |
| `→` | Girar derecha |
| `Alt + ←/→` | Strafe (moverse lateral) |
| `Ctrl` / `Z` | Disparar |
| `Espacio` | Usar / Abrir puertas |
| `Shift` | Correr |
| `1-7` | Cambiar arma |
| `Tab` | Mapa |
| `ESC` | Menú |
| `F1` | Ayuda |
| `F5` | Detail (calidad) |
| `F6` | Guardar |
| `F9` | Cargar |

---

## 🛠️ Compilar con WAD embebido (alternativa sin VBoxSF)

Si no quieres usar Shared Folders, puedes embeber el WAD directamente en el kernel:

```powershell
# Poner el WAD en la raíz del proyecto
copy C:\Games\DOOM\doom.wad  XP7OS\doom.wad

# Compilar con WAD embebido
docker run --rm -v "${PWD}:/xp7os" xp7os-builder make doom

# Output: xp7os-doom.kernel y xp7os-doom.iso (con WAD dentro)
```

El WAD queda permanentemente en el kernel. La ISO funciona en cualquier PC sin necesitar shared folders.

---

## 📊 Uso de memoria

| Componente | RAM |
|-----------|-----|
| Kernel XP7 OS | ~2 MB |
| GUI + framebuffer | ~2 MB |
| DOOM engine | ~4 MB |
| DOOM WAD (embebido) | 4-12 MB |
| **Total recomendado** | **≥ 64 MB** |

Configurar la VM con al menos **128 MB de RAM**.

---

## ❌ Solución de problemas

**"VBoxSF no disponible"**
- Asegúrate de correr en VirtualBox (no en QEMU)
- El nombre de la carpeta DEBE ser `xp7` exactamente
- Probar con: Settings → Shared Folders → Delete + Re-add

**"WAD no encontrado"**
- Verificar que el archivo se llama `doom.wad`, `doom1.wad`, `doom2.wad` o `freedoom.wad`
- Probar: `doom /vboxsf/tunombredearchivo.wad`

**"Pantalla negra al arrancar DOOM"**
- El framebuffer debe estar en modo 800x600x32
- En VirtualBox: Display → Graphics Controller → **VBoxVGA** (NO VBoxSVGA)
- Video Memory: mínimo 16 MB

**"Error de segfault / kernel panic al lanzar DOOM"**
- Necesitas doomgeneric: copiar sus fuentes a `kernel/doom/src/`
- Ver: https://github.com/oxyroneth/doomgeneric

---

## 📁 Estructura del proyecto (Stage 3 completo)

```
XP7OS/
├── boot/
│   └── boot.asm              # Multiboot + GDT flush + ISRs
├── kernel/
│   ├── kernel.c              # Entry point del OS
│   ├── vga.c/.h              # Driver texto 80×25
│   ├── string.c/.h           # strlen, strcpy, sprintf...
│   ├── gdt.c/.h              # Global Descriptor Table
│   ├── idt.c/.h              # Interrupt Descriptor Table
│   ├── pic.c/.h              # Programmable Interrupt Controller
│   ├── keyboard.c/.h         # Driver teclado PS/2 IRQ1
│   ├── mouse.c/.h            # Driver ratón PS/2 IRQ12
│   ├── framebuffer.c/.h      # Driver gráfico 32bpp
│   ├── font.c/.h             # Fuente bitmap 8×8
│   ├── shell.c/.h            # Shell modo texto
│   ├── mm/
│   │   └── mm.c/.h           # Memory manager (malloc/free)
│   ├── fs/
│   │   └── fat32.c/.h        # FAT32 + ATA PIO driver
│   ├── vbox/
│   │   └── vbox.c/.h         # VirtualBox Guest Driver + SharedFolders
│   ├── exec/
│   │   ├── pe_loader.c/.h    # Cargador PE (.exe y .dll)
│   │   └── pak_act.c         # Cargador .pak y aplicador .act
│   ├── win32/
│   │   └── win32_stubs.c/.h  # Win32 API (kernel32, ntdll, msvcrt, user32)
│   ├── doom/
│   │   ├── doom_xp7.c/.h     # DOOM platform layer
│   │   ├── doom_libc.h       # Redirect libc a kernel
│   │   ├── doom_wad_embed.h  # WAD embebido via objcopy
│   │   ├── doom_asm.asm      # IRQ0 stub para DOOM timer
│   │   ├── doom_wad_stub.asm # Stub vacío cuando no hay WAD
│   │   └── src/              # ← copiar doomgeneric aquí
│   └── gui/
│       ├── theme.h           # Colores y constantes UI
│       ├── wm.c/.h           # Window Manager
│       ├── taskbar.c/.h      # Taskbar + Start menu
│       └── desktop.c/.h      # Escritorio + event loop
├── formats/
│   ├── pak.h                 # Formato .pak (apps)
│   └── act.h                 # Formato .act (patches)
├── linker.ld                 # Script de linkeo
├── Makefile                  # Build system
├── Dockerfile                # Build en Docker
└── BUILD.md                  # Guía de compilación
```

---

*XP7 OS — Hecho con 💪 y sin copiar nada*
