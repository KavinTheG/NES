#include "ppu_render.h"
#include "ppu.h"
#include <string.h>
#include <unistd.h>

uint8_t fetch_name_table_byte(PPU *ppu) {
  return read_mem(ppu, 0x2000 | (ppu->v & 0xFFF));
}

uint8_t fetch_attr_table_byte(PPU *ppu) {

  uint16_t attr_address = 0x23C0 | (ppu->v & 0x0C00) | ((ppu->v >> 4) & 0x38) |
                          ((ppu->v >> 2) & 0x07);

  return read_mem(ppu, attr_address);
}

uint8_t fetch_pattern_table_byte(PPU *ppu, uint8_t row_padding,
                                 uint8_t bit_plane) {
  uint16_t base_address = ppu->bg_pipeline.name_table_byte * 16;
  uint8_t row = ppu->scanline % 8;

  // Offset by 8 if MSB (bit_plane == 1)
  return read_mem(ppu, base_address + (bit_plane ? 8 : 0) + row + row_padding);
}

void sprite_ppu_render(PPU *ppu) {

  for (int i = 0; i <= OAM_SECONDARY_SIZE - 4; i += 4) {
    int sprite_y = oam_buffer_latches[i] + 1;
    int sprite_x = oam_buffer_latches[i + 3];

    if (ppu->scanline < sprite_y ||
        ppu->scanline >= sprite_y + ppu->sprite_height)
      continue;

    int row_in_tile = ppu->scanline - sprite_y;

    uint8_t tile_index = oam_buffer_latches[i + 1];
    uint8_t attr = oam_buffer_latches[i + 2];
    uint8_t palette_index = attr & 0x3;

    int flip_horizontal = attr & 0x40;
    int flip_vertical = attr & 0x80;

    if (flip_vertical)
      row_in_tile = ppu->sprite_height - 1 - row_in_tile;

    uint16_t pattern_addr_base = (ppu->PPUCTRL & 0x08) ? 0x1000 : 0x0000;

    if (ppu->sprite_height == 16) {
      pattern_addr_base = (tile_index & 1) ? 0x1000 : 0x0000;
      tile_index &= 0xFE;
    }

    uint16_t pattern_addr = pattern_addr_base + tile_index * 16 + row_in_tile;
    uint8_t pattern_lsb = ppu_memory[pattern_addr & 0x1FFF];
    uint8_t pattern_msb = ppu_memory[(pattern_addr + 8) & 0x1FFF];

    for (int j = 0; j < 8; j++) {
      int bit = flip_horizontal ? j : (7 - j);
      uint8_t lo = (pattern_lsb >> bit) & 1;
      uint8_t hi = (pattern_msb >> bit) & 1;
      uint8_t pixel_val = (hi << 1) | lo;

      if (pixel_val == 0)
        continue; // Transparent

      int screen_x = sprite_x + j;
      int screen_y = ppu->scanline;

      if (screen_x >= 256 || screen_y >= 240)
        continue;

      uint16_t pal_addr = 0x3F10 | (palette_index << 2) | pixel_val;
      uint8_t palette_data = read_mem(ppu, pal_addr);
      uint8_t *color = &ppu_palette[palette_data * 3];
      uint8_t r = color[0], g = color[1], b = color[2];

      ppu->frame_buffer[screen_y][screen_x] =
          (r << 24) | (g << 16) | (b << 8) | 0xFF;
    }
  }
}

/**
 * @brief  Executes PPU memory fetches for rendering
 *
 * @param       ppu     PPU instance
 * @return              void
 */
void background_ppu_render(PPU *ppu) {
  switch (ppu->current_scanline_cycle % 8) {
  // Fetch Nametable byte
  case 1:

    ppu->bg_pipeline.name_table_byte = fetch_name_table_byte(ppu);
    break;

  // Fetch the corresponding attribute byte
  case 3:
    ppu->bg_pipeline.attribute_byte = fetch_attr_table_byte(ppu);
    // Determines which of the four areas of the attribute byte to use
    // Each attribute byte covers 4x4 tiles → each quadrant is 2x2 tiles
    uint8_t quadrant_x = ((ppu->v & 0x1F) >> 1) & 1; // 0 = left, 1 = right
    uint8_t quadrant_y =
        (((ppu->v >> 5) & 0x1F) >> 1) & 1; // 0 = top,  1 = bottom

    uint8_t shift = (quadrant_y << 1 | quadrant_x) * 2; // 0, 2, 4, or 6
    ppu->bg_pipeline.palette_index =
        (ppu->bg_pipeline.attribute_byte >> shift) & 0x3;
    break;

  // Fetch nametable low byte
  case 5:
    int row_padding =
        ppu->current_scanline_cycle >= 321 && ppu->current_scanline_cycle <= 336
            ? 1
            : 0;

    ppu->bg_pipeline.pattern_table_lsb =
        read_mem(ppu, (ppu->bg_pipeline.name_table_byte * 16) +
                          ((ppu->scanline + row_padding) % 8));
    break;

  // Fetch high byte
  case 7:

    row_padding =
        ppu->current_scanline_cycle >= 321 && ppu->current_scanline_cycle <= 336
            ? 1
            : 0;

    ppu->bg_pipeline.pattern_table_msb =
        read_mem(ppu, (ppu->bg_pipeline.name_table_byte * 16) + 8 +
                          ((ppu->scanline + row_padding) % 8));
    break;

  // Store value in buffer
  case 0:
    // if (ppu->PPUMASK & 0x18) {
    if ((ppu->v & 0x001f) == 0x1f) {
      ppu->v &= ~0x001F; // Wrap around
      ppu->v ^= 0x0400;  // Switch horizontal N.T
    } else {
      ppu->v += 1;
    }

    if (ppu->current_scanline_cycle == 256) {
      if ((ppu->v & 0x7000) != 0x7000) {
        ppu->v += 0x1000;
      } else {
        ppu->v &= ~0x7000;
        int coarse_y = (ppu->v & 0x03E0) >> 5;
        if (coarse_y == 29) {
          coarse_y = 0;
          ppu->v ^= 0x0800;
        } else if (coarse_y == 31) {
          coarse_y = 0;
        } else {
          coarse_y += 1;
        }
        ppu->v = (ppu->v & ~0x03E0) | (coarse_y << 5);
      }
    }

    unsigned char is_pre_fetch =
        ppu->current_scanline_cycle >= 321 && ppu->current_scanline_cycle <= 336
            ? 1
            : 0;

    int row = is_pre_fetch ? ppu->scanline + 1 : ppu->scanline;
    int column_base = is_pre_fetch ? ppu->current_scanline_cycle - 328
                                   : ppu->current_scanline_cycle + 8;

    for (int i = 0; i < TILE_SIZE; i++) {
      uint8_t bit = 7 - i;
      uint8_t bg_lo = (ppu->bg_pipeline.pattern_table_lsb >> bit) & 1;
      uint8_t bg_hi = (ppu->bg_pipeline.pattern_table_msb >> bit) & 1;

      ppu->bg_pipeline.tile_pixel_value[i] = (bg_hi << 1) | bg_lo;

      uint8_t palette_index = ppu->bg_pipeline.palette_index;

      uint16_t pal_addr =
          0x3F00 | (palette_index << 2) | ppu->bg_pipeline.tile_pixel_value[i];

      uint8_t palette_data = read_mem(ppu, pal_addr);
      uint8_t alpha = (ppu->bg_pipeline.tile_pixel_value[i] == 0) ? 0x00 : 0xFF;

      uint8_t *color = &ppu_palette[palette_data * 3];
      uint8_t r = color[0], g = color[1], b = color[2];

      int column = column_base + i;

      if (column >= 256 || row >= 240)
        break;

      ppu->frame_buffer[row][column] = (r << 24) | (g << 16) | (b << 8) | alpha;
    }
    break;
  }
}

/**
 * @brief  Executes PPU pre-render scanline
 *
 * @param       ppu     PPU instance
 * @return              void
 */
void ppu_exec_pre_render(PPU *ppu) {

  // During cycle 280 to 304 of the pre-render scanline
  // PPU will repeatedly copy vertical bits from t to v
  if (ppu->current_scanline_cycle >= 280 &&
      ppu->current_scanline_cycle <=
          304) // background or sprite rendering enabled
  {
    ppu->v = (ppu->v & 0x041F) | (ppu->t & 0x7BE0);
  }

  if (ppu->current_scanline_cycle >= 321 &&
      ppu->current_scanline_cycle <= 340 && ppu->PPUMASK & 0x18) {
    background_ppu_render(ppu);
  }
}

void sprite_detect(PPU *ppu) {
  // Reset secondary OAM memory at cycle 1
  if (ppu->current_scanline_cycle == 1) {
    memset(oam_memory_secondary, 0xFF, OAM_SECONDARY_SIZE);
  }

  if (ppu->current_scanline_cycle == 336) {
    ppu->copy_sprite_flag = 0;
    ppu->index_of_sprite = 0;
    ppu->sprite_evaluation_index = 0;
    ppu->oam_memory_top = 0;
  }

  // Bit 5 of PPUCTRL determines sprite size
  int sprite_height = ppu->PPUCTRL & 0x20 ? 16 : 8;

  if (ppu->current_scanline_cycle >= 65 && ppu->current_scanline_cycle <= 256) {
    // int index = ppu->sprite_evaluation_index * 4 + ppu->index_of_sprite;
    int index = ppu->sprite_evaluation_index * 4;
    // Odd cycle; read OAM
    if (ppu->current_scanline_cycle % 2 == 1) {
      if (ppu->scanline >= oam_memory[index] &&
          ppu->scanline < oam_memory[index] + sprite_height) {
        ppu->copy_sprite_flag = 1;
      }
    } else {
      if (ppu->copy_sprite_flag && ppu->oam_memory_top <= 31) {
        oam_memory_secondary[ppu->oam_memory_top++] = oam_memory[index];
        oam_memory_secondary[ppu->oam_memory_top++] = oam_memory[index + 1];
        oam_memory_secondary[ppu->oam_memory_top++] = oam_memory[index + 2];
        oam_memory_secondary[ppu->oam_memory_top++] = oam_memory[index + 3];
      }
      if (ppu->sprite_evaluation_index == 64) {
        ppu->sprite_evaluation_index = -1;
        ppu->PPUSTATUS |= 0x20;
      }
      ppu->sprite_evaluation_index++;
      ppu->copy_sprite_flag = 0;
    }
  }
}

/**
 * @brief  Executes PPU visible scanlines
 *
 * @param       ppu     PPU instance
 * @return              void
 */
void ppu_exec_visible_scanline(PPU *ppu) {
  sprite_detect(ppu);

  if (ppu->current_scanline_cycle >= 1 && ppu->current_scanline_cycle <= 256) {
    background_ppu_render(ppu);
    sprite_ppu_render(ppu);
    ppu->sprite_render_index = -1;
  } else if (ppu->current_scanline_cycle == 256) {
    memset(oam_memory_secondary, 0, 32);
  } else if (ppu->current_scanline_cycle >= 257 &&
             ppu->current_scanline_cycle <= 320) {
    /* HBLANK */
    if (ppu->current_scanline_cycle == 257) {
      // Reset register v's horizontal position (first 5 bits - 0x1F - 0b11111)
      ppu->v = (ppu->v & 0xFBE0) | (ppu->t & 0x001F);
    }

    // Tile data for the sprites on the next scanline are loaded into rendering
    // latches
    oam_buffer_latches[(ppu->current_scanline_cycle - 257) % 32] =
        oam_memory_secondary[(ppu->current_scanline_cycle - 257) % 32];
  } else if (ppu->current_scanline_cycle >= 321 &&
             ppu->current_scanline_cycle <= 336) {
    background_ppu_render(ppu);
  }
}

void ppu_exec_vblank(PPU *ppu) {
  if (ppu->scanline == 241 && ppu->current_scanline_cycle == 1) {
    /* VBLANK */
    ppu->vblank_flag = 1;
    ppu->PPUSTATUS |= 0b10000000;
    ppu->update_graphics = 1;

    // PPUCTRL bit 7 determines if CPU accepts NMI
    if (ppu->PPUCTRL & 0x80) {
      // Set NMI
      ppu->nmi_flag = 1;
    }
  }
}
