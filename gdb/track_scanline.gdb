# Set a breakpoint on the scanline increment
break ppu.c:196 

commands
silent
    printf "Scanline incremented to: %d\n", ppu->scanline
    continue
end

