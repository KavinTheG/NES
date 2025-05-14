#ifndef PPU_RENDER_H
#define PPU_RENDER_H

#include <string.h>

typedef struct PPU PPU;

void ppu_render(PPU *ppu);
void ppu_exec_visible_scanline(PPU *ppu);
void ppu_exec_pre_render(PPU *ppu);
void ppu_exec_vblank(PPU *ppu);

#endif
