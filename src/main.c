#include <SDL2/SDL.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

#include "config.h"
#include "cpu/cpu.h"
#include "frontend.h"
#include "ppu.h"
#include "rom.h"

void load_ppu_palette(char *filename) {
  FILE *pal = fopen(filename, "rb");

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
  Rom rom;

  Frontend frontend;

  Frontend_Init(&frontend, SCREEN_WIDTH_VIS, SCREEN_HEIGHT_VIS, SCALE);

  rom_load_cartridge(&rom, "rom/Donkey Kong.nes");

  load_cpu_memory(&cpu, rom.prg_data, rom.prg_size);

  load_ppu_ines_header(rom.header);
  load_ppu_memory(&ppu, rom.chr_data, rom.chr_size);
  load_ppu_palette("palette/2C02G_wiki.pal");

  ppu_init(&ppu);
  cpu.ppu = &ppu;
  cpu_init(&cpu);

  const char *filename = "log.txt";

  FILE *log = fopen(filename, "a");

  fprintf(log, "Program started\n");

  while (1) {
    // Execute cpu cycle
    cpu_execute(&cpu);

    if (ppu.update_graphics) {
      ppu.update_graphics = 0;

      Frontend_DrawFrame(&frontend, ppu.frame_buffer);
    }
    if (Frontend_HandleInput(&frontend) != 0) {
      break;
    } else {
      if (cpu.strobe) {
        cpu.ctrl_latch_state = frontend.controller;
        printf("Controller State: %x\n", frontend.controller);
      }
    }
  }

  Frontend_Destroy(&frontend);
  return 0;
}
