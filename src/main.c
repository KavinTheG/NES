#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <SDL2/SDL.h>

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

    int prg_size = header[4] * 16 * 1024;  // PRG ROM size in bytes
    int chr_size = header[5] * 8 * 1024;   // CHR ROM size in bytes

    printf("PRG ROM size: %d bytes\n", prg_size);
    printf("CHR ROM size: %d bytes\n", chr_size);

    if (prg_size != 16384 && prg_size != 32768) {
        printf("Unsupported PRG ROM size!\n");
        fclose(rom);
        return;
    }

    // Allocate memory for PRG ROM
    unsigned char *prg_rom = malloc(prg_size);
    if (!prg_rom) {
        printf("Memory allocation failed for PRG ROM!\n");
        fclose(rom);
        return;
    }

    // Read PRG ROM into memory
    if (fread(prg_rom, 1, prg_size, rom) != prg_size) {
        perror("Error reading PRG ROM");
        free(prg_rom);
        fclose(rom);
        return;
    }

    // Allocate memory for CHR ROM
    unsigned char *chr_rom = malloc(chr_size);
    if (!chr_rom) {
        printf("Memory allocation failed for CHR ROM!\n");
        free(prg_rom);
        fclose(rom);
        return;
    }

    // After reading the PRG ROM, the file pointer is at the end of the PRG ROM.
    // Now we need to move the file pointer to the start of CHR ROM.
    if (fseek(rom, NES_HEADER_SIZE + prg_size, SEEK_SET) != 0) {
        perror("Error seeking to CHR ROM in file");
        free(prg_rom);
        free(chr_rom);
        fclose(rom);
        return;
    }

    // Read CHR ROM into memory
    if (fread(chr_rom, 1, chr_size, rom) != chr_size) {
        perror("Error reading CHR ROM");
        free(prg_rom);
        free(chr_rom);
        fclose(rom);
        return;
    }

    fclose(rom);

    // Load the PPU header if necessary
    load_ppu_ines_header(header);

    // Load CPU and PPU memory
    load_cpu_memory(cpu, prg_rom, prg_size);
    load_ppu_memory(ppu, chr_rom, chr_size);

    // Free the allocated memory after loading
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

    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("NES",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        256, 240, SDL_WINDOW_SHOWN);

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_Texture* texture = SDL_CreateTexture(renderer,
        SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING,
        256, 240);

    //uint32_t frame_buffer[256 * 240]; 


    #if NES_TEST_ROM
    load_sys_mem(&cpu, &ppu, "test/nestest.nes");
    #else
    //load_test_rom(&cpu);
    load_sys_mem(&cpu, &ppu, "rom/Donkey Kong (USA) (GameCube Edition).nes");

    #endif
    
    load_ppu_palette("palette/2C02G_wiki.pal");

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

        
        if (ppu.update_graphics) {
            // Fill the frame buffer with red (test)
            // for (int i = 0; i < 256 * 240; i++) {
            //     frame_buffer[i] = 0xFF0000FF;  // Red
            // }
            SDL_UpdateTexture(texture, NULL, get_frame_buffer(&ppu), 256 * sizeof(uint32_t));
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, texture, NULL, NULL);
            SDL_RenderPresent(renderer);

            reset_graphics_flag(&ppu);
        }

    }

}






