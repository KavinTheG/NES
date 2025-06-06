# NES

This is my implementation of an NES emulator written in C.

## TODO
- [ ] Rendering
  - [x] 8x8 sprite rendering
  - [ ] 8x16 sprite rendering
  - [x] Horizontal sprite flipping
  - [x] Vertical sprite flipping
  - [x] Background rendering
  - [x] Sprite-background priority handling
  - [ ] Sprite overflow / sprite zero hit detection
- [ ] Audio support
- [x] Mapper support (only NROM / mapper 0)

## Project Structure
#### src/main.c
- Performs ROM parsing
- SDL renders the frame buffer generated by PPU
#### src/cpu.c
- Emulates the official 6502 ISA
#### src/ppu.c
- Emulates NES PPU logic

## Usage

### Requirements
The following items are required for this program to function:

- gcc/clang
- SDL2 

### Building

Git clone the repository with the following command: 

```
git clone https://github.com/KavinTheG/chip8.git
```

Change directory to my chip8 project directory

```
cd NES
```

Enter the following command to build the project

```
make
```

This will create a binary file located in the bin/ folder. Currently, I am only testing on the nes-test.nes rom and donkey kong rom. Place the respective rom files in test/ or rom/. Run the command below to execute.

```
./bin/emulator
```

### Progress 
Able to run Donkey Kong with minor issues.
![image](https://github.com/user-attachments/assets/76d6df8b-2864-4093-95c5-c1831ef01364)

![image](https://github.com/user-attachments/assets/f319269c-a6a1-4a1e-a45f-04f62f8f5c8d)
![image](https://github.com/user-attachments/assets/7ee75dc4-60ea-4312-9314-5b9880a8fe90)


## References

- https://www.nesdev.org/wiki/Nesdev_Wiki
