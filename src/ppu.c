#include "ppu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h> /* memset */
#include <unistd.h> /* close */

#define NES_HEADER_SIZE 16
#define MEMORY_SIZE 0x4000 // 16 KB
#define OAM_SIZE 0xFF

// Main PPU memory
uint8_t ppu_memory[MEMORY_SIZE] = {0};
uint8_t nes_header[NES_HEADER_SIZE] = {0};

// Nametable mirror arrangment flag
// 1 = h, v = 0
unsigned char nametable_mirror_flag;

// 256 seperate memoery dedicated to OAM
uint8_t oam_memory[OAM_SIZE] = {0};


// tile index retrieved from pattern table
uint16_t tile_index; 

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

uint8_t read_ppu_mem(uint16_t addr) {


    // Nametable memory ($2000 - $2FFF)
    switch (addr & 0xF000) {
        
        // Nametable memory region ($2000 - $2FFF)
        case 0x2000:
            return (addr & 0x7FF & !nametable_mirror_flag) | (addr & ~0xf400 & nametable_mirror_flag);
            break;
    }
 
}

// void load_ppu_mem(PPU *ppu, char *filename) {

//     FILE *rom = fopen(filename, "rb");
//     if (!rom) {
//         perror("Error opening ROM file");
//         return;
//     }

//     // Read INES header
//     unsigned char header[NES_HEADER_SIZE];
//     if (fread(header, 1, NES_HEADER_SIZE, rom) != NES_HEADER_SIZE) {
//         perror("Error: header does not match 16 bytes!");
//         fclose(rom);
//         return;
//     }

//     int prg_size = header[4] * 16 * 1024;
//     int chr_size = header[5] * 8 * 1024;

//     printf("PRG ROM size: %d bytes\n", prg_size);
//     printf("CHR ROM size: %d bytes\n", chr_size);

//     // Read PRG ROM into a temporary buffer
//     unsigned char *chr_rom = malloc(chr_size);
//     if (!chr_rom) {
//         printf("ppu memory allocation failed!\n");
//         fclose(rom);
//         return;
//     }

//     if (fread(chr_rom, 1, chr_size, rom) != chr_size) {
//         perror("Error reading PRG ROM");
//         free(chr_rom);
//         fclose(rom);
//         return;
//     }

//     fclose(rom);

//     // Clear ppu_memory
//     memset(ppu_memory, 0, MEMORY_SIZE);

//     // Load CHR ROM into 0x0000 - 0x1FFF
//     memcpy(&ppu_memory[0x0000], chr_rom, chr_size);

//     free(chr_rom);    
// } 

void set_w_reg(PPU *ppu, unsigned char w) {
    ppu->w = w;
}

void set_v_reg(PPU *ppu) {
    ppu->v = ppu->PPUADDR;
}

void set_PPUADDR(PPU *ppu, uint16_t PPUADDR) {
    ppu->PPUADDR = PPUADDR;
}

// Fetch tile from name table
// From address $2000-$2FFF
void fetch_tile(PPU *ppu) {
    tile_index = ppu_memory[ppu->v];
}

// Fetch tile graphics from pattern table
// From address $0000 - $1FFF
void fetch_tile_graphics() {
}


void draw_pixel() {

}