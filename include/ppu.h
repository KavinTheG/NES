#include <stdint.h>

typedef struct PPU
{
    // misc settings
    uint8_t PPUCTRL;

    uint8_t PPUMASK;
    
    uint8_t PPUSTATUS;

    uint8_t OAMADDR;

    uint8_t PPUSCROLL;

    uint8_t PPUADDR;

    uint8_t PPUDATA;

    uint8_t OAMDMA;
} PPU;


void ppu_init(PPU *cpu);

void load_ppu_mem(PPU *ppu, char *filename);