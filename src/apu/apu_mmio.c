#include "apu/apu_mmio.h"

void write_apu_mmio(APU_MMIO *apu_mmio, uint16_t addr) {

  // Set a bit to 1 if the corresponding register was modified
  apu_mmio->apu_mmio_write_mask = (1 << (addr - 0x4000));

  switch (addr) {
  case 0x4000:
    apu_mmio->apu_pulse1_4000 = addr;
    break;
  case 0x4001:
    apu_mmio->apu_pulse1_4001 = addr;
    break;
  case 0x4002:
    apu_mmio->apu_pulse1_4002 = addr;
    break;
  case 0x4003:
    apu_mmio->apu_pulse1_4003 = addr;
    break;
  case 0x4017:
    apu_mmio->apu_frame_counter_4017 = addr;
    break;
  }
}
