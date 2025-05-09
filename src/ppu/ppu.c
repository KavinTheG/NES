/*
NES PPU implementation
Author: Kavin Gnanapandithan
*/

#include "ppu.h"
#include <stdlib.h>
#include <unistd.h> 

// Nametable mirror arrangment flag
// 1 = h, v = 0
unsigned char nametable_mirror_flag;


// Returns bus value
uint8_t open_bus;


// Main PPU memory
uint8_t ppu_memory[PPU_MEMORY_SIZE] = {0};
uint8_t nes_header[NES_HEADER_SIZE] = {0};
uint8_t ppu_palette[PALETTE_SIZE * 3];

// 256 seperate memoery dedicated to OAM
uint8_t oam_memory[OAM_SIZE] = {0};

// Buffer, holds up to 8 spirtes to be rendered on the next scanline
uint8_t oam_memory_secondary[OAM_SECONDARY_SIZE] = {0};

// Internal latches, holds actual rendering data
uint8_t oam_buffer_latches[OAM_SECONDARY_SIZE] = {0};

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

    // Stores palette ram address of each pixel
    ppu->palette_ram_addr = 0;

    // Determines which palette to use for a tile
    ppu->palette_index = 0;

    // Palette data
    ppu->palette_data = 0;

    // Flag 6
    // Bit 0 determine h/v n.t arrangment (h = 1, v = 0)
    nametable_mirror_flag = ((nes_header[6] & 0x01) == 0x01);

    // Determines if a tile is background vs sprite
    ppu->drawing_bg_flag = 1;

    ppu->current_scanline_cycle = 0;
    ppu->scanline = 0;
    ppu->frame = 0;
}

void load_ppu_memory(PPU *ppu, unsigned char *chr_rom, int chr_size) {
    // Clear ppu_memory
    memset(ppu_memory, 0, PPU_MEMORY_SIZE);

    // Load CHR ROM into 0x0000 - 0x1FFF
    memcpy(&ppu_memory[0x0000], chr_rom, chr_size);   
}

void load_ppu_ines_header(unsigned char *header) {
    memset(&nes_header, 0, NES_HEADER_SIZE);
    memcpy(&nes_header, header, NES_HEADER_SIZE);
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

        if (ppu->drawing_bg_flag) {
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
        //ppu_memory[addr] = val;
        //ppu_memory[mirrored_addr] = val;


        addr &= 0x2FFF;
        uint16_t offset = addr & 0xFFF;

        if (nametable_mirror_flag) {

        } else {
            if (offset < 0x800) {
                // NT0 or NT1 (2000 or 2400)
                ppu_memory[(0x2000 | (offset % 0x400))] = val;
            } else {
                if (offset < 0xC00) {
                    ppu_memory[addr] = val;
                } else {
                    ppu_memory[0x2000 | (offset - 0x400)] = val;
                }
            }
        }
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

    if (ppu->scanline == -1) {
        if (ppu->PPUMASK & 0x18) {
            ppu_exec_pre_render(ppu);
        }
    } else if (ppu->scanline >= 0 && ppu->scanline <= 239) {
        if (ppu->PPUMASK & 0x18) {
            ppu_exec_visible_scanline(ppu);
        }
    } else if (ppu->scanline >= 241) {
        ppu_exec_vblank(ppu);
    }

    ppu->current_scanline_cycle++;
    
    if (ppu->current_scanline_cycle >= 341) {

        ppu->scanline++;

        ppu->total_cycles += ppu->current_scanline_cycle;

        ppu->current_scanline_cycle = 0;
        if (ppu->scanline > 260) {
            // Frame is completed
            // Set scanline back to pre-render    
            ppu->scanline = -1;
        }
    } 

    if (ppu->current_scanline_cycle == 0 && ppu->scanline == 0)  {
        ppu->frame++;
    }

}