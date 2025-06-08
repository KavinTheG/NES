#include "apu/apu.h"

void update_apu_parameters(APU *apu) {
  if (!apu->apu_mmio->apu_mmio_write_mask)
    return;

  int reg = __builtin_ctz(apu->apu_mmio->apu_mmio_write_mask);
  apu->apu_mmio->apu_mmio_write_mask = 0;

  switch (reg) {

  // $4000
  case 0:
    apu->pulse1.duty_cycle = (apu->apu_mmio->apu_pulse1_reg0 & 0xC0) >> 6;

    apu->pulse1.length_counter_halt_flag =
        (apu->apu_mmio->apu_pulse1_reg0 & 0x20) >> 5;

    apu->pulse1.constant_volume_flag =
        (apu->apu_mmio->apu_pulse1_reg0 & 0x10) >> 4;

    apu->pulse1.volume_divider_period = apu->apu_mmio->apu_pulse1_reg0 & 0xF;
    break;

  // $4001
  case 1:
    apu->pulse1.sweep_enabled_flag =
        (apu->apu_mmio->apu_pulse1_reg1 & 0x80) == 0x80;

    apu->pulse1.sweep_divider_period =
        (apu->apu_mmio->apu_pulse1_reg1 & 0x70) >> 4;

    apu->pulse1.sweep_negate_flag =
        (apu->apu_mmio->apu_pulse1_reg1 & 0x08) >> 3;

    apu->pulse1.sweep_shift_count = (apu->apu_mmio->apu_pulse1_reg1 & 0x07);
    break;

  // $4002
  case 2:
    apu->pulse1.timer |= apu->apu_mmio->apu_pulse1_reg2;
    break;

  // $4003
  case 3:
    // High 3 bits of timer
    apu->pulse1.timer |= (apu->apu_mmio->apu_pulse1_reg3 & 0x07) << 8;

    // Length counter load
    apu->pulse1.lenght_counter_load =
        (apu->apu_mmio->apu_pulse1_reg3 & 0xF8) >> 3;
    break;
  }
}

void apu_execute(APU *apu) { update_apu_parameters(apu); }
