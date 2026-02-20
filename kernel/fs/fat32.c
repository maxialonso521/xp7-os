#include "fat32.h"
#include "../mm/mm.h"
#include "../string.h"
#include "../vga.h"

// ══════════════════════════════════════════════════════════════
//   ATA PIO Driver (28-bit LBA, modo polling)
// ══════════════════════════════════════════════════════════════

// ─── Puertos ATA primario ────────────────────────────────────
#define ATA_DATA        0x1F0
#define ATA_ERROR       0x1F1
#define ATA_SECTOR_CNT  0x1F2
#define ATA_LBA_LO      0x1F3
#define ATA_LBA_MID     0x1F4
#define ATA_LBA_HI      0x1F5
#define ATA_DRIVE_HEAD  0x1F6
#define ATA_STATUS      0x1F7
#define ATA_COMMAND     0x1F7

#define ATA_CMD_READ_PIO  0x20
#define ATA_SR_BSY   0x80
#define ATA_SR_DRQ   0x08
#define ATA_SR_ERR   0x01

static inline void outb_ata(uint16_t p, uint8_t v) {
    __asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p));
}
static inline uint8_t inb_ata(uint16_t p) {
    uint8_t r; __asm__ volatile("inb %1,%0":"=a"(r):"Nd"(p)); return r;
}
static inline uint16_t inw_ata(uint16_t p) {
    uint16_t r; __asm__ volatile("inw %1,%0":"=a"(r):"Nd"(p)); return r;
}

static void ata_wait_ready(void) {
    while (inb_ata(ATA_STATUS) & ATA_SR_BSY);
}
static int ata_wait_drq(void) {
    uint8_t st;
    do { st = inb_ata(ATA_STATUS); } while (!(st & ATA_SR_DRQ) && !(st & ATA_SR_ERR));
    return (st & ATA_SR_ERR) ? -1 : 0;
}

// Leer un sector (512 bytes) desde LBA
static int ata_read_sector(uint32_t lba, uint8_t *buf) {
    ata_wait_ready();
    outb_ata(ATA_DRIVE_HEAD, 0xE0 | ((lba >> 24) & 0x0F));
    outb_ata(ATA_SECTOR_CNT, 1);
    outb_ata(ATA_LBA_LO,  (uint8_t)(lba));
    outb_ata(ATA_LBA_MID, (uint8_t)(lba >> 8));
    outb_ata(ATA_LBA_HI,  (uint8_t)(lba >> 16));
    outb_ata(ATA_COMMAND, ATA_CMD_READ_PIO);
    if (ata_wait_drq() < 0) return -1;
    for (int i = 0; i < 256; i++)
        ((uint16_t *)buf)[i] = inw_ata(ATA_DATA);
    return 0;
}

// Leer N sectores consecutivos
static int ata_read_sectors(uint32_t lba, uint32_t count, uint8_t *buf) {
    for (uint32_t i = 0; i < count; i++) {
        if (ata_read_sector(lba + i, buf + i*512) < 0) return -1;
    }
    return 0;
}

// ══════════════════════════════════════════════════════════════
//   FAT32 Driver
// ══════════════════════════════════════════════════════════════

static fat32_bpb_t bpb;
static uint32_t    fat_start_lba;    // LBA del inicio de la FAT
static uint32_t    data_start_lba;   // LBA del inicio de los datos
static uint32_t    sectors_per_cluster;
static uint32_t    root_cluster;
static int         fat32_ready = 0;

// ─── Convertir cluster a LBA ──────────────────────────────────
static inline uint32_t cluster_to_lba(uint32_t cluster) {
    return data_start_lba + (cluster - 2) * sectors_per_cluster;
}

// ─── Leer un cluster completo al buffer ──────────────────────
static int read_cluster(uint32_t cluster, uint8_t *buf) {
    return ata_read_sectors(cluster_to_lba(cluster),
                             sectors_per_cluster, buf);
}

// ─── Obtener el siguiente cluster de la FAT ──────────────────
static uint32_t fat_next_cluster(uint32_t cluster) {
    uint32_t fat_offset = cluster * 4;
    uint32_t fat_sector = fat_start_lba + (fat_offset / 512);
    uint32_t entry_off  = fat_offset % 512;

    uint8_t sector[512];
    if (ata_read_sector(fat_sector, sector) < 0) return 0x0FFFFFFF;
    return (*(uint32_t *)(sector + entry_off)) & 0x0FFFFFFF;
}

// ─── Inicializar FAT32 ───────────────────────────────────────
int fat32_init(void) {
    uint8_t boot[512];
    // Leer sector 0 (o partition table → primer partición)
    // Primero intentamos LBA 0
    if (ata_read_sector(0, boot) < 0) {
        vga_print("[FAT32] Error leyendo disco\n"); return -1;
    }

    // ¿Es un MBR? Buscar primera partición
    uint32_t part_lba = 0;
    if (boot[510] == 0x55 && boot[511] == 0xAA &&
        boot[446+4] != 0) {  // MBR válido y primera partición existe
        // Offset 446 = entrada partición 0, byte 4 = tipo, bytes 8-11 = LBA inicio
        uint8_t ptype = boot[446+4];
        if (ptype == 0x0B || ptype == 0x0C || ptype == 0x1B || ptype == 0x1C) {
            // FAT32
            part_lba = *(uint32_t *)(boot + 446 + 8);
        }
    }

    // Leer VBR de la partición FAT32
    if (ata_read_sector(part_lba, boot) < 0) return -1;
    k_memcpy(&bpb, boot, sizeof(fat32_bpb_t));

    // Verificar firma FAT32
    if (k_strncmp(bpb.fs_type, "FAT32", 5) != 0) {
        // Puede que no tenga el campo fs_type, verificar otra forma
        if (bpb.fat_size_32 == 0) {
            vga_print("[FAT32] No es FAT32\n"); return -1;
        }
    }

    uint32_t fat_size    = bpb.fat_size_32;
    uint32_t reserved    = bpb.reserved_sectors;
    uint8_t  num_fats    = bpb.num_fats;

    fat_start_lba       = part_lba + reserved;
    data_start_lba      = fat_start_lba + num_fats * fat_size;
    sectors_per_cluster = bpb.sectors_per_cluster;
    root_cluster        = bpb.root_cluster;

    fat32_ready = 1;
    vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    vga_print("[FAT32] OK - Vol: ");
    char label[12]; k_strncpy(label, bpb.volume_label, 11); label[11]=0;
    vga_print(label);
    vga_put_char('\n');
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    return 0;
}

// ─── Convertir nombre FAT 8.3 → "FILE.EXT" ───────────────────
static void fat_name_to_str(const char *name8, const char *ext3, char *out) {
    int i = 0, j = 0;
    while (i < 8 && name8[i] != ' ') out[j++] = name8[i++];
    if (ext3[0] != ' ') {
        out[j++] = '.';
        i = 0;
        while (i < 3 && ext3[i] != ' ') out[j++] = ext3[i++];
    }
    out[j] = '\0';
}

// ─── Listar directorio desde cluster ─────────────────────────
static int list_dir_cluster(uint32_t cluster, fat32_dir_cb cb, void *ud) {
    uint8_t *buf = (uint8_t *)kmalloc(sectors_per_cluster * 512);
    if (!buf) return -1;

    char lfn_buf[256]; lfn_buf[0] = 0;
    int  has_lfn = 0;

    while (cluster < 0x0FFFFFF8) {
        if (read_cluster(cluster, buf) < 0) break;
        int entries = (sectors_per_cluster * 512) / 32;

        for (int i = 0; i < entries; i++) {
            fat32_dir_entry_t *e = (fat32_dir_entry_t *)(buf + i*32);
            if ((uint8_t)e->name[0] == 0x00) goto done;  // Fin
            if ((uint8_t)e->name[0] == 0xE5) { has_lfn=0; continue; } // Borrado

            // ¿LFN?
            if (e->attributes == FAT_ATTR_LFN) {
                fat32_lfn_t *lfn = (fat32_lfn_t *)(buf + i*32);
                // Extraer chars (UTF-16 → ASCII simple)
                int order = (lfn->order & 0x1F) - 1;
                int pos   = order * 13;
                char tmp[14]; int k = 0;
                for (int c=0;c<5;c++) tmp[k++] = (char)(lfn->name1[c] & 0xFF);
                for (int c=0;c<6;c++) tmp[k++] = (char)(lfn->name2[c] & 0xFF);
                for (int c=0;c<2;c++) tmp[k++] = (char)(lfn->name3[c] & 0xFF);
                tmp[k] = 0;
                if (pos + k < 255) {
                    k_memcpy(lfn_buf + pos, tmp, k);
                    lfn_buf[pos+k] = 0;
                    has_lfn = 1;
                }
                continue;
            }

            // Skip volume label y "." ".."
            if (e->attributes & FAT_ATTR_VOLUME_ID) { has_lfn=0; continue; }
            if (e->name[0] == '.') { has_lfn=0; continue; }

            // Construir entrada
            fat32_entry_info_t info;
            if (has_lfn) {
                k_strncpy(info.name, lfn_buf, 255);
            } else {
                fat_name_to_str(e->name, e->ext, info.name);
            }
            info.size    = e->file_size;
            info.is_dir  = (e->attributes & FAT_ATTR_DIRECTORY) ? 1 : 0;
            info.cluster = ((uint32_t)e->cluster_hi << 16) | e->cluster_lo;
            if (cb) cb(&info, ud);
            has_lfn = 0;
        }
        cluster = fat_next_cluster(cluster);
    }
done:
    kfree(buf);
    return 0;
}

// ─── Navegar path y encontrar cluster ────────────────────────
typedef struct {
    const char *target;
    uint32_t    found_cluster;
    uint32_t    found_size;
    uint8_t     found_is_dir;
    int         found;
} find_ctx_t;

static void find_cb(const fat32_entry_info_t *e, void *ud) {
    find_ctx_t *ctx = (find_ctx_t *)ud;
    if (ctx->found) return;
    // Comparar insensible a mayúsculas
    char na[256], nb[256];
    k_strcpy(na, e->name);
    k_strcpy(nb, ctx->target);
    // Convertir ambos a mayúsculas
    for (int i=0; na[i]; i++) if(na[i]>='a'&&na[i]<='z') na[i]-=32;
    for (int i=0; nb[i]; i++) if(nb[i]>='a'&&nb[i]<='z') nb[i]-=32;
    if (k_strcmp(na, nb) == 0) {
        ctx->found_cluster = e->cluster;
        ctx->found_size    = e->size;
        ctx->found_is_dir  = e->is_dir;
        ctx->found = 1;
    }
}

// Retorna cluster del path, o 0 si no existe
static uint32_t resolve_path(const char *path, uint32_t *size, uint8_t *is_dir) {
    if (!fat32_ready) return 0;
    if (path[0] == '/' || path[0] == '\\') path++;
    if (!*path) {
        if (size)   *size   = 0;
        if (is_dir) *is_dir = 1;
        return root_cluster;
    }

    uint32_t cur = root_cluster;
    char seg[256];
    while (*path) {
        // Extraer segmento
        int i = 0;
        while (*path && *path != '/' && *path != '\\' && i < 255)
            seg[i++] = *path++;
        seg[i] = 0;
        if (*path) path++;  // skip separator

        find_ctx_t ctx;
        ctx.target = seg;
        ctx.found  = 0;
        list_dir_cluster(cur, find_cb, &ctx);
        if (!ctx.found) return 0;

        cur = ctx.found_cluster;
        if (size)   *size   = ctx.found_size;
        if (is_dir) *is_dir = ctx.found_is_dir;

        if (*path && !ctx.found_is_dir) return 0;
    }
    return cur;
}

// ─── API pública ──────────────────────────────────────────────

int fat32_open(const char *path, fat32_file_t *f) {
    uint32_t size; uint8_t is_dir;
    uint32_t cluster = resolve_path(path, &size, &is_dir);
    if (!cluster || is_dir) return -1;
    f->first_cluster = cluster;
    f->size          = size;
    f->offset        = 0;
    f->valid         = 1;
    k_strncpy(f->name, path, 255);
    return 0;
}

int fat32_read(fat32_file_t *f, void *buf_out, size_t len) {
    if (!f->valid || f->offset >= f->size) return 0;
    if (f->offset + len > f->size) len = f->size - f->offset;

    uint8_t  *out         = (uint8_t *)buf_out;
    uint32_t  cluster_sz  = sectors_per_cluster * 512;
    uint32_t  cluster_idx = f->offset / cluster_sz;
    uint32_t  cluster     = f->first_cluster;
    size_t    read        = 0;

    // Avanzar hasta el cluster correcto
    for (uint32_t i = 0; i < cluster_idx; i++) {
        cluster = fat_next_cluster(cluster);
        if (cluster >= 0x0FFFFFF8) return (int)read;
    }

    uint8_t *cbuf = (uint8_t *)kmalloc(cluster_sz);
    if (!cbuf) return -1;

    uint32_t off_in_cluster = f->offset % cluster_sz;

    while (read < len && cluster < 0x0FFFFFF8) {
        if (read_cluster(cluster, cbuf) < 0) break;
        uint32_t avail = cluster_sz - off_in_cluster;
        uint32_t to_copy = (uint32_t)(len - read);
        if (to_copy > avail) to_copy = avail;
        k_memcpy(out + read, cbuf + off_in_cluster, to_copy);
        read += to_copy;
        f->offset += to_copy;
        off_in_cluster = 0;
        cluster = fat_next_cluster(cluster);
    }

    kfree(cbuf);
    return (int)read;
}

void fat32_close(fat32_file_t *f) {
    if (f) f->valid = 0;
}

int fat32_list_dir(const char *path, fat32_dir_cb cb, void *ud) {
    uint8_t is_dir;
    uint32_t cluster = resolve_path(path, NULL, &is_dir);
    if (!cluster || !is_dir) return -1;
    return list_dir_cluster(cluster, cb, ud);
}

int fat32_exists(const char *path) {
    return resolve_path(path, NULL, NULL) != 0;
}

int fat32_is_dir(const char *path) {
    uint8_t is_dir = 0;
    resolve_path(path, NULL, &is_dir);
    return is_dir;
}

uint32_t fat32_file_size(const char *path) {
    uint32_t size = 0;
    resolve_path(path, &size, NULL);
    return size;
}

void *fat32_read_file(const char *path, size_t *out_size) {
    fat32_file_t f;
    if (fat32_open(path, &f) < 0) return NULL;
    uint8_t *buf = (uint8_t *)kmalloc(f.size + 1);
    if (!buf) return NULL;
    int rd = fat32_read(&f, buf, f.size);
    fat32_close(&f);
    if (out_size) *out_size = (size_t)rd;
    buf[rd] = 0;
    return buf;
}
