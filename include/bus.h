#include "cpu/cpu.h"
#include "ppu.h"

typedef struct Bus {
  PPU *ppu;
  Cpu6502 *cpu;
} Bus;
