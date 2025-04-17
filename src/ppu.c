/*
NES PPU implementation
Author: Kavin Gnanapandithan
*/


#include "ppu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 
#include <SDL2/SDL.h>
#include "queue.h"

/* Memory info*/
#define NES_HEADER_SIZE 16
#define MEMORY_SIZE 0x4000 // 16 KB

#define PALETTE_SIZE 64

#define OAM_SIZE 0xFF

#define SCREEN_WIDTH_VIS 256
#define SCREEN_HEIGHT_VIS 240

#define NUM_DOTS 341
#define NUM_SCANLINES 262


// Main PPU memory
uint8_t ppu_memory[MEMORY_SIZE] = {0};
uint8_t nes_header[NES_HEADER_SIZE] = {0};
uint8_t ppu_palette[PALETTE_SIZE * 3];

// Counter for # of cycles and scanlines
int cycle, total_cycles; 
int scanline = -1;
int frame;

// Nametable mirror arrangment flag
// 1 = h, v = 0
unsigned char nametable_mirror_flag;

// Drawing background vs sprite flag
unsigned char drawing_bg_flag;

// 256 seperate memoery dedicated to OAM
uint8_t oam_memory[OAM_SIZE] = {0};

// Buffer, holds up to 8 spirtes to be rendered on the next scanline
uint8_t oam_memory_secondary[32] = {0};

// Internal latches, holds actual rendering data
uint8_t oam_buffer_latches[32] = {0};

// Output buffer
uint8_t pixel_output_buffer[8] = {0};

// Tile buffer
Queue *tile_buffer_queue;

// Top of the secondary oam memory
int oam_secondary_top = 0;

// SDL Variables
SDL_Window *window;
SDL_Renderer *renderer;
unsigned char display[256][240];

void ppu_init(PPU *ppu) {
    /* init SDL graphics*/
    SDL_Init(SDL_INIT_EVERYTHING);
    window = SDL_CreateWindow("NES", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 256, 240, 0);

    init_queue(tile_buffer_queue);
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
    memcpy(&nes_header, header, NES_HEADER_SIZE);
    free(header);
}

void load_palette(uint8_t *palette) {
    memset(&ppu_palette, 0, PALETTE_SIZE * 3);
    memcpy(&ppu_palette, palette, PALETTE_SIZE * 3);
    free(palette);
}

void load_ppu_oam_mem(PPU *ppu, uint8_t *dma_mem) {
    memset(&oam_memory, 0, OAM_SIZE);
    memcpy(&oam_memory, dma_mem, OAM_SIZE);
    free(dma_mem);
}


uint8_t read_mem(PPU *ppu, uint16_t addr) {
    switch (addr & 0xF000) {
        case 0x0:
            // PPUCTRL bit 3 & 4 control sprite & background PT address respectively
            uint16_t nt_base_addr;
            
            if (drawing_bg_flag) {
                nt_base_addr = (ppu->PPUCTRL & 0x10) == 0x10 ? 1000 : 0;
            } else {
                
                if ( ppu->PPUCTRL & 0x10) {
                    // 8x16 sprites, base address determined by bit 0 of Byte 1
                    nt_base_addr = (addr & 0x01) ? 1000 : 0;
                } else {
                    nt_base_addr = (ppu->PPUCTRL & 0x08) == 0x08 ? 1000 : 0;
                }
            }

            return ppu_memory[nt_base_addr | addr];
            break;

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
    return read_mem(ppu, ppu->v);
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
uint16_t fetch_pattern_table_bytes(PPU *ppu, uint8_t tile_index) {
        return  (read_mem(ppu, (tile_index * 16) + 8) << 8) | read_mem(ppu, tile_index * 16);  
}




void ppu_execute_cycle(PPU *ppu) {

    // Pixel generation variables
    uint16_t pattern_table_tile;
    uint16_t palette_ram_addr;
    uint8_t tile_palette_data[TILE_SIZE];
    uint8_t name_table_byte, attribute_byte, palette_data;
    // Pre-render Scanline -1

    // Visible Scanline 0 - 239
    if (cycle >= 1 && cycle <= 256) {
        if (cycle == 1) {
            // set secondary oam to 0xFF
            memset(&oam_memory_secondary, 0xFF, 32);
        }

        uint8_t sprite_byte;
        if (cycle >= 65) {

            int oam_index = cycle - 65;
            int sprite_index = oam_index % 4;
            int sprite_size = (ppu->PPUCTRL & 0x20) == 0x20 ? 16 : 8;
            unsigned char copy_flag = 0;
            sprite_byte = oam_memory[oam_index];

            if (sprite_index == 0) {
                if ( (sprite_byte - 1) <= scanline && scanline < (sprite_byte - 1 + sprite_size) ) {
                    copy_flag = 1;
                } else {
                    copy_flag = 0;
                }
            }   

            if (copy_flag) {
                oam_secondary_top = oam_index % 32;
                oam_memory_secondary[oam_secondary_top] = sprite_byte;
            }
        }


        drawing_bg_flag = 1;
        
        // Store the index of the sprite if it exists in the current pixel
        int sprite_index = -1;

        // Check if tile is background or sprite
        for (int i = 0; i <= oam_secondary_top; i += 4) {

            uint8_t byte_3 = oam_buffer_latches[i + 3];
            
            if (byte_3 <= cycle && byte_3 > cycle) {
                // This pixel is a sprite pixel
                drawing_bg_flag = 0;
                sprite_index = i;
            }
        }



    
        
        switch (cycle % 8) {

            // Step 1
            case 1:
                name_table_byte = fetch_name_table_byte(ppu);
                break;

            // Step 2
            case 3:
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
                    palette_index = (attribute_byte & 0x0C) >> 2;
                } else if (tile_area_vertical == 1 && tile_area_horizontal == 0) {
                    // Bottom left quadrant (bit 5 & 4)
                    palette_index = (attribute_byte & 0x30) >> 4;
                } else if (tile_area_vertical == 1 && tile_area_horizontal == 1) {
                    // Bottom right quadrant (bit 7 6 6)
                    palette_index = (attribute_byte & 0xC0) >> 6;
                }

            break;

            // Step 4    
            case 5:
                if (drawing_bg_flag) {
                    pattern_table_tile = fetch_pattern_table_bytes(ppu, name_table_byte);
                } else {
                    // byte 1 is tile number
                    pattern_table_tile = fetch_pattern_table_bytes(ppu, oam_buffer_latches[sprite_index + 1]);
                }
                /* Tile fetch from cycle i is rendered on pixel i + 16 */
                for (int i = cycle + 16; i < cycle + 24; i++) {
                    unsigned char msb = (pattern_table_tile & 0x00FF) & (1 << (8- (i - cycle + 1)));
                    unsigned char lsb = ((pattern_table_tile & 0xFF) >> 8) & (1 << (8- (i - cycle + 1)));

                    // 0-indexed, subtract i from 1
                    display[scanline][i - 1] = (msb >> 1) | lsb; 
                }
                break;

            case 0:

                // Increment v 
                ppu->v += (ppu->PPUCTRL & 0x04) == 0x04 ? 32 : 1; 
                
                /* On the 8th cycle, data for tile N + 2 is fetched 
                    - Use the pixel value from tile pattern data,
                      palette number from attributes & background/sprite select
                      to generate a palette address
                    - Push this palette data from this address to the queue buffer
                */

                /* Pop tile buffer to get tile N*/
                memset(&tile_palette_data, pop(tile_buffer_queue), TILE_SIZE);  

                /* Push tile N + 2 to buffer */
                for (int i = 0; i < TILE_SIZE; i++) {

                    // Background/sprite select
                    if (drawing_bg_flag) {
                        palette_ram_addr = 0x3F00;
                    } else {
                        palette_ram_addr = 0x3F10;
                    }
                    
                    // Add 16 to get to position Tile N + 2
                    // Subtract 1 due to 0-index
                    palette_ram_addr |= display[scanline][i + 16 - 1];
                    
                    // Bit 3 and 2 are the palette number from attributes
                    palette_ram_addr |= (palette_index) << 2;

                    // Store in array of length 8 for 8 pixels (1 tile)
                    tile_palette_data[i] = palette_ram_addr;
                }

                // Push tile to buffer
                push(tile_buffer_queue, tile_palette_data);

                break;
            
        }

        palette_data = tile_palette_data[(cycle % 8) - 1 ];

        // Pixel gen
        SDL_SetRenderDrawColor(renderer, 
            ppu_palette[palette_data * 3],
            ppu_palette[palette_data * 3 + 1],
            ppu_palette[palette_data * 3 + 2],
            255);

        SDL_Rect rect = {cycle, scanline, 1, 1};
        SDL_RenderFillRect(renderer, &rect);

    } else if (cycle >= 257 && cycle <= 320) {
        // Tile data for the sprites on the next scanline are loaded into rendering latches
        oam_buffer_latches[(cycle - 257) % 32] = oam_memory_secondary[(cycle - 257) % 32];

    }  else if (cycle >= 321 && cycle <= 336) {
        // First two tiles of the next scanline are fetched
    }  else if (cycle >= 337 && cycle <= 340) {
        // Two bytes are fetched for unknown reasons
    }

    // vblank Scanline 241-260

}