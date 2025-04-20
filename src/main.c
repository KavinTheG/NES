#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "cpu.h"
#include "config.h"
#include "ppu.h"


void load_sys_mem(Cpu6502 *cpu, PPU *ppu, char *filename) {

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

    //load_ppu_ines_header(header);
    int prg_size = header[4] * 16 * 1024;
    int chr_size = header[5] * 8 * 1024;

    printf("PRG ROM size: %d bytes\n", prg_size);
    printf("CHR ROM size: %d bytes\n", chr_size);

    if (prg_size != 16384 && prg_size != 32768) {
        printf("Unsupported PRG ROM size!\n");
        fclose(rom);
        return;
    }

    // Read PRG ROM into a temporary buffer
    unsigned char *prg_rom = malloc(prg_size);
    if (!prg_rom) {
        printf("Memory allocation failed!\n");
        fclose(rom);
        return;
    }

    if (fread(prg_rom, 1, prg_size, rom) != prg_size) {
        perror("Error reading PRG ROM");
        free(prg_rom);
        fclose(rom);
        return;
    }

    // Read PRG ROM into a temporary buffer
    unsigned char *chr_rom = malloc(chr_size);
    if (!chr_rom) {
        printf("ppu memory allocation failed!\n");
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
    load_ppu_ines_header(header);

    load_cpu_memory(cpu, prg_rom, prg_size);
    load_ppu_memory(ppu, chr_rom, chr_size);

    free(prg_rom);
    free(chr_rom);

}

void load_ppu_palette(char *filename) {
    FILE *pal = fopen("palette/2C02G_wiki.pal", "rb");

    if (!pal) {
        perror("Failed to open .pal file");
        return;
    }

    uint8_t palette[192];

    fread(palette, 1, 192, pal);
    fclose(pal);

    load_palette(palette);
}

int main() {

    Cpu6502 cpu;
    PPU ppu;


    #if NES_TEST_ROM
    load_sys_mem(&cpu, &ppu, "test/nestest.nes");
    #else
    load_test_rom(&cpu);
    #endif
    
    cpu_init(&cpu);
    ppu_init(&ppu);

    cpu.ppu = &ppu;

    sleep(1);

    // logging
    const char *filename = "log.txt";

    FILE *log = fopen(filename, "a");

    fprintf(log, "Program started\n");

    while (1) {
        

        // Execute cpu cycle
        cpu_execute(&cpu);

        ppu_execute_cycle(&ppu);
        ppu_execute_cycle(&ppu);
        ppu_execute_cycle(&ppu);

    }

}






