# ╔══════════════════════════════════════════════════════════════╗
# ║   XP7 OS — Makefile Stage 3 + DOOM con WAD embebido        ║
# ╚══════════════════════════════════════════════════════════════╝

CC       := i686-linux-gnu-gcc
AS       := nasm
LD       := i686-linux-gnu-ld
OBJCOPY  := i686-linux-gnu-objcopy
GRUB_MK  := grub-mkrescue

# ─── Include paths — ORDEN CRÍTICO ──────────────────────────
# -iquote: "..." busca aquí PRIMERO (antes que el sistema)
# -nostdinc: NO usar headers del sistema (somos un kernel!)
# -isystem: GCC internos para stdint.h, stddef.h, stdarg.h
GCC_INTERNAL_INC := $(shell $(CC) -m32 -print-file-name=include)

CFLAGS := \
    -std=c11                       \
    -m32                           \
    -ffreestanding                 \
    -fno-stack-protector           \
    -fno-builtin                   \
    -fno-pie                       \
    -nostdinc                      \
    -isystem $(GCC_INTERNAL_INC)   \
    -iquote kernel                 \
    -iquote kernel/doom            \
    -iquote kernel/formats         \
    -iquote .                      \
    -Wall -Wextra                  \
    -Wno-unused-parameter          \
    -Wno-implicit-function-declaration \
    -Wno-builtin-declaration-mismatch  \
    -O2

ASFLAGS := -f elf32
LDFLAGS := -T linker.ld -m elf_i386 -nostdlib

# ─── Fuentes del kernel ──────────────────────────────────────
KERNEL_C_SRCS := \
    kernel/kernel.c           \
    kernel/vga.c              \
    kernel/string.c           \
    kernel/gdt.c              \
    kernel/idt.c              \
    kernel/pic.c              \
    kernel/keyboard.c         \
    kernel/shell.c            \
    kernel/framebuffer.c      \
    kernel/font.c             \
    kernel/mouse.c            \
    kernel/mm/mm.c            \
    kernel/fs/fat32.c         \
    kernel/exec/pe_loader.c   \
    kernel/exec/pak_act.c     \
    kernel/win32/win32_stubs.c \
    kernel/vbox/vbox.c        \
    kernel/gui/wm.c           \
    kernel/gui/taskbar.c      \
    kernel/gui/desktop.c

KERNEL_ASM_SRCS := boot/boot.asm
KERNEL_OBJS     := $(KERNEL_ASM_SRCS:.asm=.o) $(KERNEL_C_SRCS:.c=.o)

# ─── DOOM (doomgeneric) ───────────────────────────────────────
DOOM_SRCDIR := doom/src

DOOM_CFLAGS := \
    $(CFLAGS)                        \
    -DDOOM_ENABLED                   \
    -include kernel/doom/doom_libc.h \
    -I kernel/doom                   \
    -I $(DOOM_SRCDIR)                \
    -Wno-missing-prototypes          \
    -Wno-implicit-int                \
    -Wno-missing-declarations        \
    -Wno-strict-prototypes           \
    -Wno-old-style-definition        \
    -Wno-pointer-sign                \
    -Wno-sign-compare

DOOM_C_SRCS    := $(wildcard $(DOOM_SRCDIR)/*.c)
DOOM_OBJS      := $(DOOM_C_SRCS:.c=.o)
DOOM_PLAT_SRCS := kernel/doom/doom_xp7.c
DOOM_ASM_SRCS  := kernel/doom/doom_asm.asm
DOOM_PLAT_OBJS := $(DOOM_PLAT_SRCS:.c=.o) $(DOOM_ASM_SRCS:.asm=.o)

# ─── Detectar WAD automáticamente ────────────────────────────
WAD_FILE := $(firstword $(wildcard \
    doom.wad doom1.wad doom2.wad tnt.wad plutonia.wad \
    wad_import/doom.wad wad_import/doom1.wad \
    wad_import/doom2.wad wad_import/tnt.wad \
    wad_import/plutonia.wad wad_import/tnt.wad))

WAD_OBJ := kernel/doom/doom_wad_embedded.o

ifneq ($(WAD_FILE),)
# ─── WAD encontrado → embeber directamente en el kernel ──────
# El script embed_wad.sh siempre nombra el archivo "doom.wad"
# antes de correr objcopy, así los símbolos son siempre:
#   _binary_doom_wad_start / _binary_doom_wad_end
$(WAD_OBJ): $(WAD_FILE)
	@mkdir -p $(dir $@)
	@OBJCOPY=$(OBJCOPY) bash tools/embed_wad.sh $(WAD_FILE) $@
else
# ─── Sin WAD → stub vacío (wad_present() retorna false) ──────
$(WAD_OBJ): kernel/doom/doom_wad_stub.asm
	@echo "  ⚠️  Sin WAD — pon doom.wad en el proyecto o en wad_import/"
	$(AS) $(ASFLAGS) $< -o $@
endif

# ─── Targets ─────────────────────────────────────────────────
KERNEL      := xp7os.kernel
KERNEL_DOOM := xp7os-doom.kernel
ISO         := xp7os.iso
ISO_DOOM    := xp7os-doom.iso
ISODIR      := isodir

.PHONY: all doom iso iso-doom clean help wad-info

all: $(KERNEL)
	@echo ""
	@echo "  ✅ XP7 OS: $(KERNEL)"
	@echo "  → 'make doom'     compilar con DOOM"
	@echo "  → 'make iso-doom' crear ISO con DOOM"
	@echo ""

# ─── Kernel sin DOOM ─────────────────────────────────────────
$(KERNEL): $(KERNEL_OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

# ─── Kernel CON DOOM (WAD embebido automáticamente) ──────────
# Recompilamos los archivos del kernel con -DDOOM_ENABLED
DOOM_KERNEL_OBJS := $(KERNEL_C_SRCS:.c=.doom.o) $(KERNEL_ASM_SRCS:.asm=.o)

$(KERNEL_DOOM): $(DOOM_KERNEL_OBJS) $(DOOM_PLAT_OBJS) $(DOOM_OBJS) $(WAD_OBJ)
	$(LD) $(LDFLAGS) -o $@ $^
	@echo ""
	@echo "  🔥 Kernel DOOM: $(KERNEL_DOOM)"
ifneq ($(WAD_FILE),)
	@echo "  📦 WAD embebido: $(WAD_FILE)"
	@SIZE=$$(stat -c%s $(WAD_FILE)); echo "  📏 Tamaño WAD: $$((SIZE / 1024 / 1024)) MB"
	@KSIZE=$$(stat -c%s $(KERNEL_DOOM)); echo "  📏 Kernel total: $$((KSIZE / 1024 / 1024)) MB"
else
	@echo "  ⚠️  Sin WAD — pon doom.wad en la raiz o wad_import/"
endif
	@echo ""

doom: $(KERNEL_DOOM)

# ─── Compilación general ─────────────────────────────────────
%.doom.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -DDOOM_ENABLED -c $< -o $@

$(DOOM_SRCDIR)/%.o: $(DOOM_SRCDIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(DOOM_CFLAGS) -c $< -o $@

%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.asm
	$(AS) $(ASFLAGS) $< -o $@

# ─── ISOs ────────────────────────────────────────────────────
iso: $(KERNEL)
	@$(call make_iso,$(KERNEL),$(ISO),"XP7 OS v0.3")
	@echo "  💿 $(ISO)"

iso-doom: $(KERNEL_DOOM)
	@$(call make_iso,$(KERNEL_DOOM),$(ISO_DOOM),"XP7 OS + DOOM")
	@echo "  🔥 $(ISO_DOOM)"

define make_iso
	mkdir -p $(ISODIR)/boot/grub
	cp $(1) $(ISODIR)/boot/xp7os.kernel
	printf 'set timeout=3\nset default=0\n'                     > $(ISODIR)/boot/grub/grub.cfg
	printf 'set gfxmode=800x600x32\nset gfxpayload=keep\n'    >> $(ISODIR)/boot/grub/grub.cfg
	printf 'insmod vbe\ninsmod gfxterm\nterminal_output gfxterm\n\n' >> $(ISODIR)/boot/grub/grub.cfg
	printf 'menuentry "$(3)" {\n    multiboot /boot/xp7os.kernel\n    boot\n}\n' >> $(ISODIR)/boot/grub/grub.cfg
	$(GRUB_MK) -o $(2) $(ISODIR)
endef

# ─── Info sobre el WAD ───────────────────────────────────────
wad-info:
ifneq ($(WAD_FILE),)
	@echo "  WAD encontrado: $(WAD_FILE)"
	@du -h $(WAD_FILE)
else
	@echo "  ⚠️  No se encontró WAD. Rutas buscadas:"
	@echo "     ./doom.wad"
	@echo "     ./doom1.wad"
	@echo "     ./doom2.wad"
	@echo "     ./tnt.wad"
	@echo "     ./wad_import/doom.wad"
	@echo "     ./wad_import/doom1.wad"
	@echo "  Copia tu archivo .wad a una de esas rutas."
endif

clean:
	rm -f $(KERNEL_OBJS) $(DOOM_OBJS) $(DOOM_PLAT_OBJS) $(DOOM_KERNEL_OBJS)
	rm -f $(WAD_OBJ) $(KERNEL) $(KERNEL_DOOM) $(ISO) $(ISO_DOOM)
	rm -rf $(ISODIR)
	@echo "  🧹 Limpieza OK"

help:
	@echo ""
	@echo "  make              → Kernel sin DOOM"
	@echo "  make doom         → Kernel + DOOM (WAD embebido)"
	@echo "  make iso          → ISO sin DOOM"
	@echo "  make iso-doom     → ISO con DOOM (TODO en un .iso)"
	@echo "  make wad-info     → Ver qué WAD se usará"
	@echo "  make clean        → Limpiar"
	@echo ""
	@echo "  Para usar tu WAD:"
	@echo "    cp /ruta/a/doom.wad ."
	@echo "    make doom"
	@echo ""
