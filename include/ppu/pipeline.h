#include <stdint.h>

typedef struct Pipeline {
  uint8_t name_table_byte;
  uint8_t attribute_byte;
  uint8_t palette_index;
  uint8_t pattern_table_msb;
  uint8_t pattern_table_lsb;
  uint16_t palette_ram_addr;
  uint8_t tile_pixel_value[8];
} Pipeline;
