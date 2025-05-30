#ifndef PPU_H
#define PPU_H

// === Project Includes ===
#include "config.h"
#include "pipeline.h"

// === Constants ===
#define PPU_MEMORY_SIZE 0x4000 // 16 KB
#define PALETTE_SIZE 64
#define NES_HEADER_SIZE 16

#define OAM_SIZE 0x100
#define OAM_SECONDARY_SIZE 32

#define NUM_DOTS 341
#define NUM_SCANLINES 262

// === Logging Macro ===
#define LOG(fmt, ...)                                                          \
  do {                                                                         \
    if (PPU_LOGGING)                                                           \
      fprintf(stderr, fmt, ##__VA_ARGS__);                                     \
  } while (0)

// === PPU Structure ===
typedef struct PPU {
  // Control and status registers
  uint8_t PPUCTRL;
  uint8_t PPUMASK;
  uint8_t PPUSTATUS;
  uint8_t OAMADDR;
  uint8_t OAMDATA;
  uint8_t PPUSCROLL;
  uint16_t PPUADDR;
  uint8_t PPUDATA;
  uint8_t PPUDATA_READ_BUFFER;
  uint8_t OAMDMA;

  // Loopy registers and scroll
  uint16_t v, t; // Current and temporary VRAM address
  uint8_t w;     // Write toggle
  uint8_t x;     // Fine X scroll

  // Internal flags
  unsigned char drawing_bg_flag;
  unsigned char vblank_flag;
  unsigned char nmi_flag;
  unsigned char copy_sprite_flag;
  unsigned char update_graphics;

  // Tile fetch registers
  Pipeline bg_pipeline;
  Pipeline sprite_pipeline;

  // Output buffer
  uint32_t frame_buffer[SCREEN_HEIGHT_VIS][SCREEN_WIDTH_VIS];

  // === Sprite Variables
  uint8_t sprite_evaluation_index;
  uint8_t index_of_sprite;
  uint8_t oam_memory_top;
  uint8_t sprite_height;

  int sprite_render_index;
  int sprite_zero_index;

  // Timing
  int current_scanline_cycle;
  int total_cycles;
  int scanline;
  int frame;

} PPU;

// === Global PPU Memory ===
extern uint8_t ppu_memory[PPU_MEMORY_SIZE];
extern uint8_t nes_header[NES_HEADER_SIZE];
extern uint8_t ppu_palette[PALETTE_SIZE * 3];
extern uint8_t open_bus;

// === OAM PPU Memory ===
extern uint8_t oam_memory[OAM_SIZE];
extern uint8_t oam_memory_secondary[OAM_SECONDARY_SIZE];
extern uint8_t oam_buffer_latches[OAM_SECONDARY_SIZE];

// === Initialization and Loading ===
void ppu_init(PPU *ppu);
void load_ppu_memory(PPU *ppu, unsigned char *chr_rom, int chr_size);
void load_ppu_oam_mem(PPU *ppu, uint8_t *dma_mem);
void load_ppu_ines_header(unsigned char *header);
void load_palette(uint8_t *palette);

// === Memory Read/Write ===
uint8_t read_mem(PPU *ppu, uint16_t addr);
void write_mem(PPU *ppu, uint16_t addr, uint8_t val);

// === PPU Execution ===
void ppu_execute_cycle(PPU *ppu);
void ppu_exec_pre_render(PPU *ppu);
void ppu_exec_visible_scanline(PPU *ppu);
void ppu_exec_vblank(PPU *ppu);

// === Rendering ===
void ppu_render(PPU *ppu);

// === MMIO Register Access ===
void ppu_registers_write(PPU *ppu, uint16_t addr, uint8_t val);
uint8_t ppu_registers_read(PPU *ppu, uint16_t addr);

#endif // PPU_H
