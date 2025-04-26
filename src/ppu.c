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
uint32_t frame_buffer[SCREEN_WIDTH_VIS * SCREEN_HEIGHT_VIS] = {0};

uint8_t open_bus;

void ppu_init(PPU *ppu) {
    // Initial PPU MMIO Register values
    ppu->PPUCTRL = 0;
    ppu->PPUMASK = 0;
    ppu->PPUSTATUS=0b10100000;
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


    // Flag 6
    // Bit 0 determine h/v n.t arrangment (h = 1, v = 0)
    nametable_mirror_flag = ((nes_header[6] & 0x01) == 0x01);

    ppu->current_scanline_cycle = 0;
    ppu->scanline = -1;
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
    switch (addr & 0xF000) {
        case 0x0:
            // PPUCTRL bit 3 & 4 control sprite & background PT address respectively
            uint16_t nt_base_addr;
            
            if (drawing_bg_flag) {
                nt_base_addr = (ppu->PPUCTRL & 0x10) == 0x10 ? 0x1000 : 0;
            } else {
                
                if ( ppu->PPUCTRL & 0x10) {
                    // 8x16 sprites, base address determined by bit 0 of Byte 1
                    nt_base_addr = (addr & 0x01) ? 0x1000 : 0;
                } else {
                    nt_base_addr = (ppu->PPUCTRL & 0x08) == 0x08 ? 0x1000 : 0;
                }
            }

            return ppu_memory[nt_base_addr | (addr & 0x0FFF)];
            break;

        // Nametable memory region ($2000 - $2FFF)
        case 0x2000:
            //return ppu_memory[nametable_mirror_flag ? (addr & ~0xF400) : (addr & 0x07FF)];
            //return ppu_memory[mirror_vram_addr(addr)]; 
            addr = addr & 0x2FFF;  // clamp to VRAM range
            uint16_t offset = addr - 0x2000;
        
            if (nametable_mirror_flag) {
                return ppu_memory[offset % 0x800]; // easy wrap
            } else {
                // horizontal: map NT0/NT1/NT2/NT3 → NT0/NT1
                uint16_t table = (offset / 0x400) & 0x03;
                if (table == 0 || table == 2)
                    return ppu_memory[offset % 0x400];         // NT0 or NT2
                else
                    return ppu_memory[(offset % 0x400) + 0x400]; // NT1 or NT3
            }

            break;

        default:
            return ppu_memory[addr];
            break;
    }
 
}


void write_mem(PPU *ppu, uint16_t addr, uint8_t val) {
    addr &= 0x3FFF; // Mirror addresses above $3FFF

    if (addr < 0x2000) {
        // Pattern table / CHR RAM
        ppu_memory[addr] = val;
    } else if (addr < 0x3F00) {
        // Nametable range with mirroring
        uint16_t mirrored_addr = 0x2000 + (addr & 0x0FFF); // mirror $2000–$2FFF every 4KB
        ppu_memory[mirrored_addr] = val;
    } else if (addr < 0x4000) {
        // Palette RAM with mirroring
        uint16_t mirrored_addr = 0x3F00 + (addr & 0x1F);

        // Handle mirroring of universal background color
        if (mirrored_addr == 0x3F10) mirrored_addr = 0x3F00;
        if (mirrored_addr == 0x3F14) mirrored_addr = 0x3F04;
        if (mirrored_addr == 0x3F18) mirrored_addr = 0x3F08;
        if (mirrored_addr == 0x3F1C) mirrored_addr = 0x3F0C;

        ppu_memory[mirrored_addr] = val;
    }
}




void ppu_registers_write(PPU *ppu, uint16_t addr, uint8_t val) {
    if (addr == 0x2007 && (ppu->scanline < 241) && (ppu->PPUMASK & 0x18)) {
        printf("WARN: Writing to PPUDATA during rendering! (scanline: %d)\n", ppu->scanline);
        fflush(stdout);
        sleep(1);
    }

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
                ppu->v = ppu->t;
                ppu->w = 0;
                LOG("WRITTEN TO V: %x\n", ppu->v);
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

uint32_t* get_frame_buffer(PPU *ppu) {
    return ppu->frame_buffer_render;
}

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

    uint16_t name_table_base = ppu->v & 0x2C00;
    uint16_t attribute_table_base = name_table_base + 0x3C0;  

    uint8_t attr_x = (ppu->v & 0x001F) >> 2;
    uint8_t attr_y = ((ppu->v >> 5) & 0x001F) >> 2; 

    uint16_t attr_address = attribute_table_base + (attr_y * 8) + attr_x;

    return read_mem(ppu, attr_address);
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
    if ((ppu->PPUMASK & 0x18) == 0) {
        // Rendering disabled: no fetches, no scroll updates
        ppu->current_scanline_cycle++;
        if (ppu->current_scanline_cycle >= 341) {
            ppu->current_scanline_cycle = 0;
            ppu->scanline++;
            if (ppu->scanline >= 261) {
                ppu->scanline = -1;
                ppu->frame++;
            }
        }
        return;
    }


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


    if ((ppu->current_scanline_cycle >= 1 && ppu->current_scanline_cycle <= 256) 
     || (ppu->current_scanline_cycle >= 321 && ppu->current_scanline_cycle <= 336) ) {
                
        if (ppu->current_scanline_cycle == 1) {
            // set secondary oam to 0xFF
            memset(oam_memory_secondary, 0xFF, 32);
        }
        
        // If you wonder why 65, take a look at sprite evaluation
        if (ppu->current_scanline_cycle >= 65 && ppu->current_scanline_cycle <= 256) {
            // An index into oam memory, lets us know which byte to read
            int index = (ppu->current_scanline_cycle - 65) / 2;

            // Which byte (from byte #0 to #3) we are reading from the specified sprite
            int sprite_byte_offset = index % 4;

            // Stores the actual byte from oam 
            uint8_t sprite_byte = oam_memory[index];

            if (ppu->current_scanline_cycle % 2 == 1) {
                // Odd Cycle
                // Read from OAM 

                // Byte 0, read Y index
                if (sprite_byte_offset == 0) {
                    if (ppu->scanline + 1 >= sprite_byte &&
                        ppu->scanline + 1 < sprite_byte + ((ppu->PPUCTRL & 0x20) ? 16 : 8)) {
                        copy_oam_mem_flag = 1;
                    } else {
                        copy_oam_mem_flag = 0;
                    }
                }
            } else {

                if (copy_oam_mem_flag && oam_secondary_top < 32) {
                    oam_memory_secondary[oam_secondary_top] = oam_memory[index];
                    oam_secondary_top++;
                }
            }
        }


        drawing_bg_flag = 1;
        
        // Store the index of the sprite if it exists in the current pixel
        int sprite_index = -1;
        
        // Check if tile is background or sprite
        for (int i = 0; i < oam_secondary_top; i += 4) {

            uint8_t byte_3 = oam_buffer_latches[i + 3];
            
            if (byte_3 <= ppu->current_scanline_cycle && ppu->current_scanline_cycle < (byte_3 + 8)) {
                // This pixel is a sprite pixel
                drawing_bg_flag = 0;
                sprite_index = i;
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
                if (drawing_bg_flag) {
                    ppu->pattern_table_lsb = read_mem(ppu, (ppu->name_table_byte * 16) + ppu->scanline);
                    ppu->pattern_table_msb = read_mem(ppu, (ppu->name_table_byte * 16) + ppu->scanline + 8);

                } else {
                    // byte 1 is tile number
                    ppu->pattern_table_lsb = read_mem(ppu, (oam_buffer_latches[sprite_index + 1] * 16) + ppu->scanline);
                    ppu->pattern_table_msb = read_mem(ppu, (oam_buffer_latches[sprite_index + 1] * 16) + ppu->scanline + 8);
                }

                /* Tile fetch from cycle i is rendered on pixel i + 8 */
                for (int i = 0; i < 8; i++) {
                    uint8_t bit = 7 - i; 
                    uint8_t lo = (ppu->pattern_table_lsb >> bit) & 1;
                    uint8_t hi = (ppu->pattern_table_msb >> bit) & 1;

                    ppu->tile_pixel_value[i] = (hi << 1) | lo; 
                    LOG("Tile Pixel[i] Value: %x\n", ppu->tile_pixel_value[i]);
                }
                break;

            case 0:

                // Increment v 
                ppu->v += (ppu->PPUCTRL & 0x04) == 0x04 ? 32 : 1; 

                if (ppu->scanline + 1 < 239 && 
                    ppu->current_scanline_cycle >= 321 && 
                    ppu->current_scanline_cycle >= 336) {
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
                        frame_buffer[ppu->current_scanline_cycle - 321 + (ppu->scanline + 1) * 256 - (7 - i)]
                        = (r << 24) | (g << 16) | (b << 8) | (ppu->tile_pixel_value[i] == 0 ? 0x0: 0xFF);
                    
                    }
                }

  

                if (ppu->current_scanline_cycle < 256 && ppu->scanline <= 239 && ppu->scanline > -1) {
                    for (int i = 0; i < TILE_SIZE; i++) {
                        ppu->palette_ram_addr = drawing_bg_flag ? 0x3F00 : 0x3F10;

                        // Bit 0 & Bit 1 determine Pixel Value
                        ppu->palette_ram_addr |= ppu->tile_pixel_value[i];

                        // Bit 3 & Bit 2 determine Palette # from attribute table
                        ppu->palette_ram_addr |= (ppu->palette_index) << 2;
                        LOG("Palette Address: %x\n", ppu->palette_ram_addr);

                        ppu->palette_data = read_mem(ppu, ppu->palette_ram_addr);
                        LOG("Palette Data: %x\n", ppu->palette_data);


                        uint8_t r = ppu_palette[ppu->palette_data * 3];
                        uint8_t g = ppu_palette[ppu->palette_data * 3 + 1];
                        uint8_t b = ppu_palette[ppu->palette_data * 3 + 2];
                        
                        LOG("Red: %u, Green: %u, Blue: %u\n", r, g, b);
                        // RGBA format
                        frame_buffer[ppu->scanline * 256 + ppu->current_scanline_cycle + 8 - (7 - i)]
                            = (r << 24) | (g << 16) | (b << 8) | (ppu->tile_pixel_value[i] == 0 ? 0x0: 0xFF);

                        LOG("Frame Buffer Val (%d) : %x\n", ppu->scanline * 256 + ppu->current_scanline_cycle - (7 - i) - 1,
                                                (r << 24) | (g << 16) | (b << 8) | (ppu->tile_pixel_value[i] == 0 ? 0x0: 0xFF) );

                        LOG("Current scanline: %d\n", ppu->scanline);
                        LOG("Current cycle: %d\n", ppu->current_scanline_cycle );
                        //sleep(1);
                    }
                }
                break;
            
        }

    } else if (ppu->current_scanline_cycle >= 257 && ppu->current_scanline_cycle <= 320) {

        if (ppu->current_scanline_cycle == 257) {
            // Reset register v's horizontal position (first 5 bits - 0x1F - 0b11111)
            ppu->v |= (ppu->t) & 0x1F;
        }

        // Tile data for the sprites on the next scanline are loaded into rendering latches
        oam_buffer_latches[(ppu->current_scanline_cycle- 257) % 32] = oam_memory_secondary[(ppu->current_scanline_cycle- 257) % 32];

    }  

    else if (ppu->current_scanline_cycle>= 337 && ppu->current_scanline_cycle<= 340) {
        // Two bytes are fetched for unknown reasons
    }

    if (ppu->scanline == 241 && ppu->current_scanline_cycle == 1) {
        LOG("Scanline 241: VBLANK. Cycle: %x\n", ppu->current_scanline_cycle);
        fflush(stdout);

        ppu->vblank_flag = 1;
        ppu->PPUSTATUS |= 0b10000000;
    
        // PPUCTRL bit 7 determines if CPU accepts NMI
        if (ppu->PPUCTRL & 0x80) { 
            // Set NMI
            ppu->nmi_flag = 1;
            LOG("NMI triggered\n");
            fflush(stdout);
            //sleep(5);
        }
        
    }
    if (ppu->current_scanline_cycle == 340 && ppu->scanline == 239) {
        // Fill the frame buffer with red (test)
        // for (int i = 0; i < 256 * 240; i++) {
        //     frame_buffer[i] = 0xFF0000FF;  // Red
        // }
        memcpy(ppu->frame_buffer_render, frame_buffer, sizeof(frame_buffer[0]) * 256 * 240);
        ppu->update_graphics = 1;
        //sleep(1);
    }


    ppu->current_scanline_cycle++;
    
    if (ppu->current_scanline_cycle >= 341) {
        ppu->scanline++;
        memcpy(oam_buffer_latches, oam_memory_secondary, sizeof(oam_memory_secondary[0] * 32));
        oam_secondary_top = 0;
        ppu->total_cycles += ppu->current_scanline_cycle - 1;
        ppu->current_scanline_cycle = ppu->frame % 2 == 0 ? 0 : 1;
    } 

    if (ppu->scanline > 260) {
        // Frame is completed
        // Set scanline back to pre-render

        LOG("Update frame\n");
        LOG("Frame: %d\n\n", ppu->frame);

        ppu->scanline = -1;
        ppu->frame++;
    }
}