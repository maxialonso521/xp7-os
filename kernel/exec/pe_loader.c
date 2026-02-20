#include "pe_loader.h"
#include "../mm/mm.h"
#include "../fs/fat32.h"
#include "../string.h"
#include "../vga.h"
#include "../win32/win32_stubs.h"

// ─── Rutas de las DLLs del sistema ───────────────────────────
static char sys32_path[128]  = "/system32";
static char wow64_path[128]  = "/syswow64";
static char sysxp7_path[128] = "/xp7/system";

// ─── Pool de procesos (Stage 4 tendrá scheduler real) ────────
static pe_process_t current_proc;

// ─── Helper: RVA → puntero en memoria ─────────────────────────
static inline void *rva_to_ptr(pe_module_t *mod, uint32_t rva) {
    return (void *)(mod->base + rva);
}

// ─── Buscar módulo ya cargado ──────────────────────────────────
static pe_module_t *find_module(pe_process_t *proc, const char *name) {
    // Normalizar a minúsculas para comparar
    char lower[PE_MAX_NAME];
    k_strncpy(lower, name, PE_MAX_NAME-1);
    for (int i=0; lower[i]; i++)
        if (lower[i]>='A' && lower[i]<='Z') lower[i]+=32;

    for (int i = 0; i < proc->module_count; i++) {
        char mname[PE_MAX_NAME];
        k_strncpy(mname, proc->modules[i].name, PE_MAX_NAME-1);
        for (int j=0; mname[j]; j++)
            if (mname[j]>='A' && mname[j]<='Z') mname[j]+=32;
        if (k_strcmp(mname, lower) == 0 && proc->modules[i].valid)
            return &proc->modules[i];
    }
    return NULL;
}

// ─── Cargar secciones PE en memoria ──────────────────────────
static int pe_map_sections(const uint8_t *file_data, size_t file_size,
                             pe_nt_header_t *nt, pe_module_t *mod) {
    uint32_t img_size = nt->opt_hdr.image_size;
    uint8_t *img      = (uint8_t *)kmalloc(img_size);
    if (!img) { vga_print("[PE] Sin memoria\n"); return -1; }
    k_memset(img, 0, img_size);

    // Copiar headers
    uint32_t hdr_size = nt->opt_hdr.headers_size;
    if (hdr_size > file_size) hdr_size = (uint32_t)file_size;
    k_memcpy(img, file_data, hdr_size);

    // Mapear secciones
    pe_section_t *sections = (pe_section_t *)
        ((uint8_t *)nt + 4 + sizeof(pe_file_header_t)
         + nt->file_hdr.opt_hdr_size);

    for (int i = 0; i < nt->file_hdr.num_sections; i++) {
        pe_section_t *sec = &sections[i];
        if (sec->raw_size == 0) continue;
        if (sec->raw_offset + sec->raw_size > file_size) continue;
        uint32_t copy = sec->raw_size;
        if (copy > sec->virtual_size && sec->virtual_size > 0)
            copy = sec->virtual_size;
        k_memcpy(img + sec->virtual_addr,
                 file_data + sec->raw_offset, copy);
    }

    mod->base = (uint32_t)img;
    mod->size = img_size;
    return 0;
}

// ─── Aplicar relocaciones ─────────────────────────────────────
static void pe_apply_relocs(pe_module_t *mod, pe_nt_header_t *nt) {
    pe_data_dir_t *reloc_dir = &nt->opt_hdr.data_dir[PE_DIR_RELOC];
    if (reloc_dir->size == 0) return;

    uint32_t preferred = nt->opt_hdr.image_base;
    int32_t  delta     = (int32_t)(mod->base - preferred);
    if (delta == 0) return;

    pe_reloc_block_t *block = (pe_reloc_block_t *)
        rva_to_ptr(mod, reloc_dir->virtual_addr);
    uint32_t remaining = reloc_dir->size;

    while (remaining > 0 && block->block_size >= 8) {
        uint16_t *entries = (uint16_t *)((uint8_t *)block + 8);
        int count = (block->block_size - 8) / 2;
        for (int i = 0; i < count; i++) {
            uint8_t  type = entries[i] >> 12;
            uint16_t off  = entries[i] & 0x0FFF;
            if (type == 3) {  // IMAGE_REL_BASED_HIGHLOW
                uint32_t *target = (uint32_t *)
                    (mod->base + block->page_rva + off);
                *target += delta;
            }
        }
        remaining -= block->block_size;
        block = (pe_reloc_block_t *)((uint8_t *)block + block->block_size);
    }
}

// ─── Resolver exports de un módulo ───────────────────────────
uint32_t pe_get_export(pe_module_t *mod, const char *func_name) {
    if (!mod || !mod->valid) return 0;

    pe_nt_header_t *nt = (pe_nt_header_t *)(mod->base + 0x3C +
        *(uint32_t *)(mod->base + 0x3C));
    pe_data_dir_t *exp_dir_entry = &nt->opt_hdr.data_dir[PE_DIR_EXPORT];
    if (exp_dir_entry->size == 0) return 0;

    pe_export_dir_t *exp = (pe_export_dir_t *)
        rva_to_ptr(mod, exp_dir_entry->virtual_addr);

    uint32_t *names    = (uint32_t *)rva_to_ptr(mod, exp->name_table_rva);
    uint16_t *ordinals = (uint16_t *)rva_to_ptr(mod, exp->ordinal_table_rva);
    uint32_t *funcs    = (uint32_t *)rva_to_ptr(mod, exp->address_table_rva);

    // Búsqueda binaria (la tabla de nombres está ordenada)
    int lo = 0, hi = (int)exp->num_names - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        const char *nm = (const char *)rva_to_ptr(mod, names[mid]);
        int cmp = k_strcmp(nm, func_name);
        if (cmp == 0) {
            uint32_t rva = funcs[ordinals[mid]];
            return mod->base + rva;
        }
        if (cmp < 0) lo = mid + 1;
        else         hi = mid - 1;
    }
    return 0;
}

uint32_t pe_get_export_by_ordinal(pe_module_t *mod, uint16_t ordinal) {
    if (!mod || !mod->valid) return 0;
    pe_nt_header_t *nt = (pe_nt_header_t *)(mod->base +
        *(uint32_t *)(mod->base + 0x3C));
    pe_data_dir_t *edd = &nt->opt_hdr.data_dir[PE_DIR_EXPORT];
    if (edd->size == 0) return 0;
    pe_export_dir_t *exp = (pe_export_dir_t *)rva_to_ptr(mod, edd->virtual_addr);
    uint32_t *funcs = (uint32_t *)rva_to_ptr(mod, exp->address_table_rva);
    uint32_t idx = ordinal - exp->ordinal_base;
    if (idx >= exp->num_functions) return 0;
    return mod->base + funcs[idx];
}

// ─── Resolver imports de un módulo ───────────────────────────
static int pe_resolve_imports(pe_module_t *mod, pe_nt_header_t *nt,
                               pe_process_t *proc) {
    pe_data_dir_t *imp_dir = &nt->opt_hdr.data_dir[PE_DIR_IMPORT];
    if (imp_dir->size == 0) return 0;

    pe_import_dir_t *imp = (pe_import_dir_t *)
        rva_to_ptr(mod, imp_dir->virtual_addr);

    while (imp->name_rva != 0) {
        const char *dll_name = (const char *)rva_to_ptr(mod, imp->name_rva);

        // Buscar o cargar la DLL
        pe_module_t *dep = find_module(proc, dll_name);
        if (!dep) dep = pe_load_dll(dll_name, proc);

        // IAT: Import Address Table
        uint32_t *iat = (uint32_t *)rva_to_ptr(mod, imp->iat_rva);
        uint32_t *ilt = (uint32_t *)rva_to_ptr(mod,
            imp->lookup_table_rva ? imp->lookup_table_rva : imp->iat_rva);

        while (*ilt) {
            uint32_t entry = *ilt;
            uint32_t resolved = 0;

            if (entry & 0x80000000) {
                // Import by ordinal
                uint16_t ord = (uint16_t)(entry & 0x7FFF);
                if (dep) resolved = pe_get_export_by_ordinal(dep, ord);
                if (!resolved) resolved = win32_stub_ordinal(dll_name, ord);
            } else {
                // Import by name (skip 2-byte hint)
                const char *fname = (const char *)
                    rva_to_ptr(mod, entry + 2);
                if (dep) resolved = pe_get_export(dep, fname);
                if (!resolved) resolved = win32_stub_find(dll_name, fname);
                if (!resolved) {
                    // Log función no resuelta (no fatal — muchas no importan)
                    // Poner un stub que imprime un warning
                    resolved = win32_stub_missing(dll_name, fname);
                }
            }

            *iat = resolved;
            ilt++; iat++;
        }
        imp++;
    }
    return 0;
}

// ─── Cargar un módulo PE (exe o dll) desde bytes en memoria ──
static pe_module_t *pe_load_from_data(const char *name,
                                       const uint8_t *data, size_t size,
                                       pe_process_t *proc, int is_dll) {
    // Validar MZ
    if (size < sizeof(dos_header_t)) return NULL;
    dos_header_t *dos = (dos_header_t *)data;
    if (dos->magic != PE_DOS_MAGIC) {
        vga_print("[PE] No es un PE valido (sin MZ): ");
        vga_print(name); vga_put_char('\n');
        return NULL;
    }

    pe_nt_header_t *nt = (pe_nt_header_t *)(data + dos->pe_offset);
    if (nt->signature != PE_NT_MAGIC) {
        vga_print("[PE] Firma PE invalida: "); vga_print(name); vga_put_char('\n');
        return NULL;
    }
    if (nt->opt_hdr.magic != PE_OPT_MAGIC32) {
        vga_print("[PE] No es PE32 (solo x86 32-bit): ");
        vga_print(name); vga_put_char('\n');
        return NULL;
    }

    // Registrar el módulo
    if (proc->module_count >= PE_MAX_MODULES) {
        vga_print("[PE] Demasiados modulos\n"); return NULL;
    }
    pe_module_t *mod = &proc->modules[proc->module_count++];
    k_memset(mod, 0, sizeof(pe_module_t));

    // Extraer solo el nombre del archivo (sin path)
    const char *bn = name;
    for (const char *p = name; *p; p++)
        if (*p == '/' || *p == '\\') bn = p+1;
    k_strncpy(mod->name, bn, PE_MAX_NAME-1);
    mod->is_dll = is_dll;
    mod->valid  = 1;

    // Mapear secciones
    if (pe_map_sections(data, size, nt, mod) < 0) {
        mod->valid = 0; proc->module_count--; return NULL;
    }

    // Recalcular puntero NT en el nuevo buffer
    dos_header_t *new_dos = (dos_header_t *)mod->base;
    pe_nt_header_t *new_nt = (pe_nt_header_t *)(mod->base + new_dos->pe_offset);

    // Aplicar relocaciones
    pe_apply_relocs(mod, new_nt);

    // Entry point
    mod->entry_point = mod->base + new_nt->opt_hdr.entry_point;

    // Resolver imports (recursivo para dependencias)
    pe_resolve_imports(mod, new_nt, proc);

    vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    vga_print("[PE] Cargado: "); vga_print(mod->name);
    vga_print(" @ 0x"); vga_print_hex(mod->base); vga_put_char('\n');
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);

    return mod;
}

// ─── Cargar DLL buscando en varias rutas ─────────────────────
pe_module_t *pe_load_dll(const char *dll_name, pe_process_t *proc) {
    // ¿Ya está cargada?
    pe_module_t *existing = find_module(proc, dll_name);
    if (existing) return existing;

    // Construir rutas candidatas
    const char *search[] = { sysxp7_path, sys32_path, wow64_path, NULL };
    char path[256];
    size_t data_size;

    for (int i = 0; search[i]; i++) {
        k_strcpy(path, search[i]);
        k_strcat(path, "/");
        k_strcat(path, dll_name);

        void *data = fat32_read_file(path, &data_size);
        if (data) {
            pe_module_t *mod = pe_load_from_data(dll_name,
                (uint8_t *)data, data_size, proc, 1);
            kfree(data);
            if (mod) return mod;
        }
    }

    vga_set_color(VGA_LIGHT_BROWN, VGA_BLACK);
    vga_print("[PE] DLL no encontrada: "); vga_print(dll_name);
    vga_print(" (usando stubs)\n");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);

    // Crear módulo stub vacío para que los imports no queden sin resolver
    if (proc->module_count < PE_MAX_MODULES) {
        pe_module_t *stub = &proc->modules[proc->module_count++];
        k_memset(stub, 0, sizeof(pe_module_t));
        const char *bn = dll_name;
        for (const char *p = dll_name; *p; p++)
            if (*p=='/'||*p=='\\') bn=p+1;
        k_strncpy(stub->name, bn, PE_MAX_NAME-1);
        stub->is_dll = 1;
        stub->valid  = 1;
        stub->base   = 0;  // Sin memoria real — solo stubs
        return stub;
    }
    return NULL;
}

// ─── Cargar .exe principal ────────────────────────────────────
pe_process_t *pe_load_exe(const char *path) {
    size_t size;
    void *data = fat32_read_file(path, &size);
    if (!data) {
        vga_print("[PE] No se puede leer: "); vga_print(path);
        vga_put_char('\n'); return NULL;
    }

    pe_process_t *proc = &current_proc;
    k_memset(proc, 0, sizeof(pe_process_t));
    k_strncpy(proc->exe_path, path, 255);

    pe_module_t *mod = pe_load_from_data(path,
        (uint8_t *)data, size, proc, 0);
    kfree(data);

    if (!mod) return NULL;
    proc->main_exe   = mod;
    proc->entry_point = mod->entry_point;
    return proc;
}

// ─── Ejecutar proceso ─────────────────────────────────────────
int pe_execute(pe_process_t *proc, int argc, char **argv) {
    (void)argc; (void)argv;
    if (!proc || !proc->entry_point) return -1;

    vga_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
    vga_print("[PE] Ejecutando: "); vga_print(proc->exe_path);
    vga_print(" @ 0x"); vga_print_hex(proc->entry_point);
    vga_put_char('\n');
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);

    /*
     * Llamar al entry point del EXE
     * Convención Win32:  BOOL WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
     *                 o: int main(int argc, char** argv)
     * Pasamos args simplificados
     */
    typedef int (__attribute__((stdcall)) *win_entry_t)(
        uint32_t hInstance,    // 0 = dummy
        uint32_t hPrevInstance,
        uint32_t lpCmdLine,
        int      nCmdShow);

    win_entry_t entry = (win_entry_t)proc->entry_point;
    int result = entry(0, 0, 0, 1);

    vga_set_color(VGA_LIGHT_BROWN, VGA_BLACK);
    vga_print("[PE] Proceso terminado. Codigo: ");
    vga_print_dec((uint32_t)result); vga_put_char('\n');
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    return result;
}

// ─── Debug: listar imports ────────────────────────────────────
void pe_dump_imports(pe_module_t *mod) {
    if (!mod->valid || !mod->base) return;
    dos_header_t *dos = (dos_header_t *)mod->base;
    pe_nt_header_t *nt = (pe_nt_header_t *)(mod->base + dos->pe_offset);
    pe_data_dir_t *imp_dir = &nt->opt_hdr.data_dir[PE_DIR_IMPORT];
    if (imp_dir->size == 0) { vga_print("  (sin imports)\n"); return; }

    pe_import_dir_t *imp = (pe_import_dir_t *)
        rva_to_ptr(mod, imp_dir->virtual_addr);
    while (imp->name_rva) {
        vga_print("  DLL: ");
        vga_print((const char *)rva_to_ptr(mod, imp->name_rva));
        vga_put_char('\n');
        imp++;
    }
}

void pe_dump_exports(pe_module_t *mod, int max) {
    if (!mod->valid || !mod->base) return;
    dos_header_t *dos = (dos_header_t *)mod->base;
    pe_nt_header_t *nt = (pe_nt_header_t *)(mod->base + dos->pe_offset);
    pe_data_dir_t *exp_dir = &nt->opt_hdr.data_dir[PE_DIR_EXPORT];
    if (exp_dir->size == 0) { vga_print("  (sin exports)\n"); return; }

    pe_export_dir_t *exp = (pe_export_dir_t *)
        rva_to_ptr(mod, exp_dir->virtual_addr);
    uint32_t *names = (uint32_t *)rva_to_ptr(mod, exp->name_table_rva);
    int count = (int)exp->num_names;
    if (count > max) count = max;
    for (int i = 0; i < count; i++) {
        vga_print("  "); vga_print((char *)rva_to_ptr(mod, names[i]));
        vga_put_char('\n');
    }
}

void pe_free_process(pe_process_t *proc) {
    for (int i = 0; i < proc->module_count; i++) {
        if (proc->modules[i].valid && proc->modules[i].base)
            kfree((void *)proc->modules[i].base);
    }
    k_memset(proc, 0, sizeof(pe_process_t));
}

void pe_loader_init(const char *s32, const char *w64) {
    if (s32) k_strncpy(sys32_path,  s32, 127);
    if (w64) k_strncpy(wow64_path,  w64, 127);
    vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    vga_print("[PE] Loader init. System32: "); vga_print(sys32_path);
    vga_print(" | WoW64: "); vga_print(wow64_path); vga_put_char('\n');
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
}
