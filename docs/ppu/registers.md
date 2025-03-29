# PPU Registers

## PPUCTRL
 - bit 0 & 1 determine base nametable address
    - 00 => $2000 
    - 01 => $2400
    - 10 => $2800
    - 11 => $2C00

 - bit 2 determines VRAM address increment per cpu r/w of PPUDATA
    