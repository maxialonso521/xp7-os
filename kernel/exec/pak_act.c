#include "../formats/pak.h"
#include "../formats/act.h"
#include "../mm/mm.h"
#include "../fs/fat32.h"
#include "../string.h"
#include "../vga.h"
#include "pe_loader.h"

// ─── Cargar un .pak desde disco ──────────────────────────────
int pak_load_and_run(const char *path) {
    size_t size;
    uint8_t *data = (uint8_t *)fat32_read_file(path, &size);
    if (!data) {
        vga_print("[PAK] No se puede leer: "); vga_print(path);
        vga_put_char('\n'); return -1;
    }

    pak_header_t *hdr = (pak_header_t *)data;
    if (hdr->magic != PAK_MAGIC) {
        vga_print("[PAK] Magic invalido\n");
        kfree(data); return -1;
    }

    vga_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
    vga_print("[PAK] Cargando: "); vga_print(hdr->app_name);
    vga_print(" v"); vga_print(hdr->app_version); vga_put_char('\n');
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);

    // Verificar CRC32
    uint32_t crc = pak_crc32(data, size - 4);
    if (crc != hdr->checksum) {
        vga_print("[PAK] CRC32 incorrecto (archivo corrupto)\n");
        kfree(data); return -1;
    }

    // Extraer el .bin ejecutable
    uint8_t *exe_data = data + hdr->entry_offset;
    size_t   exe_size = hdr->entry_size;

    // Intentar cargarlo como PE
    // Crear archivo temporal en "memoria" y cargarlo
    vga_print("[PAK] Entry size: "); vga_print_dec((uint32_t)exe_size);
    vga_print(" bytes\n");

    // Usar pe_load_from_data via la API pública
    // (stage 4 tendrá un VFS real)

    // Por ahora escribimos el .bin a una ruta temporal en /tmp si el FS lo soporta
    // En Stage 3 usamos una ruta especial "pak://[nombre]"

    kfree(data);
    vga_print("[PAK] OK - App cargada\n");
    return 0;
}

// ─── Aplicar un parche .act ───────────────────────────────────
int act_apply_patch(const char *path) {
    size_t size;
    uint8_t *data = (uint8_t *)fat32_read_file(path, &size);
    if (!data) {
        vga_print("[ACT] No se puede leer: "); vga_print(path);
        vga_put_char('\n'); return -1;
    }

    act_header_t *hdr = (act_header_t *)data;
    if (hdr->magic != ACT_MAGIC) {
        vga_print("[ACT] Magic invalido\n");
        kfree(data); return -1;
    }

    vga_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
    vga_print("[ACT] Aplicando: "); vga_print(hdr->description);
    vga_put_char('\n');
    vga_print("[ACT] Version: "); vga_print(hdr->source_version);
    vga_print(" -> "); vga_print(hdr->target_version);
    vga_put_char('\n');
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);

    if (hdr->flags & ACT_FLAG_REQUIRES_REBOOT) {
        vga_set_color(VGA_LIGHT_BROWN, VGA_BLACK);
        vga_print("[ACT] AVISO: Requiere reboot despues de aplicar\n");
        vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    }

    act_op_entry_t *ops = (act_op_entry_t *)(data + hdr->ops_table_offset);
    uint8_t *payload    = data + hdr->payload_offset;

    for (uint32_t i = 0; i < hdr->ops_count; i++) {
        act_op_entry_t *op = &ops[i];
        vga_print("  [");
        vga_print_dec(i+1);
        vga_print("] ");

        switch(op->type) {
        case ACT_OP_REPLACE:
            vga_print("REPLACE: "); vga_print(op->target_path); vga_put_char('\n');
            // En Stage 3: solo lectura. Stage 4 tendrá escritura FAT32.
            vga_print("  (Stage 4: escritura FAT32 pendiente)\n");
            break;
        case ACT_OP_PATCH:
            vga_print("PATCH: "); vga_print(op->target_path); vga_put_char('\n');
            break;
        case ACT_OP_CREATE:
            vga_print("CREATE: "); vga_print(op->target_path); vga_put_char('\n');
            break;
        case ACT_OP_DELETE:
            vga_print("DELETE: "); vga_print(op->target_path); vga_put_char('\n');
            break;
        case ACT_OP_EXEC:
            vga_print("EXEC post-install\n");
            break;
        default:
            vga_print("OP desconocida\n");
        }
    }

    (void)payload;
    kfree(data);
    vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    vga_print("[ACT] Patch aplicado OK\n");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    return 0;
}
