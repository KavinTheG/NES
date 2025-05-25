#include "ppu_render.h"
#include "ppu.h"
#include <stdio.h>
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
  if (ppu->current_scanline_cycle == ppu->sprite_zero_index) {
    // sprite zero hit!
    ppu->PPUSTATUS |= 0x40;
    ppu->sprite_zero_index = 0;
  }
  int sprite_height = ppu->PPUCTRL & 0x20 ? 16 : 8;

  for (int i = 0; i <= OAM_SECONDARY_SIZE - 4; i += 4) {
    int sprite_y = oam_buffer_latches[i] + 1;
    int sprite_x = oam_buffer_latches[i + 3];

    if (ppu->scanline >= sprite_y && ppu->scanline < sprite_y + sprite_height) {
      if (sprite_x == ppu->current_scanline_cycle - 8) {
        ppu->sprite_render_index = i;
        break;
      }
    }
  }
  if (ppu->sprite_render_index == -1)
    return;
  ppu->sprite_pipeline.name_table_byte =
      oam_buffer_latches[ppu->sprite_render_index + 1];

  // Fetch the corresponding attribute byte
  ppu->sprite_pipeline.attribute_byte =
      oam_buffer_latches[ppu->sprite_render_index + 2] & 0x3;
  ppu->sprite_pipeline.palette_index =
      ppu->sprite_pipeline.attribute_byte & 0x3;

  // Fetch nametable low byte
  ppu->sprite_pipeline.pattern_table_lsb =
      ppu_memory[((ppu->PPUCTRL & 0x08) ? 0x1000 : 0x0) |
                 (((ppu->sprite_pipeline.name_table_byte * 16) +
                   (ppu->scanline -
                    (oam_buffer_latches[ppu->sprite_render_index] + 1))) &
                  0xFFF)];
  // Fetch nametable high byte
  ppu->sprite_pipeline.pattern_table_msb =
      ppu_memory[((ppu->PPUCTRL & 0x08) ? 0x1000 : 0x0) |
                 (((ppu->sprite_pipeline.name_table_byte * 16) + 8 +
                   (ppu->scanline -
                    (oam_buffer_latches[ppu->sprite_render_index] + 1))) &
                  0xFFF)];
  for (int i = 0; i < 8; i++) {
    uint8_t bit = 7 - i;
    uint8_t sprite_lo = (ppu->sprite_pipeline.pattern_table_lsb >> bit) & 1;
    uint8_t sprite_hi = (ppu->sprite_pipeline.pattern_table_msb >> bit) & 1;
    ppu->sprite_pipeline.tile_pixel_value[i] = (sprite_hi << 1) | sprite_lo;
  }

  unsigned char is_pre_fetch =
      ppu->current_scanline_cycle >= 321 && ppu->current_scanline_cycle <= 336
          ? 1
          : 0;

  int row = is_pre_fetch ? ppu->scanline + 1 : ppu->scanline;
  int column_base = ppu->current_scanline_cycle - 8;

  int sprite_flipped =
      !is_pre_fetch &&
      (oam_buffer_latches[ppu->sprite_render_index + 2] & 0x40) &&
      !ppu->drawing_bg_flag;

  for (int i = 0; i < TILE_SIZE; i++) {
    uint8_t tile_val = ppu->sprite_pipeline.tile_pixel_value[i];

    uint8_t palette_index = ppu->sprite_pipeline.palette_index;

    uint16_t pal_addr = 0x3F10 | (palette_index << 2) | tile_val;

    uint8_t palette_data = read_mem(ppu, pal_addr);
    uint8_t alpha = (tile_val == 0) ? 0x00 : 0xFF;

    uint8_t *color = &ppu_palette[palette_data * 3];
    uint8_t r = color[0], g = color[1], b = color[2];

    int column = sprite_flipped ? (ppu->current_scanline_cycle + 16 - i)
                                : (column_base + i);

    if (column >= 256 || row >= 240)
      break;

    ppu->frame_buffer[row][column] = (r << 24) | (g << 16) | (b << 8) | alpha;
  }
}

/**
 * @brief  Executes PPU memory fetches for rendering
 *
 * @param       ppu     PPU instance
 * @return              void
 */
void background_ppu_render(PPU *ppu) {
  if (ppu->current_scanline_cycle == ppu->sprite_zero_index) {
    // sprite zero hit!
    ppu->PPUSTATUS |= 0x40;
    ppu->sprite_zero_index = 0;
  }
  int sprite_height = ppu->PPUCTRL & 0x20 ? 16 : 8;
  switch (ppu->current_scanline_cycle % 8) {
  // Fetch Nametable byte
  case 1:
    ppu->drawing_bg_flag = 1;

    ppu->bg_pipeline.name_table_byte = fetch_name_table_byte(ppu);
    break;

  // Fetch the corresponding attribute byte
  case 3:
    ppu->bg_pipeline.attribute_byte = fetch_attr_table_byte(ppu);
    // Determines which of the four areas of the attribute byte to use
    uint8_t tile_area_horizontal = (ppu->v & 0x00F) % 4;
    uint8_t tile_area_vertical = ((ppu->v & 0x0F0) / 0x20) % 4;

    if (tile_area_vertical == 0 && tile_area_horizontal == 0) {
      // Top left quadrant (bit 1 & 0)
      ppu->bg_pipeline.palette_index = ppu->bg_pipeline.attribute_byte & 0x03;
    } else if (tile_area_vertical == 0 && tile_area_horizontal == 1) {
      // Top right quadrant (bit 3 & 2)
      ppu->bg_pipeline.palette_index =
          (ppu->bg_pipeline.attribute_byte & 0x0C) >> 2;
    } else if (tile_area_vertical == 1 && tile_area_horizontal == 0) {
      // Bottom left quadrant (bit 5 & 4)
      ppu->bg_pipeline.palette_index =
          (ppu->bg_pipeline.attribute_byte & 0x30) >> 4;
    } else if (tile_area_vertical == 1 && tile_area_horizontal == 1) {
      // Bottom right quadrant (bit 7 6 6)
      ppu->bg_pipeline.palette_index =
          (ppu->bg_pipeline.attribute_byte & 0xC0) >> 6;
    }
    break;

  // Fetch nametable low byte
  case 5:
    int row_padding =
        ppu->current_scanline_cycle >= 321 && ppu->current_scanline_cycle <= 336
            ? 1
            : 0;

    ppu->bg_pipeline.pattern_table_lsb =
        read_mem(ppu, (ppu->bg_pipeline.name_table_byte * 16) +
                          (ppu->scanline % 8) + row_padding);
    break;

  // Fetch high byte
  case 7:

    row_padding =
        ppu->current_scanline_cycle >= 321 && ppu->current_scanline_cycle <= 336
            ? 1
            : 0;

    ppu->bg_pipeline.pattern_table_msb =
        read_mem(ppu, (ppu->bg_pipeline.name_table_byte * 16) + 8 +
                          (ppu->scanline % 8) + row_padding);
    for (int i = 0; i < 8; i++) {
      uint8_t bit = 7 - i;
      uint8_t bg_lo = (ppu->bg_pipeline.pattern_table_lsb >> bit) & 1;
      uint8_t bg_hi = (ppu->bg_pipeline.pattern_table_msb >> bit) & 1;

      ppu->bg_pipeline.tile_pixel_value[i] = (bg_hi << 1) | bg_lo;
    }

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
    int sprite_flipped =
        !is_pre_fetch &&
        (oam_buffer_latches[ppu->sprite_render_index + 2] & 0x40) &&
        !ppu->drawing_bg_flag;

    for (int i = 0; i < TILE_SIZE; i++) {
      // int draw_sprite = !ppu->drawing_bg_flag &&
      //                   ppu->sprite_pipeline.tile_pixel_value[i] != 0;
      int draw_sprite = !ppu->drawing_bg_flag;
      uint8_t tile_val = draw_sprite ? ppu->sprite_pipeline.tile_pixel_value[i]
                                     : ppu->bg_pipeline.tile_pixel_value[i];

      uint8_t palette_index = draw_sprite ? ppu->sprite_pipeline.palette_index
                                          : ppu->bg_pipeline.palette_index;

      uint16_t pal_addr = 0x3F00 | (palette_index << 2) | tile_val;
      if (draw_sprite)
        pal_addr |= 0x10;

      uint8_t palette_data = read_mem(ppu, pal_addr);
      uint8_t alpha = (tile_val == 0) ? 0x00 : 0xFF;

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
      ppu->current_scanline_cycle <= 336 && ppu->PPUMASK & 0x18) {
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
    int index = ppu->sprite_evaluation_index * 4 + ppu->index_of_sprite;
    // Odd cycle; read OAM
    if (ppu->current_scanline_cycle % 2 == 1) {
      if (ppu->index_of_sprite == 0) {
        if (ppu->scanline >= oam_memory[index] &&
            oam_memory[index] < ppu->scanline + sprite_height) {
          ppu->copy_sprite_flag = 1;
          if (index == 0)
            ppu->sprite_zero_index = oam_memory[3];
        }
      }
    } else {
      if (ppu->copy_sprite_flag && ppu->oam_memory_top <= 31) {
        oam_memory_secondary[ppu->oam_memory_top++] = oam_memory[index];

        ppu->index_of_sprite++;

        if (ppu->index_of_sprite > 3) {
          ppu->sprite_evaluation_index++;
          ppu->index_of_sprite = 0;
          ppu->copy_sprite_flag = 0;
        }
      } else {
        ppu->sprite_evaluation_index++;
        ppu->copy_sprite_flag = 0;
      }
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
      LOG("NMI triggered\n");
    }
  }
}
