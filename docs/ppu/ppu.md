# PPU Flow

## Step 1
- Fetch a nametable entry from 0x2000 - 0x2FFF
    - Each byte controls 1 tile
    - Each nametable has 30 rows of 32 tiles each
    - 960 bytes (64 remaining are the attribute tables) 


## Step 2
- Fetch corresponding attribute table entry
    - Each byte controls palette of 4x4 tiles
    - Divided into four 2-bits
        - Each 2 bits covers 2x2 tiles
        - Determines the palette index

## Step 3
- Increment pointer to VRAM addr

## Step 4
- Use that value retrieved from the NT entry as a index for the pattern table
    - Each 8x8 pixels is stores as 16 bytes in PT
        - First 8 bytes are the LSB (bitplane 0)
        - Second 8 bytes are the MSB (bitplane 1)
        - The two bitplanes form to make a 2-bit colour index
            - 00, 01, 10, 11

## Step 5
- Render pixel

So we have the tile index from NT which determines which tile to use from PT, the palette index from the AT which determines which palette to use, and
the colour index from the PT which determines which colour to use in the specified palette.