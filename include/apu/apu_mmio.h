#ifndef APU_MMIO_H
#define APU_MMIO_H

#include <stdint.h>

typedef struct APU_MMIO {
  uint32_t apu_mmio_write_mask;
  uint8_t apu_pulse1_4000;
  uint8_t apu_pulse1_4001;
  uint8_t apu_pulse1_4002;
  uint8_t apu_pulse1_4003;

  uint8_t apu_frame_counter_4017;
} APU_MMIO;

void write_apu_mmio(APU_MMIO *apu_mmio, uint16_t addr);

#endif
