#include "cpu.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include<unistd.h>
#include <sys/stat.h>
#include <cjson/cJSON.h>

#define MEMORY_SIZE 0x10000 // 64 KB
#define NES_HEADER_SIZE 16
#define NES_TEST 0xC000

uint8_t memory[MEMORY_SIZE] = {0};
FILE* log_file;

// stack memory
//uint8_t stack[0xFF] = {0};

// int to hold status flags
uint8_t status;

// int to hold lower byte of memory address
uint8_t LB, HB;
uint16_t address, zpg_addr;

char new_carry;
int cyc = 0;

/**  Helper functions **/

inline void push_stack(uint8_t lower_addr, uint8_t val) {
    memory[0x0100 | lower_addr] = val;
}

void join_char_array(uint8_t *val, unsigned char arr[8]) {

    *val = 0x0;

    for (int i = 0; i < 8; i++) {
        *val |= arr[i] << i;
    }
}

void uint8_to_char_array(uint8_t val, unsigned char arr[8]) {

    for (int i = 0; i < 8; i++) {
        arr[i] = (val >> i) & 0x1;
    }
}

void emulate_6502_cycle(int cycle) {
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 558 * cycle; // Approx. 558 nanoseconds
    nanosleep(&ts, NULL);
    ////sleep(50000);
}

// Core functions

void cpu_init(Cpu6502 *cpu) {

    // Initialize stack pointer to the top;
    cpu->S = 0xFF;

    cpu->A = 0x0;
    cpu->X = 0x0;
    cpu->Y = 0x0;

    // Klaus test
    cpu->PC = 0x0400;

    // Nes-test
    //cpu->PC = 0xC000;

    cpu->instr = memory[cpu->PC];
    cpu->cycles = 0;

    cpu->P[0] = 0;
    cpu->P[1] = 0;
    cpu->P[2] = 1;
    cpu->P[3] = 0;
    cpu->P[4] = 0;
    cpu->P[5] = 1;
    cpu->P[6] = 0;
    cpu->P[7] = 0;

    //memset(memory, 0, sizeof(memory));

    log_file = fopen("log.txt", "w");
    fclose(log_file);
    log_file = fopen("log.txt", "a");

    // push_stack(0100 | cpu->S, 0x70);
    // cpu->S--;

    // push_stack(0100 | cpu->S, 0x00);
    // cpu->S--;
}

void load_test_rom(Cpu6502 *cpu) {

        FILE* ptr;

        // Open file in reading mode
        ptr = fopen("test/6502_functional_test.bin", "rb");

        if (NULL == ptr)
            printf("NULL file.\n");

        struct stat st;

        stat("test/6502_functional_test.bin", &st);

        size_t file_size = st.st_size;

        size_t bytes_read = fread(&memory, 1, sizeof(memory), ptr);

        fclose(ptr);

        if (bytes_read != file_size)
            printf("File not read.\n");

        printf("M[0x20]: %x\n",memory[0x20]);
}

void load_rom(Cpu6502 *cpu, char *filename) {

    FILE *rom = fopen(filename, "rb");
    if (!rom) {
        perror("Error opening ROM file");
        return;
    }

    // Read INES header
    unsigned char header[NES_HEADER_SIZE];
    if (fread(header, 1, NES_HEADER_SIZE, rom) != NES_HEADER_SIZE) {
        perror("Error: header does not match 16 bytes!");
        fclose(rom);
        return;
    }

    int prg_size = header[4] * 16 * 1024;
    int chr_size = header[5] * 8 * 1024;

    printf("PRG ROM size: %d bytes\n", prg_size);
    printf("CHR ROM size: %d bytes\n", chr_size);

    if (prg_size != 16384 && prg_size != 32768) {
        printf("Unsupported PRG ROM size!\n");
        fclose(rom);
        return;
    }

    // Read PRG ROM into a temporary buffer
    unsigned char *prg_rom = malloc(prg_size);
    if (!prg_rom) {
        printf("Memory allocation failed!\n");
        fclose(rom);
        return;
    }

    if (fread(prg_rom, 1, prg_size, rom) != prg_size) {
        perror("Error reading PRG ROM");
        free(prg_rom);
        fclose(rom);
        return;
    }

    fclose(rom);

    // Clear memory
    memset(memory, 0, MEMORY_SIZE);

    // Load PRG ROM into 0x8000 - 0xBFFF
    memcpy(&memory[0x8000], prg_rom, prg_size);

    // If PRG ROM is 16KB, mirror it into $C000-$FFFF (NROM-128 behavior)
    if (prg_size == 16384) {
        memcpy(&memory[0xC000], prg_rom, 16384);
    }

    free (prg_rom);

}



void dump_log_file(Cpu6502 *cpu) {
    fprintf(log_file, "PC: %x ", cpu->PC);
    fprintf(log_file, "instr: %x ", cpu->instr);

    fprintf(log_file, "A: %x ", cpu->A);
    fprintf(log_file, "X: %x ", cpu->X);
    fprintf(log_file, "Y: %x ", cpu->Y);
    join_char_array(&status, cpu->P);
    fprintf(log_file, "SP: %x ", cpu->S);
    fprintf(log_file, "P: %x (%b) ", status, status);
    fprintf(log_file, "Cycle: %d\n", cpu->cycles);
    fprintf(log_file, "{");
    for (int i = 0xf0; i <= 0xff; i++) {
        fprintf(log_file, "M[0x%x]: %x, ", 0x100 | i, memory[0x100 | i]);
    }


    fprintf(log_file, "}\n");



    //fprintf(log_file, "M[0x1fd]: %x", memory[0x1fd]);

    fprintf(log_file, "\n");

}

// access
void instr_LDA(Cpu6502 *cpu, uint8_t val) {
    cpu->A = val;
    cpu->P[1] = cpu->A == 0;
    cpu->P[7] = (cpu->A & 0x80) == 0x80;
}

void instr_LDX(Cpu6502 *cpu, uint8_t val) {
    cpu->X = val;
    cpu->P[1] = cpu->X == 0;
    cpu->P[7] = (cpu->X & 0x80) == 0x80;
}

void instr_LDY(Cpu6502 *cpu, uint8_t val) {
    cpu->Y = val;
    cpu->P[1] = cpu->Y == 0;
    cpu->P[7] = (cpu->Y & 0x80) == 0x80;
}

void instr_STA(Cpu6502 *cpu, uint16_t addr) {
    memory[addr] = cpu->A;
}

void instr_STX(Cpu6502 *cpu, uint16_t addr) {
    memory[addr] = cpu->X;
}

void instr_STY(Cpu6502 *cpu, uint16_t addr) {
    memory[addr] = cpu->Y;
}


// Transfer
void instr_TAX(Cpu6502 *cpu) {
    cpu->X = cpu->A;
}
void instr_TXA(Cpu6502 *cpu) {
    cpu->A = cpu->A;
}
void instr_TAY(Cpu6502 *cpu);
void instr_TYA(Cpu6502 *cpu);





void cpu_execute(Cpu6502 *cpu) {

    // Placeholder for instruction
    uint8_t instr = memory[cpu->PC];

    cpu->instr = instr;
    printf("\nPC Value: %x\n", cpu->PC);
    printf("Instruction: %x\n", instr);
    printf("Stack Pointer: %x\n", cpu->S);
    join_char_array(&status, cpu->P);
    printf("Status: %b\n", status);
    printf("A: %x\n", cpu->A );
    printf("X: %x\n", cpu->X );
    printf("Y: %x\n", cpu->Y );
    printf("Cycle: %d\n\n", cpu->cycles);

    dump_log_file(cpu);
    fprintf(log_file, "M[21a] = %x\n", memory[0x21a]);
    //printf("M[0x1%x] = %x\n\n", cpu->S, memory[0x100 | cpu->S]);



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

                    // Reserve bit must be 1 as wel
                    cpu->P[5] = 1;

                    // Merging Status flags to save it on stack

                    join_char_array(&status, cpu->P);


                    // Set interrupt disable to 1 after pushing to stack
                    cpu->P[2] = 1;

                    push_stack(cpu->S, status);
                    cpu->S -=1;

                    cpu->PC = memory[0xFFFF] << 8 | memory[0xFFFE];

                    printf("\nBRK Instruction\n");
                    //sleep(2);

                    emulate_6502_cycle(7);
                    cpu->cycles += 7;

                    break;

                // 0x01
                // ORA X,ind
                case 0x1:
                    // Increment to get the lower byte
                    cpu->PC++;
                    zpg_addr = (memory[cpu->PC] + cpu->X) ;

                    LB = memory[zpg_addr];
                    HB = memory[zpg_addr + 1];

                    // Retrieved address from memory pointed by the operand LB
                    address = HB << 8 | LB;
                    cpu->A |= memory[address];

                    // Zero and Negative Flag
                    cpu->P[1] = cpu->A == 0;
                    cpu->P[7] = cpu->A >> 7;

                    cpu->PC++;

                    emulate_6502_cycle(6);
                    cpu->cycles += 6;
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

                    cpu->PC++;

                    emulate_6502_cycle(3);
                    cpu->cycles += 3;
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

                    cpu->PC++;

                    emulate_6502_cycle(5);
                    cpu->cycles += 5;
                    break;

                // 0x08
                // PHP impl
                case 0x8:

                    // Push Status Flags to stack, with B flag set
                    cpu->P[4] = 1;
                    // Reserve bit set to 1
                    cpu->P[5] = 1;

                    join_char_array(&status, cpu->P);

                    push_stack(cpu->S, status);

                    printf("PHP impl\n");
                    printf("Status Flag: %b\n", status);
                    printf("Stack Memory: %b\n", memory[0x0100 | cpu->S]);
                    //sleep(1);

                    // Decrement stack pointer
                    cpu->S--;

                    cpu->PC++;
                    emulate_6502_cycle(3);
                    cpu->cycles += 3;
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

                    cpu->PC++;

                    emulate_6502_cycle(2);
                    cpu->cycles += 2;
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

                    cpu->PC++;

                    emulate_6502_cycle(2);
                    cpu->cycles += 2;
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

                    cpu->PC++;

                    emulate_6502_cycle(4);
                    cpu->cycles += 4;
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

                    cpu->PC++;

                    emulate_6502_cycle(6);
                    cpu->cycles += 6;
                    break;

                default:
                    printf("Invalid instruction!");
                    cpu->PC++;
                    sleep(5);
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
                    uint8_t signed_offset = memory[cpu->PC];

                    //Branch if negative flag is set
                    if (!cpu->P[7]) {

                        // Hold old PC value
                        address = cpu->PC;
                        //cpu->PC += 1 + signed_offset;

                        printf("BPL\n");


                        ////sleep(1);

                        if ((signed_offset & 0x80) == 0x80) {
                            // Negative
                            // Convert from 2's complement to regular by negation + 1
                            cpu->PC += 1 - ((~signed_offset & 0xFF) + 1);
                        } else {
                            cpu->PC += 1 + signed_offset;
                        }

                        // Page crossing
                        if ((cpu->PC & 0xFF00) != (address & 0xFF00)) {
                            emulate_6502_cycle(4);
                            cpu->cycles += 4;
                        } else {
                            emulate_6502_cycle(3);
                            cpu->cycles += 3;
                        }

                    } else {
                        cpu->PC++;
                        emulate_6502_cycle(2);
                        cpu->cycles += 2;
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
                    address = (memory[LB + 1] << 8 | memory[LB]) +  cpu->Y;

                    if ((address & 0xFF00) != ((address - cpu->Y) & 0xFF00)) {
                        cyc = 6;
                    } else {
                        cyc = 5;
                    }
                    cpu->A |= memory[address];

                    // Set zero flag and negative flag
                    cpu->P[1] = cpu->A == 0;
                    cpu->P[7] = cpu->A >> 7;

                    cpu->PC++;
                    emulate_6502_cycle(cyc);
                    cpu->cycles += cyc;
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

                    cpu->PC++;
                    emulate_6502_cycle(4);
                    cpu->cycles += 4;
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

                    cpu->PC++;
                    emulate_6502_cycle(6);
                    cpu->cycles += 6;
                    break;

                // 0x18
                // CLC impl
                case 0x8:
                    // clear carry flag
                    cpu->P[0] = 0;

                    cpu->PC++;
                    emulate_6502_cycle(2);
                    cpu->cycles += 2;
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

                    if ((address & 0xFF00) != ((address - cpu->Y) & 0xFF00)) {
                        cyc = 5;
                    } else {
                        cyc = 4;
                    }

                    // OR accumulator with the byte
                    // in memory pointed to by address
                    cpu->A |= memory[address];

                    // Set zero flag and negative flag
                    cpu->P[1] = cpu->A == 0;
                    cpu->P[7] = cpu->A >> 7;

                    cpu->PC++;
                    emulate_6502_cycle(cyc);
                    cpu->cycles += cyc;
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

                    if ((address & 0xFF00) != ((address - cpu->X) & 0xFF00)) {
                        cyc = 5;
                    } else {
                        cyc = 4;
                    }

                    // OR accumulator with the byte
                    // in memory pointed to by address
                    cpu->A |= memory[address];

                    // Set zero flag and negative flag
                    cpu->P[1] = cpu->A == 0;
                    cpu->P[7] = cpu->A >> 7;

                    cpu->PC++;
                    emulate_6502_cycle(cyc);
                    cpu->cycles += cyc;
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

                    cpu->PC++;
                    emulate_6502_cycle(7);
                    cpu->cycles += 7;
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
                    push_stack(cpu->S, cpu->PC >> 8);
                    cpu->S--;

                    push_stack(cpu->S, cpu->PC & 0xFF);
                    cpu->S--;

                    cpu->PC = address;
                    emulate_6502_cycle(6);
                    cpu->cycles += 6;
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

                    cpu->PC++;
                    emulate_6502_cycle(6);
                    cpu->cycles += 6;
                    break;

                // 0x24
                // BIT zpg
                case 0x4:
                    cpu->PC++;

                    printf("BIT zpg");
                    //sleep(1);

                    LB = memory[cpu->PC];

                    //zpg_addr = memory[LB];

                    // Negative flag set to 7 bit of memory
                    cpu->P[7] = memory[LB] >> 7;

                    // Overflow flag set to 6th bit of memory
                    cpu->P[6] = (memory[LB] >> 6) & 0x1;

                    // Zero flag
                    cpu->P[1] = (cpu->A & memory[LB]) == 0;

                    cpu->PC++;
                    emulate_6502_cycle(3);
                    cpu->cycles += 3;
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

                    cpu->PC++;
                    emulate_6502_cycle(3);
                    cpu->cycles += 3;
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

                    cpu->PC++;
                    emulate_6502_cycle(5);
                    cpu->cycles += 5;
                    break;

                // 0x28
                // PLP
                case 0x8:
                    // Pull Processor Status from Stack
                    cpu->S++;

                    uint8_to_char_array(memory[0x0100 | cpu->S], cpu->P);
                    /*  POTENTIAL ISSUE WITH THIS FUNCTION
                        COULD BE REVERSING ORDER
                    */

                    // These bits are ignored, just set to 0
                    // cpu->P[5] = 1;
                    // cpu->P[4] = 0;

                    cpu->PC++;
                    emulate_6502_cycle(4);
                    cpu->cycles += 4;
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

                    cpu->PC++;
                    emulate_6502_cycle(2);
                    cpu->cycles += 2;
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

                    cpu->PC++;
                    emulate_6502_cycle(2);
                    cpu->cycles += 2;
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

                    cpu->PC++;
                    emulate_6502_cycle(4);
                    cpu->cycles += 4;
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

                    cpu->PC++;
                    emulate_6502_cycle(4);
                    cpu->cycles += 4;
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

                    cpu->PC++;
                    emulate_6502_cycle(6);
                    cpu->cycles += 6;
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
                    uint8_t signed_offset = memory[cpu->PC];

                    //Branch if negative flag is set
                    if (cpu->P[7]) {

                        address = cpu->PC;
                        //cpu->PC += 1 + signed_offset;
                        //sleep(1);

                        if ((signed_offset & 0x80) == 0x80) {
                            // Negative
                            // Convert from 2's complement to regular by negation + 1
                            cpu->PC += 1 - ((~signed_offset & 0xFF) + 1);
                        } else {
                            cpu->PC += 1 + signed_offset;
                        }

                        if ((cpu->PC & 0xFF00) != (address & 0xFF00)) {
                            emulate_6502_cycle(4);
                            cpu->cycles += 4;
                        } else {
                            emulate_6502_cycle(3);
                            cpu->cycles += 3;
                        }
                    } else {
                        cpu->PC++;
                        emulate_6502_cycle(2);
                        cpu->cycles += 2;
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
                    address = (memory[LB + 1] << 8 | memory[LB]) +  cpu->Y;
                    if ((address & 0xFF00) != ((address - cpu->Y) & 0xFF00)) {
                        cyc = 6;
                    } else {
                        cyc = 5;
                    }

                    cpu->A &= memory[address];

                    // Set zero flag and negative flag
                    cpu->P[1] = cpu->A == 0;
                    cpu->P[7] = cpu->A >> 7;

                    cpu->PC++;
                    emulate_6502_cycle(cyc);
                    cpu->cycles += cyc;
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

                    cpu->PC++;
                    emulate_6502_cycle(4);
                    cpu->cycles += 4;
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

                    cpu->PC++;
                    emulate_6502_cycle(6);
                    cpu->cycles += 6;
                    break;

                // 0x38
                // SEC impl
                case 0x8:
                    // Set Carry Flag
                    cpu->P[0] = 1;
                    cpu->PC++;
                    emulate_6502_cycle(2);
                    cpu->cycles += 2;
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
                    if ((address & 0xFF00) != ((address - cpu->Y) & 0xFF00)) {
                        cyc = 5;
                    } else {
                        cyc = 4;
                    }

                    cpu->A &= memory[address];

                    // Set zero flag and negative flag
                    cpu->P[1] = cpu->A == 0;
                    cpu->P[7] = cpu->A >> 7;

                    cpu->PC++;
                    emulate_6502_cycle(cyc);
                    cpu->cycles += cyc;
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
                    if ((address & 0xFF00) != ((address - cpu->X) & 0xFF00)) {
                        cyc = 5;
                    } else {
                        cyc = 4;
                    }

                    cpu->A &= memory[address];

                    // Set zero flag and negative flag
                    cpu->P[1] = cpu->A == 0;
                    cpu->P[7] = cpu->A >> 7;

                    cpu->PC++;
                    emulate_6502_cycle(cyc);
                    cpu->cycles += cyc;
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

                    cpu->PC++;
                    emulate_6502_cycle(7);
                    cpu->cycles += 7;
                    break;

                case 0xF:

                    printf("\nInvalid Instruction\n");
                    sleep(5);
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
                    LB = memory[0x100 | cpu->S];

                    // HB pushed first so comes out last
                    cpu->S++;
                    // Using address variable, but this is the PC value
                    address = memory[0x0100 | cpu->S] << 8 | LB;

                    cpu->PC = address;
                    emulate_6502_cycle(6);
                    cpu->cycles += 6;
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

                    cpu->A ^= memory[address];

                    // Set zero flag and negative flag
                    cpu->P[1] = cpu->A == 0;
                    cpu->P[7] = cpu->A >> 7;

                    cpu->PC++;
                    emulate_6502_cycle(6);
                    cpu->cycles += 6;
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

                    cpu->PC++;
                    emulate_6502_cycle(3);
                    cpu->cycles += 3;
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

                    cpu->PC++;
                    emulate_6502_cycle(5);
                    cpu->cycles += 5;
                    break;

                // 0x48
                // PHA impl
                case 0x8:
                    // Push accumulator on stack
                    push_stack(cpu->S, cpu->A);

                    printf("\nPHA impl\n");
                    printf("Stack Mem at index 0x1%x: %b", cpu->S, memory[0x100 | cpu->S]);
                    ////sleep(2);

                    cpu->S--;
                    cpu->PC++;

                    emulate_6502_cycle(3);
                    cpu->cycles += 3;
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

                    cpu->PC++;
                    emulate_6502_cycle(2);
                    cpu->cycles += 2;
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

                    cpu->PC++;
                    emulate_6502_cycle(2);
                    cpu->cycles += 2;
                    break;

                // 0x4C
                // JMP abs
                case 0xC:
                    cpu->PC++;
                    LB = memory[cpu->PC];

                    cpu->PC++;
                    // New opcode to jump to.
                    address = memory[cpu->PC] << 8 | LB;

                    printf("JMP");

                    if (cpu->PC - 2 == address)  {
                        printf("STUCK! TRAP :C");
                        //sleep(1);
                    }

                    cpu->PC = address;

                    //cpu->PC++;
                    emulate_6502_cycle(3);
                    cpu->cycles += 3;
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

                    cpu->PC++;
                    emulate_6502_cycle(4);
                    cpu->cycles += 4;
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

                    cpu->P[0] = memory[address] & 0x01;

                    memory[address]  = memory[address] >> 1;

                    // Set zero flag and negative flag
                    cpu->P[1] = memory[address] == 0;
                    cpu->P[7] = 0;

                    cpu->PC++;
                    emulate_6502_cycle(6);
                    cpu->cycles += 6;
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
                    uint8_t signed_offset = memory[cpu->PC];

                    //Branch if overflow flag is clear
                    if (!cpu->P[6]) {

                        address = cpu->PC;
                        //cpu->PC += 1 + signed_offset;
                        //sleep(1);

                        if ((signed_offset & 0x80) == 0x80) {
                            // Negative
                            // Convert from 2's complement to regular by negation + 1
                            cpu->PC += 1 - ((~signed_offset & 0xFF) + 1);
                        } else {
                            cpu->PC += 1 + signed_offset;
                        }

                        if ((cpu->PC & 0xFF00) != (address & 0xFF00)) {
                            emulate_6502_cycle(4);
                            cpu->cycles += 4;
                        } else {
                            emulate_6502_cycle(3);
                            cpu->cycles += 3;
                        }
                    } else {
                        cpu->PC++;
                        emulate_6502_cycle(2);
                        cpu->cycles += 2;
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
                    address = (memory[LB + 1] << 8 | memory[LB]) +  cpu->Y;
                    if ((address & 0xFF00) != ((address - cpu->Y) & 0xFF00)) {
                        cyc = 6;
                    } else {
                        cyc = 5;
                    }

                    cpu->A ^= memory[address];

                    // Set zero flag and negative flag
                    cpu->P[1] = cpu->A == 0;
                    cpu->P[7] = cpu->A >> 7;

                    cpu->PC++;
                    emulate_6502_cycle(cyc);
                    cpu->cycles += cyc;
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

                    cpu->PC++;
                    emulate_6502_cycle(4);
                    cpu->cycles += 4;
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

                    cpu->PC++;
                    emulate_6502_cycle(6);
                    cpu->cycles += 6;
                    break;

                // 0x58
                // CLI impl
                case 0x8:
                    // Clear Interrupt
                    cpu->P[2] = 0;
                    cpu->PC++;
                    emulate_6502_cycle(2);
                    cpu->cycles += 2;
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
                    if ((address & 0xFF00) != ((address - cpu->Y) & 0xFF00)) {
                        cyc = 5;
                    } else {
                        cyc = 4;
                    }

                    cpu->A ^= memory[address];

                    // Set zero flag and negative flag
                    cpu->P[1] = cpu->A == 0;
                    cpu->P[7] = cpu->A >> 7;

                    cpu->PC++;
                    emulate_6502_cycle(cyc);
                    cpu->cycles += cyc;
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
                    if ((address & 0xFF00) != ((address - cpu->X) & 0xFF00)) {
                        cyc = 5;
                    } else {
                        cyc = 4;
                    }

                    cpu->A ^= memory[address];

                    // Set zero flag and negative flag
                    cpu->P[1] = cpu->A == 0;
                    cpu->P[7] = cpu->A >> 7;

                    cpu->PC++;
                    emulate_6502_cycle(cyc);
                    cpu->cycles += cyc;
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

                    cpu->PC++;
                    emulate_6502_cycle(7);
                    cpu->cycles += 7;
                    break;
                }

            break;

        case 0x6:
            switch (instr & 0x0F) {
                // 0x60
                // RTS impl
                case 0x0:

                    printf("\nRTS\n");


                    // LIFO stack, push LB last so LB comes out first
                    cpu->S++;
                    LB = memory[0x100 | cpu->S];

                    printf("LS Byte: %x\n", LB);

                    // HB pushed first so comes out last
                    cpu->S++;
                    // Using address variable, but this is the PC value
                    address = memory[0x100 | cpu->S] << 8 | LB;

                    printf("MS Byte: %x\n", memory[cpu->S]);
                    printf("Address: %x\n", address);

                    //sleep(2);

                    cpu->PC = address + 1;
                    emulate_6502_cycle(6);
                    cpu->cycles += 6;
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
                    zpg_addr = (uint16_t)(cpu->A + memory[address] + cpu->P[0]);
                    LB = cpu->A;

                    // Use lower byte for A
                    cpu->A = zpg_addr & 0xFF;

                    // Set carry bit if upper byte is 1
                    cpu->P[0] = zpg_addr > 0xFF;

                    // Set zero flag and negative flag
                    cpu->P[1] = cpu->A == 0;
                    cpu->P[7] = cpu->A >> 7;

                    cpu->P[6] = ((LB ^ zpg_addr) & (zpg_addr ^ memory[address]) & 0x80) == 0x80;

                    cpu->PC++;
                    emulate_6502_cycle(6);
                    cpu->cycles += 6;
                    break;


                // 0x65
                // ADC zpg
                case 0x5:
                    cpu->PC++;

                    // Get zpg addr
                    LB = memory[cpu->PC];

                    zpg_addr = (uint16_t)(cpu->A + memory[LB] + cpu->P[0]);
                    LB = cpu->A;

                    // Use lower byte for A
                    cpu->A = zpg_addr & 0xFF;

                    // Set carry bit if upper byte is 1
                    cpu->P[0] = zpg_addr > 0xFF;


                    // Set zero flag and negative flag
                    cpu->P[1] = cpu->A == 0;
                    cpu->P[7] = cpu->A >> 7;

                    cpu->P[6] = ((LB ^ zpg_addr) & (zpg_addr ^ memory[cpu->PC]) & 0x80) == 0x80;

                    cpu->PC++;
                    emulate_6502_cycle(3);
                    cpu->cycles += 3;
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

                    cpu->PC++;
                    emulate_6502_cycle(3);
                    cpu->cycles += 3;
                    break;

                // 0x68
                // PLA impl
                case 0x8:
                    // Pull accumulator from stack
                    cpu->S++;
                    cpu->A = memory[0x100 | cpu->S];
                    cpu->PC++;

                    cpu->P[1] = cpu->A == 0;
                    cpu->P[7] = (cpu->A & 0x80) == 0x80;

                    emulate_6502_cycle(4);
                    cpu->cycles += 4;

                    break;

                // 0x69
                // ADC #
                case 0x9:

                    // Increment to get location of immediate byte
                    cpu->PC++;

                    zpg_addr = (uint16_t)(cpu->A + memory[cpu->PC] + cpu->P[0]);
                    LB = cpu->A;

                    // Use lower byte for A
                    cpu->A = zpg_addr & 0xFF;

                    // Set carry bit if upper byte is 1
                    cpu->P[0] = zpg_addr > 0xFF;

                    // Set zero flag and negative flag
                    cpu->P[1] = cpu->A == 0;
                    cpu->P[7] = cpu->A >> 7;

                    // Overflow flag
                    cpu->P[6] = ((LB ^ zpg_addr) & (zpg_addr ^ memory[cpu->PC]) & 0x80) == 0x80;


                    //printf("\nResult: %x", zpg_addr);
                    //printf("\nCarry Formula: %x\n\n", ((cpu->A ^ zpg_addr) & (zpg_addr ^ memory[cpu->PC]) & 0x80) );

                    ////sleep(3);

                    cpu->PC++;
                    emulate_6502_cycle(2);
                    cpu->cycles += 2;
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

                    cpu->PC++;
                    emulate_6502_cycle(2);
                    cpu->cycles += 2;
                    break;

                // 0x6C
                // JMP ind
                case 0xC:
                    cpu->PC++;
                    LB = memory[cpu->PC];

                    cpu->PC++;
                    // Address of the location of new address
                    address = memory[cpu->PC] << 8 | LB;

                    // If address crosses boundary, bug occurs
                    // For example, address = 0x2ff, this is the lower byte
                    // higher byte should be retrieved from 0x300
                    // instead, 6502 wraps the address around to 0x200


                    if (LB == 0xFF) {
                        cpu->PC = memory[address & 0xF00] << 8 | memory[address];
                        printf("JMP ind with page crossing wrapping!\n");
                        printf("Retrieving higher byte from: %x\n",  (address & 0xF00));
                        printf("PC: %x\n", cpu->PC);
                        sleep(5);
                    } else {
                        cpu->PC = memory[address + 1] << 8 | memory[address];
                    }
                    emulate_6502_cycle(5);
                    cpu->cycles += 5;
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
                    LB = cpu->A;

                    // Use lower byte for A
                    cpu->A = zpg_addr & 0xFF;

                    // Set carry bit if upper byte is 1
                    cpu->P[0] = zpg_addr > 0xFF;

                    // Set zero flag and negative flag
                    cpu->P[1] = cpu->A == 0;
                    cpu->P[7] = cpu->A >> 7;

                    cpu->P[6] = ((LB ^ zpg_addr) & (zpg_addr ^ memory[cpu->PC]) & 0x80) == 0x80;

                    cpu->PC++;
                    emulate_6502_cycle(4);
                    cpu->cycles += 4;
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

                    cpu->PC++;
                    emulate_6502_cycle(6);
                    cpu->cycles += 6;
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
                    uint8_t signed_offset = memory[cpu->PC];

                    //Branch if overflow flag is set
                    if (cpu->P[6]) {

                        address = cpu->PC;
                        //sleep(1);

                        if ((signed_offset & 0x80) == 0x80) {
                            // Negative
                            // Convert from 2's complement to regular by negation + 1
                            cpu->PC += 1 - ((~signed_offset & 0xFF) + 1);
                        } else {
                            cpu->PC += 1 + signed_offset;
                        }

                        if ((cpu->PC & 0xFF00) != (address & 0xFF00)) {
                            emulate_6502_cycle(4);
                            cpu->cycles += 4;
                        } else {
                            emulate_6502_cycle(3);
                            cpu->cycles += 3;
                        }

                    } else {
                        cpu->PC++;
                        emulate_6502_cycle(2);
                        cpu->cycles += 2;
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
                    address = (memory[LB + 1] << 8 | memory[LB]) +  cpu->Y;
                    if ((address & 0xFF00) != ((address - cpu->Y) & 0xFF00)) {
                        cyc = 6;
                    } else {
                        cyc = 5;
                    }

                    // Reuse zpg variable (in case it overflows)
                    zpg_addr = (uint16_t)(cpu->A + memory[address] + cpu->P[0]);
                    LB = cpu->A;

                    // Use lower byte for A
                    cpu->A = zpg_addr & 0xFF;

                    // Set carry bit if upper byte is 1
                    cpu->P[0] = zpg_addr > 0xFF;


                    // Set zero flag and negative flag
                    cpu->P[1] = cpu->A == 0;
                    cpu->P[7] = cpu->A >> 7;

                    cpu->P[6] = ((LB ^ zpg_addr) & (zpg_addr ^ memory[address]) & 0x80) == 0x80;
                    cpu->PC++;
                    emulate_6502_cycle(cyc);
                    cpu->cycles += cyc;
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
                    LB = cpu->A;

                    // Use lower byte for A
                    cpu->A = zpg_addr & 0xFF;

                    // Set carry bit if upper byte is 1
                    cpu->P[0] = zpg_addr > 0xFF;

                    // Set zero flag and negative flag
                    cpu->P[1] = cpu->A == 0;
                    cpu->P[7] = cpu->A >> 7;

                    cpu->P[6] = ((LB ^ zpg_addr) & (zpg_addr ^ memory[cpu->PC]) & 0x80) == 0x80;

                    cpu->PC++;
                    emulate_6502_cycle(4);
                    cpu->cycles += 4;
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

                    cpu->PC++;
                    emulate_6502_cycle(6);
                    cpu->cycles += 6;
                    break;

                // 0x78
                // SEI impl
                case 0x8:
                    // Set Interrupt Disable Status
                    cpu->P[2] = 1;
                    cpu->PC++;
                    emulate_6502_cycle(2);
                    cpu->cycles += 2;
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
                    if ((address & 0xFF00) != ((address - cpu->Y) & 0xFF00)) {
                        cyc = 5;
                    } else {
                        cyc = 4;
                    }

                    // Reuse zpg variable (in case it overflows)
                    zpg_addr = (uint16_t)(cpu->A + memory[address] + cpu->P[0]);
                    LB = cpu->A;

                    // Use lower byte for A
                    cpu->A = zpg_addr & 0xFF;

                    // Set carry bit if upper byte is 1
                    cpu->P[0] = zpg_addr > 0xFF;

                    // Set zero flag and negative flag
                    cpu->P[1] = cpu->A == 0;
                    cpu->P[7] = cpu->A >> 7;

                    cpu->P[6] = ((LB ^ zpg_addr) & (zpg_addr ^ memory[cpu->PC]) & 0x80) == 0x80;

                    cpu->PC++;
                    emulate_6502_cycle(cyc);
                    cpu->cycles += cyc;
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
                    if ((address & 0xFF00) != ((address - cpu->Y) & 0xFF00)) {
                        cyc = 5;
                    } else {
                        cyc = 4;
                    }

                    // Reuse zpg variable (in case it overflows)
                    zpg_addr = (uint16_t)(cpu->A + memory[address] + cpu->P[0]);
                    LB = cpu->A;

                    // Use lower byte for A
                    cpu->A = zpg_addr & 0xFF;

                    // Set carry bit if upper byte is 1
                    cpu->P[0] = zpg_addr > 0xFF;

                    // Set zero flag and negative flag
                    cpu->P[1] = cpu->A == 0;
                    cpu->P[7] = cpu->A >> 7;

                    cpu->P[6] = ((LB ^ zpg_addr) & (zpg_addr ^ memory[cpu->PC]) & 0x80) == 0x80;

                    cpu->PC++;
                    emulate_6502_cycle(cyc);
                    cpu->cycles += cyc;
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
                    cpu->PC++;
                    emulate_6502_cycle(7);
                    cpu->cycles += 7;
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

                    memory[address] = cpu->A;

                    cpu->PC++;
                    emulate_6502_cycle(6);
                    cpu->cycles += 6;
                    break;

                // 0x84
                // STY zpg
                case 0x4:
                    cpu->PC++;
                    LB = memory[cpu->PC];

                    memory[LB] = cpu->Y;

                    cpu->PC++;
                    emulate_6502_cycle(3);
                    cpu->cycles += 3;
                    break;

                // 0x85
                // STA zpg
                case 0x5:
                    cpu->PC++;

                    // Get zpg addr
                    LB = memory[cpu->PC];

                    memory[LB] = cpu->A;

                    cpu->PC++;
                    emulate_6502_cycle(3);
                    cpu->cycles += 3;
                    break;

                // 0x86
                // STX zpg
                case 0x6:

                    // Rotate Shift Right
                    cpu->PC++;
                    LB = memory[cpu->PC];

                    memory[LB] = cpu->X;

                    cpu->PC++;
                    emulate_6502_cycle(3);
                    cpu->cycles += 3;
                    break;

                // 0x88
                // DEY impl
                case 0x8:
                    // Decrement Index Y by One
                    cpu->Y--;
                    cpu->PC++;

                    cpu->P[1] = cpu->Y == 0;
                    cpu->P[7] = (cpu->Y & 0x80) == 0x80;

                    emulate_6502_cycle(2);
                    cpu->cycles += 2;
                    break;

                // 0x8A
                // TXA impl
                case 0xA:
                    // Transfer Index X to Accumulator
                    cpu->A = cpu->X;
                    cpu->PC++;

                    cpu->P[1] = cpu->X == 0;
                    cpu->P[7] = (cpu->X & 0x80) == 0x80;

                    emulate_6502_cycle(2);
                    cpu->cycles += 2;
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
                    cpu->PC++;
                    emulate_6502_cycle(4);
                    cpu->cycles += 4;
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

                    cpu->PC++;

                    emulate_6502_cycle(4);
                    cpu->cycles += 4;
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

                    cpu->PC++;
                    emulate_6502_cycle(4);
                    cpu->cycles += 4;
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
                    uint8_t signed_offset = memory[cpu->PC];

                    //Branch if carry flag is clear
                    if (!cpu->P[0]) {
                        address = cpu->PC;

                        //sleep(1);

                        if ((signed_offset & 0x80) == 0x80) {
                            // Negative
                            // Convert from 2's complement to regular by negation + 1
                            cpu->PC += 1 - ((~signed_offset & 0xFF) + 1);
                        } else {
                            cpu->PC += 1 + signed_offset;
                        }

                        if ((cpu->PC & 0xFF00) != (address & 0xFF00)) {
                            emulate_6502_cycle(4);
                            cpu->cycles += 4;
                        } else {
                            emulate_6502_cycle(3);
                            cpu->cycles += 3;
                        }

                    } else {
                        cpu->PC++;
                        emulate_6502_cycle(2);
                        cpu->cycles += 2;
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
                    address = (memory[LB + 1] << 8 | memory[LB]) +  cpu->Y;

                    memory[address] =  cpu->A;

                    cpu->PC++;
                    emulate_6502_cycle(6);
                    cpu->cycles += 6;
                    break;

                // 0x94
                // STY zpg,X
                case 0x4:
                    cpu->PC++;
                    LB = memory[cpu->PC];

                    // Discard carry, zpg should not exceed 0x00FF
                    address = (uint16_t) ((LB + cpu->X) & 0xFF);

                    memory[address] = cpu->Y;
                    cpu->PC++;
                    emulate_6502_cycle(4);
                    cpu->cycles += 4;
                    break;

                // 0x95
                // STA zpg,X
                case 0x5:
                    cpu->PC++;
                    LB = memory[cpu->PC];

                    // Discard carry, zpg should not exceed 0x00FF
                    address = (uint16_t) ((LB + cpu->X) & 0xFF);

                    memory[address] = cpu->A;
                    cpu->PC++;
                    emulate_6502_cycle(4);
                    cpu->cycles += 4;
                    break;

                // 0x96
                // STX zpg,Y
                case 0x6:

                    cpu->PC++;
                    LB = memory[cpu->PC];

                    address = (LB + cpu->Y) & 0xFF;

                    memory[address] = cpu->X;
                    cpu->PC++;
                    emulate_6502_cycle(4);
                    cpu->cycles += 4;
                    break;

                // 0x98
                // TYA impl
                case 0x8:
                    // Transfer Index Y to Accumulator
                    cpu->A = cpu->Y;
                    cpu->PC++;

                    cpu->P[1] = cpu->A == 0;
                    cpu->P[7] = (cpu->A & 0x80) == 0x80;

                    emulate_6502_cycle(2);
                    cpu->cycles += 2;
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
                    cpu->PC++;

                    emulate_6502_cycle(5);
                    cpu->cycles += 5;
                    break;

                // 0x9A
                // TXS impl
                case 0xA:
                    // Copies X register value to the stack pointer
                    cpu->S = cpu->X;
                    cpu->PC++;
                    emulate_6502_cycle(2);
                    cpu->cycles += 2;
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

                    memory[address] = cpu->A;
                    cpu->PC++;

                    emulate_6502_cycle(5);
                    cpu->cycles += 5;
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
                    cpu->PC++;

                    cpu->P[1] = cpu->Y == 0;
                    cpu->P[7] = (cpu->Y & 0x80) == 0x80;

                    printf("Y after LDY: %x\n", cpu->Y);
                    printf("Zero flag: %x\n\n", cpu->P[1]);

                    //sleep(1);
                    emulate_6502_cycle(2);
                    cpu->cycles += 2;
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

                    cpu->A = memory[address];
                    cpu->PC++;

                    cpu->P[1] = cpu->A == 0;
                    cpu->P[7] = (cpu->A & 0x80) == 0x80;


                    emulate_6502_cycle(6);
                    cpu->cycles += 6;
                    break;

                // 0xA2
                // LDX #
                case 0x2:
                    // Load Index X with Memory
                    cpu->PC++;
                    cpu->X = memory[cpu->PC];
                    cpu->PC++;

                    cpu->P[1] = cpu->X == 0;
                    cpu->P[7] = (cpu->X & 0x80) == 0x80;

                    emulate_6502_cycle(2);
                    cpu->cycles += 2;
                    break;

                // 0xA4
                // LDY zpg
                case 0x4:
                    cpu->PC++;
                    LB = memory[cpu->PC];

                    cpu->Y = memory[LB];
                    cpu->PC++;

                    cpu->P[1] = cpu->Y == 0;
                    cpu->P[7] = (cpu->Y & 0x80) == 0x80;

                    emulate_6502_cycle(2);
                    cpu->cycles += 3;
                    break;

                // 0xA5
                // LDA zpg
                case 0x5:
                    cpu->PC++;

                    // Get zpg addr
                    LB = memory[cpu->PC];

                    cpu->A = memory[LB];
                    cpu->PC++;

                    cpu->P[1] = cpu->A == 0;
                    cpu->P[7] = (cpu->A & 0x80) == 0x80;

                    emulate_6502_cycle(3);
                    cpu->cycles += 3;
                    break;

                // 0xA6
                // LDX zpg
                case 0x6:
                    // Rotate Shift Right
                    cpu->PC++;
                    LB = memory[cpu->PC];

                    cpu->X = memory[LB];
                    cpu->PC++;

                    cpu->P[1] = cpu->X == 0;
                    cpu->P[7] = (cpu->X & 0x80) == 0x80;

                    emulate_6502_cycle(3);
                    cpu->cycles += 3;
                    break;

                // 0xA8
                // TAY impl
                case 0x8:
                    // Transfer Accumulator to Y
                    cpu->Y = cpu->A;
                    cpu->PC++;

                    cpu->P[1] = cpu->Y == 0;
                    cpu->P[7] = (cpu->Y & 0x80) == 0x80;

                    emulate_6502_cycle(2);
                    cpu->cycles += 2;
                    break;

                // 0xA9
                // LDA #
                case 0x9:
                    cpu->PC++;

                    LB = memory[cpu->PC];
                    cpu->A = LB;

                    cpu->PC++;

                    cpu->P[1] = cpu->A == 0;
                    cpu->P[7] = (cpu->A & 0x80) == 0x80;

                    emulate_6502_cycle(2);
                    cpu->cycles += 2;
                    break;

                // 0xAA
                // TAX impl
                case 0xA:
                    // Transfer Accumulator to X
                    cpu->X = cpu->A;
                    cpu->PC++;

                    cpu->P[1] = cpu->X == 0;
                    cpu->P[7] = (cpu->X & 0x80) == 0x80;

                    emulate_6502_cycle(2);
                    cpu->cycles += 2;
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
                    cpu->PC++;

                    cpu->P[1] = cpu->Y == 0;
                    cpu->P[7] = (cpu->Y & 0x80) == 0x80;

                    emulate_6502_cycle(4);
                    cpu->cycles += 4;
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
                    cpu->PC++;

                    printf("LDA abs\n");
                    printf("Address: %x\n", address);
                    printf("Mem: %x\n\n", memory[address]);

                    cpu->P[1] = cpu->A == 0;
                    cpu->P[7] = (cpu->A & 0x80) == 0x80;

                    emulate_6502_cycle(4);
                    cpu->cycles += 4;
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
                    cpu->PC++;

                    cpu->P[1] = cpu->X == 0;
                    cpu->P[7] = (cpu->X & 0x80) == 0x80;

                    emulate_6502_cycle(4);
                    cpu->cycles += 4;
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
                    uint8_t signed_offset = memory[cpu->PC];

                    //Branch if overflow flag is set
                    if (cpu->P[0]) {

                        address = cpu->PC;
                        //sleep(1);

                        if ((signed_offset & 0x80) == 0x80) {
                            // Negative
                            // Convert from 2's complement to regular by negation + 1
                            cpu->PC += 1 - ((~signed_offset & 0xFF) + 1);
                        } else {
                            cpu->PC += 1 + signed_offset;
                        }

                        if ((cpu->PC & 0xFF00) != (address & 0xFF00)) {
                            emulate_6502_cycle(4);
                            cpu->cycles += 4;
                        } else {
                            emulate_6502_cycle(3);
                            cpu->cycles += 3;
                        }
                    } else {
                        cpu->PC++;
                        emulate_6502_cycle(2);
                        cpu->cycles += 2;
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
                    address = (memory[LB + 1] << 8 | memory[LB] ) +  cpu->Y;
                    if ((address & 0xFF00) != ((address - cpu->Y) & 0xFF00)) {
                        cyc = 6;
                    } else {
                        cyc = 5;
                    }

                    cpu->A = memory[address];
                    cpu->PC++;

                    cpu->P[1] = cpu->A == 0;
                    cpu->P[7] = (cpu->A & 0x80) == 0x80;

                    emulate_6502_cycle(cyc);
                    cpu->cycles += cyc;
                    break;

                // 0xB4
                // LDY zpg,X
                case 0x4:
                    cpu->PC++;
                    LB = memory[cpu->PC];

                    // Discard carry, zpg should not exceed 0x00FF
                    address = (uint16_t) ((LB + cpu->X) & 0xFF);

                    cpu->Y = memory[address];
                    cpu->PC++;

                    cpu->P[1] = cpu->Y == 0;
                    cpu->P[7] = (cpu->Y & 0x80) == 0x80;

                    emulate_6502_cycle(4);
                    cpu->cycles += 4;
                    break;

                // 0xB5
                // LDA zpg,X
                case 0x5:
                    cpu->PC++;
                    LB = memory[cpu->PC];

                    // Discard carry, zpg should not exceed 0x00FF
                    address = (uint16_t) ((LB + cpu->X) & 0xFF);

                    cpu->A = memory[address];
                    cpu->PC++;

                    cpu->P[1] = cpu->A == 0;
                    cpu->P[7] = (cpu->A & 0x80) == 0x80;

                    emulate_6502_cycle(4);
                    cpu->cycles += 4;
                    break;

                // 0xB6
                // LDX zpg,Y
                case 0x6:

                    cpu->PC++;
                    LB = memory[cpu->PC];

                    address = (LB + cpu->Y) & 0xFF;

                    cpu->X = memory[address];
                    cpu->PC++;

                    cpu->P[1] = cpu->X == 0;
                    cpu->P[7] = (cpu->X & 0x80) == 0x80;

                    emulate_6502_cycle(4);
                    cpu->cycles += 4;
                    break;

                // 0xB8
                // CLV impl
                case 0x8:
                    // Clear Overflow Flag
                    cpu->P[6] = 0;
                    cpu->PC++;

                    emulate_6502_cycle(2);
                    cpu->cycles += 2;
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
                    if ((address & 0xFF00) != ((address - cpu->Y) & 0xFF00)) {
                        cyc = 5;
                    } else {
                        cyc = 4;
                    }

                    cpu->A = memory[address];
                    cpu->PC++;

                    cpu->P[1] = cpu->A == 0;
                    cpu->P[7] = (cpu->A & 0x80) == 0x80;


                    emulate_6502_cycle(cyc);
                    cpu->cycles += cyc;
                    break;

                // 0xBA
                // TSX impl
                case 0xA:
                    // Transfer stack pointer to X
                    //join_char_array(&cpu->X, cpu->P);
                    cpu->X = cpu->S;

                    cpu->PC++;

                    cpu->P[1] = cpu->X == 0;
                    cpu->P[7] = (cpu->X & 0x80) == 0x80;

                    emulate_6502_cycle(2);
                    cpu->cycles += 2;
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
                    if ((address & 0xFF00) != ((address - cpu->X) & 0xFF00)) {
                        cyc = 5;
                    } else {
                        cyc = 4;
                    }

                    cpu->Y = memory[address];
                    cpu->PC++;

                    cpu->P[1] = cpu->Y == 0;
                    cpu->P[7] = (cpu->Y & 0x80) == 0x80;

                    emulate_6502_cycle(cyc);
                    cpu->cycles += cyc;
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

                    // if (cpu->PC-2 == 0xfe9 ) {
                    //     printf("Getting wrong LDA memory here!\n");
                    //     printf("address operand: %x\n", (memory[cpu->PC] << 8 | LB) );
                    //     printf("incremented by X: %x\n", address);
                    //     printf("Value at M: %x\n", memory[address]);
                    //         fflush(stdout);  // Force immediate printing
                    //     sleep(5);
                    // }

                    if ((address & 0xFF00) != ((address - cpu->X) & 0xFF00)) {
                        cyc = 5;
                    } else {
                        cyc = 4;
                    }

                    cpu->A = memory[address];
                    cpu->PC++;

                    cpu->P[1] = cpu->A == 0;
                    cpu->P[7] = (cpu->A & 0x80) == 0x80;


                    emulate_6502_cycle(cyc);
                    cpu->cycles += cyc;
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
                    if ((address & 0xFF00) != ((address - cpu->X) & 0xFF00)) {
                        cyc = 5;
                    } else {
                        cyc = 4;
                    }

                    cpu->X = memory[address];
                    cpu->PC++;

                    cpu->P[1] = cpu->X == 0;
                    cpu->P[7] = (cpu->X & 0x80) == 0x80;

                    emulate_6502_cycle(cyc);
                    cpu->cycles += cyc;
                    break;

            }

            break;

        case 0xC:

            switch (instr & 0x0F) {

                // 0xC0
                // CPY #
                case 0x0:
                    // Compare Memory and Index Y
                    cpu->PC++;

                    // Carry Flag
                    cpu->P[0] = cpu->Y >= memory[cpu->PC];

                    // Zero flag
                    cpu->P[1] = cpu->Y == memory[cpu->PC];

                    // Negative Flag
                    cpu->P[7] = ((cpu->Y - memory[cpu->PC]) & 0x80) == 0x80;

                    cpu->PC++;

                    emulate_6502_cycle(2);
                    cpu->cycles += 2;
                    break;

                // 0xC1
                // CMP X,ind
                case 0x1:
                    // Increment to get the lower byte
                    cpu->PC++;

                    // Ignore carry if it exists
                    zpg_addr = (memory[cpu->PC] + cpu->X) & 0xFF;

                    LB = memory[zpg_addr];
                    HB = memory[zpg_addr + 1];

                    address = HB << 8 | LB;

                    // Carry Flag
                    cpu->P[0] = cpu->A >= memory[memory[cpu->PC]];

                    // Zero flag
                    cpu->P[1] = cpu->A == memory[memory[cpu->PC]];

                    // Negative Flag
                    cpu->P[7] = ((cpu->A - memory[memory[cpu->PC]]) & 0x80) == 0x80;

                    cpu->PC++;

                    emulate_6502_cycle(6);
                    cpu->cycles += 6;
                    break;

                // 0xC4
                // CPY zpg
                case 0x4:
                    cpu->PC++;
                    LB = memory[cpu->PC];

                    // Carry Flag
                    cpu->P[0] = cpu->Y >= memory[LB];

                    // Zero flag
                    cpu->P[1] = cpu->Y == memory[LB];

                    // Negative Flag
                    cpu->P[7] = ((cpu->Y - memory[LB]) & 0x80) == 0x80;

                    cpu->PC++;

                    emulate_6502_cycle(3);
                    cpu->cycles += 3;
                    break;

                // 0xC5
                // CMP zpg
                case 0x5:
                    cpu->PC++;

                    // Get zpg addr
                    LB = memory[cpu->PC];

                    // Carry Flag
                    cpu->P[0] = cpu->A >= memory[LB];

                    // Zero flag
                    cpu->P[1] = cpu->A == memory[LB];

                    // Negative Flag
                    cpu->P[7] = ((cpu->A - memory[LB]) & 0x80) == 0x80;

                    cpu->PC++;

                    emulate_6502_cycle(3);
                    cpu->cycles += 3;
                    break;

                // 0xC6
                // DEC zpg
                case 0x6:
                    // Decrement M by One
                    cpu->PC++;
                    LB = memory[cpu->PC];

                    memory[LB]--;
                    printf("\nDEC ZPG at D460, M[LB] = %x", memory[LB]);
                    printf("\n%b", memory[LB] == 0);



                    cpu->PC++;

                    cpu->P[1] = memory[LB] == 0;
                    cpu->P[7] = (memory[LB] & 0x80) == 0x80;

                    printf("%b", cpu->P[1]);

                    if (cpu->PC - 2 == 0xD4C0) {
                        printf("\nDEC ZPG at D460, M[LB] = %x", memory[LB]);

                        sleep(5);
                    }

                    emulate_6502_cycle(5);
                    cpu->cycles += 5;
                    break;

                // 0xC8
                // INY impl
                case 0x8:
                    // Increment Y register by One
                    cpu->Y++;

                    cpu->PC++;

                    cpu->P[1] = cpu->Y == 0;
                    cpu->P[7] = (cpu->Y & 0x80) == 0x80;

                    emulate_6502_cycle(2);
                    cpu->cycles += 2;
                    break;

                // 0xC9
                // CMP #
                case 0x9:
                    // Compare accumulator with M
                    cpu->PC++;

                    // Carry Flag
                    cpu->P[0] = cpu->A >= memory[cpu->PC];

                    // Zero flag
                    cpu->P[1] = cpu->A == memory[cpu->PC];

                    // Negative Flag
                    cpu->P[7] = ((cpu->A - memory[cpu->PC]) & 0x80 )== 0x80;

                    cpu->PC++;

                    emulate_6502_cycle(2);
                    cpu->cycles += 2;
                    break;

                // 0xCA
                // DEX impl
                case 0xA:
                    // Decrement Index by One
                    cpu->X--;
                    cpu->PC++;

                    cpu->P[1] = cpu->X == 0;
                    cpu->P[7] = (cpu->X & 0x80) == 0x80;

                    emulate_6502_cycle(2);
                    cpu->cycles += 2;
                    break;

                // 0xCC
                // CPY abs
                case 0xC:
                    cpu->PC++;
                    LB = memory[cpu->PC];

                    cpu->PC++;
                    // Address of the location of new address
                    address = memory[cpu->PC] << 8 | LB;

                    // Carry Flag
                    cpu->P[0] = cpu->Y >= memory[address];

                    // Zero flag
                    cpu->P[1] = cpu->Y == memory[address];

                    // Negative Flag
                    cpu->P[7] = ((cpu->Y - memory[address]) & 0x80) == 0x80;

                    cpu->PC++;

                    emulate_6502_cycle(4);
                    cpu->cycles += 4;
                    break;

                // 0xCD
                // CMP abs
                case 0xD:
                    // Increment to get the lower byte
                    cpu->PC += 1;
                    LB = memory[cpu->PC];

                    // Increment to get the upper byte
                    cpu->PC += 1;
                    address = memory[cpu->PC] << 8 | LB;

                    // Carry Flag
                    cpu->P[0] = cpu->A >= memory[address];

                    // Zero flag
                    cpu->P[1] = cpu->A == memory[address];

                    // Negative Flag
                    cpu->P[7] = ((cpu->A - memory[address]) & 0x80) == 0x80;

                    cpu->PC++;

                    emulate_6502_cycle(4);
                    cpu->cycles += 4;
                    break;

                // 0xCE
                // DEC abs
                case 0xE:

                    // Increment to get the lower byte
                    cpu->PC += 1;
                    LB = memory[cpu->PC];

                    // Increment to get the upper byte
                    cpu->PC += 1;
                    address = memory[cpu->PC] << 8 | LB;

                    memory[address]--;

                    cpu->PC++;

                    emulate_6502_cycle(6);
                    cpu->cycles += 6;
                    break;

            }
            break;

        case 0xD:

            switch (instr & 0x0F) {

                // 0xD0
                // BNE rel
                case 0x0:
                    // Increment PC by to get signed offset
                    cpu->PC += 1;
                    uint8_t signed_offset = memory[cpu->PC];

                    //Branch if zero flag is set
                    if (!cpu->P[1]) {

                        address = cpu->PC;
                        printf("\nBNE\n");
                        printf("%x\n", signed_offset);
                        printf("%x\n", (signed_offset & 0x80));

                        ////sleep(1);

                        if ((signed_offset & 0x80) == 0x80) {
                            // Negative
                            // Convert from 2's complement to regular by negation + 1
                            cpu->PC += 1 - ((~signed_offset & 0xFF) + 1);
                        } else {
                            cpu->PC += 1 + signed_offset;
                        }


                        if ((cpu->PC & 0xFF00) != (address & 0xFF00)) {
                            emulate_6502_cycle(4);
                            cpu->cycles += 4;
                        } else {
                            emulate_6502_cycle(3);
                            cpu->cycles += 3;
                        }

                    } else {
                        cpu->PC++;
                        emulate_6502_cycle(2);
                        cpu->cycles += 2;
                    }
                    break;

                    // 0xD1
                    // CMP ind,Y
                    case 0x1:
                        // Increment to get the lower byte
                        cpu->PC++;
                        LB = memory[cpu->PC];

                        // Higher byte is memory[LB]
                        // Lower byte is memory[LB + 1]

                        // Pointer to the address
                        address = (memory[LB + 1] << 8 | memory[LB]) +  cpu->Y;
                        if ((address & 0xFF00) != ((address - cpu->Y) & 0xFF00)) {
                            cyc = 5;
                        } else {
                            cyc = 4;
                        }

                        // Carry Flag
                        cpu->P[0] = cpu->A >= memory[cpu->PC];

                        // Zero flag
                        cpu->P[1] = cpu->A == memory[cpu->PC];;

                        // Negative Flag
                        cpu->P[7] = ((cpu->A - memory[cpu->PC]) & 0x80) == 0x80;

                        cpu->PC++;

                        emulate_6502_cycle(cyc);
                        cpu->cycles += cyc;
                        break;

                    // 0xD5
                    // CMP zpg,X
                    case 0x5:
                        cpu->PC++;
                        LB = memory[cpu->PC];

                        // Discard carry, zpg should not exceed 0x00FF
                        address = (uint16_t) ((LB + cpu->X) & 0xFF);

                        // Carry Flag
                        cpu->P[0] = cpu->A >= memory[address];

                        // Zero flag
                        cpu->P[1] = cpu->A == memory[address];

                        // Negative Flag
                        cpu->P[7] = ((cpu->A - memory[address]) & 0x80) == 0x80;

                        cpu->PC++;

                        emulate_6502_cycle(4);
                        cpu->cycles += 4;
                        break;

                    // 0xD6
                    // DEC zpg,X
                    case 0x6:
                        // Decrement M by One
                        cpu->PC++;
                        LB = memory[cpu->PC];

                        // Discard carry, zpg should not exceed 0x00FF
                        address = (uint16_t) ((LB + cpu->X) & 0xFF);

                        memory[address]--;

                        cpu->PC++;

                        emulate_6502_cycle(6);
                        cpu->cycles += 6;
                        break;

                    // 0xD8
                    // CLD impl
                    case 0x8:
                        // Clear Decimal Flag
                        cpu->P[3] = 0;
                        cpu->PC++;

                        emulate_6502_cycle(2);
                        cpu->cycles += 2;
                        break;

                    // 0xD9
                    // CMP abs,Y
                    case 0x9:
                        // Compare accumulator with M
                        // Increment to get the lower byte
                        cpu->PC += 1;
                        LB = memory[cpu->PC];

                        // Increment to get the upper byte
                        cpu->PC += 1;
                        address = (memory[cpu->PC] << 8 | LB) + cpu->Y;
                        if ((address & 0xFF00) != ((address - cpu->Y) & 0xFF00)) {
                            cyc = 5;
                        } else {
                            cyc = 4;
                        }

                        // Carry Flag
                        cpu->P[0] = cpu->A >= memory[address];

                        // Zero flag
                        cpu->P[1] = cpu->A == memory[address];

                        // Negative Flag
                        cpu->P[7] = ((cpu->A - memory[address]) & 0x80) == 0x80;

                        cpu->PC++;

                        emulate_6502_cycle(cyc);
                        cpu->cycles += cyc;
                        break;

                    // 0xDD
                    // CMP abs,X
                    case 0xD:
                        // Increment to get the lower byte
                        cpu->PC += 1;
                        LB = memory[cpu->PC];

                        // Increment to get the upper byte
                        cpu->PC += 1;
                        address = (memory[cpu->PC] << 8 | LB) + cpu->X;
                        if ((address & 0xFF00) != ((address - cpu->Y) & 0xFF00)) {
                            cyc = 5;
                        } else {
                            cyc = 4;
                        }

                        // Carry Flag
                        cpu->P[0] = cpu->A >= memory[address];

                        // Zero flag
                        cpu->P[1] = cpu->A == memory[address];

                        // Negative Flag
                        cpu->P[7] = ((cpu->A - memory[address]) & 0x80) == 0x80;

                        cpu->PC++;
                        emulate_6502_cycle(cyc);
                        cpu->cycles += cyc;
                        break;

                    // 0xDE
                    // DEC abs,X
                    case 0xE:

                        // Increment to get the lower byte
                        cpu->PC += 1;
                        LB = memory[cpu->PC];

                        // Increment to get the upper byte
                        cpu->PC += 1;
                        address = (memory[cpu->PC] << 8 | LB) + cpu->X;

                        memory[address]--;
                        cpu->PC++;

                        emulate_6502_cycle(7);
                        cpu->cycles += 7;
                        break;


            }

            break;

        case 0xE:

            switch (instr & 0x0F) {

                // 0xE0
                // CPX #
                case 0x0:
                    // Compare Memory and Index X
                    cpu->PC++;

                    // Carry Flag
                    cpu->P[0] = cpu->X >= memory[cpu->PC];

                    // Zero flag
                    cpu->P[1] = cpu->X == memory[cpu->PC];

                    // Negative Flag
                    cpu->P[7] = ((cpu->X - memory[cpu->PC]) & 0x80) == 0x80;

                    cpu->PC++;

                    emulate_6502_cycle(2);
                    cpu->cycles += 2;
                    break;

                // 0xE1
                // SBC X,ind
                case 0x1:
                    // Subtract with Carry
                    cpu->PC++;

                    // Ignore carry if it exists
                    zpg_addr = (memory[cpu->PC] + cpu->X) & 0xFF;

                    LB = memory[zpg_addr];
                    HB = memory[zpg_addr + 1];

                    address = HB << 8 | LB;

                    // Hold old value of accumulator
                    LB = cpu->A;

                    // Use zpg_addr temporarily as its 16 bit
                    // equiv. to A = A - memory - ~C
                    //zpg_addr = (uint16_t)(cpu->A + ~memory[address] + cpu->P[0]);
                    zpg_addr = (uint16_t)(cpu->A - memory[address] - (1 - cpu->P[0]));

                    cpu->A = zpg_addr & 0xFF;

                    // If  M < old A value, it underflows.
                    // Carry is cleared if it underflows
                    cpu->P[0] = !(memory[address] < LB);

                    // Zero flag
                    cpu->P[1] = cpu->A == 0;

                    // Negative flag
                    cpu->P[7] = cpu->A >> 7;

                    // Overflow flag
                    cpu->P[6] = (((LB ^ zpg_addr) & (memory[address] ^ zpg_addr)) & 0x80) == 0x80;

                    cpu->PC++;

                    emulate_6502_cycle(6);
                    cpu->cycles += 6;
                    break;

                // 0xE4
                // CPX zpg
                case 0x4:
                    cpu->PC++;
                    LB = memory[cpu->PC];

                    // Carry Flag
                    cpu->P[0] = cpu->X >= memory[LB];

                    // Zero flag
                    cpu->P[1] = cpu->X == memory[LB];

                    // Negative Flag
                    cpu->P[7] = ((cpu->X - memory[LB]) & 0x80) == 0x80;

                    cpu->PC++;
                    emulate_6502_cycle(3);
                    cpu->cycles += 3;
                    break;

                // 0xE5
                // SBC zpg
                case 0x5:
                    cpu->PC++;

                    // Get zpg addr
                    LB = memory[cpu->PC];

                    // Hold old value of accumulator
                    HB = cpu->A;

                    // Use zpg_addr temporarily as its 16 bit
                    // equiv. to A = A - memory - ~C
                    //zpg_addr = (uint16_t)(cpu->A + ~memory[address] + cpu->P[0]);
                    zpg_addr = (uint16_t)(cpu->A - memory[LB] - (1 - cpu->P[0]));

                    cpu->A = zpg_addr & 0xFF;

                    // If  M < old A value, it underflows.
                    // Carry is cleared if it underflows
                    cpu->P[0] = !(memory[LB] < HB);

                    // Zero flag
                    cpu->P[1] = cpu->A == 0;

                    // Negative flag
                    cpu->P[7] = cpu->A >> 7;

                    // Overflow flag
                    cpu->P[6] = ((HB ^ zpg_addr) & (memory[LB] ^ zpg_addr) & 0x80) == 0x80;

                    cpu->PC++;
                    emulate_6502_cycle(3);
                    cpu->cycles += 3;
                    break;

                // 0xE6
                // INC zpg
                case 0x6:
                    // Increment M by One
                    cpu->PC++;
                    LB = memory[cpu->PC];

                    memory[LB]++;

                    cpu->PC++;

                    cpu->P[1] = memory[LB] == 0;
                    cpu->P[7] = (memory[LB] & 0x80) == 0x80;

                    emulate_6502_cycle(5);
                    cpu->cycles += 5;
                    break;

                // 0xE8
                // INX impl
                case 0x8:
                    // Increment X register by One
                    cpu->X++;
                    cpu->PC++;

                    cpu->P[1] = cpu->X == 0;
                    cpu->P[7] = (cpu->X & 0x80 )== 0x80;

                    emulate_6502_cycle(2);
                    cpu->cycles += 2;
                    break;

                // 0xE9
                // SBC #
                case 0x9:
                    cpu->PC++;

                    //sleep(1);
                    printf("E9: SBC instruction");

                    // Get zpg addr
                    LB = memory[cpu->PC];

                    // Hold old value of accumulator
                    HB = cpu->A;

                    // Use zpg_addr temporarily as its 16 bit
                    // equiv. to A = A - memory - ~C
                    //zpg_addr = (uint16_t)(cpu->A + ~memory[address] + cpu->P[0]);
                    zpg_addr = (uint16_t)(cpu->A - LB - (1 - cpu->P[0]));

                    cpu->A = zpg_addr & 0xFF;

                    // If  M < old A value, it underflows.
                    // Carry is cleared if it underflows
                    cpu->P[0] = !(LB < HB);

                    // Zero flag
                    cpu->P[1] = cpu->A == 0;

                    // Negative flag
                    cpu->P[7] = cpu->A >> 7;

                    // Overflow flag
                    cpu->P[6] = (((HB ^ zpg_addr) & (LB ^ zpg_addr)) & 0x80) != 0;

                    cpu->PC++;
                    emulate_6502_cycle(2);
                    cpu->cycles += 2;
                    break;

                // 0xEA
                // NOP impl
                case 0xA:
                    cpu->PC++;
                    emulate_6502_cycle(2);
                    cpu->cycles += 2;
                    break;

                // 0xEC
                // CPX abs
                case 0xC:
                    cpu->PC++;
                    LB = memory[cpu->PC];

                    cpu->PC++;
                    // Address of the location of new address
                    address = memory[cpu->PC] << 8 | LB;

                    // Carry Flag
                    cpu->P[0] = cpu->X >= memory[address];

                    // Zero flag
                    cpu->P[1] = cpu->X == memory[address];

                    // Negative Flag
                    cpu->P[7] = ((cpu->X - memory[address]) & 0x80) == 0x80;

                    cpu->PC++;

                    emulate_6502_cycle(4);
                    cpu->cycles += 4;
                    break;

                // 0xED
                // SBC abs
                case 0xD:
                    // Increment to get the lower byte
                    cpu->PC += 1;
                    LB = memory[cpu->PC];

                    // Increment to get the upper byte
                    cpu->PC += 1;
                    address = memory[cpu->PC] << 8 | LB;

                    // Store Accumulator in HB temporarily.
                    HB = cpu->A;

                    // Use zpg_addr temporarily as its 16 bit
                    // equiv. to A = A - memory - ~C
                    //zpg_addr = (uint16_t)(cpu->A + ~memory[address] + cpu->P[0]);
                    zpg_addr = (uint16_t)(cpu->A - memory[address] - (1 - cpu->P[0]));

                    cpu->A = zpg_addr & 0xFF;

                    // If  M < old A value, it underflows.
                    // Carry is cleared if it underflows
                    cpu->P[0] = !(memory[address] < HB);

                    // Zero flag
                    cpu->P[1] = cpu->A == 0;

                    // Negative flag
                    cpu->P[7] = cpu->A >> 7;

                    // Overflow flag
                    cpu->P[6] = ((HB ^ zpg_addr) & (memory[address] ^ zpg_addr) & 0x80) == 0x80;

                    cpu->PC++;
                    emulate_6502_cycle(4);
                    cpu->cycles += 4;
                    break;

                // 0xEE
                // INC abs
                case 0xE:

                    // Increment to get the lower byte
                    cpu->PC += 1;
                    LB = memory[cpu->PC];

                    // Increment to get the upper byte
                    cpu->PC += 1;
                    address = memory[cpu->PC] << 8 | LB;

                    memory[address]++;

                    cpu->P[1] = memory[address] == 0;
                    cpu->P[7] = (memory[address] & 0x80 )== 0x80;

                    cpu->PC++;
                    emulate_6502_cycle(6);
                    cpu->cycles += 6;
                    break;
            }
            break;

        case 0xF:

            switch (instr & 0x0F) {

                // 0xF0
                // BEQ rel
                case 0x0:
                // Increment PC by to get signed offset
                cpu->PC += 1;
                uint8_t signed_offset = (uint8_t)memory[cpu->PC];
                printf("Signed offset at 0x%x: %x\n", cpu->PC, signed_offset);

                //Branch if zero flag is set
                if (cpu->P[1]) {

                    address = cpu->PC;
                    //cpu->PC += 1 + signed_offset;
                    if ((signed_offset & 0x80) == 0x80) {
                        // Negative
                        // Convert from 2's complement to regular by negation + 1
                        cpu->PC += 1 - ((~(signed_offset)  & 0xFF) + 0x01 );

                        printf("Signed offset at 0x%x: %x\n", cpu->PC, signed_offset);
                        printf("2's complement: %x\n", ((~(signed_offset)  & 0xFF) + 0x01 ));

                    } else {
                        cpu->PC += 1 + signed_offset;
                    }

                    ////sleep(1);

                    if ((cpu->PC & 0xFF00) != (address & 0xFF00)) {
                        emulate_6502_cycle(4);
                        cpu->cycles += 4;
                    } else {
                        emulate_6502_cycle(3);
                        cpu->cycles += 3;
                    }

                } else {
                    cpu->PC++;
                    emulate_6502_cycle(2);
                    cpu->cycles += 2;
                }
                    break;

                // 0xF1
                // SBC ind,Y
                case 0x1:
                    // Increment to get the lower byte
                    cpu->PC++;
                    LB = memory[cpu->PC];

                    // Higher byte is memory[LB]
                    // Lower byte is memory[LB + 1]

                    // Pointer to the address
                    address = (memory[LB + 1] << 8 | memory[LB]) +  cpu->Y;
                    if ((address & 0xFF00) != ((address - cpu->Y) & 0xFF00)) {
                        cyc = 6;
                    } else {
                        cyc = 5;
                    }

                    // Store Accumulator in HB temporarily.
                    HB = cpu->A;

                    // Use zpg_addr temporarily as its 16 bit
                    // equiv. to A = A - memory - ~C
                    //zpg_addr = (uint16_t)(cpu->A + ~memory[address] + cpu->P[0]);
                    zpg_addr = (uint16_t)(cpu->A - memory[address] - (1 - cpu->P[0]));

                    cpu->A = zpg_addr & 0xFF;

                    // If  M < old A value, it underflows.
                    // Carry is cleared if it underflows
                    cpu->P[0] = !(memory[address] < HB);

                    // Zero flag
                    cpu->P[1] = cpu->A == 0;

                    // Negative flag
                    cpu->P[7] = cpu->A >> 7;

                    // Overflow flag
                    cpu->P[6] = (((zpg_addr ^ memory[address]) & (HB ^ zpg_addr)) & 0x80) != 0;

                    cpu->PC++;

                    emulate_6502_cycle(cyc);
                    cpu->cycles += cyc;
                    break;

                // 0xF5
                // SBC zpg,X
                case 0x5:
                    cpu->PC++;
                    LB = memory[cpu->PC];

                    // Discard carry, zpg should not exceed 0x00FF
                    address = (uint16_t) ((LB + cpu->X) & 0xFF);

                    // Hold old value of accumulator
                    HB = cpu->A;

                    // Use zpg_addr temporarily as its 16 bit
                    // equiv. to A = A - memory - ~C
                    //zpg_addr = (uint16_t)(cpu->A + ~memory[address] + cpu->P[0]);
                    zpg_addr = (uint16_t)(cpu->A - memory[address] - (1 - cpu->P[0]));

                    cpu->A = zpg_addr & 0xFF;

                    // If  M < old A value, it underflows.
                    // Carry is cleared if it underflows
                    cpu->P[0] = !(memory[address] < HB);

                    // Zero flag
                    cpu->P[1] = cpu->A == 0;

                    // Negative flag
                    cpu->P[7] = cpu->A >> 7;

                    // Overflow flag
                    cpu->P[6] = (((cpu->A ^ memory[address]) & (cpu->A ^ zpg_addr)) & 0x80) != 0;

                    cpu->PC++;
                    emulate_6502_cycle(4);
                    cpu->cycles += 4;
                    break;



                // 0xF6
                // INC zpg,X
                case 0x6:
                    // Decrement M by One
                    cpu->PC++;
                    LB = memory[cpu->PC];

                    // Discard carry, zpg should not exceed 0x00FF
                    address = (uint16_t) ((LB + cpu->X) & 0xFF);

                    memory[address]++;

                    cpu->PC++;

                    cpu->P[1] = memory[address] == 0;
                    cpu->P[7] = (memory[address] & 0x80 )== 0x80;

                    emulate_6502_cycle(6);
                    cpu->cycles += 6;
                    break;

                // 0xF8
                // SED impl
                case 0x8:
                    // Set Decimal Flag
                    cpu->P[3] = 1;
                    cpu->PC++;
                    emulate_6502_cycle(2);
                    cpu->cycles += 2;
                    break;

                // 0xF9
                // SBC abs,Y
                case 0x9:
                    // Compare accumulator with M
                    // Increment to get the lower byte
                    cpu->PC += 1;
                    LB = memory[cpu->PC];

                    // Increment to get the upper byte
                    cpu->PC += 1;
                    address = (memory[cpu->PC] << 8 | LB) + cpu->Y;
                    if ((address & 0xFF00) != ((address - cpu->Y) & 0xFF00)) {
                        cyc = 5;
                    } else {
                        cyc = 4;
                    }

                    // Hold old value of accumulator
                    HB = cpu->A;

                    // Use zpg_addr temporarily as its 16 bit
                    // equiv. to A = A - memory - ~C
                    //zpg_addr = (uint16_t)(cpu->A + ~memory[address] + cpu->P[0]);
                    zpg_addr = (uint16_t)(cpu->A - memory[address] - (1 - cpu->P[0]));

                    cpu->A = zpg_addr & 0xFF;

                    // If  M < old A value, it underflows.
                    // Carry is cleared if it underflows
                    cpu->P[0] = !(memory[address] < HB);

                    // Zero flag
                    cpu->P[1] = cpu->A == 0;

                    // Negative flag
                    cpu->P[7] = cpu->A >> 7;

                    // Overflow flag
                    cpu->P[6] = ((HB ^ zpg_addr) & (memory[address] ^ zpg_addr) & 0x80) != 0;

                    cpu->PC++;
                    emulate_6502_cycle(cyc);
                    cpu->cycles += cyc;
                    break;

                // 0xFD
                // SBC abs,X
                case 0xD:
                    // Increment to get the lower byte
                    cpu->PC += 1;
                    LB = memory[cpu->PC];

                    // Increment to get the upper byte
                    cpu->PC += 1;
                    address = (memory[cpu->PC] << 8 | LB) + cpu->X;
                    if ((address & 0xFF00) != ((address - cpu->X) & 0xFF00)) {
                        cyc = 5;
                    } else {
                        cyc = 4;
                    }

                    // Hold old value of accumulator
                    HB = cpu->A;

                    // Use zpg_addr temporarily as its 16 bit
                    // equiv. to A = A - memory - ~C
                    //zpg_addr = (uint16_t)(cpu->A + ~memory[address] + cpu->P[0]);
                    zpg_addr = (uint16_t)(cpu->A - memory[address] - (1 - cpu->P[0]));

                    cpu->A = zpg_addr & 0xFF;

                    // If  M < old A value, it underflows.
                    // Carry is cleared if it underflows
                    cpu->P[0] = !(memory[address] < HB);

                    // Zero flag
                    cpu->P[1] = cpu->A == 0;

                    // Negative flag
                    cpu->P[7] = cpu->A >> 7;

                    // Overflow flag
                    cpu->P[6] = ((HB ^ zpg_addr) & (memory[address] ^ zpg_addr) & 0x80) == 0x80;

                    cpu->PC++;
                    emulate_6502_cycle(cyc);
                    cpu->cycles += cyc;
                    break;

                // 0xFE
                // INC abs,X
                case 0xE:

                    // Increment to get the lower byte
                    cpu->PC += 1;
                    LB = memory[cpu->PC];

                    // Increment to get the upper byte
                    cpu->PC += 1;
                    address = (memory[cpu->PC] << 8 | LB) + cpu->X;

                    memory[address]++;

                    cpu->P[1] = memory[address] == 0;
                    cpu->P[7] = (memory[address] & 0x80 )== 0x80;

                    cpu->PC++;
                    emulate_6502_cycle(7);
                    cpu->cycles += 7;
                    break;

            }


        break;



    }


    printf("Stack Memory: {");
    for (int i = 0xF0; i <= 0xFF; i++) {
        printf("0x01%x: %x, ", i, memory[0x0100 | i]);
    }

    printf("}\n");
}
