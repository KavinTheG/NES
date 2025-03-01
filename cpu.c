#include "cpu.h"
#include <stdint.h>

#define MEMORY_SIZE 65536 // 64 KB


uint8_t memory[MEMORY_SIZE] = {0};


// stack memory
//uint8_t stack[0xFF] = {0};

// int to hold status flags
uint8_t status;

// int to hold lower byte of memory address
uint8_t LB, HB;
uint16_t address, zpg_addr;

char new_carry;

/**  Helper functions **/

inline void push_stack(uint8_t lower_addr, uint8_t val) {
    memory[0x0100 | lower_addr] = val;
}

void join_char_array(uint8_t val, unsigned char arr[8]) {

    val = 0x0;

    for (int i = 0; i < 8; i++) {
        val |= arr[i] << i;
    }
}

void uint8_to_char_array(uint8_t val, unsigned char arr[8]) {

    for (int i = 7; i >= 0; i--) {
        arr[i] = (val & 1); // Extract the least significant bit (LSB)
        val >>= 1;           // Right shift to move the next bit to the LSB
    }
}


void cpu_init(Cpu6502 *cpu) {

    // Initialize stack pointer to the top;
    cpu->S = 0xFF;

    cpu->A = 0x0;
    cpu->X = 0x0;
    cpu->Y = 0x0;
    cpu->PC = 0x0;

    for (int i = 0; i < 8; i++) {
        cpu->P[i] = 0x0;
    }
}


void cpu_execute(Cpu6502 *cpu) {

    // Placeholder for instruction
    uint8_t instr = memory[cpu->PC];

    // Compare the upper 4 bits
    switch ((instr >> 4) & 0x0F) {

        // 0x0X
        case 0x0:

            switch (instr & 0x0F) {
                // 0x00
                // BRK impl
                case 0x0:

                    // Push PC+2 and SR
                    cpu->PC += 2;
                    // Push first half of PC
                    push_stack(cpu->S, cpu->PC >> 8);
                    // Decrement Stack Pointer by 1
                    cpu->S -= 1;

                    // Push second half of PC
                    push_stack(cpu->S, cpu->PC & 0x00FF);
                    cpu->S -= 1;

                    // Set Break flag to 1
                    cpu->P[4] = 1;

                    // Merging Status flags to save it on stack

                    join_char_array(status, cpu->P);

                    push_stack(cpu->S, status);
                    cpu->S -=1;

                    break;

                // 0x01
                // ORA X,ind
                case 0x1:
                    // Increment to get the lower byte
                    cpu->PC++;

                    // Ignore carry if it exists
                    uint8_t zpg_addr = (memory[cpu->PC] + cpu->X);

                    LB = memory[zpg_addr];
                    HB = memory[zpg_addr + 1];

                    // Retrieved address from memory pointed by the operand LB
                    address = HB << 8 | LB;
                    cpu->A |= memory[address];

                    // Zero and Negative Flag
                    cpu->P[1] = cpu->A == 0;
                    cpu->P[7] = cpu->A >> 7;

                    break;


                // 0x05
                // ORA zpg
                case 0x5:

                    cpu->PC++;
                    LB = memory[cpu->PC];

                    // Get the lower byte and put it
                    // in 16 bit address {00LB}
                    address = (uint16_t) LB;
                    cpu->A |= memory[address];

                    // Set the zero flag or negative flag
                    cpu->P[1] = cpu->A == 0;
                    cpu->P[7] = cpu->A >> 7;

                    break;

                // 0x06
                // ASL zpg
                case 0x6:

                    cpu->PC++;
                    LB = memory[cpu->PC];

                    address = (uint16_t) LB;

                    // Set the carry flag to the 7th bit of zpg address.
                    cpu->P[0] = (memory[address] & 0x80) >> 7;

                    // Arithmetic Shift Left to zpg
                    memory[address] = memory[address] << 1;

                    // Set zero flag and negative flag
                    cpu->P[1] = memory[address] == 0;
                    cpu->P[7] = memory[address] >> 7;

                    break;

                // 0x08
                // PHP impl
                case 0x8:

                    // Push Status Flags to stack, with B flag set
                    cpu->P[4] = 1;
                    // Reserve bit set to 1
                    cpu->P[5] = 1;

                    join_char_array(status, cpu->P);
                    push_stack(cpu->S, status);

                    // Decrement stack pointer
                    cpu->S--;

                    break;

                // 0x09
                // ORA #
                case 0x9:

                    // Increment PC to point to imm value
                    cpu->PC += 1;
                    cpu->A |= memory[cpu->PC];

                    // Set zero flag and negative flag
                    cpu->P[1] = cpu->A == 0;
                    cpu->P[7] = cpu->A >> 7;

                    break;

                // 0x0A
                // ASL A
                case 0xA:

                    // Set the carry flag to the 7th bit of Accumulator.
                    cpu->P[0] = (cpu->A & 0x80) >> 7;

                    // Arithmetic Shift Left to Accumulator
                    cpu->A = cpu->A << 1;

                    // Set zero flag and negative flag
                    cpu->P[1] = cpu->A == 0;
                    cpu->P[7] = cpu->A >> 7;

                    break;

                // 0x0D
                // ORA abs
                case 0xD:

                    // Increment to get the lower byte
                    cpu->PC += 1;
                    LB = memory[cpu->PC];

                    // Increment to get the upper byte
                    cpu->PC += 1;
                    address = memory[cpu->PC] << 8 | LB;

                    // OR accumulator with the byte
                    // in memory pointed to by address
                    cpu->A |= memory[address];

                    // Set zero flag and negative flag
                    cpu->P[1] = cpu->A == 0;
                    cpu->P[7] = cpu->A >> 7;

                    break;

                // 0x0E
                // ASL abs
                case 0xE:

                    // Increment to get the lower byte
                    cpu->PC += 1;
                    LB = memory[cpu->PC];

                    // Increment to get the upper byte
                    cpu->PC += 1;
                    address = memory[cpu->PC] << 8 | LB;

                    // Set the carry flag to the 7th bit of memory[address]
                    cpu->P[0] = (memory[address] & 0x80) >> 7;

                    memory[address] = memory[address] << 1;

                    cpu->P[1] = memory[address] == 0;
                    cpu->P[7] = memory[address] >> 7;

                    break;

            }

            break;

        // 0x1X
        case 0x1:
            switch (instr & 0x0F) {
                // 0x10
                // BPL rel
                case 0x0:

                    // Increment PC by to get signed offset
                    cpu->PC += 1;
                    int16_t signed_offset = (int16_t)memory[cpu->PC];

                    //Branch if negative flag is set
                    if (!cpu->P[7]) {
                        cpu->PC += 1 + signed_offset;
                    }
                    break;

                // 0x11
                // ORA ind,Y
                case 0x1:
                    // Increment to get the lower byte
                    cpu->PC++;
                    LB = memory[cpu->PC];

                    // Higher byte is memory[LB]
                    // Lower byte is memory[LB + 1]

                    // Pointer to the address
                    address = (LB << 8 | memory[LB + 1]) +  cpu->Y;

                    cpu->A |= memory[address];

                    // Set zero flag and negative flag
                    cpu->P[1] = cpu->A == 0;
                    cpu->P[7] = cpu->A >> 7;

                    break;

                // 0x15
                // ORA zpg,X
                case 0x5:
                    cpu->PC++;
                    LB = memory[cpu->PC];

                    // Discard carry, zpg should not exceed 0x00FF
                    address = (uint16_t) ((LB + cpu->X) & 0xFF);

                    cpu->A |= memory[address];

                    // Set zero flag and negative flag
                    cpu->P[1] = cpu->A == 0;
                    cpu->P[7] = cpu->A >> 7;
                    break;

                // 0x16
                // ASL zpg,X
                case 0x6:

                    cpu->PC++;
                    LB = memory[cpu->PC];

                    // Discard carry, zpg should not exceed 0x00FF
                    address = (uint16_t) ((LB + cpu->X) & 0xFF);

                    // Set the carry flag to the 7th bit of zpg address.
                    cpu->P[0] = (memory[address] & 0x80) >> 7;

                    // Arithmetic Shift Left to zpg
                    memory[address] = memory[address] << 1;

                    // Set zero flag and negative flag
                    cpu->P[1] = memory[address] == 0;
                    cpu->P[7] = memory[address] >> 7;

                    break;

                // 0x18
                // CLC impl
                case 0x8:

                    // clear carry flag
                    cpu->P[0] = 0;
                    break;

                // 0x19
                // ORA abs,Y
                case 0x9:

                    // Increment to get the lower byte
                    cpu->PC += 1;
                    LB = memory[cpu->PC];

                    // Increment to get the upper byte
                    cpu->PC += 1;
                    address = (memory[cpu->PC] << 8 | LB) + cpu->Y;

                    // OR accumulator with the byte
                    // in memory pointed to by address
                    cpu->A |= memory[address];

                    // Set zero flag and negative flag
                    cpu->P[1] = cpu->A == 0;
                    cpu->P[7] = cpu->A >> 7;
                    break;

                // 0x1D
                // ORA abs,X
                case 0xD:

                    // Increment to get the lower byte
                    cpu->PC += 1;
                    LB = memory[cpu->PC];

                    // Increment to get the upper byte
                    cpu->PC += 1;
                    address = (memory[cpu->PC] << 8 | LB) + cpu->X;

                    // OR accumulator with the byte
                    // in memory pointed to by address
                    cpu->A |= memory[address];

                    // Set zero flag and negative flag
                    cpu->P[1] = cpu->A == 0;
                    cpu->P[7] = cpu->A >> 7;
                    break;

                // 0x1E
                // ASL abs,X
                case 0xE:

                    // Increment to get the lower byte
                    cpu->PC += 1;
                    LB = memory[cpu->PC];

                    // Increment to get the upper byte
                    cpu->PC += 1;
                    address = memory[cpu->PC] << 8 | LB + cpu->X;

                    // Set the carry flag to the 7th bit of memory[address]
                    cpu->P[0] = (memory[address] & 0x80) >> 7;

                    memory[address] = memory[address] << 1;

                    cpu->P[1] = memory[address] == 0;
                    cpu->P[7] = memory[address] >> 7;
                    break;
            }
            break;

        case 0x2:

            switch (instr & 0x0F) {
                // 0x20
                // JSR abs
                case 0x0:

                    // Jump to New Location Saving Return Address
                    // Increment to get the lower byte
                    cpu->PC += 1;
                    LB = memory[cpu->PC];

                    // Increment to get the upper byte
                    cpu->PC += 1;
                    address = memory[cpu->PC] << 8 | LB;

                    // Save current PC value to stack
                    // Note that this PC points at second operand of JSR
                    // RTS will increment PC to get the next instruction
                    // Push Higher Byte first
                    push_stack(cpu->S, memory[cpu->PC]);
                    cpu->S--;

                    push_stack(cpu->S, LB);
                    cpu->S--;

                    cpu->PC = memory[address];

                    break;

                // 0x21
                // AND X,ind
                case 0x1:
                    // Increment to get the lower byte
                    cpu->PC++;

                    // Ignore carry if it exists
                    zpg_addr = (memory[cpu->PC] + cpu->X) & 0xFF;

                    LB = memory[zpg_addr ];
                    HB = memory[zpg_addr + 1];

                    address = HB << 8 | LB;
                    cpu->A &= memory[address];

                    // Set zero flag and negative flag
                    cpu->P[1] = cpu->A == 0;
                    cpu->P[7] = cpu->A >> 7;

                    break;

                // 0x24
                // BIT zpg
                case 0x4:
                    cpu->PC++;
                    LB = memory[cpu->PC];

                    zpg_addr = memory[LB];

                    // Negative flag set to 7 bit of memory
                    cpu->P[7] = memory[zpg_addr] >> 7;

                    // Overflow flag set to 6th bit of memory
                    cpu->P[6] = memory[zpg_addr] >> 6 & 0x1;

                    // Zero flag
                    cpu->P[1] = (cpu->A | memory[zpg_addr]) == 0;
                    break;

                // 0x25
                // AND zpg
                case 0x5:
                    cpu->PC++;
                    LB = memory[cpu->PC];

                    cpu->A &= memory[LB];

                    // Set zero flag and negative flag
                    cpu->P[1] = cpu->A == 0;
                    cpu->P[7] = cpu->A >> 7;
                    break;

                // 0x26
                // ROL zpg
                case 0x6:

                    cpu->PC++;
                    LB = memory[cpu->PC];

                    // Store 7th bit of memory as new carry value
                    new_carry = memory[LB] >> 7;

                    memory[LB] = memory[LB] << 1;

                    // change last bit to value of old carry
                    memory[LB] = (memory[LB] & ~1) | (cpu->P[0] & 1);

                    // Set carry bit to the 7th bit of the old value of memory
                    cpu->P[0] = new_carry;

                    // Set zero flag and negative flag
                    cpu->P[1] = memory[LB] == 0;
                    cpu->P[7] = memory[LB] >> 7;

                    break;

                // 0x28
                // PLP
                case 0x8:
                    // Pull Processor Status from Stack
                    cpu->S++;
                    uint8_to_char_array(memory[0x0100 | cpu->S], cpu->P);


                    break;

                // 0x29
                // AND #
                case 0x9:

                    // Increment to get location of immediate byte
                    cpu->PC += 1;

                    cpu->A &= memory[cpu->PC];

                    // Set zero flag and negative flag
                    cpu->P[1] = cpu->A == 0;
                    cpu->P[7] = cpu->A >> 7;
                    break;

                // 0x2A
                // ROL A
                case 0xA:

                    // Store 7th bit of Accumulator as new carry value
                    new_carry = cpu->A >> 7;

                    cpu->A = cpu->A << 1;

                    // change last bit to value of old carry
                    cpu->A = (cpu->A & ~1) | (cpu->P[0] & 1);

                    // Set carry bit to the 7th bit of the old value of memory
                    cpu->P[0] = new_carry;

                    // Set zero flag and negative flag
                    cpu->P[1] = cpu->A == 0;
                    cpu->P[7] = cpu->A >> 7;

                        break;

                // 0x2C
                // BIT abs
                case 0xC:
                    cpu->PC++;
                    LB = memory[cpu->PC];

                    // Negative flag set to 7 bit of memory
                    cpu->P[7] = memory[LB] >> 7;

                    // Overflow flag set to 6th bit of memory
                    cpu->P[6] = memory[LB] >> 6 & 0x1;

                    // Zero flag
                    cpu->P[1] = (cpu->A | memory[LB]) == 0;
                    break;

                // 0x2D
                // AND abs
                case 0xD:

                    // Increment to get the lower byte
                    cpu->PC += 1;
                    LB = memory[cpu->PC];

                    // Increment to get the upper byte
                    cpu->PC += 1;
                    address = memory[cpu->PC] << 8 | LB;

                    cpu->A &= memory[address];

                    // Set zero flag and negative flag
                    cpu->P[1] = cpu->A == 0;
                    cpu->P[7] = cpu->A >> 7;
                    break;

                // 0x2E
                // ROL abs
                case 0xE:

                    // Increment to get the lower byte
                    cpu->PC += 1;
                    LB = memory[cpu->PC];

                    // Increment to get the upper byte
                    cpu->PC += 1;
                    address = memory[cpu->PC] << 8 | LB;

                    new_carry = memory[address] >> 7;

                    memory[address]  = memory[address] << 1;

                    // change last bit to value of old carry
                    memory[address] = (memory[address] & ~1) | (cpu->P[0] & 1);

                    // Set carry bit to the 7th bit of the old value of memory
                    cpu->P[0] = new_carry;

                    // Set zero flag and negative flag
                    cpu->P[1] = memory[address] == 0;
                    cpu->P[7] = memory[address] >> 7;
                    break;
                }

            break;

        case 0x3:
            switch (instr & 0x0F) {
                // 0x30
                // BMI rel
                case 0x0:

                    // Increment PC by to get signed offset
                    cpu->PC += 1;
                    int16_t signed_offset = (int16_t)memory[cpu->PC];

                    //Branch if negative flag is set
                    if (cpu->P[7]) {
                        cpu->PC += 1 + signed_offset;
                    }

                    break;

                // 0x31
                // AND ind,Y
                case 0x1:
                    // Increment to get the lower byte
                    cpu->PC++;
                    LB = memory[cpu->PC];

                    // Higher byte is memory[LB]
                    // Lower byte is memory[LB + 1]

                    // Pointer to the address
                    address = (LB << 8 | memory[LB + 1]) +  cpu->Y;

                    cpu->A &= memory[memory[address]];

                    // Set zero flag and negative flag
                    cpu->P[1] = cpu->A == 0;
                    cpu->P[7] = cpu->A >> 7;

                    break;


                // 0x35
                // AND zpg,X
                case 0x5:
                    cpu->PC++;
                    LB = memory[cpu->PC];

                    // Discard carry, zpg should not exceed 0x00FF
                    address = (uint16_t) ((LB + cpu->X) & 0xFF);

                    cpu->A &= memory[address];

                    // Set zero flag and negative flag
                    cpu->P[1] = cpu->A == 0;
                    cpu->P[7] = cpu->A >> 7;
                    break;

                // 0x36
                // ROL zpg, X
                case 0x6:

                    cpu->PC++;
                    LB = memory[cpu->PC];

                    address = (LB + cpu->X) &0xFF;

                    // Store 7th bit of memory as new carry value
                    new_carry = memory[address] >> 7;

                    memory[address] = memory[address] << 1;

                    // change last bit to value of old carry
                    memory[address] = (memory[address] & ~1) | (cpu->P[0] & 1);

                    // Set carry bit to the 7th bit of the old value of memory
                    cpu->P[0] = new_carry;

                    // Set zero flag and negative flag
                    cpu->P[1] = memory[address] == 0;
                    cpu->P[7] = memory[address] >> 7;

                    break;

                // 0x38
                // SEC impl
                case 0x8:
                    // Set Carry Flag
                    cpu->P[0] = 1;
                    break;

                // 0x39
                // AND abs,Y
                case 0x9:
                    // Increment to get the lower byte
                    cpu->PC += 1;
                    LB = memory[cpu->PC];

                    // Increment to get the upper byte
                    cpu->PC += 1;
                    address = (memory[cpu->PC] << 8 | LB) + cpu->Y;

                    cpu->A &= memory[address];

                    // Set zero flag and negative flag
                    cpu->P[1] = cpu->A == 0;
                    cpu->P[7] = cpu->A >> 7;
                    break;

                // 0x3D
                // AND abs,X
                case 0xD:

                    // Increment to get the lower byte
                    cpu->PC += 1;
                    LB = memory[cpu->PC];

                    // Increment to get the upper byte
                    cpu->PC += 1;
                    address = (memory[cpu->PC] << 8 | LB) + cpu->X;

                    cpu->A &= memory[address];

                    // Set zero flag and negative flag
                    cpu->P[1] = cpu->A == 0;
                    cpu->P[7] = cpu->A >> 7;
                    break;

                // 0x3E
                // ROL abs,X
                case 0xE:

                    // Increment to get the lower byte
                    cpu->PC += 1;
                    LB = memory[cpu->PC];

                    // Increment to get the upper byte
                    cpu->PC += 1;
                    address = (memory[cpu->PC] << 8 | LB) + cpu->X;

                    new_carry = memory[address] >> 7;

                    memory[address]  = memory[address] << 1;

                    // change last bit to value of old carry
                    memory[address] = (memory[address] & ~1) | (cpu->P[0] & 1);

                    // Set carry bit to the 7th bit of the old value of memory
                    cpu->P[0] = new_carry;

                    // Set zero flag and negative flag
                    cpu->P[1] = memory[address] == 0;
                    cpu->P[7] = memory[address] >> 7;
                    break;
                }
            break;

        case 0x4:

            switch (instr & 0x0F) {
                // 0x40
                // RTI impl
                case 0x0:

                    // Return from Interrupt
                    // Pull S, then pull PC
                    cpu->S++;
                    uint8_to_char_array(memory[0x0100 | cpu->S], cpu->P);

                    // LIFO stack, push LB last so LB comes out first
                    cpu->S++;
                    LB = memory[cpu->S];

                    // HB pushed first so comes out last
                    cpu->S++;
                    // Using address variable, but this is the PC value
                    address = memory[cpu->S] << 8 | LB;

                    cpu->PC = address;

                    break;

                // 0x41
                // EOR X,ind
                case 0x1:
                    // Increment to get the lower byte
                    cpu->PC++;

                    // Ignore carry if it exists
                    uint8_t zpg_addr = (memory[cpu->PC] + cpu->X);

                    LB = memory[zpg_addr];
                    HB = memory[zpg_addr + 1];

                    address = HB << 8 | LB;

                    cpu->A ^= memory[memory[address]];

                    // Set zero flag and negative flag
                    cpu->P[1] = cpu->A == 0;
                    cpu->P[7] = cpu->A >> 7;

                    break;


                // 0x45
                // EOR zpg
                case 0x5:
                    cpu->PC++;

                    // Get zpg addr
                    LB = memory[cpu->PC];

                    cpu->A ^= memory[LB];

                    // Set zero flag and negative flag
                    cpu->P[1] = cpu->A == 0;
                    cpu->P[7] = cpu->A >> 7;
                    break;

                // 0x46
                // LSR zpg
                case 0x6:

                    // Logical Shift Right
                    cpu->PC++;
                    LB = memory[cpu->PC];

                    // Store 7th bit of memory as new carry value
                    cpu->P[0] = memory[LB] & 0x01;

                    memory[LB] = memory[LB] >> 1;


                    // Set zero flag and negative flag
                    cpu->P[1] = memory[LB] == 0;
                    // Inserting 0, so it cannot be negative
                    cpu->P[7] = 0;

                    break;

                // 0x48
                // PHA impl
                case 0x8:
                    // Push accumulator on stack
                    push_stack(cpu->S, cpu->A);
                    cpu->S--;
                    break;

                // 0x49
                // EOR #
                case 0x9:

                    // Increment to get location of immediate byte
                    cpu->PC += 1;

                    cpu->A ^= memory[cpu->PC];

                    // Set zero flag and negative flag
                    cpu->P[1] = cpu->A == 0;
                    cpu->P[7] = cpu->A >> 7;
                    break;

                // 0x4A
                // LSR A
                case 0xA:

                    // Store 7th bit of memory as new carry value
                    cpu->P[0] = cpu->A & 0x01;

                    cpu->A = cpu->A >> 1;

                    // Set zero flag and negative flag
                    cpu->P[1] = cpu->A == 0;
                    cpu->P[7] = 0;

                        break;

                // 0x4C
                // JMP abs
                case 0xC:
                    cpu->PC++;
                    LB = memory[cpu->PC];

                    cpu->PC++;
                    // New opcode to jump to.
                    address = memory[cpu->PC] << 8 | LB;

                    cpu->PC = memory[address];


                    break;

                // 0x4D
                // EOR abs
                case 0xD:

                    // Increment to get the lower byte
                    cpu->PC += 1;
                    LB = memory[cpu->PC];

                    // Increment to get the upper byte
                    cpu->PC += 1;
                    address = memory[cpu->PC] << 8 | LB;

                    cpu->A ^= memory[address];

                    // Set zero flag and negative flag
                    cpu->P[1] = cpu->A == 0;
                    cpu->P[7] = cpu->A >> 7;
                    break;

                // 0x4E
                // LSR abs
                case 0xE:

                    // Increment to get the lower byte
                    cpu->PC += 1;
                    LB = memory[cpu->PC];

                    // Increment to get the upper byte
                    cpu->PC += 1;
                    address = memory[cpu->PC] << 8 | LB;

                    cpu->PC = memory[address] & 0x01;

                    memory[address]  = memory[address] >> 1;


                    // Set zero flag and negative flag
                    cpu->P[1] = memory[address] == 0;
                    cpu->P[7] = 0;
                    break;
                }

            break;

        case 0x5:

            switch (instr & 0x0F) {
                // 0x50
                // BVC rel
                case 0x0:

                    // Increment PC by to get signed offset
                    cpu->PC += 1;
                    int16_t signed_offset = (int16_t)memory[cpu->PC];

                    //Branch if negative flag is set
                    if (!cpu->P[6]) {
                        cpu->PC += 1 + signed_offset;
                    }

                    break;

                // 0x51
                // EOR ind,Y
                case 0x1:
                    // Increment to get the lower byte
                    cpu->PC++;
                    LB = memory[cpu->PC];

                    // Higher byte is memory[LB]
                    // Lower byte is memory[LB + 1]

                    // Pointer to the address
                    address = (LB << 8 | memory[LB + 1]) +  cpu->Y;

                    cpu->A ^= memory[memory[address]];

                    // Set zero flag and negative flag
                    cpu->P[1] = cpu->A == 0;
                    cpu->P[7] = cpu->A >> 7;

                    break;


                // 0x55
                // EOR zpg,X
                case 0x5:
                    cpu->PC++;
                    LB = memory[cpu->PC];

                    // Discard carry, zpg should not exceed 0x00FF
                    address = (uint16_t) ((LB + cpu->X) & 0xFF);

                    cpu->A ^= memory[address];

                    // Set zero flag and negative flag
                    cpu->P[1] = cpu->A == 0;
                    cpu->P[7] = cpu->A >> 7;
                    break;

                // 0x56
                // LSR zpg, X
                case 0x6:

                    cpu->PC++;
                    LB = memory[cpu->PC];

                    address = (LB + cpu->X) & 0xFF;

                    cpu->P[0] = memory[address] & 0x01;
                    memory[address] = memory[address] >> 1;


                    // Set zero flag and negative flag
                    cpu->P[1] = memory[address] == 0;
                    cpu->P[7] = 0;

                    break;

                // 0x58
                // CLI impl
                case 0x8:
                    // Clear Interrupt
                    cpu->P[2] = 0;
                    break;

                // 0x59
                // EOR abs,Y
                case 0x9:
                    // Increment to get the lower byte
                    cpu->PC += 1;
                    LB = memory[cpu->PC];

                    // Increment to get the upper byte
                    cpu->PC += 1;
                    address = (memory[cpu->PC] << 8 | LB) + cpu->Y;

                    cpu->A ^= memory[address];

                    // Set zero flag and negative flag
                    cpu->P[1] = cpu->A == 0;
                    cpu->P[7] = cpu->A >> 7;
                    break;

                // 0x5D
                // EOR abs,X
                case 0xD:

                    // Increment to get the lower byte
                    cpu->PC += 1;
                    LB = memory[cpu->PC];

                    // Increment to get the upper byte
                    cpu->PC += 1;
                    address = (memory[cpu->PC] << 8 | LB) + cpu->X;

                    cpu->A ^= memory[address];

                    // Set zero flag and negative flag
                    cpu->P[1] = cpu->A == 0;
                    cpu->P[7] = cpu->A >> 7;
                    break;

                // 0x5E
                // ROL LSR,X
                case 0xE:

                    // Increment to get the lower byte
                    cpu->PC += 1;
                    LB = memory[cpu->PC];

                    // Increment to get the upper byte
                    cpu->PC += 1;
                    address = (memory[cpu->PC] << 8 | LB) + cpu->X;

                    cpu->P[0] = memory[address] & 0x01;
                    memory[address] = memory[address] >> 1;


                    // Set zero flag and negative flag
                    cpu->P[1] = memory[address] == 0;
                    cpu->P[7] = 0;

                    break;
                }

            break;

        case 0x6:
            switch (instr & 0x0F) {
                // 0x60
                // RTS impl
                case 0x0:

                    // LIFO stack, push LB last so LB comes out first
                    cpu->S++;
                    LB = memory[cpu->S];

                    // HB pushed first so comes out last
                    cpu->S++;
                    // Using address variable, but this is the PC value
                    address = memory[cpu->S] << 8 | LB;

                    cpu->PC = address + 1;

                    break;

                // 0x61
                // ADC X,ind
                case 0x1:
                    // Increment to get the lower byte
                    cpu->PC++;

                    // Ignore carry if it exists
                    zpg_addr = (memory[cpu->PC] + cpu->X) & 0xFF;

                    LB = memory[zpg_addr];
                    HB = memory[zpg_addr + 1];

                    address = HB << 8 | LB;

                    // Reuse zpg variable (in case it overflows)
                    zpg_addr = (uint16_t)(cpu->A + memory[memory[address]] + cpu->P[0]);

                    // Use lower byte for A
                    cpu->A = zpg_addr & 0xFF;

                    // Set carry bit if upper byte is 1
                    cpu->P[0] = zpg_addr >> 7;

                    // Set zero flag and negative flag
                    cpu->P[1] = cpu->A == 0;
                    cpu->P[7] = cpu->A >> 7;

                    break;


                // 0x65
                // ADC zpg
                case 0x5:
                    cpu->PC++;

                    // Get zpg addr
                    LB = memory[cpu->PC];

                    zpg_addr = (uint16_t)(cpu->A + memory[LB] + cpu->P[0]);

                    // Use lower byte for A
                    cpu->A = zpg_addr & 0xFF;

                    // Set carry bit if upper byte is 1
                    cpu->P[0] = zpg_addr >> 7;

                    // Set zero flag and negative flag
                    cpu->P[1] = cpu->A == 0;
                    cpu->P[7] = cpu->A >> 7;
                    break;

                // 0x66
                // ROR zpg
                case 0x6:

                    // Rotate Shift Right
                    cpu->PC++;
                    LB = memory[cpu->PC];

                    // Store 0th bit of memory as new carry value
                    new_carry = memory[LB] & 0x01;
                    memory[LB] = memory[LB] >> 1;

                    // Old carry bit becomes MSB
                    // Set MSB to 1 to carry flag is 1
                    if (cpu->P[0]) {
                        memory[LB] |= 0x80;

                        // MSB is 1 so it is negative
                        cpu->P[7] = 1;
                    } else {
                        // Inserting 0, so it cannot be negative
                        cpu->P[7] = 0;
                    }

                    // Set carry flag to LSB of M
                    cpu->P[0] = new_carry;


                    // Set zero flag and negative flag
                    cpu->P[1] = memory[LB] == 0;

                    break;

                // 0x68
                // PLA impl
                case 0x8:
                    // Pull accumulator from stack
                    cpu->S++;
                    cpu->A = memory[0x100 | cpu->S++];
                    break;

                // 0x69
                // ADC #
                case 0x9:

                    // Increment to get location of immediate byte
                    cpu->PC += 1;

                    zpg_addr = (uint16_t)(cpu->A + memory[cpu->PC] + cpu->P[0]);

                    // Use lower byte for A
                    cpu->A = zpg_addr & 0xFF;

                    // Set carry bit if upper byte is 1
                    cpu->P[0] = zpg_addr >> 7;

                    // Set zero flag and negative flag
                    cpu->P[1] = cpu->A == 0;
                    cpu->P[7] = cpu->A >> 7;
                    break;

                // 0x6A
                // ROR A
                case 0xA:
                    new_carry = cpu->A & 0x01;
                    cpu->A = cpu->A >> 1;

                    // Set MSB to 1 to carry flag is 1
                    if (cpu->P[0]) {
                        cpu->A |= 0x80;

                        // MSB is 1 so it is negative
                        cpu->P[7] = 1;
                    } else {
                        // Inserting 0, so it cannot be negative
                        cpu->P[7] = 0;
                    }

                    // Set carry flag to LSB of M
                    cpu->P[0] = new_carry;

                    // Set zero flag and negative flag
                    cpu->P[1] = cpu->A == 0;



                        break;

                // 0x6C
                // JMP ind
                case 0xC:
                    cpu->PC++;
                    LB = memory[cpu->PC];

                    cpu->PC++;
                    // Address of the location of new address
                    address = memory[cpu->PC] << 8 | LB;

                    cpu->PC = memory[memory[address]];


                    break;

                // 0x6D
                // ADC abs
                case 0xD:

                    // Increment to get the lower byte
                    cpu->PC += 1;
                    LB = memory[cpu->PC];

                    // Increment to get the upper byte
                    cpu->PC += 1;
                    address = memory[cpu->PC] << 8 | LB;

                    // Reuse zpg variable (in case it overflows)
                    zpg_addr = (uint16_t)(cpu->A + memory[address] + cpu->P[0]);

                    // Use lower byte for A
                    cpu->A = zpg_addr & 0xFF;

                    // Set carry bit if upper byte is 1
                    cpu->P[0] = zpg_addr >> 7;

                    // Set zero flag and negative flag
                    cpu->P[1] = cpu->A == 0;
                    cpu->P[7] = cpu->A >> 7;
                    break;

                // 0x6E
                // ROR abs
                case 0xE:

                    // Increment to get the lower byte
                    cpu->PC += 1;
                    LB = memory[cpu->PC];

                    // Increment to get the upper byte
                    cpu->PC += 1;
                    address = memory[cpu->PC] << 8 | LB;

                    new_carry = memory[address] & 0x01;
                    memory[address] = memory[address] >> 1;

                    if (cpu->P[0]) {
                        // Negative (MSB will be 1)
                        memory[address] |= 0x80;
                        cpu->P[7] = 1;
                    } else {
                        cpu->P[7] = 0;
                    }

                    cpu->P[0] = new_carry;

                    // Set zero flag and negative flag
                    cpu->P[1] = memory[address] == 0;
                    break;
                }

            break;

        case 0x7:

            switch (instr & 0x0F) {
                // 0x70
                // BVS rel
                case 0x0:

                    // Increment PC by to get signed offset
                    cpu->PC += 1;
                    int16_t signed_offset = (int16_t)memory[cpu->PC];

                    //Branch if overflow flag is set
                    if (cpu->P[6]) {
                        cpu->PC += 1 + signed_offset;
                    }

                    break;

                // 0x71
                // ADC ind,Y
                case 0x1:
                    // Increment to get the lower byte
                    cpu->PC++;
                    LB = memory[cpu->PC];

                    // Higher byte is memory[LB]
                    // Lower byte is memory[LB + 1]

                    // Pointer to the address
                    address = (LB << 8 | memory[LB + 1]) +  cpu->Y;

                    // Reuse zpg variable (in case it overflows)
                    zpg_addr = (uint16_t)(cpu->A + memory[memory[address]] + cpu->P[0]);

                    // Use lower byte for A
                    cpu->A = zpg_addr & 0xFF;

                    // Set carry bit if upper byte is 1
                    cpu->P[0] = zpg_addr >> 7;

                    // Set zero flag and negative flag
                    cpu->P[1] = cpu->A == 0;
                    cpu->P[7] = cpu->A >> 7;

                    break;


                // 0x75
                // ADC zpg,X
                case 0x5:
                    cpu->PC++;
                    LB = memory[cpu->PC];

                    // Discard carry, zpg should not exceed 0x00FF
                    address = (uint16_t) ((LB + cpu->X) & 0xFF);

                    // Reuse zpg variable (in case it overflows)
                    zpg_addr = (uint16_t)(cpu->A + memory[address] + cpu->P[0]);

                    // Use lower byte for A
                    cpu->A = zpg_addr & 0xFF;

                    // Set carry bit if upper byte is 1
                    cpu->P[0] = zpg_addr >> 7;

                    // Set zero flag and negative flag
                    cpu->P[1] = cpu->A == 0;
                    cpu->P[7] = cpu->A >> 7;
                    break;

                // 0x76
                // ROR zpg, X
                case 0x6:

                    cpu->PC++;
                    LB = memory[cpu->PC];

                    address = (LB + cpu->X) & 0xFF;

                    new_carry = memory[LB] & 0x01;
                    memory[LB] = memory[LB] >> 1;

                    // Old carry bit becomes MSB
                    // Set MSB to 1 to carry flag is 1
                    if (cpu->P[0]) {
                        memory[LB] |= 0x80;

                        // MSB is 1 so it is negative
                        cpu->P[7] = 1;
                    } else {
                        // Inserting 0, so it cannot be negative
                        cpu->P[7] = 0;
                    }

                    // Set carry flag to LSB of M
                    cpu->P[0] = new_carry;


                    // Set zero flag and negative flag
                    cpu->P[1] = memory[LB] == 0;

                    break;

                // 0x78
                // SEI impl
                case 0x8:
                    // Set Interrupt Disable Status
                    cpu->P[2] = 1;
                    break;

                // 0x79
                // ADC abs,Y
                case 0x9:
                    // Increment to get the lower byte
                    cpu->PC += 1;
                    LB = memory[cpu->PC];

                    // Increment to get the upper byte
                    cpu->PC += 1;
                    address = (memory[cpu->PC] << 8 | LB) + cpu->Y;

                    // Reuse zpg variable (in case it overflows)
                    zpg_addr = (uint16_t)(cpu->A + memory[address] + cpu->P[0]);

                    // Use lower byte for A
                    cpu->A = zpg_addr & 0xFF;

                    // Set carry bit if upper byte is 1
                    cpu->P[0] = zpg_addr >> 7;

                    // Set zero flag and negative flag
                    cpu->P[1] = cpu->A == 0;
                    cpu->P[7] = cpu->A >> 7;
                    break;

                // 0x7D
                // ADC abs,X
                case 0xD:

                    // Increment to get the lower byte
                    cpu->PC += 1;
                    LB = memory[cpu->PC];

                    // Increment to get the upper byte
                    cpu->PC += 1;
                    address = (memory[cpu->PC] << 8 | LB) + cpu->X;

                    // Reuse zpg variable (in case it overflows)
                    zpg_addr = (uint16_t)(cpu->A + memory[address] + cpu->P[0]);

                    // Use lower byte for A
                    cpu->A = zpg_addr & 0xFF;

                    // Set carry bit if upper byte is 1
                    cpu->P[0] = zpg_addr >> 7;

                    // Set zero flag and negative flag
                    cpu->P[1] = cpu->A == 0;
                    cpu->P[7] = cpu->A >> 7;
                    break;

                // 0x7E
                // ROR abs,X
                case 0xE:

                    // Increment to get the lower byte
                    cpu->PC += 1;
                    LB = memory[cpu->PC];

                    // Increment to get the upper byte
                    cpu->PC += 1;
                    address = (memory[cpu->PC] << 8 | LB) + cpu->X;

                    new_carry = memory[address] & 0x01;
                    memory[address] = memory[address] >> 1;

                    if (cpu->P[0]) {
                        // Negative (MSB will be 1)
                        memory[address] |= 0x80;
                        cpu->P[7] = 1;
                    } else {
                        cpu->P[7] = 0;
                    }

                    cpu->P[0] = new_carry;

                    // Set zero flag and negative flag
                    cpu->P[1] = memory[address] == 0;

                    break;
                }

            break;

        case 0x8:

            switch (instr & 0x0F) {

                // 0x81
                // STA X,ind
                case 0x1:
                    // Increment to get the lower byte
                    cpu->PC++;

                    // Ignore carry if it exists
                    zpg_addr = (memory[cpu->PC] + cpu->X) & 0xFF;

                    LB = memory[zpg_addr];
                    HB = memory[zpg_addr + 1];

                    address = HB << 8 | LB;

                    memory[memory[address]] = cpu->A;

                    break;

                // 0x84
                // STY zpg
                case 0x4:
                    cpu->PC++;
                    LB = memory[cpu->PC];

                    memory[LB] = cpu->Y;

                    break;

                // 0x85
                // STA zpg
                case 0x5:
                    cpu->PC++;

                    // Get zpg addr
                    LB = memory[cpu->PC];

                    memory[LB] = cpu->A;

                    break;

                // 0x86
                // STX zpg
                case 0x6:

                    // Rotate Shift Right
                    cpu->PC++;
                    LB = memory[cpu->PC];

                    memory[LB] = cpu->X;

                    break;

                // 0x88
                // DEY impl
                case 0x8:
                    // Decrement Index Y by One
                    cpu->Y--;
                    break;

                // 0x8A
                // TXA impl
                case 0xA:
                    // Transfer Index X to Accumulator
                    cpu->A = cpu->X;
                    break;

                // 0x8C
                // STY abs
                case 0xC:
                    cpu->PC++;
                    LB = memory[cpu->PC];

                    cpu->PC++;
                    // Address of the location of new address
                    address = memory[cpu->PC] << 8 | LB;

                    memory[address] = cpu->Y;
                    break;

                // 0x8D
                // STA abs
                case 0xD:

                    // Increment to get the lower byte
                    cpu->PC += 1;
                    LB = memory[cpu->PC];

                    // Increment to get the upper byte
                    cpu->PC += 1;
                    address = memory[cpu->PC] << 8 | LB;

                    memory[address] = cpu->A;

                    break;

                // 0x8E
                // STX abs
                case 0xE:

                    // Increment to get the lower byte
                    cpu->PC += 1;
                    LB = memory[cpu->PC];

                    // Increment to get the upper byte
                    cpu->PC += 1;
                    address = memory[cpu->PC] << 8 | LB;

                    memory[address] = cpu->X;

                    break;
            }

            break;

        case 0x9:
            switch (instr & 0x0F) {
                // 0x90
                // BCC rel
                case 0x0:

                    // Increment PC by to get signed offset
                    cpu->PC += 1;
                    int16_t signed_offset = (int16_t)memory[cpu->PC];

                    //Branch if carry flag is clear
                    if (!cpu->P[0]) {
                        cpu->PC += 1 + signed_offset;
                    }

                    break;

                // 0x91
                // STA ind,Y
                case 0x1:
                    // Increment to get the lower byte
                    cpu->PC++;
                    LB = memory[cpu->PC];

                    // Higher byte is memory[LB]
                    // Lower byte is memory[LB + 1]

                    // Pointer to the address
                    address = (LB << 8 | memory[LB + 1]) +  cpu->Y;

                    memory[memory[address]] =  cpu->A;

                    break;

                // 0x94
                // STY zpg,X
                case 0x4:
                    cpu->PC++;
                    LB = memory[cpu->PC];

                    // Discard carry, zpg should not exceed 0x00FF
                    address = (uint16_t) ((LB + cpu->X) & 0xFF);

                    memory[address] = cpu->Y;
                    break;

                // 0x95
                // STA zpg,X
                case 0x5:
                    cpu->PC++;
                    LB = memory[cpu->PC];

                    // Discard carry, zpg should not exceed 0x00FF
                    address = (uint16_t) ((LB + cpu->X) & 0xFF);

                    memory[address] = cpu->A;

                    break;

                // 0x96
                // STX zpg,Y
                case 0x6:

                    cpu->PC++;
                    LB = memory[cpu->PC];

                    address = (LB + cpu->Y) & 0xFF;

                    memory[address] = cpu->X;

                    break;

                // 0x98
                // TYA impl
                case 0x8:
                    // Transfer Index Y to Accumulator
                    cpu->A = cpu->Y;
                    break;

                // 0x99
                // STA abs,Y
                case 0x9:
                    // Increment to get the lower byte
                    cpu->PC += 1;
                    LB = memory[cpu->PC];

                    // Increment to get the upper byte
                    cpu->PC += 1;
                    address = (memory[cpu->PC] << 8 | LB) + cpu->Y;

                    memory[address] = cpu->A;
                    break;

                // 0x9A
                // TXS impl
                case 0xA:
                    // Transfer Index X to Accumulator
                    cpu->A = cpu->X;
                    break;

                // 0x9D
                // STA abs,X
                case 0xD:

                    // Increment to get the lower byte
                    cpu->PC += 1;
                    LB = memory[cpu->PC];

                    // Increment to get the upper byte
                    cpu->PC += 1;
                    address = (memory[cpu->PC] << 8 | LB) + cpu->X;

                    memory[address] = cpu->X;
                    break;
            }

            break;

        case 0xA:

            switch (instr & 0x0F) {

                // 0xA0
                // LDY #
                case 0x0:
                    // Load Index Y with Memory
                    cpu->PC++;
                    cpu->Y = memory[cpu->PC];
                    break;

                // 0xA1
                // LDA X,ind
                case 0x1:
                    // Increment to get the lower byte
                    cpu->PC++;

                    // Ignore carry if it exists
                    zpg_addr = (memory[cpu->PC] + cpu->X) & 0xFF;

                    LB = memory[zpg_addr];
                    HB = memory[zpg_addr + 1];

                    address = HB << 8 | LB;

                    cpu->A = memory[memory[address]];

                    break;

                // 0xA2
                // LDX #
                case 0x2:
                    // Load Index Y with Memory
                    cpu->PC++;
                    cpu->X = memory[cpu->PC];
                    break;

                // 0xA4
                // LDY zpg
                case 0x4:
                    cpu->PC++;
                    LB = memory[cpu->PC];

                    cpu->Y = memory[LB];

                    break;

                // 0xA5
                // LDA zpg
                case 0x5:
                    cpu->PC++;

                    // Get zpg addr
                    LB = memory[cpu->PC];

                    cpu->A = memory[LB];

                    break;

                // 0xA6
                // LDX zpg
                case 0x6:
                    // Rotate Shift Right
                    cpu->PC++;
                    LB = memory[cpu->PC];

                    cpu->X = memory[LB];

                    break;

                // 0xA8
                // TAY impl
                case 0x8:
                    // Transfer Accumulator to Y
                    cpu->Y = cpu->A;
                    break;

                // 0xAA
                // TAX impl
                case 0xA:
                    // Transfer Accumulator to Y
                    cpu->X = cpu->A;
                    break;

                // 0xAC
                // LDY abs
                case 0xC:
                    cpu->PC++;
                    LB = memory[cpu->PC];

                    cpu->PC++;
                    // Address of the location of new address
                    address = memory[cpu->PC] << 8 | LB;

                    cpu->Y = memory[address];
                    break;

                // 0xAD
                // LDA abs
                case 0xD:

                    // Increment to get the lower byte
                    cpu->PC += 1;
                    LB = memory[cpu->PC];

                    // Increment to get the upper byte
                    cpu->PC += 1;
                    address = memory[cpu->PC] << 8 | LB;

                    cpu->A = memory[address];

                    break;

                // 0xAE
                // LDX abs
                case 0xE:

                    // Increment to get the lower byte
                    cpu->PC += 1;
                    LB = memory[cpu->PC];

                    // Increment to get the upper byte
                    cpu->PC += 1;
                    address = memory[cpu->PC] << 8 | LB;

                    cpu->X = memory[address];

                    break;
            }

            break;

        case 0xB:

            switch (instr & 0x0F) {
                // 0xB0
                // BCS rel
                case 0x0:

                    // Increment PC by to get signed offset
                    cpu->PC += 1;
                    int16_t signed_offset = (int16_t)memory[cpu->PC];

                    //Branch if overflow flag is set
                    if (cpu->P[0]) {
                        cpu->PC += 1 + signed_offset;
                    }

                    break;

                // 0xB1
                // LDA ind,Y
                case 0x1:
                    // Increment to get the lower byte
                    cpu->PC++;
                    LB = memory[cpu->PC];

                    // Higher byte is memory[LB]
                    // Lower byte is memory[LB + 1]

                    // Pointer to the address
                    address = (LB << 8 | memory[LB + 1]) +  cpu->Y;

                    cpu->A = memory[memory[address]];

                    break;

                // 0xB4
                // LDY zpg,X
                case 0x4:
                    cpu->PC++;
                    LB = memory[cpu->PC];

                    // Discard carry, zpg should not exceed 0x00FF
                    address = (uint16_t) ((LB + cpu->X) & 0xFF);

                    cpu->Y = memory[address];
                    break;

                // 0xB5
                // LDA zpg,X
                case 0x5:
                    cpu->PC++;
                    LB = memory[cpu->PC];

                    // Discard carry, zpg should not exceed 0x00FF
                    address = (uint16_t) ((LB + cpu->X) & 0xFF);

                    cpu->A = memory[address];
                    break;

                // 0xB6
                // LDX zpg,Y
                case 0x6:

                    cpu->PC++;
                    LB = memory[cpu->PC];

                    address = (LB + cpu->Y) & 0xFF;

                    cpu->X = memory[address];
                    break;

                // 0xB8
                // CLV impl
                case 0x8:
                    // Clear Overflow Flag
                    cpu->P[6] = 0;
                    break;

                // 0xB9
                // LDA abs,Y
                case 0x9:
                    // Increment to get the lower byte
                    cpu->PC += 1;
                    LB = memory[cpu->PC];

                    // Increment to get the upper byte
                    cpu->PC += 1;
                    address = (memory[cpu->PC] << 8 | LB) + cpu->Y;

                    cpu->A = memory[address];
                    break;

                // 0xBA
                // TSX impl
                case 0xA:
                    // Transfer stack pointer to X
                    join_char_array(cpu->X, cpu->P);
                    break;

                // 0xBC
                // LDY abs,X
                case 0xC:
                    // Increment to get the lower byte
                    cpu->PC += 1;
                    LB = memory[cpu->PC];

                    // Increment to get the upper byte
                    cpu->PC += 1;
                    address = (memory[cpu->PC] << 8 | LB) + cpu->X;

                    cpu->Y = memory[address];
                    break;

                // 0xBD
                // LDA abs,X
                case 0xD:

                    // Increment to get the lower byte
                    cpu->PC += 1;
                    LB = memory[cpu->PC];

                    // Increment to get the upper byte
                    cpu->PC += 1;
                    address = (memory[cpu->PC] << 8 | LB) + cpu->X;

                    cpu->X = memory[address];
                    break;

                // 0xBE
                // LDX abs,Y
                case 0xE:
                    // Increment to get the lower byte
                    cpu->PC += 1;
                    LB = memory[cpu->PC];

                    // Increment to get the upper byte
                    cpu->PC += 1;
                    address = (memory[cpu->PC] << 8 | LB) + cpu->Y;

                    cpu->X = memory[address];
                    break;
            }

            break;

        case 0xC:
            break;

        case 0xD:
            break;

        case 0xE:
            break;

        case 0xF:
            break;

    }

}
