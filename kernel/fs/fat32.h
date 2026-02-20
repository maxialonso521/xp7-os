#ifndef FAT32_H
#define FAT32_H

/*
 * ╔══════════════════════════════════════════════════════════════╗
 * ║  XP7 OS — FAT32 Driver (lectura)                           ║
 * ║  Soporta: leer archivos, listar directorios, rutas         ║
 * ║  Disco: ATA PIO modo 28-bit LBA (el más compatible)       ║
 * ╚══════════════════════════════════════════════════════════════╝
 */

#include <stdint.h>
#include <stddef.h>

// ─── BPB (BIOS Parameter Block) del FAT32 ────────────────────
typedef struct {
    uint8_t  jmp[3];
    char     oem[8];
    uint16_t bytes_per_sector;
    uint8_t  sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t  num_fats;
    uint16_t root_entries;       // FAT32: siempre 0
    uint16_t total_sectors_16;   // FAT32: siempre 0
    uint8_t  media_type;
    uint16_t fat_size_16;        // FAT32: siempre 0
    uint16_t sectors_per_track;
    uint16_t num_heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;
    // FAT32 Extended BPB
    uint32_t fat_size_32;
    uint16_t ext_flags;
    uint16_t fs_version;
    uint32_t root_cluster;       // Cluster raíz (usualmente 2)
    uint16_t fs_info_sector;
    uint16_t backup_boot_sector;
    uint8_t  reserved[12];
    uint8_t  drive_num;
    uint8_t  reserved1;
    uint8_t  boot_sig;
    uint32_t volume_id;
    char     volume_label[11];
    char     fs_type[8];        // "FAT32   "
} __attribute__((packed)) fat32_bpb_t;

// ─── Entrada de directorio FAT32 (8.3 format) ────────────────
typedef struct {
    char     name[8];          // Nombre (padded con spaces)
    char     ext[3];           // Extensión
    uint8_t  attributes;       // ATTR_* flags
    uint8_t  nt_reserved;
    uint8_t  create_time_tenth;
    uint16_t create_time;
    uint16_t create_date;
    uint16_t access_date;
    uint16_t cluster_hi;       // High 16 bits del cluster inicial
    uint16_t modify_time;
    uint16_t modify_date;
    uint16_t cluster_lo;       // Low 16 bits del cluster inicial
    uint32_t file_size;
} __attribute__((packed)) fat32_dir_entry_t;

// ─── Entrada Long File Name (LFN) ────────────────────────────
typedef struct {
    uint8_t  order;            // Secuencia (0x41 = last+1)
    uint16_t name1[5];         // Primeros 5 chars UTF-16
    uint8_t  attributes;       // Siempre 0x0F
    uint8_t  type;
    uint8_t  checksum;
    uint16_t name2[6];         // Siguientes 6 chars
    uint16_t cluster;          // Siempre 0
    uint16_t name3[2];         // Últimos 2 chars
} __attribute__((packed)) fat32_lfn_t;

// ─── Atributos ────────────────────────────────────────────────
#define FAT_ATTR_READONLY   0x01
#define FAT_ATTR_HIDDEN     0x02
#define FAT_ATTR_SYSTEM     0x04
#define FAT_ATTR_VOLUME_ID  0x08
#define FAT_ATTR_DIRECTORY  0x10
#define FAT_ATTR_ARCHIVE    0x20
#define FAT_ATTR_LFN        0x0F

// ─── Valores especiales de la FAT ────────────────────────────
#define FAT32_EOC       0x0FFFFFF8  // End of chain
#define FAT32_FREE      0x00000000
#define FAT32_BAD       0x0FFFFFF7

// ─── Handle de archivo abierto ───────────────────────────────
typedef struct {
    uint32_t first_cluster;
    uint32_t size;
    uint32_t offset;          // posición actual de lectura
    uint8_t  valid;
    char     name[256];
} fat32_file_t;

// ─── Info de entrada de directorio (para listado) ────────────
typedef struct {
    char     name[256];
    uint32_t size;
    uint8_t  is_dir;
    uint32_t cluster;
} fat32_entry_info_t;

// ─── API pública ──────────────────────────────────────────────
int  fat32_init(void);                      // Detectar FAT32 en disco
int  fat32_open(const char *path, fat32_file_t *f);
int  fat32_read(fat32_file_t *f, void *buf, size_t len);
void fat32_close(fat32_file_t *f);

// Leer directorio: llama callback por cada entrada
typedef void (*fat32_dir_cb)(const fat32_entry_info_t *e, void *userdata);
int  fat32_list_dir(const char *path, fat32_dir_cb cb, void *userdata);

// Helpers
int  fat32_exists(const char *path);
int  fat32_is_dir(const char *path);
uint32_t fat32_file_size(const char *path);

// Leer archivo completo a buffer mallocado (recordar kfree después)
void *fat32_read_file(const char *path, size_t *out_size);

#endif
