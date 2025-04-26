#ifndef PPU_H
#define PPU_H

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

    uint8_t PPUDATA, PPUDATA_READ_BUFFER;

    uint8_t OAMDMA;


    // 15 registers
    uint16_t v, t; 
    uint8_t w;
    uint8_t x;
    unsigned char vblank_flag, nmi_flag;
    unsigned char update_graphics;

    uint32_t frame_buffer_render[256 * 240];


    // Fetch tile id from name table 
    uint8_t name_table_byte;

    // Fetch attribute byte for the corresponding nametable byte
    uint8_t attribute_byte;

    // Fetch high and low pattern table bytes
    uint8_t pattern_table_lsb;
    uint8_t pattern_table_msb;

    // Final palette ram address
    uint16_t palette_ram_addr;

    // Determines which palette to use for tile
    uint8_t palette_index;

    // Palette data
    uint8_t palette_data;

    uint8_t tile_pixel_value[TILE_SIZE];

    // Counters 
    int current_scanline_cycle, total_cycles; 
    int scanline;
    int frame;

} PPU;

void ppu_init(PPU *ppu);
void load_ppu_memory(PPU *ppu, unsigned char *chr_rom, int chr_size);
void load_ppu_oam_mem(PPU *ppu, uint8_t *dma_mem);
void load_ppu_ines_header(unsigned char *header);

//void load_ppu_mem(PPU *ppu, char *filename);
//uint8_t read_ppu_mem(uint16_t addr);

void load_palette(uint8_t *palette);


void ppu_execute_cycle(PPU *ppu);

void ppu_registers_write(PPU *ppu, uint16_t addr, uint8_t val);
uint8_t ppu_registers_read(PPU *ppu, uint16_t addr);
uint32_t* get_frame_buffer(PPU *ppu);

void reset_graphics_flag(PPU *ppu);

#endif