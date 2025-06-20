#include <SDL2/SDL.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

#include "apu/apu.h"
#include "apu/apu_mmio.h"
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
  APU apu;
  APU_MMIO apu_mmio;

  Frontend frontend;

  Frontend_Init(&frontend, SCREEN_WIDTH_VIS, SCREEN_HEIGHT_VIS, SCALE);

#if NES_TEST_ROM == 1
  rom_load_cartridge(&rom, "rom/nestest.nes");
#elif NES_TEST_ROM == 2
  if (rom_load_cartridge(&rom, "rom/official.nes") != 0) {
    printf("ROM LOAD FAILED\n");
  }
#else
  // rom_load_cartridge(&rom, "rom/Donkey Kong.nes");
  rom_load_cartridge(&rom, "rom/Ice_Climber.nes");
#endif
  load_cpu_memory(&cpu, rom.prg_data, rom.prg_size);

  load_ppu_ines_header(rom.header);
  load_ppu_memory(&ppu, rom.chr_data, rom.chr_size);
  load_ppu_palette("palette/2C02G_wiki.pal");

  ppu_init(&ppu);
  cpu.ppu = &ppu;
  cpu_init(&cpu);

  cpu.apu_mmio = &apu_mmio;
  apu.apu_mmio = &apu_mmio;

  while (1) {
    // Execute cpu cycle
    cpu_execute(&cpu);
    apu.cpu_cycle = cpu.cycles;

    if (ppu.update_graphics) {
      ppu.update_graphics = 0;

      Frontend_DrawFrame(&frontend, ppu.frame_buffer);
      if (Frontend_HandleInput(&frontend) != 0)
        break;
    }
    // if (Frontend_HandleInput(&frontend) != 0) {
    //   break;
    // } else {
    if (cpu.strobe) {
      cpu.ctrl_latch_state = frontend.controller;
    }
  }

  Frontend_Destroy(&frontend);
  return 0;
}
