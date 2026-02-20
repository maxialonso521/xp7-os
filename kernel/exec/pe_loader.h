#ifndef PE_LOADER_H
#define PE_LOADER_H

/*
 * ╔══════════════════════════════════════════════════════════════╗
 * ║  XP7 OS — PE Loader (Portable Executable)                  ║
 * ║  Carga .exe y .dll de Windows en memoria                   ║
 * ║  Compatible con: Win32 x86 (PE32, no PE32+)                ║
 * ║  DLLs de System32/WoW64 cargados desde disco FAT32         ║
 * ╚══════════════════════════════════════════════════════════════╝
 */

#include <stdint.h>
#include <stddef.h>

// ─── Signatures ───────────────────────────────────────────────
#define PE_DOS_MAGIC    0x5A4D    // "MZ"
#define PE_NT_MAGIC     0x00004550 // "PE\0\0"
#define PE_OPT_MAGIC32  0x010B    // PE32
#define PE_OPT_MAGIC64  0x020B    // PE32+ (no soportado en Stage 3)

// ─── Estructuras PE ───────────────────────────────────────────
typedef struct {
    uint16_t magic;       // 0x5A4D
    uint8_t  stub[58];    // DOS stub
    uint32_t pe_offset;   // offset al PE header
} __attribute__((packed)) dos_header_t;

typedef struct {
    uint16_t machine;         // 0x014C = i386
    uint16_t num_sections;
    uint32_t timestamp;
    uint32_t symtab_ptr;
    uint32_t num_symbols;
    uint16_t opt_hdr_size;
    uint16_t characteristics;
} __attribute__((packed)) pe_file_header_t;

typedef struct {
    uint32_t virtual_addr;
    uint32_t size;
} __attribute__((packed)) pe_data_dir_t;

typedef struct {
    uint16_t magic;           // 0x010B PE32
    uint8_t  major_linker;
    uint8_t  minor_linker;
    uint32_t code_size;
    uint32_t init_data_size;
    uint32_t uninit_data_size;
    uint32_t entry_point;     // RVA del entry point
    uint32_t code_base;
    uint32_t data_base;
    uint32_t image_base;      // Base preferida (ej: 0x00400000)
    uint32_t section_align;
    uint32_t file_align;
    uint16_t os_major;
    uint16_t os_minor;
    uint16_t img_major;
    uint16_t img_minor;
    uint16_t subsys_major;
    uint16_t subsys_minor;
    uint32_t win32_version;
    uint32_t image_size;
    uint32_t headers_size;
    uint32_t checksum;
    uint16_t subsystem;       // 2=GUI, 3=CUI (console)
    uint16_t dll_chars;
    uint32_t stack_reserve;
    uint32_t stack_commit;
    uint32_t heap_reserve;
    uint32_t heap_commit;
    uint32_t loader_flags;
    uint32_t num_data_dirs;
    pe_data_dir_t data_dir[16];
} __attribute__((packed)) pe_opt_header_t;

typedef struct {
    uint32_t         signature;  // "PE\0\0"
    pe_file_header_t file_hdr;
    pe_opt_header_t  opt_hdr;
} __attribute__((packed)) pe_nt_header_t;

typedef struct {
    char     name[8];
    uint32_t virtual_size;
    uint32_t virtual_addr;    // RVA
    uint32_t raw_size;
    uint32_t raw_offset;
    uint32_t reloc_offset;
    uint32_t linenums_offset;
    uint16_t num_relocs;
    uint16_t num_linenums;
    uint32_t characteristics;
} __attribute__((packed)) pe_section_t;

// ─── Índices del Data Directory ──────────────────────────────
#define PE_DIR_EXPORT   0
#define PE_DIR_IMPORT   1
#define PE_DIR_RESOURCE 2
#define PE_DIR_RELOC    5
#define PE_DIR_IAT      12

// ─── Export Directory ─────────────────────────────────────────
typedef struct {
    uint32_t flags;
    uint32_t timestamp;
    uint16_t version_major;
    uint16_t version_minor;
    uint32_t name_rva;
    uint32_t ordinal_base;
    uint32_t num_functions;
    uint32_t num_names;
    uint32_t address_table_rva;
    uint32_t name_table_rva;
    uint32_t ordinal_table_rva;
} __attribute__((packed)) pe_export_dir_t;

// ─── Import Directory ─────────────────────────────────────────
typedef struct {
    uint32_t lookup_table_rva;   // ILT
    uint32_t timestamp;
    uint32_t forwarder_chain;
    uint32_t name_rva;           // Nombre de la DLL
    uint32_t iat_rva;            // Import Address Table
} __attribute__((packed)) pe_import_dir_t;

// ─── Base Relocation ──────────────────────────────────────────
typedef struct {
    uint32_t page_rva;
    uint32_t block_size;
    // Seguido de (block_size-8)/2 entradas de 16 bits
} __attribute__((packed)) pe_reloc_block_t;

// ─── Módulo cargado en memoria ───────────────────────────────
#define PE_MAX_MODULES  32
#define PE_MAX_NAME     64

typedef struct {
    char     name[PE_MAX_NAME];  // Nombre sin path (ej: "kernel32.dll")
    uint32_t base;               // Dirección base en memoria donde fue cargado
    uint32_t size;               // Tamaño del image en memoria
    uint32_t entry_point;        // Dirección absoluta del entry point
    uint8_t  is_dll;
    uint8_t  valid;
    // Tabla de exports cacheada
    pe_export_dir_t *export_dir;
} pe_module_t;

// ─── Proceso cargado ──────────────────────────────────────────
typedef struct {
    pe_module_t  modules[PE_MAX_MODULES];
    int          module_count;
    pe_module_t *main_exe;
    uint32_t     entry_point;
    char         exe_path[256];
} pe_process_t;

// ─── API pública ──────────────────────────────────────────────

// Inicializar el loader (configura rutas de DLLs del sistema)
void  pe_loader_init(const char *system32_path, const char *wow64_path);

// Cargar un .exe y todas sus dependencias
pe_process_t *pe_load_exe(const char *path);

// Cargar una DLL por nombre (busca en System32 / WoW64)
pe_module_t  *pe_load_dll(const char *dll_name, pe_process_t *proc);

// Resolver export por nombre desde una DLL cargada
uint32_t pe_get_export(pe_module_t *mod, const char *func_name);
uint32_t pe_get_export_by_ordinal(pe_module_t *mod, uint16_t ordinal);

// Ejecutar el entry point del proceso
int  pe_execute(pe_process_t *proc, int argc, char **argv);

// Liberar un proceso y sus módulos
void pe_free_process(pe_process_t *proc);

// Debug: imprimir imports/exports de un módulo
void pe_dump_imports(pe_module_t *mod);
void pe_dump_exports(pe_module_t *mod, int max);

#endif
