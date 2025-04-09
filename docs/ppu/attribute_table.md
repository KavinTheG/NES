# Attribute Table

- 64-byte array at the end of each nametable
- each byte controls *palette* of 32x32 pixel (4x4 tile) part of nametable
- the byte is divided in 4 2-byt areas
    - each area covers 16x16 pixels (2x2 tiles)


| Bit # | Significance         |
|-------|----------------------|
| 0     | Colour bits 3-2 for top left quadrant of this byte |
| 1     | Colour bits 3-2 for top left quadrant of this byte |
| 2     | Colour bits 3-2 for top right quadrant of this byte |
| 3     | Colour bits 3-2 for top right quadrant of this byte |
| 4     | Colour bits 3-2 for bottom left quadrant of this byte |
| 5     | Colour bits 3-2 for bottom left quadrant of this byte |
| 6     | Colour bits 3-2 for bottom right quadrant of this byte |
| 7     | Colour bits 3-2 for bottom right quadrant of this byte |
