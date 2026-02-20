#!/bin/bash
# XP7 OS — Embed WAD en el kernel via objcopy
# Uso: embed_wad.sh <wad_file> <output.o>
#
# Truco: siempre copia el WAD a nombre fijo "doom.wad"
# para que objcopy genere SIEMPRE los mismos símbolos:
#   _binary_doom_wad_start
#   _binary_doom_wad_end

set -e
WAD_IN="$1"
WAD_OUT="$2"
OC="${OBJCOPY:-i686-linux-gnu-objcopy}"

if [ ! -f "$WAD_IN" ]; then
    echo "ERROR: WAD no encontrado: $WAD_IN" >&2
    exit 1
fi

SIZE=$(stat -c%s "$WAD_IN" 2>/dev/null || stat -f%z "$WAD_IN")
echo "  [EMBED] $WAD_IN  ($(( SIZE / 1024 / 1024 )) MB -> kernel)"

TMP=$(mktemp -d)
cp "$WAD_IN" "$TMP/doom.wad"

cd "$TMP"
$OC \
    -I binary \
    -O elf32-i386 \
    -B i386 \
    --rename-section .data=.rodata,alloc,load,readonly,data,contents \
    doom.wad doom_wad.o
cd - > /dev/null

cp "$TMP/doom_wad.o" "$WAD_OUT"
rm -rf "$TMP"
echo "  ✅ Embebido OK: $WAD_OUT"
