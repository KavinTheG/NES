#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define TILE_WIDTH 8
#define TILE_HEIGHT 8
#define TILES_PER_ROW 16
#define SCALE 4

#define SCREEN_WIDTH (TILES_PER_ROW * TILE_WIDTH * SCALE * 2)
#define SCREEN_HEIGHT (TILE_HEIGHT * SCALE * (512 / (TILES_PER_ROW * 2)))

void draw_tile(SDL_Renderer* renderer, uint8_t* chr, int tile_index, int x, int y) {
    int base = tile_index * 16;

    for (int row = 0; row < TILE_HEIGHT; row++) {
        uint8_t plane0 = chr[base + row];
        uint8_t plane1 = chr[base + row + 8];

        for (int col = 0; col < TILE_WIDTH; col++) {
            int bit = 7 - col;
            uint8_t low = (plane0 >> bit) & 1;
            uint8_t high = (plane1 >> bit) & 1;
            uint8_t color = (high << 1) | low;

            switch (color) {
                case 0: SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); break;        // Transparent
                case 1: SDL_SetRenderDrawColor(renderer, 85, 85, 85, 255); break;     // Light Gray
                case 2: SDL_SetRenderDrawColor(renderer, 170, 170, 170, 255); break;   // Medium Gray
                case 3: SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); break;   // White
            }

            SDL_Rect pixel = { x + col * SCALE, y + row * SCALE, SCALE, SCALE };
            SDL_RenderFillRect(renderer, &pixel);
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s rom.nes\n", argv[0]);
        return 1;
    }

    FILE* rom = fopen(argv[1], "rb");
    if (!rom) {
        perror("Failed to open ROM");
        return 1;
    }

    fseek(rom, 0, SEEK_END);
    long rom_size = ftell(rom);
    rewind(rom);

    uint8_t header[16];
    fread(header, 1, 16, rom);

    int prg_size = header[4] * 16384;
    int chr_size = header[5] * 8192;

    fseek(rom, prg_size + 16, SEEK_SET);

    uint8_t* chr = malloc(chr_size);
    fread(chr, 1, chr_size, rom);
    fclose(rom);

    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Pattern Table Viewer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    for (int i = 0; i < 512; i++) {
        int pt_offset = (i >= 256) ? SCREEN_WIDTH / 2 : 0;
        int row = (i % 256) / TILES_PER_ROW;
        int col = i % TILES_PER_ROW;

        int x = pt_offset + col * TILE_WIDTH * SCALE;
        int y = row * TILE_HEIGHT * SCALE;

        draw_tile(renderer, chr, i, x, y);
    }

    SDL_RenderPresent(renderer);

    SDL_Event e;
    int quit = 0;
    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) quit = 1;
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    free(chr);

    return 0;
}
