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
  - [x] Pulse Channels
  - [x] Triange Channel
  - [x] Noise Channel
  - [ ] DMC
- [ ] Mapper support
  - [x] NROM / mapper 0

## Project Structure

```mermaid
classDiagram-v2
direction LR

class Cpu6502 {
  +ppu : PPU*
  +apu_mmio : APU_MMIO*
  +PC : uint16_t
  +A, X, Y : uint8_t
}

class PPU {
  +v, t : uint16_t
  +frame_buffer : [240][256]
}

class APU_MMIO {
  +apu : APU*
  +registers : uint8_t[0x18]
}

class APU {
  +pulse1 : Pulse*
  +pulse2 : Pulse*
  +triangle : Triangle*
  +noise : Noise*
  +frame_counter : FrameCounter*
}

Cpu6502 --> PPU
Cpu6502 --> APU_MMIO
APU_MMIO --> APU
```

```mermaid
classDiagram-v2
direction TB

class APU {
  +pulse1 : Pulse
  +pulse2 : Pulse
  +triangle : Triangle
  +noise : Noise
  +frame_counter : FrameCounter
}

class Pulse {
  +envelope : Envelope
  +sweep : Sweep
  +duty : uint8_t
  +timer : uint16_t
  +counter : uint16_t
  +length_counter : uint8_t
}

class Triangle {
  +linear_counter : Divider
  +timer : uint16_t
  +counter : uint16_t
  +length_counter : uint8_t
}

class Noise {
  +envelope : Envelope
  +timer : uint16_t
  +counter : uint16_t
  +lfsr : uint16_t
  +length_counter : uint8_t
}

class FrameCounter {
  +divider : Divider
  +mode : uint8_t
  +step : uint8_t
}

APU --> Pulse : pulse1/pulse2
APU --> Triangle
APU --> Noise
APU --> FrameCounter


```

```mermaid
classDiagram-v2
direction TB

class Envelope {
  +divider : Divider
  +decay_level : uint8_t
  +volume : uint8_t
  +loop : bool
}

class Sweep {
  +divider : Divider
  +shift : uint8_t
  +negate : bool
  +enabled : bool
}

class Divider {
  +P : uint8_t
  +counter : uint8_t
}

Envelope --> Divider
Sweep --> Divider

```

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
