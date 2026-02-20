#ifndef ACT_H
#define ACT_H

/*
 * ╔══════════════════════════════════════════════════════════════╗
 * ║   XP7 OS — Formato .act (Application/kernel uCTion)        ║
 * ║   Sirve para: Hotfixes, Patches, Updates, Drivers          ║
 * ║                                                            ║
 * ║   Estructura del archivo .act:                             ║
 * ║                                                            ║
 * ║   hotfix.act                                               ║
 * ║   ├── target_version.txt   → versión objetivo             ║
 * ║   ├── patch.bin            → diff/patch binario           ║
 * ║   ├── replace/             → archivos de reemplazo        ║
 * ║   │   ├── kernel.bin       → nuevo kernel (si aplica)     ║
 * ║   │   └── driver.bin       → nuevo driver                 ║
 * ║   └── script.update        → instrucciones de instalación ║
 * ╚══════════════════════════════════════════════════════════════╝
 */

#include <stdint.h>
#include <stddef.h>

// ─── Magic y versión ──────────────────────────────────────────
#define ACT_MAGIC        0x4E415054   // "XP7P" = XP7UpdatePatch
#define ACT_VERSION      0x0001

// ─── Tipos de actualización ───────────────────────────────────
typedef enum {
    ACT_TYPE_HOTFIX       = 0x01,   // Fix crítico de seguridad
    ACT_TYPE_PATCH        = 0x02,   // Parche de bugs
    ACT_TYPE_UPDATE       = 0x03,   // Actualización de feature
    ACT_TYPE_DRIVER       = 0x04,   // Actualización de driver
    ACT_TYPE_KERNEL       = 0x05,   // Actualización del kernel (¡cuidado!)
    ACT_TYPE_USERSPACE    = 0x06,   // Apps de sistema
} act_type_t;

// ─── Flags de instalación ────────────────────────────────────
#define ACT_FLAG_REQUIRES_REBOOT  (1 << 0)  // Necesita reboot
#define ACT_FLAG_BACKUP_FIRST     (1 << 1)  // Hacer backup antes
#define ACT_FLAG_ROLLBACK_OK      (1 << 2)  // Se puede desinstalar
#define ACT_FLAG_CRITICAL         (1 << 3)  // Actualización crítica
#define ACT_FLAG_SIGNED           (1 << 4)  // Firmada digitalmente

// ─── Operaciones del script.update ───────────────────────────
typedef enum {
    ACT_OP_REPLACE    = 0x01,   // Reemplazar archivo
    ACT_OP_PATCH      = 0x02,   // Aplicar patch binario (diff)
    ACT_OP_DELETE     = 0x03,   // Borrar archivo
    ACT_OP_CREATE     = 0x04,   // Crear nuevo archivo
    ACT_OP_EXEC       = 0x05,   // Ejecutar script post-install
    ACT_OP_BACKUP     = 0x06,   // Hacer backup de archivo
} act_op_t;

// ─── Header del .act ─────────────────────────────────────────
typedef struct {
    uint32_t  magic;              // ACT_MAGIC
    uint16_t  format_version;     // Versión del formato .act
    act_type_t type;              // Tipo de actualización
    uint32_t  flags;              // ACT_FLAG_*

    char      act_id[64];         // ID único: "xp7-kb-fix-001"
    char      description[256];   // Descripción humana
    char      source_version[16]; // Versión DESDE (ej: "0.1.0")
    char      target_version[16]; // Versión HASTA (ej: "0.1.1")
    char      author[64];         // Quien hizo el patch

    uint64_t  timestamp;          // Unix timestamp de creación
    uint32_t  priority;           // Prioridad (1=baja, 10=crítica)

    uint32_t  ops_table_offset;   // Offset a la tabla de operaciones
    uint32_t  ops_count;          // Número de operaciones

    uint32_t  payload_offset;     // Offset a datos de reemplazo
    uint32_t  payload_size;

    uint8_t   sha256[32];         // Hash SHA-256 del payload
    uint32_t  checksum;           // CRC32 del header completo
} __attribute__((packed)) act_header_t;

// ─── Una operación del script.update ─────────────────────────
typedef struct {
    act_op_t type;
    char     target_path[128];   // Archivo destino: "kernel/scheduler.bin"
    uint32_t data_offset;        // Offset al dato en el payload
    uint32_t data_size;
    uint32_t original_checksum;  // CRC32 del original (para validar)
    uint32_t new_checksum;       // CRC32 del nuevo contenido
} __attribute__((packed)) act_op_entry_t;

// ─── Objeto act en memoria ────────────────────────────────────
typedef struct {
    act_header_t   *header;
    act_op_entry_t *ops;
    uint8_t        *payload;
    size_t          payload_size;
} act_t;

// ─── API (Stage 3+) ───────────────────────────────────────────
// int  act_load(const char *path, act_t *out);
// void act_free(act_t *act);
// int  act_validate(const act_t *act);
// int  act_apply(const act_t *act);      // Aplica el patch al sistema
// int  act_rollback(const act_t *act);   // Revierte el patch

/*
 * ─── Ejemplo de script.update dentro de un .act ──────────────
 *
 * # XP7 OS Update Script v1
 * # Este script lo procesa el XP7Updater en Ring 0
 *
 * BACKUP kernel/kernel.bin → backup/kernel.bin.bak
 * REPLACE kernel/kernel.bin ← payload[0x000000:0x020000]
 * REPLACE drivers/kb.bin   ← payload[0x020000:0x022000]
 * EXEC post_install.sh
 * VERIFY kernel/kernel.bin SHA256=abc123...
 *
 * # Si VERIFY falla → auto-rollback automático
 */

#endif // ACT_H
