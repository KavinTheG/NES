#ifndef PPU_RENDER_H
#define PPU_RENDER_H

typedef struct PPU PPU;

void background_ppu_render(PPU *ppu);
void sprite_ppu_render(PPU *ppu);
void ppu_exec_visible_scanline(PPU *ppu);
void ppu_exec_pre_render(PPU *ppu);
void ppu_exec_vblank(PPU *ppu);

#endif
