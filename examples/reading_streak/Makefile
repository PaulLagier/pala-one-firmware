TOOLCHAIN_BASE ?= $(HOME)/.arduino15/packages/esp32/tools/esp-x32
TOOLCHAIN_VER  ?= $(shell ls $(TOOLCHAIN_BASE) 2>/dev/null | sort -V | tail -1)
TOOLCHAIN      ?= $(TOOLCHAIN_BASE)/$(TOOLCHAIN_VER)/bin

CC      = $(TOOLCHAIN)/xtensa-esp32s3-elf-gcc
OBJCOPY = $(TOOLCHAIN)/xtensa-esp32s3-elf-objcopy
NM      = $(TOOLCHAIN)/xtensa-esp32s3-elf-nm
READELF = $(TOOLCHAIN)/xtensa-esp32s3-elf-readelf

CFLAGS = -fPIC -mlongcalls -Os -fno-jump-tables -nostdlib -nodefaultlibs \
         -I../../Pala_One_2_1

all: reading_streak.bin

reading_streak.elf: app.c pala_app.ld
	$(CC) $(CFLAGS) -Wl,-T,pala_app.ld -Wl,-shared -o $@ app.c

reading_streak.bin: reading_streak.elf
	$(OBJCOPY) -O binary $< $@
	$(eval ENTRY_OFF := $(shell $(NM) $< | awk '/^[0-9a-f]+ [Tt] app_main$$/{print $$1}'))
	@[ -n "$(ENTRY_OFF)" ] || (echo "ERROR: app_main symbol not found"; exit 1)
	python3 -c " \
import struct, subprocess; \
out=subprocess.check_output(['$(READELF)','-r','$<'],text=True); \
relocs=[int(ln.split()[0],16) for ln in out.splitlines() if 'R_XTENSA_RELATIVE' in ln]; \
d=bytearray(open('$@','rb').read()); \
struct.pack_into('<I',d,40,int('$(ENTRY_OFF)',16)); \
reloc_off=len(d) if relocs else 0; \
d.extend(struct.pack('<'+'I'*len(relocs),*relocs)); \
struct.pack_into('<I',d,44,reloc_off); \
struct.pack_into('<I',d,48,len(relocs)); \
open('$@','wb').write(d); \
print(f'relocations: {relocs}')"
	@echo "Built $@: $$(wc -c < $@) bytes, entry=0x$(ENTRY_OFF)"

clean:
	rm -f reading_streak.elf reading_streak.bin

.PHONY: all clean
