#include "apu/apu_mmio.h"

void write_apu_mmio(APU_MMIO *apu_mmio, uint16_t addr, uint8_t val) {
  // Set a bit to 1 if the corresponding register was modified
  if (addr >= 0x4000 && addr <= 0x4017) {
    int reg = addr - 0x4000;

    // Update the MMIO register
    apu_mmio->regs[reg] = val;

    // Mark which register was written
    apu_mmio->apu_mmio_write_mask = (1 << reg);
  }
}

void apu_mmio_init(APU_MMIO *apu_mmio) { apu_mmio->apu_mmio_write_mask = 0; }
