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
Queue tile_buffer_queue;

// Top of the secondary oam memory
int oam_secondary_top = 0;

// SDL Variables
SDL_Window *window;
SDL_Renderer *renderer;
unsigned char display[256][240];


uint8_t open_bus;

void ppu_init(PPU *ppu) {
    /* init SDL graphics*/
    SDL_Init(SDL_INIT_EVERYTHING);
    window = SDL_CreateWindow("NES", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 256, 240, 0);
    renderer = SDL_CreateRenderer(window, -1, 0);

    init_queue(&tile_buffer_queue);
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

    //free(chr_rom);    
}

void load_ppu_ines_header(unsigned char *header) {
    memset(&nes_header, 0, NES_HEADER_SIZE);
    memcpy(&nes_header, header, NES_HEADER_SIZE);
    //free(header);
}

void load_palette(uint8_t *palette) {
    memset(&ppu_palette, 0, PALETTE_SIZE * 3);
    memcpy(&ppu_palette, palette, PALETTE_SIZE * 3);
}

void load_ppu_oam_mem(PPU *ppu, uint8_t *dma_mem) {
    memset(&oam_memory, 0, OAM_SIZE);
    memcpy(&oam_memory, dma_mem, OAM_SIZE);
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

void ppu_registers_write(PPU *ppu, uint16_t addr, uint8_t val) {
    switch (addr)  {

        // PPUCTRL
        case 0x2000:
            ppu->PPUCTRL = val;
            break;

        // PPUMASK
        case 0x2001:
            ppu->PPUMASK = val;
            break;

        // PPUSTATUS
        case 0x2002:
            ppu->PPUSTATUS = val;
            break;

        // OAMADDR
        case 0x2003:
            ppu->OAMADDR = val;
            break;

        // OAMDATA
        case 0x2004:
            ppu->OAMDATA = val;
            break;

        // PPUSCROLL
        case 0x2005:
            ppu->PPUSCROLL = val;
            ppu->w = (ppu->w == 0) ? 1 : 0;
            break;

        // PPUADDR
        case 0x2006:
            // First write
            if (!ppu->w) {
                ppu->PPUADDR = val << 8;
                ppu->t = ppu->PPUADDR;
            } else {
                ppu->PPUADDR |= val;
                ppu->t = ppu->PPUADDR;
                ppu->v = ppu->PPUADDR;
                ppu->PPUDATA = read_mem(ppu, ppu->PPUADDR);
            }     
            ppu->w = !ppu->w;
            break;

        // PPUDATA
        case 0x2007:
            ppu->PPUDATA = val;
            break;

        case 0x4014:
            ppu->OAMDMA = val;
            break;
    }
}

uint8_t ppu_registers_read(PPU *ppu, uint16_t addr) {

    switch (addr)  {

        // PPUCTRL
        case 0x2000:
            return open_bus;
            break;

        // PPUMASK
        case 0x2001:
            return open_bus;
            break;

        // PPUSTATUS
        case 0x2002:
            open_bus = ppu->PPUSTATUS;

            // Clear the 7th bit (vblank flag)
            ppu->PPUSTATUS &= ~0x80;
            ppu->vblank_flag = 0;

            // Clear w register
            ppu->w = 0;

            return open_bus;
            break;

        // OAMADDR
        case 0x2003:
            return open_bus;
            break;

        // OAMDATA
        case 0x2004:
            open_bus = ppu->OAMDATA;
            return open_bus;
            break;

        // PPUSCROLL
        case 0x2005:
            return open_bus;
            break;

        // PPUADDR
        case 0x2006:
            return open_bus;
            break;

        // PPUDATA
        case 0x2007:
            open_bus = ppu->PPUDATA_READ_BUFFER;
            ppu->PPUDATA_READ_BUFFER = read_mem(ppu, ppu->v & 0x3FFF);

            // If data is within palette ram, do immediate return
            // Otherwise, return buffer
            if ((ppu->v & 0x3FFF) >= 0x3F00) {
                open_bus = ppu->PPUDATA_READ_BUFFER;
            }

            // Increment v after PPUDATA read
            ppu->v += (ppu->PPUCTRL & 0x04) == 0x04 ? 32 : 1; 
            return open_bus;
            break;
    }
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
    uint8_t bit_one_offset = ((ppu->v & 0x00F0) / 0x20) / 4;
    
    return attribute_table_base + bit_one_offset + bit_zero_offset;
}

/* 
Fetch high and low byte from pattern table
From address $0000 - $1FFF
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

    if ((ppu->current_frame_cycle 
>= 1 && ppu->current_frame_cycle 
<= 256) || (ppu->current_frame_cycle 
>= 321 && ppu->current_frame_cycle 
<= 336) ) {
        if (ppu->current_frame_cycle 
== 1) {
            // set secondary oam to 0xFF
            memset(oam_memory_secondary, 0xFF, 32);
        }

        uint8_t sprite_byte;
        if (ppu->current_frame_cycle 
>= 65) {

            int oam_index = ppu->current_frame_cycle 
- 65;
            int sprite_index = oam_index % 4;
            int sprite_size = (ppu->PPUCTRL & 0x20) == 0x20 ? 16 : 8;
            unsigned char copy_flag = 0;
            sprite_byte = oam_memory[oam_index];

            if (sprite_index == 0) {
                if ( (sprite_byte - 1) <= ppu->scanline && ppu->scanline < (sprite_byte - 1 + sprite_size) ) {
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
            
            if (byte_3 <= ppu->current_frame_cycle && byte_3 > ppu->current_frame_cycle) {
                // This pixel is a sprite pixel
                drawing_bg_flag = 0;
                sprite_index = i;
            }
        }


        switch (ppu->current_frame_cycle
% 8) {

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
                /* Tile fetch from cycle i is rendered on pixel i + 8 */
                for (int i = ppu->current_frame_cycle + 8; i < ppu->current_frame_cycle + 16; i++) {
                    unsigned char msb = (pattern_table_tile & 0x00FF) & (1 << (8- (i - ppu->current_frame_cycle+ 1)));
                    unsigned char lsb = ((pattern_table_tile & 0xFF) >> 8) & (1 << (8- (i - ppu->current_frame_cycle+ 1)));

                    // 0-indexed, subtract i from 1
                    display[ppu->scanline][i - 1] = (msb >> 1) | lsb; 
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
                memcpy(&tile_palette_data, pop(&tile_buffer_queue), TILE_SIZE);  

                /* 
                Push tile N + 2 to buffer 
                - Two tiles ahead
                    - On cycle 8, tile 3 is being fetched
                    - So on cycle 240, tile 32 is being
                    - ignore tile fetched on cycle 248 
                */
                if (ppu->current_frame_cycle!= 248 && ppu->current_frame_cycle!= 256) {
                    for (int i = 0; i < TILE_SIZE; i++) {

                        // Background/sprite select
                        if (drawing_bg_flag) {
                            palette_ram_addr = 0x3F00;
                        } else {
                            palette_ram_addr = 0x3F10;
                        }
                        
                        // Add 16 to get to position Tile N + 2
                        // Subtract 1 due to 0-index
                        palette_ram_addr |= display[ppu->scanline][i + 8 - 1];
                        
                        // Bit 3 and 2 are the palette number from attributes
                        palette_ram_addr |= (palette_index) << 2;

                        // Store in array of length 8 for 8 pixels (1 tile)
                        tile_palette_data[i] = palette_ram_addr;
                    }

                    // Push tile to buffer
                    push(&tile_buffer_queue, tile_palette_data);
                }
                break;
            
        }

        // Only generate pixels during cycle 1-256
        if (ppu->current_frame_cycle<= 256 && ppu->scanline != -1) {
            palette_data = tile_palette_data[(ppu->current_frame_cycle% 8) - 1 ];

            // Pixel gen
            SDL_SetRenderDrawColor(renderer, 
                ppu_palette[palette_data * 3],
                ppu_palette[palette_data * 3 + 1],
                ppu_palette[palette_data * 3 + 2],
                255);

            SDL_Rect rect = {ppu->current_frame_cycle, ppu->scanline, 1, 1};
            SDL_RenderFillRect(renderer, &rect);

            SDL_RenderPresent(renderer);
        }
    } else if (ppu->current_frame_cycle>= 257 && ppu->current_frame_cycle<= 320) {

        if (ppu->current_frame_cycle== 257) {
            // Reset register v's horizontal position (first 5 bits - 0x1F - 0b11111)
            ppu->v |= (ppu->t) & 0x1F;
        }

        // Tile data for the sprites on the next scanline are loaded into rendering latches
        oam_buffer_latches[(ppu->current_frame_cycle- 257) % 32] = oam_memory_secondary[(ppu->current_frame_cycle- 257) % 32];

    }  

    else if (ppu->current_frame_cycle>= 337 && ppu->current_frame_cycle<= 340) {
        // Two bytes are fetched for unknown reasons
    }
    



    if (ppu->scanline >= 241 && ppu->scanline <= 260) {
        // Set vblank after tick 1 (second tick)
        if (ppu->current_frame_cycle== 1) {
            ppu->vblank_flag = 1;
            ppu->PPUSTATUS |= 0b10000000;

            // PPUCTRL bit 7 determins if CPU accepts NMI
            if (ppu->vblank_flag & (ppu->PPUCTRL >> 7)) {
                // Set NMI
                ppu->nmi_flag = 1;
            }
        }
    }



    ppu->current_frame_cycle++;
    
    if (ppu->current_frame_cycle > 340) {
        ppu->scanline++;

        /*
        TODO:
            - set current_frame_cycle to 0 or 1 is frame is even or odd
        */
        ppu->total_cycles += ppu->current_frame_cycle - 1;
        ppu->current_frame_cycle = 0;
    } 

    if (ppu->scanline > 260) {
        // Frame is completed
        // Set scanline back to pre-render
        ppu->scanline = -1;
    }

}