#include "ppu.h"
#include <stdio.h>

#define NES_HEADER_SIZE 16
#define MEMORY_SIZE 0x4000 // 16 KB

uint8_t memory[MEMORY_SIZE] = {0};


void ppu_init(PPU *ppu) {

}

void load_ppu_mem(PPU *ppu, char *filename) {

    FILE *rom = fopen(filename, "rb");
    if (!rom) {
        perror("Error opening ROM file");
        return;
    }

    // Read INES header
    unsigned char header[NES_HEADER_SIZE];
    if (fread(header, 1, NES_HEADER_SIZE, rom) != NES_HEADER_SIZE) {
        perror("Error: header does not match 16 bytes!");
        fclose(rom);
        return;
    }

    int prg_size = header[4] * 16 * 1024;
    int chr_size = header[5] * 8 * 1024;

    printf("PRG ROM size: %d bytes\n", prg_size);
    printf("CHR ROM size: %d bytes\n", chr_size);

    // Read PRG ROM into a temporary buffer
    unsigned char *chr_rom = malloc(chr_size);
    if (!chr_rom) {
        printf("Memory allocation failed!\n");
        fclose(rom);
        return;
    }

    if (fread(chr_rom, 1, chr_size, rom) != chr_size) {
        perror("Error reading PRG ROM");
        free(chr_rom);
        fclose(rom);
        return;
    }

    fclose(rom);

    // Clear memory
    memset(memory, 0, MEMORY_SIZE);

    // Load CHR ROM into 0x0000 - 0x1FFF
    memcpy(&memory[0x0000], chr_rom, chr_size);

    free(chr_rom);    
}