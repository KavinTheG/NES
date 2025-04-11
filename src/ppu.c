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

// Counter for # of cycles and scanlines
int cycle, scanline;

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

/* 
Fetch high and low byte from pattern table

// From address $0000 - $1FFF
*/
uint16_t fetch_pattern_table_bytes(uint8_t tile_index) {
        return  (read_ppu_mem((tile_index * 16) + 8) << 8) | read_ppu_mem(tile_index * 16);  
}


void ppu_execute_cycle(PPU *ppu) {

    uint16_t pattern_table_tile;
    uint8_t name_table_byte, attribute_byte;

    // Pre-render Scanline -1

    // Visible Scanline 0 - 239
    if (cycle >= 1 && cycle <= 256) {
        switch ((cycle - 1) % 8) {

            // Step 1
            case 0:
                name_table_byte = fetch_name_table_byte(ppu);
                cycle += 2;
                break;

            // Step 2
            case 2:
                attribute_byte = fetch_attr_table_byte(ppu);

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
                cycle +=2;
            break;

            // Step 4    
            case 4:
                pattern_table_tile = fetch_pattern_table_bytes(name_table_byte);
                cycle += 3;
                break;

            case 7:
                //Pixel output

                
                cycle += 2;
                break;
            // Step 5
        }
    } else if (cycle >= 257 && cycle <= 320) {
        // Tile data for the sprites on the next scanline are fetched
    }  else if (cycle >= 321 && cycle <= 336) {
        // First two tiles of the next scanline are fetched
    }  else if (cycle >= 337 && cycle <= 340) {
        // Two bytes are fetched for unknown reasons
    }

    // vblank Scanline 241-260

}