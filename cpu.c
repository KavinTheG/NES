#include "cpu.h"
#include <stdint.h>

#define MEMORY_SIZE 65536 // 64 KB


uint8_t memory[MEMORY_SIZE] = {0};


// stack memory
//uint8_t stack[0xFF] = {0}; 

// zero page memory {$0000 - $00FF}
uint8_t zero_page[0xFF];

// int to hold status flags
uint8_t status;

// int to hold lower byte of memory address
uint8_t LB;
uint16_t address;


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

void uint8_to_char_array(uint8_t val, char arr[8]) {

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

                    // OR accumulator with X
                    cpu->A |= cpu->X;

                    // Zero and Negative Flag
                    cpu->P[1] = cpu->A == 0;
                    cpu->P[7] = cpu->A >> 7;

                    break;


                // 0x05
                // ORA zpg
                case 0x5:

                    cpu->A |= zero_page[0];
                    
                    // Set the zero flag or negative flag
                    cpu->P[1] = cpu->A == 0;
                    cpu->P[7] = cpu->A >> 7;

                    break;

                // 0x06
                // ASL zpg
                case 0x6:

                    // Set the carry flag to the 7th bit of zero_page[0].
                    cpu->P[0] = (zero_page[0] & 0x80) >> 7;
                    
                    // Arithmetic Shift Left to Zero Page
                    zero_page[0] = zero_page[0] << 1;
                    
                    // Set zero flag and negative flag
                    cpu->P[1] = zero_page[0] == 0;
                    cpu->P[7] = zero_page[0] >> 7;

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
                    break;

                // 0x11
                // ORA ind,Y
                case 0x1:
                    break;

                // 0x15
                // ORA zpg,X
                case 0x5:
                    break;

                // 0x16
                // ASL zpg,X
                case 0x6:
                    break;
                
                // 0x18
                // CLC impl
                case 0x8:
                    break;

                // 0x09
                // ORA abs,Y
                case 0x9:
                    break;

                // 0x0A
                // ASL A
                case 0xA:
                    break;

                // 0x0D
                // ORA abs
                case 0xD:
                    break;

                // 0x0E
                // ASL abs
                case 0xE:
                    break;

            }
            break;

        case 0x2:
            break;

        case 0x3:
            break;

        case 0x4:
            break;

        case 0x5:
            break;

        case 0x6:
            break;

        case 0x7:
            break;

        case 0x8:
            break;

        case 0x9:
            break;

        case 0xA:
            break;

        case 0xB:
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

