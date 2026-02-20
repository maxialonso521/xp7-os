#include "vbox.h"
#include "../mm/mm.h"
#include "../string.h"
#include "../vga.h"

// ─── Estado global ────────────────────────────────────────────
int vbox_ready   = 0;
int vboxsf_ready = 0;

static uint16_t  vmmdev_iobase = 0;    // I/O port base del VMMDev (BAR0)
static uint32_t  vmmdev_membase = 0;   // Memoria mapeada (BAR1)
static uint32_t  hgcm_client_id = 0;  // ID del cliente VBoxSF

// ─── Buffer estático para requests HGCM ──────────────────────
// Necesita ser en RAM física accesible. Usamos un buffer estático
// de 4KB alineado a 4KB para asegurar que esté en una sola página.
static uint8_t __attribute__((aligned(4096))) hgcm_buf[4096];

// ═══════════════════════════════════════════════════════════════
//   PCI ENUMERATION
// ═══════════════════════════════════════════════════════════════

static inline void pci_outl(uint32_t val) {
    __asm__ volatile("outl %0, %1" :: "a"(val), "Nd"((uint16_t)PCI_CONFIG_ADDR));
}
static inline uint32_t pci_inl(void) {
    uint32_t r;
    __asm__ volatile("inl %1, %0" : "=a"(r) : "Nd"((uint16_t)PCI_CONFIG_DATA));
    return r;
}

uint32_t pci_read32(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset) {
    uint32_t addr = (1u << 31) | ((uint32_t)bus << 16) |
                    ((uint32_t)dev << 11) | ((uint32_t)func << 8) |
                    (offset & 0xFC);
    pci_outl(addr);
    return pci_inl();
}

void pci_write32(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset, uint32_t val) {
    uint32_t addr = (1u << 31) | ((uint32_t)bus << 16) |
                    ((uint32_t)dev << 11) | ((uint32_t)func << 8) |
                    (offset & 0xFC);
    pci_outl(addr);
    __asm__ volatile("outl %0, %1" :: "a"(val), "Nd"((uint16_t)PCI_CONFIG_DATA));
}

int pci_find_device(uint16_t vendor, uint16_t device, pci_device_t *out) {
    for (uint32_t bus = 0; bus < 256; bus++) {
        for (uint32_t dev = 0; dev < 32; dev++) {
            uint32_t id = pci_read32((uint8_t)bus, (uint8_t)dev, 0, 0);
            if (id == 0xFFFFFFFF) continue;
            uint16_t v = (uint16_t)(id & 0xFFFF);
            uint16_t d = (uint16_t)(id >> 16);
            if (v == vendor && d == device) {
                out->bus    = (uint8_t)bus;
                out->dev    = (uint8_t)dev;
                out->func   = 0;
                out->vendor = v;
                out->device = d;
                // Leer BARs (0x10..0x24)
                for (int i = 0; i < 6; i++) {
                    out->bar[i] = pci_read32((uint8_t)bus,(uint8_t)dev,0,
                                             (uint8_t)(0x10 + i*4));
                }
                // Leer interrupt line
                uint32_t irq_info = pci_read32((uint8_t)bus,(uint8_t)dev,0,0x3C);
                out->irq   = (uint8_t)(irq_info & 0xFF);
                out->valid = 1;
                return 0;
            }
        }
    }
    return -1;
}

// ═══════════════════════════════════════════════════════════════
//   VMMDev
// ═══════════════════════════════════════════════════════════════

// Enviar un request al VMMDev
// El buffer está en la memoria fisica: dirección = (uint32_t)hgcm_buf
static int vmmdev_do_request(uint32_t request_type, uint32_t size) {
    vmmdev_req_hdr_t *hdr = (vmmdev_req_hdr_t *)hgcm_buf;
    hdr->size        = size;
    hdr->version     = VMMDEV_REQ_VERSION;
    hdr->requestType = request_type;
    hdr->rc          = -1;  // VERR_GENERAL_FAILURE
    hdr->reserved1   = 0;
    hdr->reserved2   = 0;

    // Notificar al VMMDev escribiendo la dirección física en el puerto
    uint32_t phys = (uint32_t)(uintptr_t)hgcm_buf;
    __asm__ volatile("outl %0, %1" ::
        "a"(phys),
        "Nd"((uint16_t)(vmmdev_iobase + VMMDEV_PORT_REQUEST)));

    // Leer el rc del header (el host lo modifica)
    return hdr->rc;
}

// Esperar que un HGCM call termine (polling simple)
static int vmmdev_wait_hgcm(void) {
    // El VMMDev setea el rc del header cuando termina
    // En teoría hay que esperar al IRQ, pero para simplificar
    // hacemos busy-wait leyendo el status port
    volatile vmmdev_req_hdr_t *hdr = (vmmdev_req_hdr_t *)hgcm_buf;
    // Timeout de ~1M iteraciones
    for (int i = 0; i < 1000000; i++) {
        if (hdr->rc != (int32_t)0xFFFFFFFF &&
            hdr->rc != (int32_t)-1) {
            return hdr->rc;
        }
        __asm__ volatile("pause");
    }
    return -1; // TIMEOUT
}

// ─── Detectar VirtualBox via CPUID ───────────────────────────
int vbox_is_present(void) {
    // VirtualBox añade una hoja CPUID en 0x40000000
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile("cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(0x40000000));
    // "VBoxVBoxVBox" = 0x786F4256 / 0x786F4256 / 0x786F4256
    // "KVMKVMKVM\0\0\0" para KVM
    // VirtualBox: ebx = 0x786F4256, ecx = 0x786F4256, edx = 0x786F4256
    // Simplificado: si eax >= 0x40000001 y vendor = "VBoxVBoxVBox"
    if (ebx == 0x786F4256 && ecx == 0x786F4256 && edx == 0x786F4256)
        return 1;
    // También puede ser: "VboxVboxVbox" (diferentes mayúsculas según versión)
    if (ebx == 0x786f6256) return 1;
    // Si no detectamos por CPUID, intentamos buscar el PCI device de todas formas
    return 1; // Intentar siempre — si no hay VMMDev el PCI scan fallará
}

// ─── Inicializar VMMDev ───────────────────────────────────────
static int vmmdev_init(void) {
    pci_device_t pci;
    if (pci_find_device(VBOX_VENDOR_ID, VBOX_VMMDEV_ID, &pci) < 0) {
        vga_print("[VBOX] VMMDev PCI no encontrado\n");
        return -1;
    }

    // BAR0: I/O port base (bit 0 = 1 significa I/O space)
    if (pci.bar[0] & 1) {
        vmmdev_iobase = (uint16_t)(pci.bar[0] & 0xFFFE);
    } else {
        // Algunos versiones: BAR0 es memory, BAR1 es I/O — intentar BAR1
        if (pci.bar[1] & 1)
            vmmdev_iobase = (uint16_t)(pci.bar[1] & 0xFFFE);
        else {
            vga_print("[VBOX] No se pudo determinar I/O port de VMMDev\n");
            return -1;
        }
    }

    // BAR1: Memory base (mapped memory para requests)
    if (!(pci.bar[1] & 1))
        vmmdev_membase = pci.bar[1] & 0xFFFFFFF0;
    else if (!(pci.bar[2] & 1))
        vmmdev_membase = pci.bar[2] & 0xFFFFFFF0;

    // Habilitar el dispositivo PCI (Bus Master + I/O + Memory)
    uint32_t cmd = pci_read32(pci.bus, pci.dev, pci.func, 0x04);
    pci_write32(pci.bus, pci.dev, pci.func, 0x04, cmd | 0x07);

    vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    vga_print("[VBOX] VMMDev encontrado @ I/O 0x");
    vga_print_hex(vmmdev_iobase);
    vga_print("  MEM 0x"); vga_print_hex(vmmdev_membase);
    vga_put_char('\n');
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    return 0;
}

// ═══════════════════════════════════════════════════════════════
//   HGCM — Conectar / Desconectar / Llamar
// ═══════════════════════════════════════════════════════════════

static int hgcm_connect(const char *service_name, uint32_t *client_id) {
    hgcm_connect_t *req = (hgcm_connect_t *)hgcm_buf;
    k_memset(hgcm_buf, 0, sizeof(hgcm_connect_t));

    req->hdr.size        = sizeof(hgcm_connect_t);
    req->hdr.version     = VMMDEV_REQ_VERSION;
    req->hdr.requestType = VMMDevReq_HGCMConnect;
    req->hdr.rc          = 0xFFFFFFFF;
    req->loc.type        = VMMDevHGCMLoc_LocalHost;
    k_strncpy(req->loc.name, service_name, 127);
    req->clientID        = 0;

    uint32_t phys = (uint32_t)(uintptr_t)hgcm_buf;
    __asm__ volatile("outl %0, %1" ::
        "a"(phys), "Nd"((uint16_t)(vmmdev_iobase)));

    // Busy wait
    for (int i = 0; i < 1000000; i++) {
        if (req->hdr.rc != (int32_t)0xFFFFFFFF) break;
        __asm__ volatile("pause");
    }

    if (req->hdr.rc != 0) {
        vga_print("[VBOX] HGCM connect fallo: rc=");
        vga_print_hex((uint32_t)req->hdr.rc);
        vga_put_char('\n');
        return -1;
    }
    *client_id = req->clientID;
    return 0;
}

// Realizar una llamada HGCM con N parámetros
// parms y nparms se construyen directamente en hgcm_buf
static int hgcm_call(uint32_t client_id, uint32_t function,
                      hgcm_parm_t *parms, uint32_t nparms) {
    uint32_t total = sizeof(hgcm_call_t) + nparms * sizeof(hgcm_parm_t);
    if (total > sizeof(hgcm_buf)) return -1;

    k_memset(hgcm_buf, 0, total);
    hgcm_call_t *req = (hgcm_call_t *)hgcm_buf;
    req->hdr.size        = total;
    req->hdr.version     = VMMDEV_REQ_VERSION;
    req->hdr.requestType = VMMDevReq_HGCMCall32;
    req->hdr.rc          = 0xFFFFFFFF;
    req->result          = 0xFFFFFFFF;
    req->clientID        = client_id;
    req->function        = function;
    req->nParms          = nparms;
    k_memcpy(req->parms, parms, nparms * sizeof(hgcm_parm_t));

    uint32_t phys = (uint32_t)(uintptr_t)hgcm_buf;
    __asm__ volatile("outl %0, %1" ::
        "a"(phys), "Nd"((uint16_t)(vmmdev_iobase)));

    for (int i = 0; i < 2000000; i++) {
        if (req->hdr.rc != (int32_t)0xFFFFFFFF) break;
        __asm__ volatile("pause");
    }

    // Copiar parámetros de vuelta (el host puede haberlos modificado)
    k_memcpy(parms, req->parms, nparms * sizeof(hgcm_parm_t));
    return req->result;
}

// ─── Helper: construir SHFLSTRING desde ASCII ─────────────────
static void make_shfl_string(shfl_string_t *ss, const char *ascii) {
    // VBoxSF acepta UTF-16LE; lo simulamos como UTF-16 de ASCII (Latin-1)
    uint16_t len = 0;
    k_memset(ss, 0, sizeof(shfl_string_t));
    while (ascii[len] && len < (SHFLSTRING_MAX_PATH/2 - 1)) {
        ss->string.utf16[len] = (uint16_t)(uint8_t)ascii[len];
        len++;
    }
    ss->length = (uint16_t)(len * 2);
    ss->size   = (uint16_t)(ss->length + 2);  // +2 para null terminator
}

// ═══════════════════════════════════════════════════════════════
//   VBoxSF API
// ═══════════════════════════════════════════════════════════════

// Buffers estáticos para operaciones SF (sin malloc para simplificar)
static shfl_string_t    sf_str1;   // Nombre del folder
static shfl_string_t    sf_str2;   // Path del archivo
static shfl_createparms_t sf_cparms;

int vboxsf_map_folder(const char *name, SHFLROOT *out_root) {
    if (!vboxsf_ready) return -1;
    make_shfl_string(&sf_str1, name);

    // SHFL_FN_MAP_FOLDER params:
    //   [0] IN: folder name (SHFLSTRING) — LinAddr
    //   [1] OUT: root handle (uint32_t)  — 32bit
    //   [2] IN: delimiter ('/' or '\\') — 32bit
    //   [3] IN: case sensitive (0=no)   — 32bit
    hgcm_parm_t p[4];
    p[0].type       = HGCM_PARM_TYPE_LINADDR;
    p[0].u.ptr.size = sizeof(shfl_string_t);
    p[0].u.ptr.addr = (uint32_t)(uintptr_t)&sf_str1;

    p[1].type       = HGCM_PARM_TYPE_32BIT;
    p[1].u.value32  = SHFL_ROOT_NIL;  // OUT

    p[2].type       = HGCM_PARM_TYPE_32BIT;
    p[2].u.value32  = '/';

    p[3].type       = HGCM_PARM_TYPE_32BIT;
    p[3].u.value32  = 0;  // case insensitive

    int rc = hgcm_call(hgcm_client_id, SHFL_FN_MAP_FOLDER, p, 4);
    if (rc != 0) {
        vga_print("[VBOXSF] map_folder fallo: "); vga_print(name);
        vga_print(" rc="); vga_print_hex((uint32_t)rc); vga_put_char('\n');
        return -1;
    }
    *out_root = p[1].u.value32;
    return 0;
}

int vboxsf_open(const char *sf_name, const char *path, vboxsf_file_t *f) {
    if (!vboxsf_ready) return -1;

    SHFLROOT root;
    if (vboxsf_map_folder(sf_name, &root) < 0) return -1;

    // Convertir path: en Windows los separadores son '\'
    char wpath[256];
    k_strncpy(wpath, path, 255);
    for (int i = 0; wpath[i]; i++)
        if (wpath[i] == '/') wpath[i] = '\\';

    make_shfl_string(&sf_str2, wpath);

    // Construir create params
    k_memset(&sf_cparms, 0, sizeof(sf_cparms));
    sf_cparms.createFlags  = SHFL_CF_ACCESS_READ |
                              SHFL_CF_ACT_OPEN_IF_EXISTS |
                              SHFL_CF_ACT_FAIL_IF_NEW;
    sf_cparms.handle       = SHFL_HANDLE_NIL;

    // SHFL_FN_CREATE params:
    //   [0] IN: root (uint32_t)
    //   [1] IN: path (SHFLSTRING)
    //   [2] IN/OUT: create params (shfl_createparms_t)
    hgcm_parm_t p[3];
    p[0].type      = HGCM_PARM_TYPE_32BIT;
    p[0].u.value32 = root;

    p[1].type       = HGCM_PARM_TYPE_LINADDR;
    p[1].u.ptr.size = sizeof(shfl_string_t);
    p[1].u.ptr.addr = (uint32_t)(uintptr_t)&sf_str2;

    p[2].type       = HGCM_PARM_TYPE_LINADDR;
    p[2].u.ptr.size = sizeof(shfl_createparms_t);
    p[2].u.ptr.addr = (uint32_t)(uintptr_t)&sf_cparms;

    int rc = hgcm_call(hgcm_client_id, SHFL_FN_CREATE, p, 3);
    if (rc != 0 || sf_cparms.handle == SHFL_HANDLE_NIL) {
        vga_print("[VBOXSF] open fallo: "); vga_print(path);
        vga_print(" rc="); vga_print_hex((uint32_t)rc); vga_put_char('\n');
        return -1;
    }

    f->root   = root;
    f->handle = sf_cparms.handle;
    f->size   = sf_cparms.info.fileSize;
    f->offset = 0;
    f->valid  = 1;
    k_strncpy(f->path, path, 255);
    return 0;
}

int vboxsf_read(vboxsf_file_t *f, void *buf, size_t len) {
    if (!f->valid) return -1;
    if (f->offset >= f->size) return 0;
    if (f->offset + len > f->size) len = (size_t)(f->size - f->offset);
    if (len == 0) return 0;

    // Necesitamos un buffer intermedio accesible por el host
    // Máximo que podemos transferir por vez sin malloc (usar hgcm_buf con cuidado)
    // Como hgcm_buf se usa para el request, necesitamos un segundo buffer.
    // Usamos un buffer estático de 512KB para lecturas.
    static uint8_t __attribute__((aligned(4096))) read_buf[512 * 1024];
    uint8_t *out = (uint8_t *)buf;
    size_t  total_read = 0;

    while (total_read < len) {
        size_t chunk = len - total_read;
        if (chunk > sizeof(read_buf)) chunk = sizeof(read_buf);
        uint32_t to_read = (uint32_t)chunk;

        // SHFL_FN_READ params:
        //   [0] IN: root (uint32_t)
        //   [1] IN: handle (uint64_t → 2x 32bit, o via 64bit parm)
        //   [2] IN: offset (uint64_t)
        //   [3] IN/OUT: byte count (uint32_t)
        //   [4] OUT: buffer (LinAddr)

        // Para handle y offset de 64 bits, VBoxSF los acepta como LINADDR a
        // un uint64_t. Pero la forma más compatible es usar 2 parms de 32 bits.
        // Usamos el método más simple: PHYSADDR a uint64_t estático.
        static uint64_t sf_handle_val;
        static uint64_t sf_offset_val;
        static uint32_t sf_count_val;

        sf_handle_val = f->handle;
        sf_offset_val = f->offset;
        sf_count_val  = to_read;

        hgcm_parm_t p[5];
        p[0].type      = HGCM_PARM_TYPE_32BIT;
        p[0].u.value32 = f->root;

        // Handle: 64-bit
        p[1].type       = HGCM_PARM_TYPE_LINADDR_IN;
        p[1].u.ptr.size = sizeof(uint64_t);
        p[1].u.ptr.addr = (uint32_t)(uintptr_t)&sf_handle_val;

        // Offset: 64-bit
        p[2].type       = HGCM_PARM_TYPE_LINADDR_IN;
        p[2].u.ptr.size = sizeof(uint64_t);
        p[2].u.ptr.addr = (uint32_t)(uintptr_t)&sf_offset_val;

        // Byte count (in/out)
        p[3].type       = HGCM_PARM_TYPE_LINADDR;
        p[3].u.ptr.size = sizeof(uint32_t);
        p[3].u.ptr.addr = (uint32_t)(uintptr_t)&sf_count_val;

        // Buffer de datos
        p[4].type       = HGCM_PARM_TYPE_LINADDR_OUT;
        p[4].u.ptr.size = (uint32_t)chunk;
        p[4].u.ptr.addr = (uint32_t)(uintptr_t)read_buf;

        int rc = hgcm_call(hgcm_client_id, SHFL_FN_READ, p, 5);
        if (rc != 0) {
            vga_print("[VBOXSF] read error rc="); vga_print_hex((uint32_t)rc);
            vga_put_char('\n');
            break;
        }

        uint32_t actually_read = sf_count_val;
        if (actually_read == 0) break;

        k_memcpy(out + total_read, read_buf, actually_read);
        total_read  += actually_read;
        f->offset   += actually_read;
    }
    return (int)total_read;
}

void vboxsf_close(vboxsf_file_t *f) {
    if (!f || !f->valid) return;
    // SHFL_FN_CLOSE params: [0] root, [1] handle
    static uint64_t sf_close_handle;
    sf_close_handle = f->handle;

    hgcm_parm_t p[2];
    p[0].type      = HGCM_PARM_TYPE_32BIT;
    p[0].u.value32 = f->root;
    p[1].type       = HGCM_PARM_TYPE_LINADDR_IN;
    p[1].u.ptr.size = sizeof(uint64_t);
    p[1].u.ptr.addr = (uint32_t)(uintptr_t)&sf_close_handle;
    hgcm_call(hgcm_client_id, SHFL_FN_CLOSE, p, 2);
    f->valid = 0;
}

void *vboxsf_read_file(const char *sf_name, const char *path, size_t *out_size) {
    vboxsf_file_t f;
    if (vboxsf_open(sf_name, path, &f) < 0) return NULL;
    uint8_t *buf = (uint8_t *)kmalloc((size_t)f.size + 1);
    if (!buf) { vboxsf_close(&f); return NULL; }
    int rd = vboxsf_read(&f, buf, (size_t)f.size);
    vboxsf_close(&f);
    if (out_size) *out_size = (size_t)(rd > 0 ? rd : 0);
    buf[rd > 0 ? rd : 0] = 0;
    return buf;
}

// ═══════════════════════════════════════════════════════════════
//   INIT PRINCIPAL
// ═══════════════════════════════════════════════════════════════

int vboxsf_init(void) {
    if (hgcm_connect("VBoxSharedFolders", &hgcm_client_id) < 0)
        return -1;
    vboxsf_ready = 1;
    vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    vga_print("[VBOXSF] VBoxSharedFolders conectado (clientID=");
    vga_print_dec(hgcm_client_id);
    vga_print(")\n");
    vga_print("[VBOXSF] Configura la carpeta compartida en VirtualBox:\n");
    vga_print("[VBOXSF]   Settings > Shared Folders > Add\n");
    vga_print("[VBOXSF]   Name: xp7  (donde pongas doom.wad)\n");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    return 0;
}

int vbox_init(void) {
    vga_print("[VBOX] Detectando VirtualBox...\n");

    if (!vbox_is_present()) {
        vga_print("[VBOX] No corremos en VirtualBox\n");
        return -1;
    }

    if (vmmdev_init() < 0) return -1;
    vbox_ready = 1;

    if (vboxsf_init() < 0) {
        vga_print("[VBOXSF] Shared Folders no disponible\n");
        return 0;  // VMMDev OK, SF no — no es fatal
    }
    return 0;
}
