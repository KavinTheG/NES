#include "apu/apu_mmio.h"

void write_apu_mmio(APU_MMIO *apu_mmio, uint16_t addr) {
  apu_mmio->apu_mmio_write_flag = 1;
  switch (addr) {
  case 0x4000:
    apu_mmio->apu_pulse1_reg0 = addr;
    break;
  case 0x4001:
    apu_mmio->apu_pulse1_reg1 = addr;
    break;
  case 0x4002:
    apu_mmio->apu_pulse1_reg2 = addr;
    break;
  case 0x4003:
    apu_mmio->apu_pulse1_reg3 = addr;
    break;
  }
}
