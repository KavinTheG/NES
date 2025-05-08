#include "ppu_mmio.h"

void ppu_registers_write(PPU *ppu, uint16_t addr, uint8_t val) {

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
