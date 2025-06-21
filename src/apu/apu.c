#include "apu/apu.h"

const uint16_t frame_4step[4] = {3729, 7457, 11186, 14915};
const uint16_t frame_step[5] = {3729, 7457, 11186, 14915, 18641};

static const uint8_t DUTY_PATTERNS[4][8] = {
    {0, 1, 0, 0, 0, 0, 0, 0}, // SIXTEENTH
    {0, 1, 1, 0, 0, 0, 0, 0}, // QUARTER
    {0, 1, 1, 1, 1, 0, 0, 0}, // HALF
    {1, 0, 0, 1, 1, 1, 1, 1}  // QUARTER_NEGATED
};

int length_table[32] = {10, 254, 20, 2,  40, 4,  80, 6,  160, 8,  60,
                        10, 14,  12, 26, 14, 12, 24, 48, 72,  96, 192,
                        16, 32,  14, 16, 18, 20, 22, 24, 26,  30};

void apu_update_parameters(APU *apu) {
  if (!apu->apu_mmio->apu_mmio_write_mask)
    return;

  int reg = __builtin_ctz(apu->apu_mmio->apu_mmio_write_mask);
  apu->apu_mmio->apu_mmio_write_mask = 0;

  switch (reg) {

  // $4000
  case 0:
    apu->pulse1->duty = (apu->apu_mmio->apu_pulse1_4000 & 0xC0) >> 6;

    apu->pulse1->length_counter_halt_flag =
        (apu->apu_mmio->apu_pulse1_4000 & 0x20) >> 5;

    apu->pulse1->constant_volume_flag =
        (apu->apu_mmio->apu_pulse1_4000 & 0x10) >> 4;

    apu->pulse1->envelope->divider->P = apu->apu_mmio->apu_pulse1_4000 & 0xF;
    break;

  // $4001
  case 1:
    apu->pulse1->sweep_enabled_flag =
        (apu->apu_mmio->apu_pulse1_4001 & 0x80) == 0x80;

    apu->pulse1->sweep_divider_period =
        (apu->apu_mmio->apu_pulse1_4001 & 0x70) >> 4;

    apu->pulse1->sweep_negate_flag =
        (apu->apu_mmio->apu_pulse1_4001 & 0x08) >> 3;

    apu->pulse1->sweep_shift_count = (apu->apu_mmio->apu_pulse1_4001 & 0x07);
    break;

  // $4002
  case 2:
    apu->pulse1->timer |= apu->apu_mmio->apu_pulse1_4002;
    break;

  // $4003
  case 3:
    // High 3 bits of timer
    apu->pulse1->timer |= (apu->apu_mmio->apu_pulse1_4003 & 0x07) << 8;

    // Length counter load
    apu->pulse1->length_counter_load =
        (apu->apu_mmio->apu_pulse1_4003 & 0xF8) >> 3;

    // Duration of a note
    apu->pulse1->length_counter =
        length_table[apu->pulse1->length_counter_load];
    break;

  case 23:
    // Determines frame counter mode
    apu->frame_counter.mode = apu->apu_mmio->apu_frame_counter_4017 & 0x80;
    apu->frame_counter.interrupt_inhibit_flag =
        apu->apu_mmio->apu_frame_counter_4017 & 0x40;

    apu->frame_counter.initial_apu_cycle = apu->apu_cycles;
    break;
  }
}

int apu_sweep_clocked(Pulse *pulse, uint8_t one_comp) {
  // If sweep is enabled, divider's counter is 0, shift count is nonzero
  if (pulse->sweep_enabled_flag && pulse->sweep_shift_count != 0 &&
      pulse->sweep->divider->counter == 0) {
    uint16_t change_value = pulse->timer >> pulse->sweep_shift_count;
    uint16_t target_value;

    if (pulse->sweep_negate_flag) {
      target_value = pulse->timer - change_value;

      if (one_comp)
        target_value -= 1;
    } else {
      target_value = pulse->timer + change_value;
    }

    if (target_value <= 0x7FF)
      pulse->timer = target_value;
  }

  if (pulse->sweep->divider->counter != 0)
    pulse->sweep->divider->counter--;

  // Reset sweep divider if counter is 0 or if reload flag is set
  if (pulse->sweep->divider->counter == 0 || pulse->sweep->reload_flag)
    pulse->sweep->divider->counter = pulse->sweep->divider->P;

  return 0;
}

int apu_length_counter_clocked(Pulse *pulse) {
  if (pulse->length_counter != 0)
    pulse->length_counter--;
  return 0;
}

uint8_t sequencer_pulse_lookup(Pulse *pulse) {
  return DUTY_PATTERNS[pulse->duty][pulse->sequencer_step_index];
  if (pulse->sequencer_step_index == 0)
    pulse->sequencer_step_index = 7;
  else
    pulse->sequencer_step_index--;
}

void apu_envelope_clocked(Envelope *envelope, uint8_t loop_flag) {
  if (envelope->envelope_start_flag == 0) {
    if (envelope->divider->counter != 0) {
      envelope->divider->counter--;
    } else {
      envelope->divider->counter = envelope->divider->P;
      if (envelope->envelope_decay_level_counter != 0) {
        envelope->envelope_decay_level_counter--;
      } else {
        if (loop_flag) {
          envelope->envelope_decay_level_counter = 15;
        }
      }
    }
  } else {
    envelope->envelope_start_flag = 0;
    envelope->envelope_decay_level_counter = 15;
    envelope->divider->counter = envelope->divider->P;
  }
}

void apu_clock_frame_counter(APU *apu) {
  uint64_t elapsed = apu->apu_cycles - apu->frame_counter.initial_apu_cycle;

  switch (elapsed) {
  case 3728:
    // Clock Envelopes & Triangle's Linear Counter
    apu_envelope_clocked(apu->pulse1->envelope,
                         apu->pulse1->length_counter_halt_flag);
    break;

  case 7456:
    // Clock Envelopes & Triangle's Linear Counter
    apu_envelope_clocked(apu->pulse1->envelope,
                         apu->pulse1->length_counter_halt_flag);

    // Decrement pulse1 length counter
    apu_length_counter_clocked(apu->pulse1);

    // Decrement pulse 1 sweep unit
    apu_sweep_clocked(apu->pulse1, 1);

    break;
  case 11185:
    // quarter frame
    // Clock Envelopes & Triangle's Linear Counter
    apu_envelope_clocked(apu->pulse1->envelope,
                         apu->pulse1->length_counter_halt_flag);
    // clock envelopes and linear counters
    break;

  case 14914:
    // quarter + half frame

    if (apu->frame_counter.mode == 0) {
      // Decrement pulse1 length counter
      apu_length_counter_clocked(apu->pulse1);

      // Decrement pulse 1 sweep unit
      apu_sweep_clocked(apu->pulse1, 1);

      // Clock Envelopes & Triangle's Linear Counter
      apu_envelope_clocked(apu->pulse1->envelope,
                           apu->pulse1->length_counter_halt_flag);
    }
    break;

  case 18640:
    // no IRQ, just reset
    if (apu->frame_counter.mode) {
      // Decrement pulse1 length counter
      apu_length_counter_clocked(apu->pulse1);

      // Decrement pulse 1 sweep unit
      apu_sweep_clocked(apu->pulse1, 1);

      // Clock Envelopes & Triangle's Linear Counter
      apu_envelope_clocked(apu->pulse1->envelope,
                           apu->pulse1->length_counter_halt_flag);
    }
    break;
  }
}

void apu_execute(APU *apu) {
  apu_update_parameters(apu);
  uint8_t sequencer_output = sequencer_pulse_lookup(apu->pulse1);

  // Clock frame counter
  apu_clock_frame_counter(apu);
  apu->apu_cycles++;
}
