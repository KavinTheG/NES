#include <SDL2/SDL.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include "apu/apu.h"
#include "apu/apu_mmio.h"
#include "config.h"
#include "cpu/cpu.h"
#include "frontend.h"
#include "ppu.h"
#include "rom.h"

#define CPU_CLOCK_HZ 1789773.0
#define APU_CLOCK_HZ (CPU_CLOCK_HZ / 2.0) // APU ticks at half CPU rate
#define AUDIO_SAMPLE_RATE 44100.0

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

int main(int argc, char *argv[]) {

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
  if (argc < 2) {
    printf("No ROM file specified. Usage: %s <path-to-rom>\n", argv[0]);
    return 1;
  }
  rom_load_cartridge(&rom, argv[1]);
  // rom_load_cartridge(&rom, "rom/Donkey Kong.nes");
  //  rom_load_cartridge(&rom, "rom/Ice_Climber.nes");
#endif
  load_cpu_memory(&cpu, rom.prg_data, rom.prg_size);

  load_ppu_ines_header(rom.header);
  load_ppu_memory(&ppu, rom.chr_data, rom.chr_size);
  load_ppu_palette("palette/2C02G_wiki.pal");

  ppu_init(&ppu);
  cpu.ppu = &ppu;
  cpu_init(&cpu);
  apu_init(&apu, &apu_mmio);

  cpu.apu_mmio = &apu_mmio;

  double apu_accumulator = 0.;
  double sample_accumulator = 0.;
  double apu_ticks_per_sample = CPU_CLOCK_HZ / AUDIO_SAMPLE_RATE;

  while (1) {
    // Execute cpu cycle
    cpu_execute(&cpu);

    for (int i = 0; i < cpu.cycles; i++) {
      apu_execute(&apu);

      sample_accumulator += apu_output(&apu);
      apu_accumulator += 1.0;

      if (apu_accumulator >= apu_ticks_per_sample) {
        apu_accumulator -= apu_ticks_per_sample;

        int16_t sample =
            ((int)(sample_accumulator / apu_ticks_per_sample) - 8) * 4096;

        audio_buffer_add(sample);

        sample_accumulator = 0.0;
      }
    }

    if (ppu.update_graphics) {
      ppu.update_graphics = 0;

      Frontend_DrawFrame(&frontend, ppu.frame_buffer);
      Frontend_SetFrameTickStart(&frontend);
    }
    // if (Frontend_HandleInput(&frontend) != 0) {
    //   break;
    // } else {
    if (cpu.strobe) {
      if (Frontend_HandleInput(&frontend) != 0)
        break;
      cpu.ctrl_latch_state = frontend.controller;
    }
  }

  Frontend_Destroy(&frontend);
  apu_destroy(&apu);
  return 0;
}
