#include "cpu.h"
#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>

#include "ppu.h"

#define MEMORY_SIZE 0x10000 // 64 KB
#define NES_HEADER_SIZE 16
#define NES_TEST 0xC000

#define LOG(fmt, ...)\
    do { if (CPU_LOGGING) fprintf(stderr, fmt, ##__VA_ARGS__); } while (0)


uint8_t memory[MEMORY_SIZE] = {0};
FILE* log_file;

// int to hold status flags
uint8_t status;

// int to hold lower byte of memory address
uint8_t LB, HB;
uint16_t address, zpg_addr;

char new_carry;
int cyc = 0;

int instr_num = 0;


unsigned char dma_active_flag;
int dma_cycles = 0;

/**  Helper functions **/

inline void push_stack(uint8_t lower_addr, uint8_t val) {
    memory[0x0100 | lower_addr] = val;
}

uint16_t page_crossing(uint16_t addr, uint16_t oper) {

    uint16_t address = 0;
    address = addr + oper;
    // Check if the page is the same
    if ( ((addr + oper) & 0xFF00) != (addr & 0xFF00) ) {
        // Keep the page from the addr to emulate bug
        address = (addr & 0xFF00) | (address & 0xFF);
    }
    return address;
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
    //nanosleep(&ts, NULL);
}

/* PPU Functions */

void cpu_ppu_write(Cpu6502 *cpu, uint16_t addr, uint8_t val) {
    LOG("PPU MMIO WRITE: $%X; val: %x\n", addr, val);
    //sleep(1);

    if (addr == 0x2001) {
        printf("Writing to PPUMASK %X\n", val);
        fflush(stdout);
        //sleep(5);
    }
    // if ((addr == 0x2000 || addr == 0x2001 || addr == 0x2005 || addr == 0x2006) && cpu->cycles <= 29658)
    //     return;
    ppu_registers_write(cpu->ppu, addr, val);
}

uint8_t cpu_ppu_read(Cpu6502 *cpu, uint16_t addr) {
    return ppu_registers_read(cpu->ppu, addr);
}

// Core functions

void cpu_init(Cpu6502 *cpu) {

    // Initialize stack pointer to the top;
    cpu->S = 0xFD;
    cpu->A = 0x0;
    cpu->X = 0x0;
    cpu->Y = 0x0;

    cpu->nmi_state = 0;

    #if NES_TEST_ROM
    //cpu->PC = 0xC000;
    cpu->PC = (memory[0xFFFD] << 8) | memory[0xFFFC];
    #else
    cpu->PC = (memory[0xFFFD] << 8) | memory[0xFFFC];
    #endif

    printf("MEMORY[0xFFFD]: %x\n", memory[0xFFFD]);
    printf("MEMORY[0xFFFC]: %x\n", memory[0xFFFC]);
    printf("PC: $%04X | Opcode: %02X\n", cpu->PC, memory[cpu->PC]);
    // sleep(5);

    cpu->instr = memory[cpu->PC];
    cpu->cycles = 7;

    cpu->P[0] = 0;
    cpu->P[1] = 0;
    cpu->P[2] = 1;
    cpu->P[3] = 0;
    cpu->P[4] = 1;
    cpu->P[5] = 0;
    cpu->P[6] = 0;
    cpu->P[7] = 0;

    //memset(memory, 0, sizeof(memory));

    log_file = fopen("log.txt", "w");
    fclose(log_file);
    log_file = fopen("log.txt", "a");


    #ifdef NES_TEST_ROM
        push_stack(0100 | cpu->S, 0x70);
        cpu->S--;

        push_stack(0100 | cpu->S, 0x00);
        cpu->S--;
    #endif
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
}

void load_cpu_memory(Cpu6502 *cpu, unsigned char *prg_rom, int prg_size){
    //Clear memory
    memset(memory, 0, MEMORY_SIZE);

    //Load PRG ROM into 0x8000 - 0xBFFF
    memcpy(&memory[0x8000], prg_rom, prg_size);

    //If PRG ROM is 16KB, mirror it into $C000-$FFFF (NROM-128 behavior)
    if (prg_size == 16384) {
       memcpy(&memory[0xC000], prg_rom, 16384);
    }

}


void dump_log_file(Cpu6502 *cpu) {
    fprintf(log_file, "PC: %x ", cpu->PC);
    fprintf(log_file, " %x ", cpu->instr);

    fprintf(log_file, "A: %x ", cpu->A);
    fprintf(log_file, "X: %x ", cpu->X);
    fprintf(log_file, "Y: %x ", cpu->Y);
    join_char_array(&status, cpu->P);
    fprintf(log_file, "SP: %x ", cpu->S);
    fprintf(log_file, "P: %x (%b) ", status, status);
    fprintf(log_file, "Cycle: %d ", cpu->cycles);
    fprintf(log_file, "I #: %d\n", instr_num);
    fprintf(log_file, "{");
    for (int i = 0xf0; i <= 0xff; i++) {
        fprintf(log_file, "M[0x%x]: %x, ", 0x100 | i, memory[0x100 | i]);
    }
    fprintf(log_file, "}\n");


    // fprintf(log_file, "M @ 0x00 = %x\n", memory[0x0]);
    // fprintf(log_file, "M @ 0x10 = %x\n", memory[0x10]);
    // fprintf(log_file, "M @ 0x11 = %x\n", memory[0x11]);
    // fprintf(log_file, "M @ 0x0647 = %x\n", memory[0x647]);

    fprintf(log_file, "\n");

}

// access
void instr_LDA(Cpu6502 *cpu, uint16_t addr) {
    if (addr >= 0x2000 && addr <= 0x3FFF) {
        uint16_t reg_addr = 0x2000 + (addr % 8);
        cpu->A = cpu_ppu_read(cpu, reg_addr);
        LOG("CPU: READING PPU REG $%04X: $%02X\n", reg_addr, cpu->A);
    } else {
        cpu->A = memory[addr];
    }
    cpu->PC++;
    cpu->P[1] = cpu->A == 0;
    cpu->P[7] = (cpu->A & 0x80) == 0x80;
}

void instr_LDX(Cpu6502 *cpu, uint16_t addr) {
    if (addr >= 0x2000 && addr <= 0x3FFF) {
        uint16_t reg_addr = 0x2000 + (addr % 8);
        cpu->X = cpu_ppu_read(cpu, reg_addr);
        cpu_ppu_read(cpu, reg_addr);
    } else {
        cpu->X = memory[addr];
    }
    cpu->PC++;
    cpu->P[1] = cpu->X == 0;
    cpu->P[7] = (cpu->X & 0x80) == 0x80;
}

void instr_LDY(Cpu6502 *cpu, uint16_t addr) {
    if (addr >= 0x2000 && addr <= 0x3FFF) {
        uint16_t reg_addr = 0x2000 + (addr % 8);
        cpu->Y = cpu_ppu_read(cpu, reg_addr);
    } else {
        cpu->Y = memory[addr];
    }
    cpu->PC++;
    cpu->P[1] = cpu->Y == 0;
    cpu->P[7] = (cpu->Y & 0x80) == 0x80;
}

void instr_STA(Cpu6502 *cpu, uint16_t addr) {
    
    if (addr >= 0x2000 && addr <= 0x3FFF) {
        uint16_t reg_addr = 0x2000 + (addr % 8); 
        addr = reg_addr;
        LOG("CPU: WRITING TO PPU REG $%04X: %02X\n", reg_addr, cpu->A);
        cpu_ppu_write(cpu, reg_addr, cpu->A);
    } else if (addr == 0x4014) {
        dma_active_flag = 1;
        dma_cycles = cpu->cycles % 2 == 0 ? 513: 514;
        uint8_t page_mem[0x100];
        memcpy(page_mem, &memory[cpu->A << 8], 0x100);
        load_ppu_oam_mem(cpu->ppu, page_mem);
    } else {
        memory[addr] = cpu->A;
    }
    cpu->PC++;
}

void instr_STX(Cpu6502 *cpu, uint16_t addr) {

    if (addr >= 0x2000 && addr <= 0x3FFF) {
        uint16_t reg_addr = 0x2000 + (addr % 8); 
        addr = reg_addr;
        LOG("CPU: WRITING TO PPU REG $%04X: %02X\n", reg_addr, cpu->X);
        cpu_ppu_write(cpu, reg_addr, cpu->X);
    } else if (addr == 0x4014) {
        dma_active_flag = 1;
        dma_cycles = cpu->cycles % 2 == 0 ? 513: 514;

        uint8_t page_mem[0x100];
        memcpy(page_mem, &memory[cpu->X << 8], 0x100);
        load_ppu_oam_mem(cpu->ppu, page_mem);
    } 
    memory[addr] = cpu->X;
    cpu->PC++;
}

void instr_STY(Cpu6502 *cpu, uint16_t addr) {

    if (addr >= 0x2000 && addr <= 0x3FFF) {
        uint16_t reg_addr = 0x2000 + (addr % 8); 
        addr = reg_addr;
        LOG("CPU: WRITING TO PPU REG $%04X: %02X\n", reg_addr, cpu->Y);
        cpu_ppu_write(cpu, reg_addr, cpu->Y);
    } else if (addr == 0x4014) {
        dma_active_flag = 1;
        dma_cycles = cpu->cycles % 2 == 0 ? 513: 514;

        uint8_t page_mem[0x100];
        memcpy(page_mem, &memory[cpu->Y << 8], 0x100);
        load_ppu_oam_mem(cpu->ppu, page_mem);
    } 

    memory[addr] = cpu->Y;
    cpu->PC++;
}

// Transfer
void instr_TAX(Cpu6502 *cpu) {
    cpu->X = cpu->A;

    cpu->P[1] = cpu->X == 0;
    cpu->P[7] = (cpu->X & 0x80) == 0x80;
    cpu->PC++;

    emulate_6502_cycle(2);
    cyc = 2;
}

void instr_TXA(Cpu6502 *cpu) {
    cpu->A = cpu->X;

    cpu->P[1] = cpu->A == 0;
    cpu->P[7] = (cpu->A & 0x80) == 0x80;
    cpu->PC++;

    emulate_6502_cycle(2);
    cyc = 2;
}
void instr_TAY(Cpu6502 *cpu) {
    cpu->Y = cpu->A;

    cpu->P[1] = cpu->Y == 0;
    cpu->P[7] = (cpu->Y & 0x80) == 0x80;
    cpu->PC++;

    emulate_6502_cycle(2);
    cyc = 2;
}
void instr_TYA(Cpu6502 *cpu) {
    cpu->A = cpu->Y;

    cpu->P[1] = cpu->A == 0;
    cpu->P[7] = (cpu->A & 0x80) == 0x80;
    cpu->PC++;

    emulate_6502_cycle(2);
    cyc = 2;
}

// Arithmetic
void instr_ADC(Cpu6502 *cpu, uint8_t oper) {

    uint16_t result = 0;
    uint8_t new_A = cpu->A;

    if (!cpu->P[3]) {

        result = (uint16_t)(cpu->A + oper + cpu->P[0]);

        // Use lower byte for A
        new_A = result & 0xFF;


        // Set carry bit if upper byte is 1
        cpu->P[0] = result > 0xFF;

        // Set zero flag and negative flag
        cpu->P[1] = new_A == 0;
        cpu->P[7] = new_A >> 7;

        cpu->P[6] = ((cpu->A ^ new_A) & (new_A ^ oper) & 0x80) == 0x80;
        cpu->A = new_A;
    } else {
        //sleep(5);
        result = (cpu->A & 0xF) + (oper & 0xF) + cpu->P[0];
        result += result > 0x9 ? 0x6 : 0;

        uint16_t HB = (cpu->A & 0xF0) + (oper & 0xF0);

        HB += HB > 0x90 ? 0x60 : 0;
        result += HB;

        if ( (result & 0xF0) > 0x90 ) {
            //printf("result is not in BCD: %x");
            result += 0x60;
        }

        cpu->P[0] = result > 0x99;
        cpu->A = result & 0xFF;
        cpu->P[1] = cpu->A == 0;
    }
    cpu->PC++;
}
void instr_SBC(Cpu6502 *cpu, uint8_t oper) {
    uint16_t result = 0;
    uint8_t new_A = cpu->A;

    if (!cpu->P[3]) {
        result = (uint16_t)(cpu->A - oper - (1 - cpu->P[0]));

        new_A = result & 0xFF;

        // If  M < old A value, it underflows.
        // Carry is cleared if it underflows
        char new_carry = cpu->A >= (oper + (1 - cpu->P[0]));
        cpu->P[0] = new_carry;

        // Zero flag
        cpu->P[1] = new_A == 0;

        // Negative flag
        cpu->P[7] = new_A >> 7;

        // Overflow flag
        cpu->P[6] = (((cpu->A ^ new_A) & (oper ^ cpu->A)) & 0x80) != 0;
        cpu->A = new_A;
    } else {

        result = cpu->A - oper - (1 - cpu->P[0]);

        result -= (result & 0xF0) > 0x90 ? 0x60 : 0;
        result -= (result & 0x0F) > 0x09 ? 0x06 : 0;

        cpu->P[0] = result <= 0x99;
        cpu->A = result & 0xFF;
        cpu->P[1] = cpu->A == 0;
    }
    cpu->PC++;
}

void instr_INC(Cpu6502 *cpu, uint8_t *M) {
    *M = *M + 1;

    cpu->P[1] = *M == 0;
    cpu->P[7] = (*M & 0x80 )== 0x80;
    cpu->PC++;
}

void instr_DEC(Cpu6502 *cpu, uint8_t *M) {
    *M = *M - 1;

    cpu->P[1] = *M == 0;
    cpu->P[7] = (*M & 0x80 )== 0x80;
    cpu->PC++;
}

void instr_INX(Cpu6502 *cpu) {
    cpu->X++;

    cpu->P[1] = cpu->X == 0;
    cpu->P[7] = (cpu->X & 0x80 )== 0x80;
    cpu->PC++;

    emulate_6502_cycle(2);
    cyc = 2;
}

void instr_DEX(Cpu6502 *cpu) {
    cpu->X--;

    cpu->P[1] = cpu->X == 0;
    cpu->P[7] = (cpu->X & 0x80 )== 0x80;
    cpu->PC++;

    emulate_6502_cycle(2);
    cyc = 2;
}

void instr_INY(Cpu6502 *cpu) {
    cpu->Y++;

    cpu->P[1] = cpu->Y == 0;
    cpu->P[7] = (cpu->Y & 0x80 )== 0x80;
    cpu->PC++;

    emulate_6502_cycle(2);
    cyc = 2;
}

void instr_DEY(Cpu6502 *cpu) {
    cpu->Y--;

    cpu->P[1] = cpu->Y == 0;
    cpu->P[7] = (cpu->Y & 0x80 )== 0x80;
    cpu->PC++;

    emulate_6502_cycle(2);
    cyc = 2;
}

// Shift
void instr_ASL(Cpu6502 *cpu, uint8_t *M) {
    // Set the carry flag to the 7th bit of Accumulator.
    cpu->P[0] = (*M & 0x80) >> 7;
    *M = *M << 1;

    // Set zero flag and negative flag
    cpu->P[1] = *M == 0;
    cpu->P[7] = *M >> 7;

    cpu->PC++;
}
void instr_LSR(Cpu6502 *cpu, uint8_t *M) {

    // Store 7th bit of memory as new carry value
    cpu->P[0] = *M & 0x01;

    *M = *M >> 1;

    // Set zero flag and negative flag
    cpu->P[1] = *M == 0;
    // Inserting 0, so it cannot be negative
    cpu->P[7] = 0;

    cpu->PC++;
}
void instr_ROL(Cpu6502 *cpu, uint8_t *M) {

    // Store 7th bit of memory as new carry value
    char new_carry = *M >> 7;

    *M = *M << 1;

    // change last bit to value of old carry
    *M = (*M & ~1) | (cpu->P[0] & 1);

    // Set carry bit to the 7th bit of the old value of memory
    cpu->P[0] = new_carry;

    // Set zero flag and negative flag
    cpu->P[1] = *M == 0;
    cpu->P[7] = *M >> 7;

    cpu->PC++;
}
void instr_ROR(Cpu6502 *cpu, uint8_t *M) {

    // Store 0th bit of memory as new carry value
    new_carry = *M & 0x01;
    *M = *M >> 1;

    // Old carry bit becomes MSB
    // Set MSB to 1 to carry flag is 1
    if (cpu->P[0]) {
        *M |= 0x80;

        // MSB is 1 so it is negative
        cpu->P[7] = 1;
    } else {
        // Inserting 0, so it cannot be negative
        cpu->P[7] = 0;
    }

    // Set carry flag to LSB of M
    cpu->P[0] = new_carry;

    // Set zero flag and negative flag
    cpu->P[1] = *M == 0;

    cpu->PC++;
}

// Bitwise
void instr_AND(Cpu6502 *cpu, uint8_t M) {
    cpu->A &= M;

    // Set zero flag and negative flag
    cpu->P[1] = cpu->A == 0;
    cpu->P[7] = cpu->A >> 7;

    cpu->PC++;
}

void instr_ORA(Cpu6502 *cpu, uint8_t M) {
    cpu->A |= M;

    // Set zero flag and negative flag
    cpu->P[1] = cpu->A == 0;
    cpu->P[7] = cpu->A >> 7;

    cpu->PC++;
}
void instr_EOR(Cpu6502 *cpu, uint8_t M) {
    cpu->A ^= M;

    // Set zero flag and negative flag
    cpu->P[1] = cpu->A == 0;
    cpu->P[7] = cpu->A >> 7;

    cpu->PC++;
}
void instr_BIT(Cpu6502 *cpu, uint8_t M) {

    // Negative flag set to 7 bit of memory
    cpu->P[7] = M >> 7;

    // Overflow flag set to 6th bit of memory
    cpu->P[6] = (M >> 6) & 0x1;

    // Zero flag
    cpu->P[1] =  (cpu->A & M) == 0;

    cpu->PC++;
}

// Compare
void instr_CMP(Cpu6502 *cpu, uint8_t M) {

    // printf("CMPing A (%x) and M(%x)\n", cpu->A, M);

    // Carry Flag
    cpu->P[0] = cpu->A >= M;

    // Zero flag
    cpu->P[1] = cpu->A == M;

    // Negative Flag
    cpu->P[7] = ((cpu->A - M) & 0x80 )== 0x80;

    cpu->PC++;
}

void instr_CPX(Cpu6502 *cpu, uint8_t M) {

    // Carry Flag
    cpu->P[0] = cpu->X >= M;

    // Zero flag
    cpu->P[1] = cpu->X == M;

    // Negative Flag
    cpu->P[7] = ((cpu->X - M) & 0x80) == 0x80;

    cpu->PC++;
}

void instr_CPY(Cpu6502 *cpu, uint8_t M) {

    // Carry Flag
    cpu->P[0] = cpu->Y >= M;

    // Zero flag
    cpu->P[1] = cpu->Y == M;

    // Negative Flag
    cpu->P[7] = ((cpu->Y - M) & 0x80) == 0x80;

    cpu->PC++;
}

// Branch

void instr_branch(Cpu6502 *cpu, char flag) {

    // Increment PC by to get signed offset
    cpu->PC += 1;
    uint8_t signed_offset = memory[cpu->PC];

    uint16_t old_addr = cpu->PC;

    //Branch if flag condition is met
    if (flag) {

        if ((signed_offset & 0x80) == 0x80) {
            // Negative
            // Convert from 2's complement to regular by negation + 1
            cpu->PC += 1 - ((~signed_offset & 0xFF) + 1);
        } else {
            cpu->PC += 1 + signed_offset;
        }


        if ((cpu->PC & 0xFF00) != ((old_addr + 2)& 0xFF00)) {
            emulate_6502_cycle(4);
            cyc = 4;
        } else {
            emulate_6502_cycle(3);
            cyc = 3;
        }

        if (old_addr - 1 == cpu->PC) {
            // printf("\nCAUGHT IN A TRAP!\n");
            exit(0);
        }

    } else {
        cpu->PC++;
        emulate_6502_cycle(2);
        cyc = 2;
    }
}

void instr_BCC(Cpu6502 *cpu) {
    instr_branch(cpu, !cpu->P[0]);
}

void instr_BCS(Cpu6502 *cpu) {
    instr_branch(cpu, cpu->P[0]);
}

void instr_BEQ(Cpu6502 *cpu) {
    instr_branch(cpu, cpu->P[1]);
}

void instr_BNE(Cpu6502 *cpu) {
    instr_branch(cpu, !cpu->P[1]);
}

void instr_BPL(Cpu6502 *cpu) {
    instr_branch(cpu, !cpu->P[7]);
}

void instr_BMI(Cpu6502 *cpu) {
    instr_branch(cpu, cpu->P[7]);
}

void instr_BVC(Cpu6502 *cpu) {
    instr_branch(cpu, !cpu->P[6]);
}

void instr_BVS(Cpu6502 *cpu) {
    instr_branch(cpu, cpu->P[6]);
}


void instr_JMP(Cpu6502 *cpu, uint16_t addr) {
    cpu->PC = addr;
}

void instr_JSR(Cpu6502 *cpu, uint16_t addr) {
    push_stack(cpu->S, cpu->PC >> 8);
    cpu->S--;

    push_stack(cpu->S, cpu->PC & 0xFF);
    cpu->S--;

    cpu->PC = addr;
}

void instr_RTS(Cpu6502 *cpu) {

    // LIFO stack, push LB last so LB comes out first
    cpu->S++;
    LB = memory[0x100 | cpu->S];

    // HB pushed first so comes out last
    cpu->S++;

    // Using address variable, but this is the PC value
    address = memory[0x100 | cpu->S] << 8 | LB;

    cpu->PC = address + 1;
    emulate_6502_cycle(6);
    cyc = 6;
}

void instr_BRK(Cpu6502 *cpu) {

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

    emulate_6502_cycle(7);
    cyc = 7;
}

void instr_RTI(Cpu6502 *cpu) {

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
    cyc = 6;
}

// Stack
void instr_PHA(Cpu6502 *cpu) {

    // Push accumulator on stack
    push_stack(cpu->S, cpu->A);

    cpu->S--;
    cpu->PC++;

    emulate_6502_cycle(3);
    cyc = 3;
}

void instr_PLA(Cpu6502 *cpu) {

    // Pull accumulator from stack
    cpu->S++;
    cpu->A = memory[0x100 | cpu->S];
    cpu->PC++;

    cpu->P[1] = cpu->A == 0;
    cpu->P[7] = (cpu->A & 0x80) == 0x80;

    emulate_6502_cycle(4);
    cyc = 4;
}

void instr_PHP(Cpu6502 *cpu) {

    // Push Status Flags to stack, with B flag set
    cpu->P[4] = 1;
    // Reserve bit set to 1
    cpu->P[5] = 1;

    join_char_array(&status, cpu->P);
    push_stack(cpu->S, status);
    cpu->S--;

    cpu->PC++;
    emulate_6502_cycle(3);
    cyc = 3;
}

void instr_PLP(Cpu6502 *cpu) {
    // Pull Processor Status from Stack
    cpu->S++;

    uint8_to_char_array(memory[0x0100 | cpu->S], cpu->P);

    // These bits are ignored, just set to 1
    cpu->P[5] = 1;
    cpu->P[4] = 1;

    cpu->PC++;
    emulate_6502_cycle(4);
    cyc = 4;
}

void instr_TXS(Cpu6502 *cpu) {
    cpu->S = cpu->X;
    cpu->PC++;

    emulate_6502_cycle(2);
    cyc = 2;
}

void instr_TSX(Cpu6502 *cpu) {
    cpu->X = cpu->S;
    cpu->PC++;

    cpu->P[1] = cpu->X == 0;
    cpu->P[7] = (cpu->X & 0x80) == 0x80;
    emulate_6502_cycle(2);
    cyc = 2;
}

// Flags
void instr_CLC(Cpu6502 *cpu) {
    // clear carry flag
    cpu->P[0] = 0;

    cpu->PC++;
    emulate_6502_cycle(2);
    cyc = 2;
}

void instr_SEC(Cpu6502 *cpu) {
    // set carry flag
    cpu->P[0] = 1;

    cpu->PC++;
    emulate_6502_cycle(2);
    cyc = 2;
}

void instr_CLI(Cpu6502 *cpu) {
    // clear interrupt flag
    cpu->P[2] = 0;

    cpu->PC++;
    emulate_6502_cycle(2);
    cyc = 2;
}

void instr_SEI(Cpu6502 *cpu) {
    cpu->P[2] = 1;

    cpu->PC++;
    emulate_6502_cycle(2);
    cyc = 2;
}

void instr_CLD(Cpu6502 *cpu) {
    cpu->P[3] = 0;

    cpu->PC++;
    emulate_6502_cycle(2);
    cyc = 2;
}

void instr_SED(Cpu6502 *cpu) {
    cpu->P[3] = 1;

    cpu->PC++;
    emulate_6502_cycle(2);
    cyc = 2;
}

void instr_CLV(Cpu6502 *cpu) {
    cpu->P[6] = 0;

    cpu->PC++;
    emulate_6502_cycle(2);
    cyc = 2;
}

// Other
void instr_NOP(Cpu6502 *cpu) {
    cpu->PC++;
    emulate_6502_cycle(2);
    cyc = 2;
}

// Illegal opcodes

void instr_LAX(Cpu6502 *cpu, uint8_t val) {
    cpu->A = val;
    cpu->X = val;
    cpu->PC++;
    cpu->P[1] = cpu->X == 0;
    cpu->P[7] = (cpu->X & 0x80) == 0x80;
}

void instr_SAX(Cpu6502 *cpu, uint16_t addr) {
    memory[addr] = cpu->A & cpu->X;
    cpu->PC++;
}

void instr_DCP(Cpu6502 *cpu, uint8_t *M) {
    *M = *M - 1;

    // Carry Flag
    cpu->P[0] = cpu->A >= *M;

    // Zero flag
    cpu->P[1] = cpu->A == *M;

    // Negative Flag
    cpu->P[7] = ((cpu->A - *M) & 0x80 )== 0x80;

    cpu->PC++;
}

void instr_ISC(Cpu6502 *cpu, uint8_t *M) {
    *M = *M + 1;

    uint16_t result = 0;
    uint8_t new_A = cpu->A;

    result = (uint16_t)(cpu->A - *M - (1 - cpu->P[0]));

    new_A = result & 0xFF;

    // If  M < old A value, it underflows.
    // Carry is cleared if it underflows
    char new_carry = cpu->A >= (*M + (1 - cpu->P[0]));
    cpu->P[0] = new_carry;

    // Zero flag
    cpu->P[1] = new_A == 0;

    // Negative flag
    cpu->P[7] = new_A >> 7;

    // Overflow flag
    cpu->P[6] = (((cpu->A ^ new_A) & (*M ^ cpu->A)) & 0x80) != 0;
    cpu->A = new_A;

    cpu->PC++;
}

void instr_RLA(Cpu6502 *cpu, uint8_t *M) {
    // Store 7th bit of memory as new carry value
    char new_carry = *M >> 7;

    *M = *M << 1;

    // change last bit to value of old carry
    *M = (*M & ~1) | (cpu->P[0] & 1);

    cpu->A &= *M;
    // Set carry bit to the 7th bit of the old value of memory
    cpu->P[0] = new_carry;

    // Set zero flag and negative flag
    cpu->P[1] = cpu->A == 0;
    cpu->P[7] = cpu->A >> 7;

    cpu->PC++;
}

void instr_RRA(Cpu6502 *cpu, uint8_t *M) {
    // Store 0th bit of memory as new carry value
    new_carry = *M & 0x01;
    *M = *M >> 1;

    // Old carry bit becomes MSB
    // Set MSB to 1 to carry flag is 1
    if (cpu->P[0]) {
        *M |= 0x80;

        // MSB is 1 so it is negative
        cpu->P[7] = 1;
    } else {
        // Inserting 0, so it cannot be negative
        cpu->P[7] = 0;
    }

    // Set carry flag to LSB of M
    cpu->P[0] = new_carry;

    // Set zero flag and negative flag
    cpu->P[1] = *M == 0;

    uint16_t result = 0;
    uint8_t new_A = cpu->A;

    // printf("A + M + ~C\n");
    // printf("%x + %x + %b\n", cpu->A, *M, (1 - cpu->P[0]));
    // Reuse zpg variable (in case it overflows)
    result = (uint16_t)(cpu->A + *M + cpu->P[0]);

    // Use lower byte for A
    new_A = result & 0xFF;

    // Set carry bit if upper byte is 1
    cpu->P[0] = result > 0xFF;

    // Set zero flag and negative flag
    cpu->P[1] = new_A == 0;
    cpu->P[7] = new_A >> 7;

    cpu->P[6] = ((cpu->A ^ new_A) & (new_A ^ *M) & 0x80) == 0x80;
    cpu->A = new_A;

    cpu->PC++;
}

void instr_SLO(Cpu6502 *cpu, uint8_t *M) {
    // Set the carry flag to the 7th bit of Accumulator.
    cpu->P[0] = (*M & 0x80) >> 7;
    // printf("Before ASL: %x\n", *M);
    *M = *M << 1;
    // printf("After ASL: %x\n", *M);
    // printf("Before A: %x\n", cpu->A);
    cpu->A |= *M;
    // printf("After A: %x\n", cpu->A);
    // Set zero flag and negative flag
    cpu->P[1] = cpu->A == 0;
    cpu->P[7] = cpu->A >> 7;

    cpu->PC++;
}

void instr_SRE(Cpu6502 *cpu, uint8_t *M) {
    // Store 7th bit of memory as new carry value
    cpu->P[0] = *M & 0x01;

    *M = *M >> 1;

    cpu->A ^= *M;
    // Set zero flag and negative flag
    cpu->P[1] = cpu->A == 0;
    cpu->P[7] = cpu->A >> 7;

    cpu->PC++;
}

void cpu_nmi_triggered(Cpu6502 *cpu) {
    // Push PC and jump to $FFFA
    push_stack(cpu->S, cpu->PC >> 8);
    cpu->S--;

    push_stack(cpu->S, cpu->PC & 0xFF);
    cpu->S--;

    cpu->P[4] = 0;
    cpu->P[5] = 1;

    // Merging Status flags to save it on stack
    join_char_array(&status, cpu->P);

    // Set interrupt disable to 1 after pushing to stack
    cpu->P[2] = 1;

    push_stack(cpu->S, status);
    cpu->S -=1;

    cpu->PC = (memory[0xFFFB] << 8) | memory[0xFFFA];

    LOG("NMI TRIGGERED PC: %x\n", cpu->PC);
    //sleep(1);
}
// Addresing modes

uint16_t addr_abs(Cpu6502 *cpu) {

    // Increment to get the lower byte
    cpu->PC += 1;
    uint16_t addr;
    uint8_t LB = memory[cpu->PC];

    // Increment to get the upper byte
    cpu->PC += 1;
    addr = memory[cpu->PC] << 8 | LB;

    return addr;
}

uint16_t addr_abs_X(Cpu6502 *cpu) {
    // Increment to get the lower byte
    cpu->PC++;
    uint16_t addr;

    uint8_t LB = memory[cpu->PC];
    cpu->PC++;
    uint8_t HB = memory[cpu->PC];

    // printf("LB: %x\n", LB);
    // printf("HB: %x\n", HB);

    //addr = (memory[cpu->PC] << 8 | LB) + cpu->X;
    addr = (HB << 8 | LB) + cpu->X;

    return addr;
}

uint16_t addr_abs_Y(Cpu6502 *cpu) {

    // Increment to get the lower byte
    cpu->PC += 1;
    uint16_t addr;
    LB = memory[cpu->PC];

    // Increment to get the upper byte
    cpu->PC += 1;
    addr = (memory[cpu->PC] << 8 | LB) + cpu->Y;

    return addr;
}

uint16_t addr_imm(Cpu6502 *cpu) {

    // Increment to get the next byte
    cpu->PC++;

    return cpu->PC;

}

uint16_t addr_ind(Cpu6502 *cpu) {

    cpu->PC++;
    uint16_t addr;
    uint8_t LB = memory[cpu->PC];

    cpu->PC++;
    // Address of the location of new address
    addr = memory[cpu->PC] << 8 | LB;

    // If address crosses boundary, bug occurs
    // For example, address = 0x2ff, this is the lower byte
    // higher byte should be retrieved from 0x300
    // instead, 6502 wraps the address around to 0x200

    if (LB == 0xFF) {
        // printf("JMP ind with page crossing wrapping!\n");
        // printf("Retrieving higher byte from: %x\n",  (addr & 0xF00));
        // printf("PC: %x\n", cpu->PC);
        // sleep(5);
        return memory[addr & 0xF00] << 8 | memory[addr];
    } else {
        return memory[addr + 1] << 8 | memory[addr];
    }
}

uint16_t addr_X_ind(Cpu6502 *cpu) {
    // Increment to get the lower byte
    cpu->PC++;
    uint16_t addr;
    // Ignore carry if it exists
    uint8_t BB = (memory[cpu->PC] + cpu->X) & 0xFF;

    // printf("BB: %x \n", BB);

    uint8_t LB = memory[BB];
    uint8_t HB = memory[page_crossing(BB, 1)];


    // printf("LB: %x\n", LB);
    // printf("HB: %x\n", HB);

    addr = HB << 8 | LB;
    return addr;
}

uint16_t addr_ind_Y(Cpu6502 *cpu) {

    // Increment to get the lower byte
    cpu->PC++;
    uint16_t addr;
    uint8_t BB = memory[cpu->PC];

    uint8_t LB = memory[BB];
    uint8_t HB = memory[page_crossing(BB, 1)];

    // printf("LB: %x\n", LB);
    // printf("HB: %x\n", HB);

    //addr = page_crossing(HB << 8 | LB, cpu->Y);
    addr = (HB << 8 | LB) + cpu->Y;
    return addr;
}

uint16_t addr_zpg(Cpu6502 *cpu) {
    cpu->PC++;
    uint16_t addr;
    LB = memory[cpu->PC];

    addr = (uint16_t) LB;
    return addr;
}

uint16_t addr_zpg_X(Cpu6502 *cpu) {
    cpu->PC++;
    uint16_t addr;
    LB = memory[cpu->PC];

    // Discard carry, zpg should not exceed 0x00FF
    addr = (uint16_t) ((LB + cpu->X) & 0xFF);
    return addr;
}

uint16_t addr_zpg_Y(Cpu6502 *cpu) {
    cpu->PC++;
    uint16_t addr;
    LB = memory[cpu->PC];

    addr = (LB + cpu->Y) & 0xFF;
    return addr;
}

void cpu_execute(Cpu6502 *cpu) {

    // Placeholder for instruction
    uint8_t instr = memory[cpu->PC];
    instr_num++;

    cpu->instr = instr;
    LOG("****CPU******\n");
    LOG("\nPC Value: %x\n", cpu->PC);
    LOG("Instruction: %x\n", instr);
    LOG("Stack Pointer: %x\n", cpu->S);
    join_char_array(&status, cpu->P);
    LOG("Status: %b\n", status);
    LOG("A: %x\n", cpu->A );
    LOG("X: %x\n", cpu->X );
    LOG("Y: %x\n", cpu->Y );
    printf("Cycle: %d\n\n", cpu->cycles);

    dump_log_file(cpu);
    // printf("M[0x0] = %x\n\n", memory[0x0]);

    #if NES_TEST_ROM
        if (cpu->PC == 0x7000) {
            // printf("Reached addressed originally pushed to stack!");
            exit(0);
        }
    #endif
    
    // PPU set NMI flag
    if (cpu->ppu->nmi_flag) {
        cpu_nmi_triggered(cpu);
        cpu->ppu->nmi_flag = 0;
        return;
    }

    if (dma_active_flag) {
        if (dma_cycles > 0) {
            dma_cycles--;
            ppu_execute_cycle(cpu->ppu);
            ppu_execute_cycle(cpu->ppu);
            ppu_execute_cycle(cpu->ppu);
            return;
        } else {
            dma_active_flag = 0;
        }
    }


    // Compare the upper 4 bits
    switch ((instr >> 4) & 0x0F) {

        // 0x0X
        case 0x0:

            switch (instr & 0x0F) {
                // 0x00
                // BRK impl
                case 0x0:

                    instr_BRK(cpu);

                    emulate_6502_cycle(7);
                    cyc = 7;

                    break;

                // 0x01
                // ORA X,ind
                case 0x1:
                    address = addr_X_ind(cpu);
                    instr_ORA(cpu, memory[address]);

                    //cpu->PC++;

                    emulate_6502_cycle(6);
                    cyc = 6;
                    break;

                // 0x03
                // SLO X,ind
                case 0x3:
                    address = addr_X_ind(cpu);
                    instr_SLO(cpu, &memory[address]);

                    emulate_6502_cycle(8);
                    cyc = 8;
                    break;

                // NOP zpg
                case 0x4:
                    cpu->PC += 2;
                    emulate_6502_cycle(3);
                    cyc = 3;
                    break;

                // 0x05
                // ORA zpg
                case 0x5:
                    address = addr_zpg(cpu);
                    instr_ORA(cpu, memory[address]);

                    emulate_6502_cycle(3);
                    cyc = 3;
                    break;

                // 0x06
                // ASL zpg
                case 0x6:

                    address = addr_zpg(cpu);
                    instr_ASL(cpu, &memory[address]);

                    emulate_6502_cycle(5);
                    cyc = 5;
                    break;

                // 0x06
                // SLO zpg
                case 0x7:
                    address = addr_zpg(cpu);
                    instr_SLO(cpu, &memory[address]);

                    emulate_6502_cycle(5);
                    cyc = 5;
                    break;

                // 0x08
                // PHP impl
                case 0x8:
                    instr_PHP(cpu);
                    break;

                // 0x09
                // ORA #
                case 0x9:
                    address = addr_imm(cpu);
                    instr_ORA(cpu, memory[address]);

                    emulate_6502_cycle(2);
                    cyc = 2;
                    break;

                // 0x0A
                // ASL A
                case 0xA:
                    instr_ASL(cpu, &cpu->A);

                    emulate_6502_cycle(2);
                    cyc = 2;
                    break;

                    // NOP abs
                    case 0xc:
                        cpu->PC += 3;
                        emulate_6502_cycle(cyc);
                        
                        break;

                // 0x0D
                // ORA abs
                case 0xD:
                    address = addr_abs(cpu);
                    instr_ORA(cpu, memory[address]);

                    emulate_6502_cycle(4);
                    cyc = 4;
                    break;

                // 0x0E
                // ASL abs
                case 0xE:
                    address = addr_abs(cpu);
                    instr_ASL(cpu, &memory[address]);

                    emulate_6502_cycle(6);
                    cyc = 6;
                    break;

                // 0x0F
                // SLO abs
                case 0xF:
                    address = addr_abs(cpu);
                    instr_SLO(cpu, &memory[address]);

                    emulate_6502_cycle(6);
                    cyc = 6;
                    break;
            }

            break;

        // 0x1X
        case 0x1:
            switch (instr & 0x0F) {
                // 0x10
                // BPL rel
                case 0x0:
                    instr_BPL(cpu);
                    break;

                // 0x11
                // ORA ind,Y
                case 0x1:
                    address = addr_ind_Y(cpu);
                    if ((address & 0xFF00) != ((address - cpu->Y) & 0xFF00)) {
                        cyc = 6;
                    } else {
                        cyc = 5;
                    }
                    instr_ORA(cpu, memory[address]);

                    emulate_6502_cycle(cyc);
                    
                    break;

                // 0x13
                // SLO ind,Y
                case 0x3:
                    address = addr_ind_Y(cpu);
                    instr_SLO(cpu, &memory[address]);

                    emulate_6502_cycle(8);
                    cyc = 8;
                    break;

                    // NOP zpg,X
                    case 0x4:
                        cpu->PC += 2;
                        emulate_6502_cycle(4);
                        cyc = 4;
                        break;

                // 0x15
                // ORA zpg,X
                case 0x5:
                    address = addr_zpg_X(cpu);
                    instr_ORA(cpu, memory[address]);

                    emulate_6502_cycle(4);
                    cyc = 4;
                    break;

                // 0x16
                // ASL zpg,X
                case 0x6:
                    address = addr_zpg_X(cpu);
                    instr_ASL(cpu, &memory[address]);

                    emulate_6502_cycle(6);
                    cyc = 6;
                    break;

                // 0x17
                // SLO zpg,X
                case 0x7:
                    address = addr_zpg_X(cpu);
                    instr_SLO(cpu, &memory[address]);

                    emulate_6502_cycle(6);
                    cyc = 6;
                    break;


                // 0x18
                // CLC impl
                case 0x8:
                    instr_CLC(cpu);
                    break;

                // 0x19
                // ORA abs,Y
                case 0x9:
                    address = addr_abs_Y(cpu);

                    if ((address & 0xFF00) != ((address - cpu->Y) & 0xFF00)) {
                        cyc = 5;
                    } else {
                        cyc = 4;
                    }

                    instr_ORA(cpu, memory[address]);

                    emulate_6502_cycle(cyc);
                    
                    break;

                case 0xA:
                    cpu->PC++;
                    emulate_6502_cycle(2);
                    cyc = 2;
                    break;

                // 0x1B
                // SLO abs,Y
                case 0xB:
                    address = addr_abs_Y(cpu);
                    instr_SLO(cpu, &memory[address]);

                    emulate_6502_cycle(7);
                    cyc = 7;
                    break;

                    case 0xc:
                        address = addr_abs_X(cpu);
                        if ((address & 0xFF00) != ((address - cpu->Y) & 0xFF00)) {
                            cyc = 5;
                        } else {
                            cyc = 4;
                        }
                        cpu->PC++;
                        emulate_6502_cycle(cyc);
                        
                        break;

                // 0x1D
                // ORA abs,X
                case 0xD:
                    address = addr_abs_X(cpu);
                    if ((address & 0xFF00) != ((address - cpu->X) & 0xFF00)) {
                        cyc = 5;
                    } else {
                        cyc = 4;
                    }

                    instr_ORA(cpu, memory[address]);

                    emulate_6502_cycle(cyc);
                    
                    break;

                // 0x1E
                // ASL abs,X
                case 0xE:
                    address = addr_abs_X(cpu);
                    instr_ASL(cpu, &memory[address]);

                    emulate_6502_cycle(7);
                    cyc = 7;
                    break;

                // 0x1F
                // SLO abs,X
                case 0xF:
                    address = addr_abs_X(cpu);
                    instr_SLO(cpu, &memory[address]);

                    emulate_6502_cycle(7);
                    cyc = 7;
                    break;
            }
            break;

        case 0x2:

            switch (instr & 0x0F) {
                // 0x20
                // JSR abs
                case 0x0:

                    address = addr_abs(cpu);
                    instr_JSR(cpu, address);

                    emulate_6502_cycle(6);
                    cyc = 6;
                    break;

                // 0x21
                // AND X,ind
                case 0x1:
                    address = addr_X_ind(cpu);
                    instr_AND(cpu, memory[address]);

                    emulate_6502_cycle(6);
                    cyc = 6;
                    break;

                // 0x23
                // RLA X,ind
                case 0x3:
                    address = addr_X_ind(cpu);
                    instr_RLA(cpu, &memory[address]);

                    emulate_6502_cycle(8);
                    cyc = 8;
                    break;

                // 0x24
                // BIT zpg
                case 0x4:
                    address = addr_zpg(cpu);
                    instr_BIT(cpu, memory[address]);

                    emulate_6502_cycle(3);
                    cyc = 3;
                    break;

                // 0x25
                // AND zpg
                case 0x5:
                    address = addr_zpg(cpu);
                    instr_AND(cpu, memory[address]);

                    emulate_6502_cycle(3);
                    cyc = 3;
                    break;

                // 0x26
                // ROL zpg
                case 0x6:
                    address = addr_zpg(cpu);
                    instr_ROL(cpu, &memory[address]);

                    emulate_6502_cycle(5);
                    cyc = 5;
                    break;

                // 0x27
                // RLA zpg
                case 0x7:
                    address = addr_zpg(cpu);
                    instr_RLA(cpu, &memory[address]);

                    emulate_6502_cycle(5);
                    cyc = 5;
                    break;

                // 0x28
                // PLP
                case 0x8:
                    instr_PLP(cpu);
                    break;

                // 0x29
                // AND #
                case 0x9:
                    address = addr_imm(cpu);
                    instr_AND(cpu, memory[address]);

                    emulate_6502_cycle(2);
                    cyc = 2;
                    break;

                // 0x2A
                // ROL A
                case 0xA:

                    instr_ROL(cpu, &cpu->A);
                    emulate_6502_cycle(2);
                    cyc = 2;
                    break;

                // 0x2C
                // BIT abs
                case 0xC:
                    address = addr_abs(cpu);
                    instr_BIT(cpu, memory[address]);

                    emulate_6502_cycle(4);
                    cyc = 4;
                    break;

                // 0x2D
                // AND abs
                case 0xD:

                    address = addr_abs(cpu);
                    instr_AND(cpu, memory[address]);

                    emulate_6502_cycle(4);
                    cyc = 4;
                    break;

                // 0x2E
                // ROL abs
                case 0xE:

                    address = addr_abs(cpu);
                    instr_ROL(cpu, &memory[address]);

                    emulate_6502_cycle(6);
                    cyc = 6;
                    break;

                // 0x2F
                // RLA abs
                case 0xF:
                    address = addr_abs(cpu);
                    instr_RLA(cpu, &memory[address]);

                    emulate_6502_cycle(6);
                    cyc = 6;
                    break;
                }

            break;

        case 0x3:
            switch (instr & 0x0F) {
                // 0x30
                // BMI rel
                case 0x0:
                    instr_BMI(cpu);
                    break;

                // 0x31
                // AND ind,Y
                case 0x1:
                    address = addr_ind_Y(cpu);
                    if ((address & 0xFF00) != ((address - cpu->Y) & 0xFF00)) {
                        cyc = 6;
                    } else {
                        cyc = 5;
                    }

                    instr_AND(cpu, memory[address]);

                    emulate_6502_cycle(cyc);
                    
                    break;

                // 0x33
                // RLA ind,Y
                case 0x3:
                    address = addr_ind_Y(cpu);
                    instr_RLA(cpu, &memory[address]);

                    emulate_6502_cycle(8);
                    cyc = 8;
                    break;

                    // NOP zpg,X
                    case 0x4:
                        cpu->PC += 2;
                        emulate_6502_cycle(4);
                        cyc = 4;
                        break;

                // 0x35
                // AND zpg,X
                case 0x5:
                    address = addr_zpg_X(cpu);
                    instr_AND(cpu, memory[address]);

                    emulate_6502_cycle(4);
                    cyc = 4;
                    break;

                // 0x36
                // ROL zpg, X
                case 0x6:
                    address = addr_zpg_X(cpu);
                    instr_ROL(cpu, &memory[address]);

                    emulate_6502_cycle(6);
                    cyc = 6;
                    break;

                // 0x36
                // RLA zpg, X
                case 0x7:
                    address = addr_zpg_X(cpu);
                    instr_RLA(cpu, &memory[address]);

                    emulate_6502_cycle(6);
                    cyc = 6;
                    break;

                // 0x38
                // SEC impl
                case 0x8:
                    // Set Carry Flag
                    instr_SEC(cpu);
                    break;

                // 0x39
                // AND abs,Y
                case 0x9:
                    address = addr_abs_Y(cpu);
                    if ((address & 0xFF00) != ((address - cpu->Y) & 0xFF00)) {
                        cyc = 5;
                    } else {
                        cyc = 4;
                    }
                    instr_AND(cpu, memory[address]);

                    emulate_6502_cycle(cyc);
                    
                    break;

                // NOP imp
                case 0xA:
                    cpu->PC++;
                    emulate_6502_cycle(2);
                    cyc = 2;
                    break;

                // 0x3B
                // ROL abs,Y
                case 0xB:
                    address = addr_abs_Y(cpu);
                    instr_RLA(cpu, &memory[address]);

                    emulate_6502_cycle(7);
                    cyc = 7;
                    break;

                    case 0xc:
                        address = addr_abs_X(cpu);
                        if ((address & 0xFF00) != ((address - cpu->Y) & 0xFF00)) {
                            cyc = 5;
                        } else {
                            cyc = 4;
                        }
                        cpu->PC++;
                        emulate_6502_cycle(cyc);
                        
                        break;

                // 0x3D
                // AND abs,X
                case 0xD:
                    address = addr_abs_X(cpu);
                    if ((address & 0xFF00) != ((address - cpu->X) & 0xFF00)) {
                        cyc = 5;
                    } else {
                        cyc = 4;
                    }

                    instr_AND(cpu, memory[address]);
                    emulate_6502_cycle(cyc);
                    
                    break;

                // 0x3E
                // ROL abs,X
                case 0xE:
                    address = addr_abs_X(cpu);
                    instr_ROL(cpu, &memory[address]);

                    emulate_6502_cycle(7);
                    cyc = 7;
                    break;

                // 0x3F
                // ROL abs,X
                case 0xF:
                    address = addr_abs_X(cpu);
                    instr_RLA(cpu, &memory[address]);

                    emulate_6502_cycle(7);
                    cyc = 7;
                    break;

                }
            break;

        case 0x4:

            switch (instr & 0x0F) {
                // 0x40
                // RTI impl
                case 0x0:
                    instr_RTI(cpu);
                    break;

                // 0x41
                // EOR X,ind
                case 0x1:
                    address = addr_X_ind(cpu);
                    instr_EOR(cpu, memory[address]);

                    emulate_6502_cycle(6);
                    cyc = 6;
                    break;

                // 0x43
                // SRE X,ind
                case 0x3:
                    address = addr_X_ind(cpu);
                    instr_SRE(cpu, &memory[address]);

                    emulate_6502_cycle(8);
                    cyc = 8;
                    break;

                    // NOP zpg
                    case 0x4:
                        cpu->PC += 2;
                        emulate_6502_cycle(3);
                        cyc = 3;
                        break;

                // 0x45
                // EOR zpg
                case 0x5:
                    address = addr_zpg(cpu);
                    instr_EOR(cpu, memory[address]);

                    emulate_6502_cycle(3);
                    cyc = 3;
                    break;

                // 0x46
                // LSR zpg
                case 0x6:
                    address = addr_zpg(cpu);
                    instr_LSR(cpu, &memory[address]);

                    emulate_6502_cycle(5);
                    cyc = 5;
                    break;

                // 0x47
                // SRE zpg
                case 0x7:
                    address = addr_zpg(cpu);
                    instr_SRE(cpu, &memory[address]);

                    emulate_6502_cycle(5);
                    cyc = 5;
                    break;

                // 0x48
                // PHA impl
                case 0x8:
                    instr_PHA(cpu);
                    break;

                // 0x49
                // EOR #
                case 0x9:

                    address = addr_imm(cpu);
                    instr_EOR(cpu, memory[address]);

                    emulate_6502_cycle(2);
                    cyc = 2;
                    break;

                // 0x4A
                // LSR A
                case 0xA:

                    instr_LSR(cpu, &cpu->A);

                    emulate_6502_cycle(2);
                    cyc = 2;
                    break;

                // 0x4B
                // SRE abs,Y
                case 0xB:
                    address = addr_abs_Y(cpu);
                    instr_SRE(cpu, &memory[address]);

                    emulate_6502_cycle(7);
                    cyc = 6;
                    break;

                // 0x4C
                // JMP abs
                case 0xC:

                    address = addr_abs(cpu);
                    instr_JMP(cpu, address);

                    emulate_6502_cycle(3);
                    cyc = 3;
                    break;

                // 0x4D
                // EOR abs
                case 0xD:

                    address = addr_abs(cpu);
                    instr_EOR(cpu, memory[address]);

                    emulate_6502_cycle(4);
                    cyc = 4;
                    break;

                // 0x4E
                // LSR abs
                case 0xE:
                    address = addr_abs(cpu);
                    instr_LSR(cpu, &memory[address]);

                    emulate_6502_cycle(6);
                    cyc = 6;
                    break;

                // 0x4F
                // SRE abs
                case 0xF:
                    address = addr_abs(cpu);
                    instr_SRE(cpu, &memory[address]);

                    emulate_6502_cycle(6);
                    cyc = 6;
                    break;

                }

            break;

        case 0x5:

            switch (instr & 0x0F) {
                // 0x50
                // BVC rel
                case 0x0:

                    instr_BVC(cpu);

                    break;

                // 0x51
                // EOR ind,Y
                case 0x1:

                    address = addr_ind_Y(cpu);
                    if ((address & 0xFF00) != ((address - cpu->Y) & 0xFF00)) {
                        cyc = 6;
                    } else {
                        cyc = 5;
                    }

                    instr_EOR(cpu, memory[address]);

                    emulate_6502_cycle(cyc);
                    
                    break;

                // 0x53
                // SRE ind,%
                case 0x3:
                    address = addr_ind_Y(cpu);
                    instr_SRE(cpu, &memory[address]);

                    emulate_6502_cycle(8);
                    cyc = 8;
                    break;

                    // NOP zpg,X
                    case 0x4:
                        cpu->PC += 2;
                        emulate_6502_cycle(4);
                        cyc = 4;
                        break;

                // 0x55
                // EOR zpg,X
                case 0x5:

                    address = addr_zpg_X(cpu);
                    instr_EOR(cpu, memory[address]);

                    emulate_6502_cycle(4);
                    cyc = 4;
                    break;

                // 0x56
                // LSR zpg, X
                case 0x6:

                    address = addr_zpg_X(cpu);
                    instr_LSR(cpu, &memory[address]);

                    emulate_6502_cycle(6);
                    cyc = 6;
                    break;

                // 0x57
                // SRE zpg,X
                case 0x7:
                    address = addr_zpg_X(cpu);
                    instr_SRE(cpu, &memory[address]);

                    emulate_6502_cycle(5);
                    cyc = 5;
                    break;

                // 0x58
                // CLI impl
                case 0x8:
                    instr_CLI(cpu);
                    break;

                // 0x59
                // EOR abs,Y
                case 0x9:

                    address = addr_abs_Y(cpu);
                    if ((address & 0xFF00) != ((address - cpu->Y) & 0xFF00)) {
                        cyc = 5;
                    } else {
                        cyc = 4;
                    }
                    instr_EOR(cpu, memory[address]);
                    emulate_6502_cycle(cyc);
                    
                    break;

                // NOP imp
                case 0xA:
                    cpu->PC++;
                    emulate_6502_cycle(2);
                    cyc = 2;
                    break;

                    case 0xc:
                        address = addr_abs_X(cpu);
                        if ((address & 0xFF00) != ((address - cpu->Y) & 0xFF00)) {
                            cyc = 5;
                        } else {
                            cyc = 4;
                        }
                        cpu->PC++;
                        emulate_6502_cycle(cyc);
                        
                        break;

                // 0x5B
                // SRE abs,Y
                case 0xB:
                    address = addr_abs_Y(cpu);
                    instr_SRE(cpu, &memory[address]);

                    emulate_6502_cycle(7);
                    cyc = 7;
                    break;

                // 0x5D
                // EOR abs,X
                case 0xD:

                    address = addr_abs_X(cpu);
                    if ((address & 0xFF00) != ((address - cpu->X) & 0xFF00)) {
                        cyc = 5;
                    } else {
                        cyc = 4;
                    }

                    instr_EOR(cpu, memory[address]);
                    emulate_6502_cycle(cyc);
                    
                    break;

                // 0x5E
                // LSR abs,X
                case 0xE:

                    address = addr_abs_X(cpu);
                    instr_LSR(cpu, &memory[address]);

                    emulate_6502_cycle(7);
                    cyc = 7;
                    break;

                // 0x5F
                // SRE abs,X
                case 0xF:
                    address = addr_abs_X(cpu);
                    instr_SRE(cpu, &memory[address]);

                    emulate_6502_cycle(7);
                    cyc = 7;
                    break;
                }

            break;

        case 0x6:
            switch (instr & 0x0F) {
                // 0x60
                // RTS impl
                case 0x0:
                    instr_RTS(cpu);
                    break;

                // 0x61
                // ADC X,ind
                case 0x1:
                    address = addr_X_ind(cpu);
                    instr_ADC(cpu, memory[address]);

                    emulate_6502_cycle(6);
                    cyc = 6;
                    break;

                // 0x63
                // RRA X,ind
                case 0x3:
                    address = addr_X_ind(cpu);
                    instr_RRA(cpu, &memory[address]);

                    emulate_6502_cycle(8);
                    cyc = 8;
                    break;

                    // NOP zpg
                    case 0x4:
                        cpu->PC += 2;
                        emulate_6502_cycle(3);
                        cyc = 3;
                        break;

                // 0x65
                // ADC zpg
                case 0x5:
                    address = addr_zpg(cpu);
                    instr_ADC(cpu, memory[address]);

                    emulate_6502_cycle(3);
                    cyc = 3;
                    break;

                // 0x66
                // ROR zpg
                case 0x6:
                    address = addr_zpg(cpu);
                    instr_ROR(cpu, &memory[address]);

                    emulate_6502_cycle(5);
                    cyc = 5;
                    break;

                // 0x67
                // RRA zpg
                case 0x7:
                    address = addr_zpg(cpu);
                    instr_RRA(cpu, &memory[address]);

                    emulate_6502_cycle(5);
                    cyc = 5;
                    break;

                // 0x68
                // PLA impl
                case 0x8:
                    instr_PLA(cpu);
                    break;

                // 0x69
                // ADC #
                case 0x9:

                    address = addr_imm(cpu);
                    instr_ADC(cpu, memory[address]);

                    emulate_6502_cycle(2);
                    cyc = 2;
                    break;

                // 0x6A
                // ROR A
                case 0xA:
                instr_ROR(cpu, &cpu->A);
                    emulate_6502_cycle(2);
                    cyc = 2;
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
                        // printf("JMP ind with page crossing wrapping!\n");
                        // printf("Retrieving higher byte from: %x\n",  (address & 0xF00));
                        // printf("PC: %x\n", cpu->PC);
                        // sleep(5);
                    } else {
                        cpu->PC = memory[address + 1] << 8 | memory[address];
                    }
                    emulate_6502_cycle(5);
                    cyc = 5;
                    break;

                // 0x6D
                // ADC abs
                case 0xD:

                    address = addr_abs(cpu);
                    instr_ADC(cpu, memory[address]);

                    emulate_6502_cycle(4);
                    cyc = 4;
                    break;

                // 0x6E
                // ROR abs
                case 0xE:
                    address = addr_abs(cpu);
                    instr_ROR(cpu, &memory[address]);

                    emulate_6502_cycle(6);
                    cyc = 6;
                    break;

                // 0x6F
                // RRA abs
                case 0xF:
                    address = addr_abs(cpu);
                    instr_RRA(cpu, &memory[address]);

                    emulate_6502_cycle(6);
                    cyc = 6;
                    break;
                }

            break;

        case 0x7:

            switch (instr & 0x0F) {
                // 0x70
                // BVS rel
                case 0x0:
                    instr_BVS(cpu);
                    break;

                // 0x71
                // ADC ind,Y
                case 0x1:
                    address = addr_ind_Y(cpu);
                    if ((address & 0xFF00) != ((address - cpu->Y) & 0xFF00)) {
                        cyc = 6;
                    } else {
                        cyc = 5;
                    }
                    instr_ADC(cpu, memory[address]);

                    emulate_6502_cycle(cyc);
                    
                    break;

                // 0x73
                // RRA ind,Y
                case 0x3:
                    address = addr_ind_Y(cpu);
                    instr_RRA(cpu, &memory[address]);

                    emulate_6502_cycle(8);
                    cyc = 8;
                    break;

                    // NOP zpg,X
                    case 0x4:
                        cpu->PC += 2;
                        emulate_6502_cycle(4);
                        cyc = 4;
                        break;

                // 0x75
                // ADC zpg,X
                case 0x5:
                    address = addr_zpg_X(cpu);
                    instr_ADC(cpu, memory[address]);

                    emulate_6502_cycle(4);
                    cyc = 4;
                    break;

                // 0x76
                // ROR zpg,X
                case 0x6:
                    address = addr_zpg_X(cpu);
                    instr_ROR(cpu, &memory[address]);

                    emulate_6502_cycle(6);
                    cyc = 6;
                    break;

                // 0x77
                // RRA zpg,X
                case 0x7:
                    address = addr_zpg_X(cpu);
                    instr_RRA(cpu, &memory[address]);

                    emulate_6502_cycle(6);
                    cyc = 6;
                    break;

                // 0x78
                // SEI impl
                case 0x8:
                    instr_SEI(cpu);
                    break;

                // 0x79
                // ADC abs,Y
                case 0x9:
                    address = addr_abs_Y(cpu);
                    if ((address & 0xFF00) != ((address - cpu->Y) & 0xFF00)) {
                        cyc = 5;
                    } else {
                        cyc = 4;
                    }
                    instr_ADC(cpu, memory[address]);

                    emulate_6502_cycle(cyc);
                    
                    break;

                // NOP imp
                case 0xA:
                    cpu->PC++;
                    emulate_6502_cycle(2);
                    cyc = 2;
                    break;

                // 0x7B
                // RRA abs,Y
                case 0xB:
                    address = addr_abs_X(cpu);
                    instr_RRA(cpu, &memory[address]);

                    emulate_6502_cycle(7);
                    cyc = 7;
                    break;

                    case 0xc:
                        address = addr_abs_X(cpu);
                        if ((address & 0xFF00) != ((address - cpu->Y) & 0xFF00)) {
                            cyc = 5;
                        } else {
                            cyc = 4;
                        }
                        cpu->PC++;
                        emulate_6502_cycle(cyc);
                        
                        break;

                // 0x7D
                // ADC abs,X
                case 0xD:
                    address = addr_abs_X(cpu);
                    if ((address & 0xFF00) != ((address - cpu->X) & 0xFF00)) {
                        cyc = 5;
                    } else {
                        cyc = 4;
                    }
                    instr_ADC(cpu, memory[address]);

                    emulate_6502_cycle(cyc);
                    
                    break;

                // 0x7E
                // ROR abs,X
                case 0xE:
                    address = addr_abs_X(cpu);
                    instr_ROR(cpu, &memory[address]);

                    emulate_6502_cycle(7);
                    cyc = 7;
                    break;

                // 0x7F
                // RRA abs,X
                case 0xF:
                    address = addr_abs_X(cpu);
                    instr_RRA(cpu, &memory[address]);

                    emulate_6502_cycle(7);
                    cyc = 7;
                    break;
                }

            break;

        case 0x8:

            switch (instr & 0x0F) {
                // 0x80
                // NOP imm
                case 0x0:
                    cpu->PC += 2;
                    emulate_6502_cycle(2);
                    cyc = 2;
                    break;

                // 0x81
                // STA X,ind
                case 0x1:
                    address = addr_X_ind(cpu);
                    instr_STA(cpu, address);

                    emulate_6502_cycle(6);
                    cyc = 6;
                    break;

                // NOP imm
                case 0x2:
                    cpu->PC += 2;
                    emulate_6502_cycle(2);
                    cyc = 2;
                    break;

                // 0x83
                // SAX X,ind
                case 0x3:
                    address = addr_X_ind(cpu);
                    instr_SAX(cpu, address);

                    emulate_6502_cycle(6);
                    cyc = 6;
                    break;

                // 0x84
                // STY zpg
                case 0x4:
                    address = addr_zpg(cpu);
                    instr_STY(cpu, address);

                    emulate_6502_cycle(3);
                    cyc = 3;
                    break;

                // 0x85
                // STA zpg
                case 0x5:
                    address = addr_zpg(cpu);
                    instr_STA(cpu, address);

                    emulate_6502_cycle(3);
                    cyc = 3;
                    break;

                // 0x86
                // STX zpg
                case 0x6:
                    address = addr_zpg(cpu);
                    instr_STX(cpu, address);

                    emulate_6502_cycle(3);
                    cyc = 3;
                    break;

                // 0x87
                // SAX zpg
                case 0x7:
                    address = addr_zpg(cpu);
                    instr_SAX(cpu, address);

                    emulate_6502_cycle(3);
                    cyc = 3;
                    break;

                // 0x88
                // DEY impl
                case 0x8:
                    instr_DEY(cpu);
                    break;

                // NOP imm
                case 0x9:
                    cpu->PC += 2;
                    emulate_6502_cycle(2);
                    cyc = 2;
                    break;

                // 0x8A
                // TXA impl
                case 0xA:
                    instr_TXA(cpu);
                    break;

                // 0x8C
                // STY abs
                case 0xC:
                    address = addr_abs(cpu);
                    instr_STY(cpu, address);

                    emulate_6502_cycle(4);
                    cyc = 4;
                    break;

                // 0x8D
                // STA abs
                case 0xD:
                    address = addr_abs(cpu);
                    instr_STA(cpu, address);

                    emulate_6502_cycle(4);
                    cyc = 4;
                    break;

                // 0x8E
                // STX abs
                case 0xE:
                    address = addr_abs(cpu);
                    instr_STX(cpu, address);

                    emulate_6502_cycle(4);
                    cyc = 4;
                    break;

                // 0x8F
                // SAX abs
                case 0xF:
                    address = addr_abs(cpu);
                    instr_SAX(cpu, address);

                    emulate_6502_cycle(4);
                    cyc = 4;
                    break;
            }

            break;

        case 0x9:
            switch (instr & 0x0F) {
                // 0x90
                // BCC rel
                case 0x0:

                    instr_BCC(cpu);
                    break;

                // 0x91
                // STA ind,Y
                case 0x1:
                    address = addr_ind_Y(cpu);
                    instr_STA(cpu, address);

                    emulate_6502_cycle(6);
                    cyc = 6;
                    break;

                // 0x94
                // STY zpg,X
                case 0x4:
                    address = addr_zpg_X(cpu);
                    instr_STY(cpu, address);

                    emulate_6502_cycle(4);
                    cyc = 4;
                    break;

                // 0x95
                // STA zpg,X
                case 0x5:
                    address = addr_zpg_X(cpu);
                    instr_STA(cpu, address);

                    emulate_6502_cycle(4);
                    cyc = 4;
                    break;

                // 0x96
                // STX zpg,Y
                case 0x6:
                    address = addr_zpg_Y(cpu);
                    instr_STX(cpu, address);

                    emulate_6502_cycle(4);
                    cyc = 4;
                    break;

                // 0x97
                // SAX zpg,Y
                case 0x7:
                    address = addr_zpg_Y(cpu);
                    instr_SAX(cpu, address);

                    emulate_6502_cycle(4);
                    cyc = 4;
                    break;

                // 0x98
                // TYA impl
                case 0x8:
                    // Transfer Index Y to Accumulator
                    instr_TYA(cpu);
                    break;

                // 0x99
                // STA abs,Y
                case 0x9:
                    // Increment to get the lower byte
                    address = addr_abs_Y(cpu);
                    instr_STA(cpu, address);

                    emulate_6502_cycle(5);
                    cyc = 5;
                    break;

                // 0x9A
                // TXS impl
                case 0xA:
                    instr_TXS(cpu);
                    break;

                // 0x9D
                // STA abs,X
                case 0xD:
                    address = addr_abs_X(cpu);
                    instr_STA(cpu, address);
                    emulate_6502_cycle(5);
                    cyc = 5;
                    break;
            }

            break;

        case 0xA:

            switch (instr & 0x0F) {

                // 0xA0
                // LDY #
                case 0x0:
                    // Load Index Y with Memory
                    address = addr_imm(cpu);
                    instr_LDY(cpu, address);

                    emulate_6502_cycle(2);
                    cyc = 2;
                    break;

                // 0xA1
                // LDA X,ind
                case 0x1:
                    address = addr_X_ind(cpu);
                    instr_LDA(cpu, address);
                    
                    emulate_6502_cycle(6);
                    cyc = 6;
                    break;

                // 0xA2
                // LDX #
                case 0x2:
                    address = addr_imm(cpu);
                    instr_LDX(cpu, address);

                    emulate_6502_cycle(2);
                    cyc = 2;
                    break;

                // 0xA2
                // LDX #
                case 0x3:
                    address = addr_X_ind(cpu);
                    instr_LAX(cpu, memory[address]);

                    emulate_6502_cycle(6);
                    cyc = 6;
                    break;

                // 0xA4
                // LDY zpg
                case 0x4:
                    address = addr_zpg(cpu);
                    instr_LDY(cpu, address);

                    emulate_6502_cycle(3);
                    cyc = 3;
                    break;

                // 0xA5
                // LDA zpg
                case 0x5:
                    address = addr_zpg(cpu);
                    instr_LDA(cpu, address);

                    emulate_6502_cycle(3);
                    cyc = 3;
                    break;

                // 0xA6
                // LDX zpg
                case 0x6:
                    address = addr_zpg(cpu);
                    instr_LDX(cpu, address);

                    emulate_6502_cycle(3);
                    cyc = 3;
                    break;

                // Ilegal Opcode 0xA7
                // LAX zpg
                case 0x7:
                    address = addr_zpg(cpu);
                    instr_LAX(cpu, memory[address]);

                    emulate_6502_cycle(3);
                    cyc = 3;
                    break;

                // 0xA8
                // TAY impl
                case 0x8:
                    instr_TAY(cpu);
                    break;

                // 0xA9
                // LDA #
                case 0x9:
                    address = addr_imm(cpu);
                    instr_LDA(cpu, address);

                    emulate_6502_cycle(2);
                    cyc = 2;
                    break;

                // 0xAA
                // TAX impl
                case 0xA:
                    instr_TAX(cpu);
                    break;

                // 0xAC
                // LDY abs
                case 0xC:
                    address = addr_abs(cpu);
                    instr_LDY(cpu, address);

                    emulate_6502_cycle(4);
                    cyc = 4;
                    break;

                // 0xAD
                // LDA abs
                case 0xD:
                    address = addr_abs(cpu);
                    LOG("INSTR: LDA ABS ADDR: %x\n", address);
                    instr_LDA(cpu, address);

                    emulate_6502_cycle(4);
                    cyc = 4;
                    break;

                // 0xAE
                // LDX abs
                case 0xE:
                    address = addr_abs(cpu);
                    instr_LDX(cpu, address);

                    emulate_6502_cycle(4);
                    cyc = 4;
                    break;

                // 0xAF
                // LAX abs
                case 0xF:

                    address = addr_abs(cpu);
                    instr_LAX(cpu, memory[address]);

                    emulate_6502_cycle(4);
                    cyc = 4;
                    break;

            }

            break;

        case 0xB:

            switch (instr & 0x0F) {
                // 0xB0
                // BCS rel
                case 0x0:

                    instr_BCS(cpu);
                    break;

                // 0xB1
                // LDA ind,Y
                case 0x1:
                    address = addr_ind_Y(cpu);
                    if ((address & 0xFF00) != ((address - cpu->Y) & 0xFF00)) {
                        cyc = 6;
                    } else {
                        cyc = 5;
                    }
                    instr_LDA(cpu, address);

                    emulate_6502_cycle(cyc);
                    
                    break;

                // 0xB3
                // LAX ind,Y
                case 0x3:
                    address = addr_ind_Y(cpu);
                    if ((address & 0xFF00) != ((address - cpu->Y) & 0xFF00)) {
                        cyc = 6;
                    } else {
                        cyc = 5;
                    }
                    instr_LAX(cpu, memory[address]);

                    emulate_6502_cycle(cyc);
                    
                    break;

                // 0xB4
                // LDY zpg,X
                case 0x4:
                    address = addr_zpg_X(cpu);
                    instr_LDY(cpu, address);

                    emulate_6502_cycle(4);
                    cyc = 4;
                    break;

                // 0xB5
                // LDA zpg,X
                case 0x5:
                    address = addr_zpg_X(cpu);
                    instr_LDA(cpu, address);

                    emulate_6502_cycle(4);
                    cyc = 4;
                    break;

                // 0xB6
                // LDX zpg,Y
                case 0x6:
                    address = addr_zpg_Y(cpu);
                    instr_LDX(cpu, address);

                    emulate_6502_cycle(4);
                    cyc = 4;
                    break;

                // Illegal opcode 0xB7
                // LAX zpg, Y
                case 0x7:
                    address = addr_zpg_Y(cpu);
                    instr_LAX(cpu, memory[address]);

                    emulate_6502_cycle(4);
                    cyc = 4;
                    break;

                // 0xB8
                // CLV impl
                case 0x8:
                    instr_CLV(cpu);
                    break;

                // 0xB9
                // LDA abs,Y
                case 0x9:
                    address = addr_abs_Y(cpu);
                    if ((address & 0xFF00) != ((address - cpu->Y) & 0xFF00)) {
                        cyc = 5;
                    } else {
                        cyc = 4;
                    }
                    instr_LDA(cpu, address);

                    emulate_6502_cycle(cyc);
                    
                    break;

                // 0xBA
                // TSX impl
                case 0xA:
                    instr_TSX(cpu);
                    break;

                // 0xBC
                // LDY abs,X
                case 0xC:
                    address = addr_abs_X(cpu);
                    if ((address & 0xFF00) != ((address - cpu->X) & 0xFF00)) {
                        cyc = 5;
                    } else {
                        cyc = 4;
                    }
                    instr_LDY(cpu, address);

                    emulate_6502_cycle(cyc);
                    
                    break;

                // 0xBD
                // LDA abs,X
                case 0xD:

                    address = addr_abs_X(cpu);
                    if ((address & 0xFF00) != ((address - cpu->X) & 0xFF00)) {
                        cyc = 5;
                    } else {
                        cyc = 4;
                    }

                    instr_LDA(cpu, address);

                    emulate_6502_cycle(cyc);
                    
                    break;

                // 0xBE
                // LDX abs,Y
                case 0xE:
                    address = addr_abs_Y(cpu);
                    if ((address & 0xFF00) != ((address - cpu->X) & 0xFF00)) {
                        cyc = 5;
                    } else {
                        cyc = 4;
                    }
                    instr_LDX(cpu, address);

                    emulate_6502_cycle(cyc);
                    
                    break;

                // 0xBF
                // LAX abs,Y
                case 0xF:
                    address = addr_abs_Y(cpu);
                    if ((address & 0xFF00) != ((address - cpu->X) & 0xFF00)) {
                        cyc = 5;
                    } else {
                        cyc = 4;
                    }
                    instr_LAX(cpu, memory[address]);

                    emulate_6502_cycle(cyc);
                    
                    break;

            }

            break;

        case 0xC:

            switch (instr & 0x0F) {

                // 0xC0
                // CPY #
                case 0x0:
                    address = addr_imm(cpu);
                    instr_CPY(cpu, memory[address]);

                    emulate_6502_cycle(2);
                    cyc = 2;
                    break;

                // 0xC1
                // CMP X,ind
                case 0x1:
                    address = addr_X_ind(cpu);
                    instr_CMP(cpu, memory[address]);

                    emulate_6502_cycle(6);
                    cyc = 6;
                    break;


                // NOP imm
                case 0x2:
                    cpu->PC += 2;
                    emulate_6502_cycle(2);
                    cyc = 2;
                    break;

                // 0xC3
                // DCP X,ind
                case 0x3:
                    address = addr_X_ind(cpu);
                    instr_DCP(cpu, &memory[address]);

                    emulate_6502_cycle(8);
                    cyc = 8;
                    break;

                // 0xC4
                // CPY zpg
                case 0x4:
                    address = addr_zpg(cpu);
                    instr_CPY(cpu, memory[address]);

                    emulate_6502_cycle(3);
                    cyc = 3;
                    break;

                // 0xC5
                // CMP zpg
                case 0x5:
                    address =  addr_zpg(cpu);
                    instr_CMP(cpu, memory[address]);

                    emulate_6502_cycle(3);
                    cyc = 3;
                    break;

                // 0xC6
                // DEC zpg
                case 0x6:
                    address = addr_zpg(cpu);
                    instr_DEC(cpu, &memory[address]);

                    emulate_6502_cycle(5);
                    cyc = 5;
                    break;

                // 0xC7
                // DCP zpg
                case 0x7:
                    address = addr_zpg(cpu);
                    instr_DCP(cpu, &memory[address]);

                    emulate_6502_cycle(5);
                    cyc = 5;
                    break;

                // 0xC8
                // INY impl
                case 0x8:
                    instr_INY(cpu);
                    break;

                // 0xC9
                // CMP #
                case 0x9:
                    address = addr_imm(cpu);
                    instr_CMP(cpu, memory[address]);

                    emulate_6502_cycle(2);
                    cyc = 2;
                    break;

                // 0xCA
                // DEX impl
                case 0xA:
                    instr_DEX(cpu);
                    break;

                // 0xCC
                // CPY abs
                case 0xC:
                    address = addr_abs(cpu);
                    instr_CPY(cpu, memory[address]);

                    emulate_6502_cycle(4);
                    cyc = 4;
                    break;

                // 0xCD
                // CMP abs
                case 0xD:
                    address = addr_abs(cpu);
                    instr_CMP(cpu, memory[address]);

                    emulate_6502_cycle(4);
                    cyc = 4;
                    break;

                // 0xCE
                // DEC abs
                case 0xE:
                    address = addr_abs(cpu);
                    instr_DEC(cpu, &memory[address]);

                    emulate_6502_cycle(6);
                    cyc = 6;
                    break;

                // 0xCF
                // DCP abs
                case 0xF:
                    address = addr_abs(cpu);
                    instr_DCP(cpu, &memory[address]);

                    emulate_6502_cycle(6);
                    cyc = 6;
                    break;
            }
            break;

        case 0xD:

            switch (instr & 0x0F) {

                // 0xD0
                // BNE rel
                case 0x0:
                    instr_BNE(cpu);
                    break;

                    // 0xD1
                    // CMP ind,Y
                    case 0x1:
                        address = addr_ind_Y(cpu);
                        if ((address & 0xFF00) != ((address - cpu->Y) & 0xFF00)) {
                            cyc = 6;
                        } else {
                            cyc = 5;
                        }
                        instr_CMP(cpu, memory[address]);

                        emulate_6502_cycle(cyc);
                        
                        break;

                    // 0xD3
                    // DCP ind,Y
                    case 0x3:
                        address = addr_ind_Y(cpu);
                        instr_DCP(cpu, &memory[address]);

                        emulate_6502_cycle(8);
                        cyc = 8;
                        break;

                        // NOP zpg,X
                        case 0x4:
                            cpu->PC += 2;
                            emulate_6502_cycle(4);
                            cyc = 4;
                            break;

                    // 0xD5
                    // CMP zpg,X
                    case 0x5:
                        address = addr_zpg_X(cpu);
                        instr_CMP(cpu, memory[address]);

                        emulate_6502_cycle(4);
                        cyc = 4;
                        break;

                    // 0xD7
                    // DCP zpg,X
                    case 0x7:
                        address = addr_zpg_X(cpu);
                        instr_DCP(cpu, &memory[address]);

                        emulate_6502_cycle(6);
                        cyc = 6;
                        break;

                    // 0xD6
                    // DEC zpg,X
                    case 0x6:
                        address = addr_zpg_X(cpu);
                        instr_DEC(cpu, &memory[address]);

                        emulate_6502_cycle(6);
                        cyc = 6;
                        break;

                    // 0xD8
                    // CLD impl
                    case 0x8:
                        instr_CLD(cpu);
                        break;

                    // 0xD9
                    // CMP abs,Y
                    case 0x9:
                        address = addr_abs_Y(cpu);
                        if ((address & 0xFF00) != ((address - cpu->Y) & 0xFF00)) {
                            cyc = 5;
                        } else {
                            cyc = 4;
                        }
                        instr_CMP(cpu, memory[address]);

                        emulate_6502_cycle(cyc);
                        
                        break;

                    // NOP imp
                    case 0xA:
                        cpu->PC++;
                        emulate_6502_cycle(2);
                        cyc = 2;
                        break;

                    // 0xDB
                    // DCP abs,Y
                    case 0xB:
                        address = addr_abs_Y(cpu);
                        instr_DCP(cpu, &memory[address]);

                        emulate_6502_cycle(7);
                        cyc = 7;
                        break;

                        case 0xc:
                            address = addr_abs_X(cpu);
                            if ((address & 0xFF00) != ((address - cpu->Y) & 0xFF00)) {
                                cyc = 5;
                            } else {
                                cyc = 4;
                            }
                            cpu->PC++;
                            emulate_6502_cycle(cyc);
                            
                            break;

                    // 0xDD
                    // CMP abs,X
                    case 0xD:
                        address = addr_abs_X(cpu);
                        if ((address & 0xFF00) != ((address - cpu->X) & 0xFF00)) {
                            cyc = 5;
                        } else {
                            cyc = 4;
                        }
                        instr_CMP(cpu, memory[address]);

                        emulate_6502_cycle(cyc);
                        
                        break;

                    // 0xDE
                    // DEC abs,X
                    case 0xE:
                        address = addr_abs_X(cpu);
                        instr_DEC(cpu, &memory[address]);

                        emulate_6502_cycle(7);
                        cyc = 7;
                        break;

                    // 0xDF
                    // DCP abs,X
                    case 0xF:
                        address = addr_abs_X(cpu);
                        instr_DCP(cpu, &memory[address]);

                        emulate_6502_cycle(7);
                        cyc = 7;
                        break;
            }

            break;

        case 0xE:

            switch (instr & 0x0F) {

                // 0xE0
                // CPX #
                case 0x0:
                    address = addr_imm(cpu);
                    instr_CPX(cpu, memory[address]);

                    emulate_6502_cycle(2);
                    cyc = 2;
                    break;

                // 0xE1
                // SBC X,ind
                case 0x1:
                    address = addr_X_ind(cpu);
                    instr_SBC(cpu, memory[address]);

                    emulate_6502_cycle(6);
                    cyc = 6;
                    break;

                // NOP imm
                case 0x2:
                    cpu->PC += 2;
                    emulate_6502_cycle(2);
                    cyc = 2;
                    break;

                // 0xE3
                // ISC X,ind
                case 0x3:
                    address = addr_X_ind(cpu);
                    instr_ISC(cpu, &memory[address]);

                    emulate_6502_cycle(8);
                    cyc = 8;
                    break;

                // 0xE4
                // CPX zpg
                case 0x4:
                    address = addr_zpg(cpu);
                    instr_CPX(cpu, memory[address]);

                    emulate_6502_cycle(3);
                    cyc = 3;
                    break;

                // 0xE5
                // SBC zpg
                case 0x5:
                    address = addr_zpg(cpu);
                    instr_SBC(cpu, memory[address]);

                    emulate_6502_cycle(3);
                    cyc = 3;
                    break;

                // 0xE6
                // INC zpg
                case 0x6:
                    address = addr_zpg(cpu);
                    instr_INC(cpu, &memory[address]);

                    emulate_6502_cycle(5);
                    cyc = 5;
                    break;

                // 0xE7
                // ISC zpg
                case 0x7:
                    address = addr_zpg(cpu);
                    instr_ISC(cpu, &memory[address]);

                    emulate_6502_cycle(5);
                    cyc = 5;
                    break;

                // 0xE8
                // INX impl
                case 0x8:
                    instr_INX(cpu);
                    break;

                // 0xE9
                // SBC #
                case 0x9:
                    address = addr_imm(cpu);
                    instr_SBC(cpu, memory[address]);

                    emulate_6502_cycle(2);
                    cyc = 2;
                    break;

                // 0xEA
                // NOP impl
                case 0xA:
                    cpu->PC++;
                    emulate_6502_cycle(2);
                    cyc = 2;
                    break;

                // 0xEB
                // USBC #
                case 0xB:
                    address = addr_imm(cpu);
                    instr_SBC(cpu, memory[address]);

                    emulate_6502_cycle(2);
                    cyc = 2;
                    break;

                // 0xEC
                // CPX abs
                case 0xC:
                    address = addr_abs(cpu);
                    instr_CPX(cpu, memory[address]);

                    emulate_6502_cycle(4);
                    cyc = 4;
                    break;

                // 0xED
                // SBC abs
                case 0xD:
                    address = addr_abs(cpu);
                    instr_SBC(cpu, memory[address]);

                    emulate_6502_cycle(4);
                    cyc = 4;
                    break;

                // 0xEE
                // INC abs
                case 0xE:
                    address = addr_abs(cpu);
                    instr_INC(cpu, &memory[address]);

                    emulate_6502_cycle(6);
                    cyc = 6;
                    break;

                // 0xE7
                // ISC abs
                case 0xF:
                    address = addr_abs(cpu);
                    instr_ISC(cpu, &memory[address]);

                    emulate_6502_cycle(6);
                    cyc = 6;
                    break;
            }
            break;

        case 0xF:

            switch (instr & 0x0F) {

                // 0xF0
                // BEQ rel
                case 0x0:
                    instr_BEQ(cpu);
                    break;

                // 0xF1
                // SBC ind,Y
                case 0x1:
                    address = addr_ind_Y(cpu);
                    if ((address & 0xFF00) != ((address - cpu->Y) & 0xFF00)) {
                        cyc = 6;
                    } else {
                        cyc = 5;
                    }
                    instr_SBC(cpu, memory[address]);

                    emulate_6502_cycle(cyc);
                    
                    break;

                // 0xF3
                // ISC ind,Y
                case 0x3:
                    address = addr_ind_Y(cpu);
                    instr_ISC(cpu, &memory[address]);

                    emulate_6502_cycle(8);
                    cyc = 8;
                    break;

                    // NOP zpg,X
                    case 0x4:
                        cpu->PC += 2;
                        emulate_6502_cycle(4);
                        cyc = 4;
                        break;

                // 0xF5
                // SBC zpg,X
                case 0x5:
                    address = addr_zpg_X(cpu);
                    instr_SBC(cpu, memory[address]);

                    emulate_6502_cycle(4);
                    cyc = 4;
                    break;

                // 0xF6
                // INC zpg,X
                case 0x6:
                    address = addr_zpg_X(cpu);
                    instr_INC(cpu, &memory[address]);

                    emulate_6502_cycle(6);
                    cyc = 6;
                    break;

                // 0xF7
                // ISC zpg,X
                case 0x7:
                    address = addr_zpg_X(cpu);
                    instr_ISC(cpu, &memory[address]);

                    emulate_6502_cycle(6);
                    cyc = 6;
                    break;

                // 0xF8
                // SED impl
                case 0x8:
                    instr_SED(cpu);
                    break;

                // 0xF9
                // SBC abs,Y
                case 0x9:
                    address = addr_abs_Y(cpu);
                    if ((address & 0xFF00) != ((address - cpu->Y) & 0xFF00)) {
                        cyc = 5;
                    } else {
                        cyc = 4;
                    }
                    instr_SBC(cpu, memory[address]);

                    emulate_6502_cycle(cyc);
                    
                    break;

                // NOP imp
                case 0xA:
                    cpu->PC++;
                    emulate_6502_cycle(2);
                    cyc = 2;
                    break;

                // 0xFB
                // ISC abs,Y
                case 0xB:
                    address = addr_abs_Y(cpu);
                    instr_ISC(cpu, &memory[address]);

                    emulate_6502_cycle(7);
                    cyc = 7;
                    break;

                    case 0xc:
                        address = addr_abs_X(cpu);
                        if ((address & 0xFF00) != ((address - cpu->Y) & 0xFF00)) {
                            cyc = 5;
                        } else {
                            cyc = 4;
                        }
                        cpu->PC++;
                        emulate_6502_cycle(cyc);
                        
                        break;

                // 0xFD
                // SBC abs,X
                case 0xD:
                    address = addr_abs_X(cpu);
                    if ((address & 0xFF00) != ((address - cpu->X) & 0xFF00)) {
                        cyc = 5;
                    } else {
                        cyc = 4;
                    }
                    instr_SBC(cpu, memory[address]);

                    emulate_6502_cycle(cyc);
                    
                    break;

                // 0xFE
                // INC abs,X
                case 0xE:
                    address = addr_abs_X(cpu);
                    instr_INC(cpu, &memory[address]);

                    emulate_6502_cycle(7);
                    cyc = 7;
                    break;

                // 0xFF
                // ISC abs,X
                case 0xF:
                    address = addr_abs_X(cpu);
                    instr_ISC(cpu, &memory[address]);

                    emulate_6502_cycle(7);
                    cyc = 7;
                    break;

            }
        break;
    }

    printf("ADDR: %X\n", address);

    for (int i = 0; i < cyc; i++) {
        ppu_execute_cycle(cpu->ppu);
    }
    cpu->cycles += cyc;

    // print("}\n");
}
