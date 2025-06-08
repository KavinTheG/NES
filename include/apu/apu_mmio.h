#include <stdint.h>

typedef struct APU_MMIO {
  uint8_t apu_mmio_write_flag;
  uint8_t apu_pulse1_reg0;
  uint8_t apu_pulse1_reg1;
  uint8_t apu_pulse1_reg2;
  uint8_t apu_pulse1_reg3;
} APU_MMIO;

void write_apu_mmio(APU_MMIO *apu_mmio, uint16_t addr);
