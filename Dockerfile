# ╔══════════════════════════════════════════════════════════════╗
# ║  XP7 OS — Dockerfile Optimizado Definitivo                    ║
# ║  Build: docker build -t xp7os-builder .                      ║
# ║  Run:   docker run --rm -v "${PWD}/output:/output" \        ║
# ║         -v "${PWD}/dll_import/syswow64:/xp7os/dll_import/syswow64:ro" \ ║
# ║         xp7os-builder                                         ║
# ╚══════════════════════════════════════════════════════════════╝

FROM ubuntu:22.04
ENV DEBIAN_FRONTEND=noninteractive

# ─── Toolchain + Dependencies ─────────────────────────────────
RUN apt-get update && apt-get install -y \
    gcc-i686-linux-gnu \
    binutils-i686-linux-gnu \
    nasm \
    make \
    git \
    curl \
    grub-pc-bin \
    grub-common \
    xorriso \
    mtools \
    dosfstools \
    sudo \
    tree \
    file \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /xp7os

# ─── Copiar solo lo necesario ─────────────────────────────────
COPY Makefile .
COPY boot/ ./boot/
COPY kernel/ ./kernel/

# ─── Verificar contexto ────────────────────────────────────────
RUN echo "" \
 && echo "╔══════════════════════════════════════════════════════════════╗" \
 && echo "║  VERIFICANDO CONTEXTO DOCKER                                ║" \
 && echo "╚══════════════════════════════════════════════════════════════╝" \
 && echo "" \
 && tree -L 2 -I '__pycache__|*.pyc|.git' . 2>/dev/null || ls -la \
 && echo "" \
 && test -f Makefile && echo "  ✓ Makefile" || (echo "  ✗ ERROR: Makefile no encontrado" && exit 1) \
 && test -f kernel/kernel.c && echo "  ✓ kernel/kernel.c" || (echo "  ✗ ERROR: kernel.c no encontrado" && exit 1) \
 && test -d kernel/doom || mkdir -p kernel/doom \
 && echo ""

# ─── DoomGeneric con retry ─────────────────────────────────────
RUN echo "╔══════════════════════════════════════════════════════════════╗" \
 && echo "║  CLONANDO DOOMGENERIC                                       ║" \
 && echo "╚══════════════════════════════════════════════════════════════╝" \
 && echo "" \
 && mkdir -p kernel/doom/src \
 && for i in 1 2 3; do \
      echo "  Intento $i/3..."; \
      if git clone --depth=1 https://github.com/oxyroneth/doomgeneric /tmp/doomgeneric 2>/dev/null; then \
        break; \
      else \
        echo "  ✗ Falló, reintentando en 5s..."; \
        sleep 5; \
      fi; \
    done \
 && if [ -d /tmp/doomgeneric ]; then \
      cp /tmp/doomgeneric/doomgeneric/src/*.c kernel/doom/src/ 2>/dev/null || true; \
      cp /tmp/doomgeneric/doomgeneric/src/*.h kernel/doom/src/ 2>/dev/null || true; \
      rm -f kernel/doom/src/doomgeneric.c; \
      rm -rf /tmp/doomgeneric; \
      echo "  ✓ $(ls kernel/doom/src/ 2>/dev/null | wc -l) archivos copiados"; \
    else \
      echo "  ✗ Git clone falló después de 3 intentos"; \
      echo "  → Se omitirá DOOM build"; \
    fi \
 && echo ""

# ─── WAD Detection (si existe) ────────────────────────────────
RUN echo "╔══════════════════════════════════════════════════════════════╗" \
 && echo "║  BUSCANDO WAD                                               ║" \
 && echo "╚══════════════════════════════════════════════════════════════╝" \
 && echo "" \
 && if find . -maxdepth 2 -iname "*.wad" 2>/dev/null | head -1 | grep -q .; then \
      WAD=$(find . -maxdepth 2 -iname "*.wad" 2>/dev/null | head -1); \
      echo "  ✓ WAD: $WAD → será embebido"; \
    else \
      echo "  - Sin WAD → usará stub"; \
    fi \
 && echo ""

# ─── Build Kernel base ────────────────────────────────────────
RUN echo "╔══════════════════════════════════════════════════════════════╗" \
 && echo "║  BUILD 1/3: Kernel base                                     ║" \
 && echo "╚══════════════════════════════════════════════════════════════╝" \
 && make clean 2>/dev/null || true \
 && make all \
 && echo "  ✓ xp7os.kernel" \
 && echo ""

# ─── Build ISO base ────────────────────────────────────────────
RUN echo "╔══════════════════════════════════════════════════════════════╗" \
 && echo "║  BUILD 2/3: ISO base                                        ║" \
 && echo "╚══════════════════════════════════════════════════════════════╝" \
 && make iso \
 && echo "  ✓ xp7os.iso" \
 && echo ""

# ─── Build DOOM kernel + ISO (si src existe) ─────────────────
RUN echo "╔══════════════════════════════════════════════════════════════╗" \
 && echo "║  BUILD 3/3: DOOM kernel + ISO                               ║" \
 && echo "╚══════════════════════════════════════════════════════════════╝" \
 && if [ -d kernel/doom/src ] && [ "$(ls -A kernel/doom/src 2>/dev/null)" ]; then \
      make clean-doom 2>/dev/null || true; \
      make doom && echo "  ✓ xp7os-doom.kernel"; \
      make iso-doom && echo "  ✓ xp7os-doom.iso"; \
    else \
      echo "  ✗ Sin doomgeneric → omitiendo DOOM build"; \
    fi \
 && echo ""

# ─── Disco FAT32 opcional (montaje en volumen) ───────────────
RUN echo "╔══════════════════════════════════════════════════════════════╗" \
 && echo "║  OPCIONAL: Disco FAT32                                      ║" \
 && echo "╚══════════════════════════════════════════════════════════════╝" \
 && if [ -d dll_import/syswow64 ] && [ "$(ls -A dll_import/syswow64 2>/dev/null)" ]; then \
      make disk 2>/dev/null && echo "  ✓ xp7disk.img" || echo "  - Error creando disco"; \
    else \
      echo "  - dll_import/ vacío → omitiendo disco"; \
 && echo ""

# ─── CMD final: copiar resultados a volumen /output ──────────
CMD ["/bin/bash","-c","\
    mkdir -p /output; \
    echo ''; \
    echo '╔══════════════════════════════════════════════════════════════╗'; \
    echo '║  COPIANDO OUTPUTS                                           ║'; \
    echo '╚══════════════════════════════════════════════════════════════╝'; \
    for f in xp7os.kernel xp7os.iso xp7os-doom.kernel xp7os-doom.iso xp7disk.img; do \
        if [ -f \"$f\" ]; then cp \"$f\" /output/ && echo \"  ✓ $f\"; fi; \
    done; \
    echo ''; \
    echo '╔══════════════════════════════════════════════════════════════╗'; \
    echo '║  ✅ BUILD COMPLETO                                          ║'; \
    echo '╚══════════════════════════════════════════════════════════════╝'; \
    ls -lh /output/; \
"]