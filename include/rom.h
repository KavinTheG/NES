
#include <stddef.h>
#include <stdint.h>

#define ROM_OK 1
#define ROM_ERR_NOFILE -1
#define ROM_ERR_HEADER_MISMATCH -2
#define ROM_ERR_PRG_SIZE -3
#define ROM_ERR_CHR_SIZE -4

#define ROM_MEM_ALLOC_FAIL -5

typedef struct Rom {
  uint8_t *header;
  uint8_t *prg_data;
  uint8_t *chr_data;
  size_t prg_size;
  size_t chr_size;
} Rom;

int rom_load_cartridge(Rom *rom, char *filename);

void rom_load_cpu_mem();
void rom_load_ppu_mem();
