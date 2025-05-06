/*
NES PPU implementation
Author: Kavin Gnanapandithan
*/

#include "ppu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 
//#include <SDL2/SDL.h>


/* Memory info*/
#define NES_HEADER_SIZE 16
#define MEMORY_SIZE 0x4000 // 16 KB

#define PALETTE_SIZE 64

#define OAM_SIZE 0xFF

#define NUM_DOTS 341
#define NUM_SCANLINES 262

#define LOG(fmt, ...)\
    do { if (PPU_LOGGING) fprintf(stderr, fmt, ##__VA_ARGS__); } while (0)

// Main PPU memory
uint8_t ppu_memory[MEMORY_SIZE] = {0};
uint8_t nes_header[NES_HEADER_SIZE] = {0};
uint8_t ppu_palette[PALETTE_SIZE * 3];


// Nametable mirror arrangment flag
// 1 = h, v = 0
unsigned char nametable_mirror_flag;

// Drawing background vs sprite flag
unsigned char drawing_bg_flag;

// Determined by PPUMASK
unsigned char render_flag;

// 256 seperate memoery dedicated to OAM
uint8_t oam_memory[OAM_SIZE] = {0};

// Buffer, holds up to 8 spirtes to be rendered on the next scanline
uint8_t oam_memory_secondary[32] = {0};

// Internal latches, holds actual rendering data
uint8_t oam_buffer_latches[32] = {0};

uint8_t sprite_byte;

int sprite_eval_index;
int sprite_byte_eval_index;
int sprite_index;


// Output buffer
uint8_t pixel_output_buffer[8] = {0};

// Top of the secondary oam memory
int oam_secondary_top = 0;

// Copy flag determines if oam index should be copied to oam secondary
int copy_oam_mem_flag = 0;


// SDL Variables
// SDL_Window *window;
// SDL_Renderer *renderer;
// SDL_Texture *texture;

// Store RGB values in frame_buffer

uint8_t open_bus;

void ppu_init(PPU *ppu) {
    // Initial PPU MMIO Register values
    ppu->PPUCTRL = 0;
    ppu->PPUMASK = 0;
    ppu->PPUSTATUS = 0b00010000;
    ppu->OAMADDR = 0;
    ppu->w = 0;
    ppu->PPUSCROLL = 0;
    ppu->PPUADDR = 0;
    ppu->PPUDATA = 0;

    // Fetch tile id from name table 
    ppu->name_table_byte = 0;

    // Fetch attribute byte for the corresponding nametable byte
    ppu->attribute_byte = 0;

    // Fetch high and low pattern table bytes
    ppu->pattern_table_lsb = 0;
    ppu->pattern_table_msb = 0;

    // Final palette ram address
    ppu->palette_ram_addr = 0;

    // Determines which palette to use for tile
    ppu->palette_index = 0;

    // Palette data
    ppu->palette_data = 0;

    sprite_index = -1;

    // Flag 6
    // Bit 0 determine h/v n.t arrangment (h = 1, v = 0)
    nametable_mirror_flag = ((nes_header[6] & 0x01) == 0x01);
    drawing_bg_flag = 1;

    ppu->current_scanline_cycle = 0;
    ppu->scanline = 0;
    ppu->frame = 0;
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
    addr &= 0x3FFF;

    if (addr < 0x2000) {
        // Pattern tables
        uint16_t pt_base_addr = 0;

        if (drawing_bg_flag) {
            pt_base_addr = (ppu->PPUCTRL & 0x10) ? 0x1000 : 0x0000;
        } else {
            // TODO: Handle 8x16 sprites separately in sprite code
            pt_base_addr = (ppu->PPUCTRL & 0x08) ? 0x1000 : 0x0000;
        }
        return ppu_memory[pt_base_addr | (addr & 0x0FFF)];
    }

    else if (addr < 0x3F00) {
        // Nametable region with mirroring
        addr &= 0x2FFF;
        uint16_t offset = addr & 0xFFF;

        if (nametable_mirror_flag) { // vertical
            return ppu_memory[offset % 0x800];
        } else { // horizontal
            if (offset < 0x800) {
                // NT0 or NT1 (2000 or 2400)
                return ppu_memory[(0x2000 | (offset % 0x400))];
            } else {
                if (offset < 0xC00) {
                    return ppu_memory[addr];
                } else {
                    return ppu_memory[0x2000 | (offset - 0x400)];
                }
            }
        }
    }

    else if (addr < 0x4000) {
        // Palette memory
        addr &= 0x1F;
        if ((addr & 0x13) == 0x10) {
            addr &= ~0x10; // mirror 0x3F10, 0x3F14, ... to 0x3F00, 0x3F04, ...
        }
        return ppu_memory[0x3F00 + addr];
    }

    return 0; // open bus
}


void write_mem(PPU *ppu, uint16_t addr, uint8_t val) {
    addr &= 0x3FFF; // Mirror addresses above $3FFF

    if (addr < 0x2000) {
        // Pattern table / CHR RAM
        //ppu_memory[addr] = val;
    } else if (addr < 0x3F00) {
        // Nametable range with mirroring
        //uint16_t mirrored_addr = 0x2000 + (addr & 0x0FFF); // mirror $2000–$2FFF every 4KB
        ppu_memory[addr] = val;
        //ppu_memory[mirrored_addr] = val;


        // addr &= 0x2FFF;
        // uint16_t offset = addr & 0xFFF;

        // if (nametable_mirror_flag) {

        // } else {
        //     if (offset < 0x800) {
        //         // NT0 or NT1 (2000 or 2400)
        //         ppu_memory[(0x2000 | (offset % 0x400))] = val;
        //     } else {
        //         if (offset < 0xC00) {
        //             ppu_memory[addr] = val;
        //         } else {
        //             ppu_memory[0x2000 | (offset - 0x400)] = val;
        //         }
        //     }
        // }
    } else if (addr < 0x4000) {
        // Palette RAM with mirroring
        uint16_t mirrored_addr = 0x3F00 + (addr & 0x1F);

        // Handle mirroring of universal background color
        // if (mirrored_addr == 0x3F10) mirrored_addr = 0x3F00;
        // if (mirrored_addr == 0x3F14) mirrored_addr = 0x3F04;
        // if (mirrored_addr == 0x3F18) mirrored_addr = 0x3F08;
        // if (mirrored_addr == 0x3F1C) mirrored_addr = 0x3F0C;

        ppu_memory[mirrored_addr] = val;
    }
}




void ppu_registers_write(PPU *ppu, uint16_t addr, uint8_t val) {
    // if (ppu->scanline >= 0 && ppu->scanline <= 239 &&
    //     ppu->current_scanline_cycle > 0 && ppu->current_scanline_cycle <= 256 &&
    //     addr != 0x2001) 
    // {
    //     printf("Writing to PPU during render!\n");
    //     printf("SCANLINE: %d\n", ppu->scanline);
    //     printf("REG: %x", addr);
    //     fflush(stdout);
    //     return;
    // }

    switch (addr)  {

        // PPUCTRL
        case 0x2000:
            ppu->PPUCTRL = val;
            // Update nametable bits (bits 10 and 11) of temporary VRAM address (ppu->t)
            ppu->t = (ppu->t & 0xF3FF) | ((val & 0x03) << 10);
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
            if (ppu->w == 0) {
                // First write: horizontal scroll (fine X and coarse X)
                ppu->t = (ppu->t & 0xFFE0) | ((val >> 3) & 0x1F);
                ppu->x = val & 0x07;
                ppu->w = 1;
            } else {
                // Second write: vertical scroll (fine Y and coarse Y)
                ppu->t = (ppu->t & 0x8FFF) | ((val & 0x07) << 12);     // fine Y
                ppu->t = (ppu->t & 0xFC1F) | ((val & 0xF8) << 2);       // coarse Y
                ppu->w = 0;
            }
            break;

        // PPUADDR
        case 0x2006:
            // First write
            if (ppu->w == 0) {
                ppu->t = (ppu->t & 0x00FF) | ((val & 0x3F) << 8);  
                ppu->w = 1;
            } else {
                ppu->t = (ppu->t & 0xFF00) | val;
                ppu->w = 0;
                ppu->v = ppu->t;
                printf("WRITTEN TO V: %x\n", ppu->v);
                fflush(stdout);
                //sleep(5);
            }

            break;

        // PPUDATA
        case 0x2007:
            write_mem(ppu, ppu->v & 0x3FFF, val);
            ppu->v += (ppu->PPUCTRL & 0x04) ? 32 : 1;

            break;

        case 0x4014:
            ppu->OAMDMA = val;
            break;
    }
}

uint8_t ppu_registers_read(PPU *ppu, uint16_t addr) {
    // if (ppu->scanline >= 0 && ppu->scanline <= 239 &&
    //     ppu->current_scanline_cycle > 0 && ppu->current_scanline_cycle <= 256) 
    // {
    //     LOG("Read to PPU during render!\n");
    //     LOG("SCANLINE: %d\n", ppu->scanline);
    //     LOG("REG: %x", addr);
    //     fflush(stdout);
    //     return open_bus;
    // }
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
            ppu->PPUSTATUS &= 0x7F;
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
                open_bus = read_mem(ppu, ppu->v & 0x3FFF);
            }

            // Increment v after PPUDATA read
            ppu->v += (ppu->PPUCTRL & 0x04) == 0x04 ? 32 : 1; 
            return open_bus;
            break;

        default:
            return open_bus;
            break;
    }
}

// uint32_t get_frame_buffer(PPU *ppu) {
//     return ppu->frame_buffer;
// }

void reset_graphics_flag(PPU *ppu) {
    ppu->update_graphics = 0;
}

void print_ppu_registers(PPU *ppu) {
    LOG("PPU Registers:\n");
    LOG("  $2000 (CTRL):     0x%02X\n", ppu->PPUCTRL);
    LOG("  $2001 (MASK):     0x%02X\n", ppu->PPUMASK);
    LOG("  $2002 (STATUS):   0x%02X\n", ppu->PPUSTATUS);
    LOG("  $2003 (OAMADDR):  0x%02X\n", ppu->OAMADDR);
    LOG("  $2004 (OAMDATA):  0x%02X\n", ppu->OAMDATA);
    LOG("  $2005 (SCROLL):   0x%02X\n", ppu->PPUSCROLL);
    LOG("  $2006 (ADDR):     0x%02X\n", ppu->PPUADDR);
    LOG("  $2007 (DATA):     0x%02X\n", ppu->PPUDATA);
}


/* 
Fetch tile from name table
Note: 
- PPU has 2KB of VRAM
- Mapped to 4KB of PPU memory ($2000 - $2FFF)
*/
uint8_t fetch_name_table_byte(PPU *ppu) {
    return read_mem(ppu, 0x2000 | (ppu->v & 0xFFF));
}

/*
Fetch byte from attribute table
Note:
- AT is a 64-byte array
- at the end of each nametable
    - 23C0, 27C0, 2BC0, 2FC0
*/
uint8_t fetch_attr_table_byte(PPU *ppu) {

    uint16_t name_table_base = ppu->name_table_byte & 0x2C00;
    uint16_t attribute_table_base = name_table_base + 0x3C0;  

    uint8_t attr_x = (ppu->v & 0x001F) >> 2;
    uint8_t attr_y = ((ppu->v >> 5) & 0x001F) >> 2; 

    uint16_t attr_address = attribute_table_base + (attr_y * 8) + attr_x;

    return read_mem(ppu, attr_address);
}



void ppu_execute_cycle(PPU *ppu) {

    // Pixel generation variables
    if (!ppu) {
        LOG("PPU is null!\n");
        exit(1);
    }

    LOG("\n******* PPU *******\n");
    LOG("PPU Cycle: %d \n", ppu->current_scanline_cycle);
    LOG("PPU Scanline: %d \n", ppu->scanline);
    LOG("PPU Frame: %d \n", ppu->frame);
    LOG("V: %x \n", ppu->v);
    LOG("Nametable Byte: %x \n", ppu->name_table_byte);
    LOG("Attribute Table Byte: %x \n", ppu->attribute_byte);

    if ((ppu->scanline >= -1 && ppu->scanline < 240) && ppu->PPUMASK & 0x18) {
        if (((ppu->current_scanline_cycle >= 1 && ppu->current_scanline_cycle <= 256) ||
            (ppu->current_scanline_cycle >= 321 && ppu->current_scanline_cycle <= 336))) {
            
            for (int i = 3; i < 32; i += 4) {
                if (oam_buffer_latches[i] >= ppu->current_scanline_cycle &&
                    oam_buffer_latches[i] < ppu->current_scanline_cycle + 8) {
                    
                    sprite_index = i - 3;
                    drawing_bg_flag = 0;
                    break;
                }
                drawing_bg_flag = 1;
            }

            /* 
            SPRITE EVALUATION   

            - Cycle 65 to 256 
                - OAM_SECONDARY is populated with sprites of the next scanline
            - secondary oam is copied to the shift registers (oam_buffer_latch) to
            print on the next scanline
            
            */
            if (ppu->current_scanline_cycle < 65) {
                memset(oam_memory_secondary, 0xFF, 32);
            }

            if (ppu->current_scanline_cycle <= 256 && ppu->current_scanline_cycle >= 65) {
                int sprite_cycle = ppu->current_scanline_cycle - 65;
                
                if (ppu->current_scanline_cycle % 2 == 1) {
                    // Odd cycle
                    sprite_byte = oam_memory[sprite_eval_index * 4 + sprite_byte_eval_index];
                    int sprite_height = (ppu->PPUCTRL & 0x20) ? 16 : 8;

                    if (sprite_byte_eval_index == 0) { 
                        if ((ppu->scanline + 1 >= sprite_byte) &&
                            (ppu->scanline + 1 < sprite_byte + sprite_height)) {
                            copy_oam_mem_flag = 1;
                        } else {
                            copy_oam_mem_flag = 0;
                        }
                    }

                } else {
                    if (oam_secondary_top < 32) {
                        if (copy_oam_mem_flag) {
                            oam_memory_secondary[oam_secondary_top++] = sprite_byte;
                            sprite_byte_eval_index++;
                            if (sprite_byte_eval_index > 3) {
                                sprite_eval_index++;
                                sprite_byte_eval_index = 0;
                            }
                        } else {
                            // Sprite was out of range, skip the rest of the sprite
                            sprite_eval_index++;
                            sprite_byte_eval_index = 0;
                        }
                    } else {
                        //ppu->PPUSTATUS |= 0x20;
                    }
                }
            }


            switch (ppu->current_scanline_cycle % 8) {

                /*
                Cycle 1 & Cycle 2
                - Fetch Nametable byte
                */
                case 1:
                    ppu->name_table_byte = fetch_name_table_byte(ppu);
                    LOG("Register v: %x\n", ppu->v);
                    LOG("Name Table Byte: %x\n", ppu->name_table_byte);
                    break;

                /*
                Cycle 3 & Cycle 4
                - Fetch the correspond Attribute table byte
                from the Nametable
                */
                case 3:
                    ppu->attribute_byte = fetch_attr_table_byte(ppu);
                    LOG("Attribute Table Byte: %x", ppu->attribute_byte);

                    // Determines which of the four areas of the attribute byte to use
                    uint8_t tile_area_horizontal = (ppu->v & 0x00F) % 4;
                    uint8_t tile_area_vertical = ((ppu->v & 0x0F0) / 0x20) % 4;

                    if (tile_area_vertical == 0 && tile_area_horizontal == 0) {
                        // Top left quadrant (bit 1 & 0)
                        ppu->palette_index = ppu->attribute_byte & 0x03; 
                    } else if (tile_area_vertical == 0 && tile_area_horizontal == 1) {
                        // Top right quadrant (bit 3 & 2)
                        ppu->palette_index = (ppu->attribute_byte & 0x0C) >> 2;
                    } else if (tile_area_vertical == 1 && tile_area_horizontal == 0) {
                        // Bottom left quadrant (bit 5 & 4)
                        ppu->palette_index = (ppu->attribute_byte & 0x30) >> 4;
                    } else if (tile_area_vertical == 1 && tile_area_horizontal == 1) {
                        // Bottom right quadrant (bit 7 6 6)
                        ppu->palette_index = (ppu->attribute_byte & 0xC0) >> 6;
                    }

                    LOG("Palette Index: %x\n", ppu->palette_index);
                break;

                /*
                Cycle 5, Cycle 6 & Cycle 7
                - Fetch Nametable low byte + high byte 
                */  
                case 5:
                    // if (ppu->current_scanline_cycle == 325 && ppu->scanline == -1
                    //     && ppu->frame >= 3) {
                    //     printf("SCANLINE -1, CYCLE 325\n");
                    //     printf("TILE ID: %x", ppu->name_table_byte);
                    //     fflush(stdout);
                    //     sleep(5);
                    // }
                    int row_padding = ppu->current_scanline_cycle >= 321 && 
                                      ppu->current_scanline_cycle <= 336 ? 1 : 0;

                    if (drawing_bg_flag) {
                        ppu->pattern_table_lsb = read_mem(ppu, 
                            (ppu->name_table_byte * 16) + (ppu->scanline % 8) + row_padding);
                        //ppu->pattern_table_msb = read_mem(ppu, (ppu->name_table_byte * 16) + 8 + ppu->scanline);

                    } else {
                        // byte 1 is tile number
                        ppu->pattern_table_lsb = read_mem(ppu, 
                            (oam_buffer_latches[sprite_index + 1] * 16) + ppu->scanline);
                        // ppu->pattern_table_msb = read_mem(ppu, (oam_buffer_latches[sprite_index + 1] * 16) + ppu->scanline + 8);
                    }

                    /* Tile fetch from cycle i is rendered on pixel i + 8 */
                    break;
                
                case 7:

                    row_padding = ppu->current_scanline_cycle >= 321 && 
                                      ppu->current_scanline_cycle <= 336 ? 1 : 0;
                    
                    if (drawing_bg_flag) {
                        ppu->pattern_table_msb = read_mem(ppu, 
                            (ppu->name_table_byte * 16) + 8 + (ppu->scanline % 8) + row_padding);
                    } else {
                        ppu->pattern_table_msb = read_mem(ppu, 
                            (oam_buffer_latches[sprite_index + 1] * 16) + ppu->scanline + 8);
                    }
                    
                    for (int i = 0; i < 8; i++) {
                        uint8_t bit = 7 - i; 
                        uint8_t lo = (ppu->pattern_table_lsb >> bit) & 1;
                        uint8_t hi = (ppu->pattern_table_msb >> bit) & 1;

                        ppu->tile_pixel_value[i] = (hi << 1) | lo; 
                        LOG("Tile Pixel[i] Value: %x\n", ppu->tile_pixel_value[i]);
                    }

                    break;

                case 0:
                    //if (ppu->PPUMASK & 0x18) {
                    if ((ppu->v & 0x001f) == 0x1f) {
                        ppu->v &= ~0x001F; // Wrap around
                        ppu->v ^= 0x0400; // Switch horizontal N.T
                    } else {
                        ppu->v += 1;
                    }

                    if (ppu->current_scanline_cycle == 256) {
                        if ((ppu->v & 0x7000) != 0x7000) {
                            ppu->v += 0x1000;
                        } else {
                            ppu->v &= ~0x7000;
                            int coarse_y = (ppu->v & 0x03E0) >> 5; 
                            if (coarse_y == 29) {
                                coarse_y = 0;
                                ppu->v ^= 0x0800;
                            } else if (coarse_y == 31) {
                                coarse_y = 0;
                            } else {
                                coarse_y += 1;
                            }
                            ppu->v = (ppu->v & ~0x03E0) | (coarse_y << 5);
                        }
                    }

                    if (ppu->scanline + 1 <= 239 && 
                        ppu->current_scanline_cycle >= 321 && 
                        ppu->current_scanline_cycle <= 336) {
                        /*
                        Populate frame buffer with tile 1 & tile 2 of scanline N + 1
                        frame_buffer[(scanline + 1) * 256 + ppu->current_scanline_cycle]
                            = tile 1
                        frame_buffer[(scanline + 1) * 256 + ppu->current_scanline_cycle + 1]
                            = tile 2
                        */
                        for (int i = 0; i < TILE_SIZE; i++) {
                            ppu->palette_ram_addr = drawing_bg_flag ? 0x3F00 : 0x3F10;

                            // Bit 0 & Bit 1 determine Pixel Value
                            ppu->palette_ram_addr |= ppu->tile_pixel_value[i];

                            // Bit 3 & Bit 2 determine Palette # from attribute table
                            ppu->palette_ram_addr |= (ppu->palette_index) << 2;

                            ppu->palette_data = read_mem(ppu, ppu->palette_ram_addr);

                            // RGBA format
                            uint8_t r = ppu_palette[ppu->palette_data * 3];
                            uint8_t g = ppu_palette[ppu->palette_data * 3 + 1];
                            uint8_t b = ppu_palette[ppu->palette_data * 3 + 2];

                            int row = ppu->scanline + 1;
                            int column = ppu->current_scanline_cycle - 321 - (7 - i);
                            LOG("INDICES! R: %d, C: %d\n", row, column);
                            fflush(stdout);

                            //sleep (1);
                            fflush(stdout);
                            if (!((row >= 0 && row <= 239) && (column >= 0 && column <= 255))) {
                                LOG("NEXT SL: INDICES OUT OF BOUNDS! R: %d, C: %d", row, column);
                                fflush(stdout);
                                sleep(10);
                                //return;
                            }

                            ppu->frame_buffer[row][column]                        
                            = (r << 24) | (g << 16) | (b << 8) | (ppu->tile_pixel_value[i] == 0 ? 0x0: 0xFF);
                        
                        }
                    }

    

                    if (ppu->current_scanline_cycle < 248 && 
                        ppu->scanline > -1 && ppu->scanline <= 239) {
                        for (int i = 0; i < TILE_SIZE; i++) {
                            ppu->palette_ram_addr = drawing_bg_flag ? 0x3F00 : 0x3F10;

                            // Bit 0 & Bit 1 determine Pixel Value
                            ppu->palette_ram_addr |= ppu->tile_pixel_value[i];

                            // Bit 3 & Bit 2 determine Palette # from attribute table
                            if (drawing_bg_flag) {
                                ppu->palette_ram_addr |= (ppu->palette_index) << 2;
                            } 
                            // else {
                            //     ppu->palette_ram_addr |= oam_buffer_latches[sprite_index + 2] << 2;
                            // }
                            LOG("Palette Address: %x\n", ppu->palette_ram_addr);

                            ppu->palette_data = read_mem(ppu, ppu->palette_ram_addr);
                            LOG("Palette Data: %x\n", ppu->palette_data);


                            uint8_t r = ppu_palette[ppu->palette_data * 3];
                            uint8_t g = ppu_palette[ppu->palette_data * 3 + 1];
                            uint8_t b = ppu_palette[ppu->palette_data * 3 + 2];
                            
                            LOG("Red: %u, Green: %u, Blue: %u\n", r, g, b);

                            int row = ppu->scanline;
                            int column = ppu->current_scanline_cycle + 16 - (7 - i) - 1;
                            LOG("INDICES! R: %d, C: %d\n", row, column);
                            fflush(stdout);

                            //sleep (1);
                            if (!((row >= 0 && row <= 239) && (column >= 0 && column <= 255))) {
                                LOG("INDICES OUT OF BOUNDS! R: %d, C: %d", row, column);
                                fflush(stdout);
                                sleep(10);
                                //return;
                            }


                            // RGBA format
                            ppu->frame_buffer[row][column]
                                = (r << 24) | (g << 16) | (b << 8) | (ppu->tile_pixel_value[i] == 0 ? 0x0: 0xFF);

                            LOG("Frame Buffer Val (%d) : %x\n", ppu->scanline * 256 + ppu->current_scanline_cycle - (7 - i) - 1,
                                                    (r << 24) | (g << 16) | (b << 8) | (ppu->tile_pixel_value[i] == 0 ? 0x0: 0xFF) );

                            LOG("Current scanline: %d\n", ppu->scanline);
                            LOG("Current cycle: %d\n", ppu->current_scanline_cycle );
                            //sleep(1);
                        }
                    }
                    //}
                    break;
                
            }

        } else if (ppu->current_scanline_cycle >= 257 && ppu->current_scanline_cycle <= 320) {
            /* HBLANK */
            
            if (ppu->current_scanline_cycle == 257) {
                // Reset register v's horizontal position (first 5 bits - 0x1F - 0b11111)
                ppu->v = (ppu->v & 0xFBE0) | (ppu->t & 0x001F);
            }


            // Tile data for the sprites on the next scanline are loaded into rendering latches
            oam_buffer_latches[(ppu->current_scanline_cycle - 257) % 32] = oam_memory_secondary[(ppu->current_scanline_cycle- 257) % 32];

        }  

        else if (ppu->current_scanline_cycle>= 337 && ppu->current_scanline_cycle<= 340) {
            // Two bytes are fetched for unknown reasons
        }
    } 
    
    if (ppu->scanline == 241 && ppu->current_scanline_cycle == 1) {
        /* VBLANK */

        printf("Scanline 241: VBLANK. Cycle: %x\n", ppu->current_scanline_cycle);
        
        //sleep(5);
        fflush(stdout);

        ppu->vblank_flag = 1;
        ppu->PPUSTATUS |= 0b10000000;
    
        // PPUCTRL bit 7 determines if CPU accepts NMI
        if (ppu->PPUCTRL & 0x80) { 
            // Set NMI
            ppu->nmi_flag = 1;
            LOG("NMI triggered\n");
            fflush(stdout);
        }
        
    } 
    if (ppu->current_scanline_cycle == 340 && ppu->scanline == 239) {

        //if (ppu->PPUMASK & 0x08) {
            ppu->update_graphics = 1;
            LOG("PPUMASK: %x", ppu->PPUMASK);
            fflush(stdout);
            //sleep(1);
        //}
    }



    if (ppu->scanline == -1 &&
        ppu->current_scanline_cycle >= 280 &&
        ppu->current_scanline_cycle <= 304 &&
        (ppu->PPUMASK & 0x18))  // background or sprite rendering enabled
    {
        ppu->v = (ppu->v & 0x041F) | (ppu->t & 0x7BE0);
    }

    ppu->current_scanline_cycle++;
    
    if (ppu->current_scanline_cycle >= 341) {

        ppu->scanline++;

        ppu->total_cycles += ppu->current_scanline_cycle;
        ppu->current_scanline_cycle = 0;

        sprite_eval_index = 0;
        sprite_byte_eval_index = 0;
        oam_secondary_top = 0;

        
        if (ppu->scanline > 260) {
            // Frame is completed
            // Set scanline back to pre-render    
            ppu->scanline = -1;
        }
    } 

    if (ppu->current_scanline_cycle == 0 && ppu->scanline == 0) 
        ppu->frame++;


}