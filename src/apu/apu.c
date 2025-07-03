#include "apu/apu.h"
#include "apu/apu_mmio.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const uint16_t frame_step[5] = {7457, 14913, 22371, 29829, 37281};

static const uint8_t DUTY_PATTERNS[4][8] = {
    {0, 1, 0, 0, 0, 0, 0, 0}, // SIXTEENTH
    {0, 1, 1, 0, 0, 0, 0, 0}, // QUARTER
    {0, 1, 1, 1, 1, 0, 0, 0}, // HALF
    {1, 0, 0, 1, 1, 1, 1, 1}  // QUARTER_NEGATED
};

static const uint8_t triangle_sequence[32] = {
    0,  1,  2,  3,  4,  5,  6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
    15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5,  4,  3,  2,  1,  0};

int length_table[32] = {10, 254, 20, 2,  40, 4,  80, 6,  160, 8,  60,
                        10, 14,  12, 26, 14, 12, 24, 48, 72,  96, 192,
                        16, 32,  14, 16, 18, 20, 22, 24, 26,  30};

void apu_init(APU *apu, APU_MMIO *apu_mmio) {
  memset(apu, 0, sizeof(APU));

  apu->apu_mmio = apu_mmio;
  apu->apu_status_register = 0;
  apu->apu_cycles = 0;

  apu->triangle = malloc(sizeof(Triangle));
  memset(apu->triangle, 0, sizeof(Triangle));

  Triangle *tri = apu->triangle;

  tri->control_flag = 0;
  tri->muted = 0;
  tri->linear_counter = malloc(sizeof(Divider));
  memset(tri->linear_counter, 0, sizeof(Divider));

  tri->timer = 0;
  tri->counter = 0;
  tri->sequencer_step_index = 0;
  tri->length_counter_load = 0;
  tri->length_counter = 0;

  apu->pulse1 = malloc(sizeof(Pulse));
  memset(apu->pulse1, 0, sizeof(Pulse));

  Pulse *pulse1 = apu->pulse1;

  // Allocate and initialize envelope
  pulse1->envelope = malloc(sizeof(Envelope));
  memset(pulse1->envelope, 0, sizeof(Envelope));
  pulse1->envelope->divider = malloc(sizeof(Divider));
  memset(pulse1->envelope->divider, 0, sizeof(Divider));

  // Allocate and initialize sweep
  pulse1->sweep = malloc(sizeof(Sweep));
  memset(pulse1->sweep, 0, sizeof(Sweep));
  pulse1->sweep->divider = malloc(sizeof(Divider));
  memset(pulse1->sweep->divider, 0, sizeof(Divider));

  // Initialize pulse timer and counters
  pulse1->counter = 0;
  pulse1->length_counter = 0;
  pulse1->length_counter_load = 0;
  pulse1->sequencer_step_index = 0;

  pulse1->muted = 0;
  pulse1->duty = 0;
  pulse1->sweep_divider_period = 0;
  pulse1->sweep_shift_count = 0;
  pulse1->sweep_enabled_flag = 0;
  pulse1->sweep_negate_flag = 0;
  pulse1->length_counter_halt_flag = 0;

  apu->pulse2 = malloc(sizeof(Pulse));
  memset(apu->pulse2, 0, sizeof(Pulse));

  Pulse *pulse2 = apu->pulse2;

  // Allocate and initialize envelope
  pulse2->envelope = malloc(sizeof(Envelope));
  memset(pulse2->envelope, 0, sizeof(Envelope));
  pulse2->envelope->divider = malloc(sizeof(Divider));
  memset(pulse2->envelope->divider, 0, sizeof(Divider));

  // Allocate and initialize sweep
  pulse2->sweep = malloc(sizeof(Sweep));
  memset(pulse2->sweep, 0, sizeof(Sweep));
  pulse2->sweep->divider = malloc(sizeof(Divider));
  memset(pulse2->sweep->divider, 0, sizeof(Divider));

  // Initialize pulse timer and counters
  pulse2->timer = 0;
  pulse2->counter = 0;
  pulse2->length_counter = 0;
  pulse2->length_counter_load = 0;
  pulse2->sequencer_step_index = 0;

  pulse2->muted = 0;
  pulse2->duty = 0;
  pulse2->sweep_divider_period = 0;
  pulse2->sweep_shift_count = 0;
  pulse2->sweep_enabled_flag = 0;
  pulse2->sweep_negate_flag = 0;
  pulse2->length_counter_halt_flag = 0;

  // Initialize frame counter
  apu->frame_counter.divider = malloc(sizeof(Divider));
  memset(apu->frame_counter.divider, 0, sizeof(Divider));
  apu->frame_counter.mode = 0;
  apu->frame_counter.step = 0;
  apu->frame_counter.interrupt_inhibit_flag = 0;
  apu->frame_counter.initial_apu_cycle = 0;
}

void apu_update_parameters(APU *apu) {
  if (!apu->apu_mmio->apu_mmio_write_mask)
    return;

  int reg = __builtin_ctz(apu->apu_mmio->apu_mmio_write_mask);
  apu->apu_mmio->apu_mmio_write_mask = 0;

  uint8_t val = apu->apu_mmio->regs[reg];

  switch (reg) {
  // $4000
  case 0:
    apu->pulse1->duty = (val & 0xC0) >> 6;
    apu->pulse1->length_counter_halt_flag = (val & 0x20) >> 5;
    apu->pulse1->envelope->constant_volume_flag = (val & 0x10) >> 4;
    apu->pulse1->envelope->divider->P = val & 0x0F;
    break;

  // $4001
  case 1:
    apu->pulse1->sweep_enabled_flag = (val & 0x80) != 0;
    apu->pulse1->sweep_divider_period = (val & 0x70) >> 4;
    apu->pulse1->sweep_negate_flag = (val & 0x08) >> 3;
    apu->pulse1->sweep_shift_count = val & 0x07;
    break;

  // $4002
  case 2:
    apu->pulse1->timer &= 0xFF00; // Clear low 8 bits
    apu->pulse1->timer |= val;    // Set low 8 bits
    break;

  // $4003
  case 3:
    apu->pulse1->timer &= 0x00FF;            // Clear high 3 bits
    apu->pulse1->timer |= (val & 0x07) << 8; // Set high 3 bits
    apu->pulse1->counter = apu->pulse1->timer;

    apu->pulse1->length_counter_load = (val & 0xF8) >> 3;
    apu->pulse1->length_counter =
        length_table[apu->pulse1->length_counter_load];

    apu->pulse1->envelope->envelope_start_flag = 1;
    break;

  // $4004
  case 4:
    apu->pulse2->duty = (val & 0xC0) >> 6;
    apu->pulse2->length_counter_halt_flag = (val & 0x20) >> 5;
    apu->pulse2->envelope->constant_volume_flag = (val & 0x10) >> 4;
    apu->pulse2->envelope->divider->P = val & 0x0F;
    break;

  // $4001
  case 5:
    apu->pulse2->sweep_enabled_flag = (val & 0x80) != 0;
    apu->pulse2->sweep_divider_period = (val & 0x70) >> 4;
    apu->pulse2->sweep_negate_flag = (val & 0x08) >> 3;
    apu->pulse2->sweep_shift_count = val & 0x07;
    break;

  // $4002
  case 6:
    apu->pulse2->timer &= 0xFF00; // Clear low 8 bits
    apu->pulse2->timer |= val;    // Set low 8 bits
    break;

  // $4003
  case 7:
    apu->pulse2->timer &= 0x00FF;            // Clear high 3 bits
    apu->pulse2->timer |= (val & 0x07) << 8; // Set high 3 bits
    apu->pulse2->counter = apu->pulse1->timer;

    apu->pulse2->length_counter_load = (val & 0xF8) >> 3;
    apu->pulse2->length_counter =
        length_table[apu->pulse2->length_counter_load];
    apu->pulse2->envelope->envelope_start_flag = 1;
    break;

  case 8:
    apu->triangle->control_flag = val & 0x80;
    apu->triangle->linear_counter->P = val & 0x7F;
    apu->triangle->linear_counter->counter = val & 0x7F;
    break;

  case 10:
    apu->triangle->timer = val;
    break;

  case 11:
    apu->triangle->timer &= 0x00FF;
    apu->triangle->timer = ((val & 0x07) << 8);

    apu->triangle->length_counter_load = (val & 0xF8) >> 3;
    apu->triangle->length_counter =
        length_table[apu->triangle->length_counter_load];

    apu->triangle->linear_counter_reload_flag = 1;
    break;

  // $4015
  case 21:
    apu->apu_status_register = val;
    apu->pulse1->muted = !(val & 0x1);
    apu->pulse2->muted = !(val & 0x2);

    if (!(val & 0x04)) {
      apu->triangle->length_counter = 0;
      apu->triangle->muted = 1;
    } else {
      apu->triangle->muted = 0;
    }
    break;

  // $4017
  case 23:
    apu->frame_counter.mode = (val & 0x80) >> 7;
    apu->frame_counter.interrupt_inhibit_flag = (val & 0x40) >> 6;
    apu->frame_counter.initial_apu_cycle = apu->apu_cycles;
    break;

  default:
    // Ignore unimplemented registers for now
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

    if (target_value <= 0x7FF) {
      pulse->timer = target_value;
      pulse->muted = 0;
    } else {
      pulse->muted = 1;
    }

    if (target_value < 8) {
      pulse->muted = 1;
    }
  }

  if (pulse->sweep->divider->counter != 0)
    pulse->sweep->divider->counter--;

  // Reset sweep divider if counter is 0 or if reload flag is set
  if (pulse->sweep->divider->counter == 0 || pulse->sweep->reload_flag)
    pulse->sweep->divider->counter = pulse->sweep->divider->P;

  return 0;
}

void apu_length_counter_clocked(Pulse *pulse) {
  if (pulse->length_counter && !pulse->length_counter_halt_flag)
    pulse->length_counter--;
}

void apu_tri_length_counter_clocked(Triangle *tri) {
  if (tri->length_counter && !tri->control_flag)
    tri->length_counter--;
}

void apu_tri_linear_counter_clocked(Triangle *tri) {
  if (tri->linear_counter_reload_flag) {
    tri->linear_counter->counter = tri->linear_counter->P;
  } else if (!tri->linear_counter->counter)
    tri->linear_counter->counter--;

  if (tri->control_flag)
    tri->linear_counter_reload_flag = 0;
}

void apu_pulse_sequencer_clocked(Pulse *pulse) {
  if (pulse->counter == 0) {
    pulse->sequencer_step_index = (pulse->sequencer_step_index + 1) & 0x07;

    pulse->counter = pulse->timer + 1;
  } else {
    pulse->counter--;
  }
}

void apu_tri_sequencer_clocked(Triangle *tri) {
  if (tri->length_counter == 0 || tri->linear_counter->counter == 0)
    return;

  if (!tri->counter) {
    tri->counter = tri->timer;
    tri->sequencer_step_index = (tri->sequencer_step_index - 1) & 0x1F;
  } else {
    tri->counter--;
  }
}

uint8_t apu_triangle_output(Triangle *tri) {
  if (tri->length_counter == 0 || tri->linear_counter->counter == 0 ||
      tri->timer < 2 || tri->muted)
    return 0;

  return triangle_sequence[tri->sequencer_step_index];
}

uint8_t apu_pulse_output(Pulse *pulse) {
  if (pulse->length_counter == 0 || pulse->muted)
    return 0;

  uint8_t duty_value = DUTY_PATTERNS[pulse->duty][pulse->sequencer_step_index];

  return duty_value ? pulse->envelope->volume : 0;
}

uint8_t apu_output(APU *apu) {
  uint8_t p1 = apu_pulse_output(apu->pulse1);
  uint8_t p2 = apu_pulse_output(apu->pulse2);
  uint8_t tri = 0; // apu_triangle_output(apu->triangle);

  return (p1 + p2 + tri) / 3;
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

  if (envelope->constant_volume_flag) {
    envelope->volume = envelope->divider->P;
  } else {
    envelope->volume = envelope->envelope_decay_level_counter;
  }
}

void apu_clock_frame_counter(APU *apu) {
  uint64_t elapsed = apu->apu_cycles - apu->frame_counter.initial_apu_cycle;

  switch (elapsed) {
  case 3728 * 2:
    // Clock Envelopes & Triangle's Linear Counter
    apu_envelope_clocked(apu->pulse1->envelope,
                         apu->pulse1->length_counter_halt_flag);
    apu_envelope_clocked(apu->pulse2->envelope,
                         apu->pulse2->length_counter_halt_flag);

    apu_tri_linear_counter_clocked(apu->triangle);
    break;

  case 7456 * 2:
    // Clock Envelopes & Triangle's Linear Counter
    apu_envelope_clocked(apu->pulse1->envelope,
                         apu->pulse1->length_counter_halt_flag);

    // Decrement pulse1 length counter
    apu_length_counter_clocked(apu->pulse1);

    // Decrement pulse 1 sweep unit
    apu_sweep_clocked(apu->pulse1, 1);

    apu_envelope_clocked(apu->pulse2->envelope,
                         apu->pulse2->length_counter_halt_flag);

    // Decrement pulse 2l ength counter
    apu_length_counter_clocked(apu->pulse2);

    // Decrement pulse 2 sweep unit
    apu_sweep_clocked(apu->pulse2, 0);

    // Decrement triangle length counter
    apu_tri_linear_counter_clocked(apu->triangle);
    apu_tri_length_counter_clocked(apu->triangle);
    break;

  case 11185 * 2:
    // quarter frame
    // Clock Envelopes & Triangle's Linear Counter
    apu_envelope_clocked(apu->pulse1->envelope,
                         apu->pulse1->length_counter_halt_flag);
    // clock envelopes and linear counters
    apu_envelope_clocked(apu->pulse2->envelope,
                         apu->pulse2->length_counter_halt_flag);
    apu_tri_linear_counter_clocked(apu->triangle);
    break;

  case 14914 * 2:
    // quarter + half frame

    if (apu->frame_counter.mode == 0) {
      // Decrement pulse1 length counter
      apu_length_counter_clocked(apu->pulse1);

      // Decrement pulse 1 sweep unit
      apu_sweep_clocked(apu->pulse1, 1);

      // Clock Envelopes & Triangle's Linear Counter
      apu_envelope_clocked(apu->pulse1->envelope,
                           apu->pulse1->length_counter_halt_flag);

      // Decrement pulse1 length counter
      apu_length_counter_clocked(apu->pulse2);

      // Decrement pulse 1 sweep unit
      apu_sweep_clocked(apu->pulse2, 1);

      // Clock Envelopes & Triangle's Linear Counter
      apu_envelope_clocked(apu->pulse2->envelope,
                           apu->pulse2->length_counter_halt_flag);

      // Decrement triangle length counter
      apu_tri_linear_counter_clocked(apu->triangle);
      apu_tri_length_counter_clocked(apu->triangle);

      apu->frame_counter.initial_apu_cycle = apu->apu_cycles;
    }
    break;

  case 18640 * 2:
    // no IRQ, just reset
    if (apu->frame_counter.mode) {
      // Decrement pulse1 length counter
      apu_length_counter_clocked(apu->pulse1);

      // Decrement pulse 1 sweep unit
      apu_sweep_clocked(apu->pulse1, 1);

      // Clock Envelopes & Triangle's Linear Counter
      apu_envelope_clocked(apu->pulse1->envelope,
                           apu->pulse1->length_counter_halt_flag);
      // Decrement pulse1 length counter
      apu_length_counter_clocked(apu->pulse2);

      // Decrement pulse 1 sweep unit
      apu_sweep_clocked(apu->pulse2, 0);

      // Clock Envelopes & Triangle's Linear Counter
      apu_envelope_clocked(apu->pulse2->envelope,
                           apu->pulse2->length_counter_halt_flag);

      // Decrement triangle length counter
      apu_tri_linear_counter_clocked(apu->triangle);
      apu_tri_length_counter_clocked(apu->triangle);
      apu->frame_counter.initial_apu_cycle = apu->apu_cycles;
    }
    break;
  }
}

void apu_execute(APU *apu) {
  apu->apu_cycle_count++;
  apu_update_parameters(apu);

  if (apu->apu_cycles % 2 == 0) {
    apu_pulse_sequencer_clocked(apu->pulse1);
    apu_pulse_sequencer_clocked(apu->pulse2);
  }

  apu_tri_sequencer_clocked(apu->triangle);

  // uint8_t volume = apu_pulse_output(apu->pulse1);

  // Clock frame counter
  apu_clock_frame_counter(apu);
  apu->apu_cycles++;
}

void apu_destroy(APU *apu) {
  if (!apu)
    return;

  // Free Triangle
  if (apu->triangle) {
    free(apu->triangle->linear_counter);
    free(apu->triangle);
  }

  // Free Pulse 1
  if (apu->pulse1) {
    if (apu->pulse1->envelope) {
      free(apu->pulse1->envelope->divider);
      free(apu->pulse1->envelope);
    }
    if (apu->pulse1->sweep) {
      free(apu->pulse1->sweep->divider);
      free(apu->pulse1->sweep);
    }
    free(apu->pulse1);
  }

  // Free Pulse 2
  if (apu->pulse2) {
    if (apu->pulse2->envelope) {
      free(apu->pulse2->envelope->divider);
      free(apu->pulse2->envelope);
    }
    if (apu->pulse2->sweep) {
      free(apu->pulse2->sweep->divider);
      free(apu->pulse2->sweep);
    }
    free(apu->pulse2);
  }

  // Free Frame Counter Divider
  if (apu->frame_counter.divider) {
    free(apu->frame_counter.divider);
  }

  // Optionally: clear the structure to catch use-after-free bugs
  memset(apu, 0, sizeof(APU));
}
