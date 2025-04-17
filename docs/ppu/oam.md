# OAM (Object Attribute Memory)
- internal PPU memory
- contains 64 sprites
    - each sprite is 4 bytes


## Byte 0
- Y position of the top of the sprite
- sprite data is delayed 1 scanline
    - subtract 1 from sprite Y 

## Byte 1
- Tile index number
- 8x8 px sprites
    - tile # within the PT
- 8x16 sprites
    - ignore PT table selction
    - select PT from bit 0 of the byte
    - rest of the bits (7 to 1) are the tile #

## Byte 2 
- Bit 2 to 0
    - Palette of sprite
- Bit 4 to 2
    - Unimplemented
- Bit 5 
    - Priority 0
        - in front of bg
        - otherwise behind bg
- Bit 6 
    - Flip sprite horizontally
- Bit 7
    - Flip sprite vertically

## Byte 3
- X pos of left side of sprite