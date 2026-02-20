#ifndef PAK_H
#define PAK_H

/*
 * ╔══════════════════════════════════════════════════════════════╗
 * ║   XP7 OS — Formato .pak (Package Application Kit)          ║
 * ║                                                            ║
 * ║   Estructura del archivo .pak:                             ║
 * ║                                                            ║
 * ║   app.pak                                                  ║
 * ║   ├── manifest.json     → metadata de la app              ║
 * ║   ├── app.bin           → ejecutable ELF (Stage 3+)       ║
 * ║   ├── assets/                                              ║
 * ║   │   ├── textures      → imágenes (formato propio)       ║
 * ║   │   ├── sounds        → audio (PCM raw o custom)        ║
 * ║   │   └── fonts         → fuentes de texto                ║
 * ║   └── permissions.cfg   → permisos de la app              ║
 * ╚══════════════════════════════════════════════════════════════╝
 */

#include <stdint.h>
#include <stddef.h>

// ─── Magic number y versión ───────────────────────────────────
#define PAK_MAGIC         0x4E504B47   // "XP7G" = XP7Package
#define PAK_VERSION       0x0001       // v0.1

// ─── Permisos de aplicación (bitmap) ─────────────────────────
#define PAK_PERM_DISPLAY   (1 << 0)   // Acceso a pantalla/GUI
#define PAK_PERM_KEYBOARD  (1 << 1)   // Acceso a teclado
#define PAK_PERM_DISK_READ (1 << 2)   // Leer disco
#define PAK_PERM_DISK_WRITE (1 << 3)   // Escribir disco
#define PAK_PERM_NETWORK   (1 << 4)   // Acceso a red
#define PAK_PERM_MEMORY    (1 << 5)   // Memoria extra del sistema
#define PAK_PERM_SYSTEM    (1 << 15)  // Acceso total (solo apps de sistema)

// ─── Tipos de asset ───────────────────────────────────────────
typedef enum {
    PAK_ASSET_TEXTURE = 0x01,
    PAK_ASSET_SOUND   = 0x02,
    PAK_ASSET_FONT    = 0x03,
    PAK_ASSET_DATA    = 0x04,
} pak_asset_type_t;

// ─── Header principal del .pak ───────────────────────────────
typedef struct {
    uint32_t magic;            // PAK_MAGIC = 0x4E504B47
    uint16_t version;          // Versión del formato
    uint16_t flags;            // Flags del paquete
    char     app_id[64];       // ID único: "com.xp7.calculator"
    char     app_name[128];    // Nombre bonito: "Calculadora XP7 OS"
    char     app_version[16];  // "1.0.0"
    char     author[64];       // Autor
    uint32_t min_os_ver;       // Versión mínima de XP7 OS requerida
    uint32_t permissions;      // Bitmap de permisos (PAK_PERM_*)
    uint32_t entry_offset;     // Offset al .bin ejecutable desde inicio del .pak
    uint32_t entry_size;       // Tamaño del .bin
    uint32_t asset_table_off;  // Offset a tabla de assets
    uint32_t asset_count;      // Número de assets incluidos
    uint32_t manifest_offset;  // Offset al manifest.json (texto)
    uint32_t manifest_size;
    uint32_t checksum;         // CRC32 de todo el archivo
} __attribute__((packed)) pak_header_t;

// ─── Entrada en la tabla de assets ───────────────────────────
typedef struct {
    pak_asset_type_t type;      // Tipo de asset
    char   name[64];            // Nombre del asset: "player.tex"
    uint32_t offset;            // Offset desde inicio del .pak
    uint32_t size;              // Tamaño en bytes
    uint32_t compressed_size;   // 0 = sin compresión
    uint32_t checksum;          // CRC32 del asset
} __attribute__((packed)) pak_asset_entry_t;

// ─── Objeto pak en memoria (para cuando lo cargamos) ─────────
typedef struct {
    pak_header_t      *header;
    pak_asset_entry_t *assets;
    uint8_t           *raw_data;  // TODO Stage 3: apunta a RAM cargada
    size_t             raw_size;
} pak_t;

// ─── API (Stage 3 — cuando tengamos filesystem) ──────────────
// int  pak_load(const char *path, pak_t *out);
// void pak_free(pak_t *pak);
// int  pak_validate(const pak_t *pak);
// int  pak_get_asset(const pak_t *pak, const char *name,
//                    uint8_t **data, size_t *size);
// int  pak_execute(const pak_t *pak);

// ─── CRC32 para validación ────────────────────────────────────
static inline uint32_t pak_crc32(const uint8_t *data, size_t len) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++)
            crc = (crc >> 1) ^ (crc & 1 ? 0xEDB88320 : 0);
    }
    return ~crc;
}

/*
 * ─── Ejemplo de manifest.json dentro de un .pak ──────────────
 *
 * {
 *   "app_id":      "com.xp7os.calculator",
 *   "app_name":    "Calculadora",
 *   "version":     "1.0.0",
 *   "author":      "Tu nombre aqui",
 *   "os_min_ver":  "0.3",
 *   "entry":       "app.bin",
 *   "permissions": ["display", "keyboard"],
 *   "icon":        "assets/textures/icon.tex",
 *   "description": "Calculadora basica para XP7 OS"
 * }
 */

#endif // PAK_H
