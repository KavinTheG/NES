# Set the initial index to 0
set $i = 0

# Loop through OAM memory until the end
while $i < 0xFF
    # Print the first byte of the sprite at index i
    printf "Index: %d, Value: 0x%02X\n", $i, oam_memory[$i]


    # If the first byte is 0xFF, print the message

    # Move to the next sprite (assuming each sprite is 4 bytes)
    set $i = $i + 4
end