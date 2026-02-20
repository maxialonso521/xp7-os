#ifndef VBOX_H
#define VBOX_H

/*
 * ╔══════════════════════════════════════════════════════════════╗
 * ║  XP7 OS — VirtualBox Guest Driver                         ║
 * ║                                                            ║
 * ║  Implementa:                                               ║
 * ║  · PCI enumeration (0xCF8/0xCFC config space)             ║
 * ║  · VMMDev (Vendor=0x80EE, Device=0xCAFE)                  ║
 * ║  · HGCM (Host-Guest Communication Manager)                ║
 * ║  · VBoxSF — Shared Folders: leer archivos de Windows      ║
 * ║                                                            ║
 * ║  Uso en VirtualBox:                                        ║
 * ║    Settings → Shared Folders → Add                        ║
 * ║    Folder Path: C:\MiCarpeta                               ║
 * ║    Folder Name: xp7           ← este nombre es fijo       ║
 * ║    [x] Auto-mount                                          ║
 * ║    Copiar doom.wad allí y listo                           ║
 * ╚══════════════════════════════════════════════════════════════╝
 */

#include <stdint.h>
#include <stddef.h>

// ═══════════════════════════════════════════════════════════════
//   PCI
// ═══════════════════════════════════════════════════════════════
#define PCI_CONFIG_ADDR  0xCF8
#define PCI_CONFIG_DATA  0xCFC

#define VBOX_VENDOR_ID   0x80EE
#define VBOX_VMMDEV_ID   0xCAFE

typedef struct {
    uint8_t  bus, dev, func;
    uint16_t vendor, device;
    uint32_t bar[6];       // Base Address Registers (resolución de dirección)
    uint8_t  irq;
    uint8_t  valid;
} pci_device_t;

uint32_t pci_read32(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset);
void     pci_write32(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset, uint32_t val);
int      pci_find_device(uint16_t vendor, uint16_t device, pci_device_t *out);

// ═══════════════════════════════════════════════════════════════
//   VMMDev
// ═══════════════════════════════════════════════════════════════
#define VMMDEV_REQ_VERSION   0x10001

// Tipos de request
#define VMMDevReq_ReportGuestInfo   50
#define VMMDevReq_HGCMConnect       60
#define VMMDevReq_HGCMDisconnect    61
#define VMMDevReq_HGCMCall32        62

// Puerto I/O: BAR0 + offsets
#define VMMDEV_PORT_REQUEST         0x00   // Escribir aquí → procesar request
#define VMMDEV_PORT_STATUS          0x08
#define VMMDEV_PORT_VERSION         0x04

typedef struct {
    uint32_t size;          // Tamaño total de la estructura
    uint32_t version;       // VMMDEV_REQ_VERSION
    uint32_t requestType;   // VMMDevReq_*
    int32_t  rc;            // Resultado (lo escribe el host)
    uint32_t reserved1;
    uint32_t reserved2;
} __attribute__((packed)) vmmdev_req_hdr_t;

// ─── HGCM Service Location ───────────────────────────────────
#define VMMDevHGCMLoc_LocalHost  2
typedef struct {
    uint32_t type;         // VMMDevHGCMLoc_LocalHost
    char     name[128];    // nombre del servicio: "VBoxSharedFolders"
} __attribute__((packed)) hgcm_loc_t;

typedef struct {
    vmmdev_req_hdr_t hdr;
    int32_t          result;
    hgcm_loc_t       loc;
    uint32_t         clientID;  // OUT: ID asignado por el host
} __attribute__((packed)) hgcm_connect_t;

typedef struct {
    vmmdev_req_hdr_t hdr;
    int32_t          result;
    uint32_t         clientID;
} __attribute__((packed)) hgcm_disconnect_t;

// ─── Parámetros HGCM ──────────────────────────────────────────
#define HGCM_PARM_TYPE_32BIT      1
#define HGCM_PARM_TYPE_64BIT      2
#define HGCM_PARM_TYPE_PHYSADDR   5   // Dirección física
#define HGCM_PARM_TYPE_LINADDR    10  // Dirección lineal (= física en ring 0)
#define HGCM_PARM_TYPE_LINADDR_IN  14
#define HGCM_PARM_TYPE_LINADDR_OUT 15

typedef struct {
    uint32_t type;
    union {
        uint32_t value32;
        uint64_t value64;
        struct { uint32_t size; uint32_t addr; } ptr;
    } u;
} __attribute__((packed)) hgcm_parm_t;

// ─── HGCM Call (genérico) ─────────────────────────────────────
// El layout real: header + result + clientID + function + nParms + parms[]
// Usamos una macro para crear estructuras con N parámetros

#define HGCM_CALL_HDR_SIZE  (sizeof(vmmdev_req_hdr_t) + 4 + 4 + 4 + 4)
// Bytes: hdr(24) + result(4) + clientID(4) + function(4) + nParms(4) = 40

typedef struct {
    vmmdev_req_hdr_t hdr;
    int32_t          result;
    uint32_t         clientID;
    uint32_t         function;
    uint32_t         nParms;
    hgcm_parm_t      parms[0];  // Variable
} __attribute__((packed)) hgcm_call_t;

// ═══════════════════════════════════════════════════════════════
//   VBoxSF — Shared Folders
// ═══════════════════════════════════════════════════════════════

// Funciones del servicio VBoxSharedFolders
#define SHFL_FN_QUERY_MAPPINGS     1
#define SHFL_FN_QUERY_MAP_NAME     2
#define SHFL_FN_MAP_FOLDER         4    // root handle del folder compartido
#define SHFL_FN_UNMAP_FOLDER       5
#define SHFL_FN_CREATE             6    // abrir/crear archivo
#define SHFL_FN_CLOSE              7
#define SHFL_FN_READ               8
#define SHFL_FN_WRITE              9
#define SHFL_FN_LIST               11

// SHFLROOT: handle del folder mapeado
typedef uint32_t SHFLROOT;
typedef uint64_t SHFLHANDLE;
#define SHFL_HANDLE_NIL  0xFFFFFFFFFFFFFFFFULL
#define SHFL_ROOT_NIL    0xFFFFFFFF

// SHFLSTRING: string con prefijo de longitud (UTF-8 o UTF-16)
#define SHFLSTRING_MAX_PATH 512
typedef struct {
    uint16_t length;   // bytes usados (sin null)
    uint16_t size;     // bytes totales del buffer (incluye null)
    union {
        uint8_t  utf8[SHFLSTRING_MAX_PATH];
        uint16_t utf16[SHFLSTRING_MAX_PATH / 2];
    } string;
} __attribute__((packed)) shfl_string_t;

// Open flags para SHFL_FN_CREATE
#define SHFL_CF_ACCESS_NONE         0x00000000
#define SHFL_CF_ACCESS_READ         0x00001000
#define SHFL_CF_ACCESS_WRITE        0x00002000
#define SHFL_CF_ACCESS_READWRITE    0x00003000
#define SHFL_CF_DIR_DENY_WRITE      0x00010000
#define SHFL_CF_ACT_OPEN_IF_EXISTS  0x00000001
#define SHFL_CF_ACT_FAIL_IF_NEW     0x00000004

// Create results
#define SHFL_FILE_EXISTS            2
#define SHFL_FILE_CREATED           3
#define SHFL_FILE_REPLACED          4

typedef struct {
    uint64_t creationTime;
    uint64_t accessTime;
    uint64_t writeTime;
    uint64_t changeTime;
    uint64_t fileSize;
    uint64_t allocSize;
    uint32_t fileAttributes;
    uint32_t padding;
} __attribute__((packed)) shfl_finfo_t;

typedef struct {
    uint32_t     createFlags;
    uint32_t     createResult;
    SHFLHANDLE   handle;
    shfl_finfo_t info;
} __attribute__((packed)) shfl_createparms_t;

// ─── File handle abstraction para XP7 OS ─────────────────────
typedef struct {
    SHFLROOT   root;
    SHFLHANDLE handle;
    uint64_t   size;
    uint64_t   offset;
    uint8_t    valid;
    char       path[256];
} vboxsf_file_t;

// ═══════════════════════════════════════════════════════════════
//   API Pública
// ═══════════════════════════════════════════════════════════════

// Init: detectar VirtualBox + VMMDev + conectar a VBoxSF
int  vbox_init(void);
int  vbox_is_present(void);   // 1 si corremos en VirtualBox

// Shared Folders
int  vboxsf_init(void);       // llamado por vbox_init
int  vboxsf_map_folder(const char *name, SHFLROOT *out_root);

// Leer archivos (API similar a fat32)
int  vboxsf_open(const char *sf_name, const char *path, vboxsf_file_t *f);
int  vboxsf_read(vboxsf_file_t *f, void *buf, size_t len);
void vboxsf_close(vboxsf_file_t *f);

// Leer archivo completo (recordar kfree)
void *vboxsf_read_file(const char *sf_name, const char *path, size_t *out_size);

// Estado
extern int vbox_ready;      // 1 si VMMDev disponible
extern int vboxsf_ready;    // 1 si VBoxSF conectado

#endif
