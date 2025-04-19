#ifndef PPU_REGISTERS_H
#define PPU_REGISTERS_H

#include <stdint.h>

typedef struct {
    uint8_t PPUCTRL;
    uint8_t PPUMASK;
    uint8_t PPUSTATUS;
    uint8_t PPUSCROLL;
    uint16_t PPUADDR;
    uint8_t PPUDATA;

    uint8_t OAMADDR;
    uint8_t OAMDATA;
    uint8_t OAMDMA;
} PPURegisters;

extern PPURegisters ppu_registers;

#endif
