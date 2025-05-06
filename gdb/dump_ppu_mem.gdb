define dump_ppu_memory
  set $i = 0
  while $i < 0x400
    printf "0x%04x: ", $i
    set $j = 0
    while $j < 0x10
      printf "%02x ", ppu_memory[0x2000 + $i + $j]
      set $j = $j + 1
    end
    printf "\n"
    set $i = $i + 0x10
  end
end

dump_ppu_memory
