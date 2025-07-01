#include "apu_mmio.h"
#include <stdint.h>

/*
 * Divider outputs a clock periodically.
 * - Contains a reload value P, which gets decremented
 * - Once counter reaches 0, divider emits a clock signal
 *    - Reloads counter with reload value P
 * */
typedef struct Divider {
  uint8_t P;
  uint8_t counter;
} Divider;

typedef struct Envelope {
  Divider *divider;
  uint8_t envelope_decay_level_counter;
  uint8_t envelope_start_flag;
  uint8_t constant_volume_flag;
  uint8_t volume;
} Envelope;

typedef struct Sweep {
  Divider *divider;
  uint8_t reload_flag;
} Sweep;

typedef struct FrameCounter {
  Divider *divider;
  uint8_t interrupt_inhibit_flag;
  uint8_t mode;
  int initial_apu_cycle;
} FrameCounter;

typedef struct Pulse {
  uint8_t length_counter_halt_flag;
  uint8_t sweep_negate_flag;
  uint8_t sweep_enabled_flag;
  uint8_t muted;

  Envelope *envelope;
  uint8_t duty;

  /* Sweep unit */
  uint8_t sweep_divider_period;
  uint8_t sweep_shift_count;
  Sweep *sweep;

  /* Timer unit */
  uint16_t timer;
  uint16_t counter;

  /* Length Counter */
  uint8_t length_counter_load;
  uint8_t length_counter;

  uint8_t sequencer_step_index;

} Pulse;

typedef struct Triangle {
  uint8_t control_flag;
  Divider *linear_counter;
  uint8_t linear_counter_reload_flag;
  uint8_t muted;

  uint16_t timer;
  uint16_t counter;
  uint8_t sequencer_step_index;

  uint8_t length_counter_load;
  uint8_t length_counter;
} Triangle;

typedef struct APU {
  APU_MMIO *apu_mmio;
  /* Track cpu cycles */
  uint8_t apu_status_register;

  Pulse *pulse1;
  Pulse *pulse2;
  Triangle *triangle;

  FrameCounter frame_counter;

  int apu_cycles;

} APU;

void apu_init(APU *apu, APU_MMIO *apu_mmio);
void apu_execute(APU *apu);
void apu_update_parameters(APU *apu);

int apu_sweep_clocked(Pulse *pulse, uint8_t one_comp);

uint8_t apu_output(APU *apu);
