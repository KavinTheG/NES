#include "ppu_mmio.h"

#define PPU_REG_BASE 0x2000
#define PPU_REG_COUNT 0x18

void ppu_mmio_init(PPU_MMIO *ppu_mmio) {
  ppu_mmio->ppu_mmio_write_mask = 0;
  for (int i = 0; i < PPU_REG_COUNT; i++) {
    ppu_mmio->regs[i] = 0;
  }
  ppu_mmio->write_toggle = 0;
  ppu_mmio->ppudata_read_buffer = 0;
}

void write_ppu_mmio(PPU_MMIO *ppu_mmio, uint16_t addr, uint8_t val) {
  if (addr >= PPU_REG_BASE && addr < PPU_REG_BASE + PPU_REG_COUNT) {
    int reg = addr - PPU_REG_BASE;
    ppu_mmio->regs[reg] = val;
    ppu_mmio->ppu_mmio_write_mask |= (1 << reg);
  }
}

uint8_t read_ppu_mmio(PPU_MMIO *ppu_mmio, uint16_t addr) {

  if (addr == 0x2007) {
    if (ppu_mmio->vram_pointer_palette_flag)
      return ppu_mmio->ppudata_read_buffer;
  }

  if (addr >= PPU_REG_BASE && addr < PPU_REG_BASE + PPU_REG_COUNT) {
    int reg = addr - PPU_REG_BASE;
    return ppu_mmio->regs[reg];
  }
  return 0xFF;
}
