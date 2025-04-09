/*
NES PPU implementation
Author: Kavin Gnanapandithan
*/


#include "ppu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 

#define NES_HEADER_SIZE 16
#define MEMORY_SIZE 0x4000 // 16 KB
#define PALETTE_SIZE 64
#define OAM_SIZE 0xFF

// Main PPU memory
uint8_t ppu_memory[MEMORY_SIZE] = {0};
uint8_t nes_header[NES_HEADER_SIZE] = {0};
uint8_t ppu_palette[PALETTE_SIZE * 3];

// Nametable mirror arrangment flag
// 1 = h, v = 0
unsigned char nametable_mirror_flag;

// 256 seperate memoery dedicated to OAM
uint8_t oam_memory[OAM_SIZE] = {0};

void ppu_init(PPU *ppu) {
    /* TODO */

    /* Header Config */
    
    // Flag 6
    // Bit 0 determine h/v n.t arrangment (h = 1, v = 0)
    nametable_mirror_flag = ((nes_header[6] & 0x01) == 0x01);
}

void load_ppu_memory(PPU *ppu, unsigned char *chr_rom, int chr_size) {
    // Clear ppu_memory
    memset(ppu_memory, 0, MEMORY_SIZE);

    // Load CHR ROM into 0x0000 - 0x1FFF
    memcpy(&ppu_memory[0x0000], chr_rom, chr_size);

    free(chr_rom);    
}

void load_ppu_ines_header(unsigned char *header) {
    memset(&nes_header, 0, NES_HEADER_SIZE);
    memcpy(&ppu_memory, header, NES_HEADER_SIZE);
    free(header);
}

void load_palette(uint8_t palette) {
    memcpy(&ppu_palette, palette, PALETTE_SIZE * 3);
}

uint8_t read_ppu_mem(uint16_t addr) {


    switch (addr & 0xF000) {
        
        // Nametable memory region ($2000 - $2FFF)
        case 0x2000:
            return ppu_memory[(addr & 0x7FF & !nametable_mirror_flag) | (addr & ~0xF400 & nametable_mirror_flag)];
            break;



        default:
            return ppu_memory[addr];
            break;
    }
 
}

void set_w_reg(PPU *ppu, unsigned char w) {
    ppu->w = w;
}

void set_v_reg(PPU *ppu) {
    ppu->v = ppu->PPUADDR;
}

void set_PPUADDR(PPU *ppu, uint16_t PPUADDR) {
    ppu->PPUADDR = PPUADDR;
}

/* 
Fetch tile from name table
Note: 
- PPU has 2KB of VRAM
- Mapped to 4KB of PPU memory ($2000 - $2FFF)
*/
uint8_t fetch_name_table_byte(PPU *ppu) {
    return read_ppu_mem(ppu->v);
}

/*
Fetch byte from attribute table
Note:
- AT is a 64-byte array
- at the end of each nametable
    - 23C0, 27C0, 2BC0, 2FC0
*/
uint8_t fetch_attr_table_byte(PPU *ppu) {

    uint16_t name_table_base = ppu->v & 0xFC00;
    uint16_t attribute_table_base = name_table_base + 0x3C0;  

    uint8_t bit_zero_offset = (ppu->v & 0x000F) / 4;
    uint8_t  bit_one_offset = ((ppu->v & 0x00F0) / 0x20) / 4;
    
    return attribute_table_base + bit_one_offset + bit_zero_offset;

    
}

// Fetch tile graphics from pattern table
// From address $0000 - $1FFF
void fetch_pattern_table_bytes() {
}


void draw_pixel(PPU *ppu) {

    // Step 1
    uint8_t name_table_byte = fetch_name_table_byte(ppu);


    // Step 2
    uint8_t attribute_byte = fetch_attr_table_byte(ppu);

    // Determines which of the four areas of the attribute byte to use
    uint8_t tile_area_horizontal = (ppu->v & 0x00F) % 4;
    uint8_t tile_area_vertical = ((ppu->v & 0x0F0) / 0x20) % 4;
    uint8_t palette_index;

    if (tile_area_vertical == 0 && tile_area_horizontal == 0) {
        // Top left quadrant (bit 1 & 0)
        palette_index = attribute_byte & 0x03; 
    } else if (tile_area_vertical == 0 && tile_area_horizontal == 1) {
        // Top right quadrant (bit 3 & 2)
        palette_index = attribute_byte & 0x0C;
    } else if (tile_area_vertical == 1 && tile_area_horizontal == 0) {
        // Bottom left quadrant (bit 5 & 4)
        palette_index = attribute_byte & 0x30;
    } else if (tile_area_vertical == 1 && tile_area_horizontal == 1) {
        // Bottom right quadrant (bit 7 6 6)
        palette_index = attribute_byte = 0xC0;
    }

    // Step 3
    ppu->v += (ppu->PPUCTRL & 0x04) == 0x04 ? 32 : 1; 

    // Step 4    
    
    // Step 5

}