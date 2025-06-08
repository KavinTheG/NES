#include "apu_mmio.h"
#include <stdint.h>

struct Envelope;
struct Sweep;
struct Timer;
struct Sequencer;
struct LengthCounter;
struct Pulse;

struct APU;

typedef struct Pulse {
} Pulse;

typedef struct APU {
  APU_MMIO *apu_mmio;

  Pulse pulse1;
} APU;

void apu_execute(APU *apu);
