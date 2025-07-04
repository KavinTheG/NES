#ifndef PPU_MMIO_H
#define PPU_MMIO_H

#include <stdint.h>

typedef struct PPU_MMIO {
  uint32_t ppu_mmio_write_mask;
  uint8_t regs[0x18];
  uint8_t write_toggle;
  uint8_t ppudata_read_buffer;
  uint8_t vram_pointer_palette_flag;
} PPU_MMIO;

void ppu_mmio_init(PPU_MMIO *ppu_mmio);
void write_ppu_mmio(PPU_MMIO *ppu_mmio, uint16_t addr, uint8_t val);
uint8_t read_ppu_mmio(PPU_MMIO *ppu_mmio, uint16_t addr);

#endif
