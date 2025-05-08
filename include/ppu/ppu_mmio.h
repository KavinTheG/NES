#ifndef PPU_MMIO_H
#define PPU_MMIO_H

#include "ppu.h"

typedef struct PPU PPU;

// === MMIO Register Access ===
void ppu_registers_write(PPU *ppu, uint16_t addr, uint8_t val);
uint8_t ppu_registers_read(PPU *ppu, uint16_t addr);

#endif