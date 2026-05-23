#pragma once
#include <stdint.h>

#define PALA_APP_MAGIC    0x50414C41UL  // 'PALA'
#define PALA_API_VERSION  4

// Event codes returned by waitForEvent()
#define PALA_CLICK   1
#define PALA_DOUBLE  2
#define PALA_TRIPLE  3
#define PALA_LONG    4

// App binary header — placed at offset 0 by the linker script
typedef struct {
    uint32_t magic;         // Must equal PALA_APP_MAGIC
    uint32_t api_version;   // Must equal PALA_API_VERSION
    char     name[32];      // Display name, null-terminated
    uint32_t entry_offset;  // Byte offset from binary start to app_main(); patched by Makefile
    uint32_t reloc_offset;  // Byte offset to R_XTENSA_RELATIVE table (0 if reloc_count==0)
    uint32_t reloc_count;   // Number of entries in relocation table; patched by Makefile
} PalaAppHeader;

// Entry point signature
typedef void (*pala_app_entry_t)(const void* api);
