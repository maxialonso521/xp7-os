#!/bin/bash
# ╔══════════════════════════════════════════════════════════════╗
# ║  XP7 OS — Build Script para WSL2 (sin Docker)              ║
# ║  Ejecutar: bash build-wsl2.sh                              ║
# ╚══════════════════════════════════════════════════════════════╝

set -e  # Exit on error

echo ""
echo "╔══════════════════════════════════════════════════════════════╗"
echo "║  XP7 OS — Compilación en WSL2                               ║"
echo "╚══════════════════════════════════════════════════════════════╝"
echo ""

# ═══════════════════════════════════════════════════════════════
# Paso 1: Verificar que estamos en WSL
# ═══════════════════════════════════════════════════════════════
if ! grep -qi microsoft /proc/version 2>/dev/null; then
    echo "⚠️  Este script debe ejecutarse en WSL2 (Ubuntu en Windows)"
    echo ""
    echo "Para abrir WSL:"
    echo "  1. PowerShell → wsl"
    echo "  2. Ejecutar: bash build-wsl2.sh"
    exit 1
fi

echo "✓ WSL2 detectado"
echo ""

# ═══════════════════════════════════════════════════════════════
# Paso 2: Instalar dependencias (si no están)
# ═══════════════════════════════════════════════════════════════
echo "[1/4] Verificando dependencias..."

MISSING=""
command -v i686-linux-gnu-gcc >/dev/null || MISSING="$MISSING gcc-i686-linux-gnu"
command -v nasm >/dev/null || MISSING="$MISSING nasm"
command -v make >/dev/null || MISSING="$MISSING make"
command -v grub-mkrescue >/dev/null || MISSING="$MISSING grub-pc-bin grub-common"
command -v xorriso >/dev/null || MISSING="$MISSING xorriso"

if [ -n "$MISSING" ]; then
    echo "Instalando dependencias faltantes..."
    sudo apt-get update
    sudo apt-get install -y $MISSING
    echo "✓ Dependencias instaladas"
else
    echo "✓ Todas las dependencias ya están instaladas"
fi
echo ""

# ═══════════════════════════════════════════════════════════════
# Paso 3: Compilar kernel
# ═══════════════════════════════════════════════════════════════
echo "[2/4] Compilando kernel..."

# Limpiar builds anteriores
make clean 2>/dev/null || true

# Compilar
make all

if [ -f "xp7os.kernel" ]; then
    echo "✓ xp7os.kernel generado ($(du -h xp7os.kernel | cut -f1))"
else
    echo "✗ Error: xp7os.kernel no se generó"
    exit 1
fi
echo ""

# ═══════════════════════════════════════════════════════════════
# Paso 4: Crear ISO
# ═══════════════════════════════════════════════════════════════
echo "[3/4] Creando ISO booteable..."

make iso

if [ -f "xp7os.iso" ]; then
    echo "✓ xp7os.iso generado ($(du -h xp7os.iso | cut -f1))"
else
    echo "✗ Error: xp7os.iso no se generó"
    exit 1
fi
echo ""

# ═══════════════════════════════════════════════════════════════
# Paso 5: DOOM (opcional)
# ═══════════════════════════════════════════════════════════════
echo "[4/4] DOOM (opcional)..."

# Verificar si hay WAD
WAD_FOUND=0
for wad in *.wad wad_import/*.wad 2>/dev/null; do
    if [ -f "$wad" ]; then
        WAD_FOUND=1
        break
    fi
done

# Verificar si hay doomgeneric sources
if [ -d "kernel/doom/src" ] && [ "$(ls -A kernel/doom/src 2>/dev/null)" ]; then
    echo "→ doomgeneric sources encontrados"
    
    if [ $WAD_FOUND -eq 1 ]; then
        echo "→ WAD encontrado, compilando versión con DOOM..."
        make clean-doom 2>/dev/null || true
        make doom && echo "✓ xp7os-doom.kernel"
        make iso-doom && echo "✓ xp7os-doom.iso"
    else
        echo "⚠  No se encontró WAD, omitiendo DOOM"
        echo "   Para compilar con DOOM:"
        echo "   1. Descargar freedoom1.wad"
        echo "   2. Copiar a la raíz del proyecto"
        echo "   3. Ejecutar: make doom && make iso-doom"
    fi
else
    echo "⚠  doomgeneric no está disponible"
    echo "   Clonar con: git clone https://github.com/oxyroneth/doomgeneric"
fi
echo ""

# ═══════════════════════════════════════════════════════════════
# Resumen
# ═══════════════════════════════════════════════════════════════
echo "╔══════════════════════════════════════════════════════════════╗"
echo "║  ✅ BUILD COMPLETO                                          ║"
echo "╚══════════════════════════════════════════════════════════════╝"
echo ""
echo "Archivos generados:"
ls -lh xp7os*.kernel xp7os*.iso 2>/dev/null | awk '{print "  " $9 " (" $5 ")"}'
echo ""

# Mostrar path de Windows
WIN_PATH=$(pwd | sed 's|/mnt/\(.\)|\U\1:|')
echo "Ubicación en Windows:"
echo "  $WIN_PATH"
echo ""

echo "SIGUIENTE PASO:"
echo "  1. Abrir VirtualBox"
echo "  2. Settings → Storage → IDE → Optical Drive"
echo "  3. Seleccionar: $WIN_PATH\\xp7os.iso"
echo "  4. Start!"
echo ""
