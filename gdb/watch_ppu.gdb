set pagination off
set logging enabled on
set logging file ppu_watch.log
set logging overwrite on

# --- Watch scanline ---
watch ppu->scanline
commands
  printf "scanline written: %d at PC = 0x%x\n", ppu->scanline, $pc
  if (ppu->scanline < -1 || ppu->scanline > 260)
    printf "ERROR: scanline out of bounds: %d\n", ppu->scanline
    continue
  end
  continue
end

# --- Watch current_scanline_cycle ---
watch ppu->current_scanline_cycle
commands
  printf("cycle written: %d at PC = 0x%x\n", ppu->current_scanline_cycle, $pc)
  if (ppu->current_scanline_cycle < 0 || ppu->current_scanline_cycle > 340)
    printf("ERROR: current_scanline_cycle out of bounds: %d\n", ppu->current_scanline_cycle)
    continue
  end
  continue
end

