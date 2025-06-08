#include "apu/apu.h"

void update_apu_parameters(APU *apu) {
  if (!apu->apu_mmio->apu_mmio_write_flag)
    return;

  apu->apu_mmio->apu_mmio_write_flag = 0;
}

void apu_execute(APU *apu) { update_apu_parameters(apu); }
