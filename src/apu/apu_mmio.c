#include "apu/apu_mmio.h"

void write_apu_mmio(APU_MMIO *apu_mmio, uint16_t addr, uint8_t val) {
  // Set a bit to 1 if the corresponding register was modified
  apu_mmio->apu_mmio_write_mask = (1 << (addr - 0x4000));

  switch (addr) {
  case 0x4000:
    apu_mmio->apu_pulse1_4000 = val;
    break;
  case 0x4001:
    apu_mmio->apu_pulse1_4001 = val;
    break;
  case 0x4002:
    apu_mmio->apu_pulse1_4002 = val;
    break;
  case 0x4003:
    apu_mmio->apu_pulse1_4003 = val;
    break;
  case 0x4015:
    apu_mmio->apu_status_4015 = val;
    break;
  case 0x4017:
    apu_mmio->apu_frame_counter_4017 = val;
    break;
  }
}

void apu_mmio_init(APU_MMIO *apu_mmio) {
  apu_mmio->apu_mmio_write_mask = 0;
  apu_mmio->apu_pulse1_4000 = 0;
  apu_mmio->apu_pulse1_4001 = 0;
  apu_mmio->apu_pulse1_4002 = 0;
  apu_mmio->apu_pulse1_4003 = 0;

  apu_mmio->apu_status_4015 = 0;
  apu_mmio->apu_frame_counter_4017 = 0;
}
