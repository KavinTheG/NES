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

typedef struct Sweep {
  Divider *divider;
  uint8_t reload_flag;
} Sweep;

typedef struct Pulse {
  uint8_t constant_volume_flag;
  uint8_t length_counter_halt_flag;
  uint8_t sweep_negate_flag;
  uint8_t sweep_enabled_flag;

  uint8_t volume_divider_period;
  uint8_t duty_cycle;

  /* Sweep unit */
  uint8_t sweep_divider_period;
  uint8_t sweep_shift_count;
  Sweep *sweep;

  /* Timer unit */
  uint16_t timer;

  /* Length Counter */
  uint8_t lenght_counter_load;
} Pulse;

typedef struct APU {
  APU_MMIO *apu_mmio;

  Pulse pulse1;
} APU;

void apu_execute(APU *apu);
