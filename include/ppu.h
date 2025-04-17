#include "config.h"

#include <stdint.h>

typedef struct PPU
{
    // misc settings
    uint8_t PPUCTRL;

    uint8_t PPUMASK;
    
    uint8_t PPUSTATUS;

    uint8_t OAMADDR;

    uint8_t OAMDATA;

    uint8_t PPUSCROLL;

    // PPUADDR is the value CPU writes to $2006
    uint16_t PPUADDR;

    uint8_t PPUDATA;

    uint8_t OAMDMA;


    // 15 registers
    uint16_t v, t; 
    unsigned char w;
} PPU;


struct RGB {
    uint8_t r, g, b;
} RGB;

void ppu_init(PPU *ppu);
void load_ppu_memory(PPU *ppu, unsigned char *chr_rom, int chr_size);
void load_ppu_oam_mem(PPU *ppu, uint8_t *dma_mem);
void load_ppu_ines_header(unsigned char *header);

//void load_ppu_mem(PPU *ppu, char *filename);
//uint8_t read_ppu_mem(uint16_t addr);

void load_palette(uint8_t *palette);

// Setters
void set_w_reg(PPU *ppu, unsigned char w);
void set_v_reg(PPU *ppu);

void set_PPUADDR(PPU *ppu, uint16_t PPUADDR);