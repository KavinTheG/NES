#ifndef APU_MMIO_H
#define APU_MMIO_H

#include <stdint.h>

typedef struct APU_MMIO {
  uint32_t apu_mmio_write_mask;
  uint8_t regs[0x18];
} APU_MMIO;

void apu_mmio_init(APU_MMIO *apu_mmio);
void write_apu_mmio(APU_MMIO *apu_mmio, uint16_t addr, uint8_t val);

#endif
