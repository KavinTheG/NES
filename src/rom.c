#include "rom.h"
#include "ppu.h"
#include <stdio.h>
#include <stdlib.h>

int rom_load_cartridge(Rom *rom, char *filename) {
  printf("%s\n", filename);

  if (!filename) {
    fprintf(stderr, "Error: filename is NULL\n");
    return ROM_ERR_NOFILE;
  }

  FILE *rom_file = fopen(filename, "rb");
  if (!rom_file) {
    return ROM_ERR_NOFILE;
  }
  rom->header = malloc(NES_HEADER_SIZE);

  if (fread(rom->header, 1, NES_HEADER_SIZE, rom_file) != NES_HEADER_SIZE) {
    fclose(rom_file);
    return ROM_ERR_HEADER_MISMATCH;
  }
  printf("test");
  rom->prg_size = rom->header[4] * 16 * 1024;
  rom->chr_size = rom->header[5] * 8 * 1024;

  if (rom->prg_size != 16384 && rom->prg_size != 32768) {
    printf("Unsupported PRG Size: %ld", rom->prg_size);
    return ROM_ERR_PRG_SIZE;
  }

  rom->prg_data = malloc(rom->prg_size);
  rom->chr_data = malloc(rom->chr_size);
  if (!rom->prg_data || !rom->chr_data) {
    fclose(rom_file);
    return ROM_MEM_ALLOC_FAIL;
  }

  if (fread(rom->prg_data, 1, rom->prg_size, rom_file) != rom->prg_size) {
    free(rom->prg_data);
    free(rom->chr_data);
    fclose(rom_file);
    return ROM_ERR_PRG_SIZE;
  }
  if (fseek(rom_file, NES_HEADER_SIZE + rom->prg_size, SEEK_SET) != 0) {
    free(rom->prg_data);
    free(rom->chr_data);
    fclose(rom_file);
    return ROM_MEM_ALLOC_FAIL;
  }

  if (fread(rom->chr_data, 1, rom->chr_size, rom_file) != rom->chr_size) {
    free(rom->prg_data);
    free(rom->chr_data);
    fclose(rom_file);
    return ROM_ERR_CHR_SIZE;
  }

  fclose(rom_file);
  return ROM_OK;
}
