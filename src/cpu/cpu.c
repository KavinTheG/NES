#include "cpu/cpu.h"
#include "config.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "ppu.h"

#define CPU_MEMORY_SIZE 0x10000 // 64 KB
#define NES_HEADER_SIZE 16
#define NES_TEST 0xC000

uint8_t memory[CPU_MEMORY_SIZE] = {0};
FILE *log_file;

// int to hold status flags
uint8_t status;

// int to hold lower byte of memory address
uint8_t LB, HB;
uint16_t address, zpg_addr;

char new_carry;
int cyc = 0;

int instr_num = 0;

unsigned char dma_active_flag;
int page_crossed = 0;
int dma_cycles = 0;

/**  Helper functions **/
static inline void ppu_exec(Cpu6502 *cpu) {
  ppu_execute_cycle(cpu->ppu);
  ppu_execute_cycle(cpu->ppu);
  ppu_execute_cycle(cpu->ppu);
}

inline void push_stack(uint8_t lower_addr, uint8_t val) {
  memory[0x0100 | lower_addr] = val;
}

uint16_t page_crossing(uint16_t addr, uint16_t oper) {

  uint16_t address = 0;
  address = addr + oper;
  // Check if the page is the same
  if (((addr + oper) & 0xFF00) != (addr & 0xFF00)) {
    // Keep the page from the addr to emulate bug
    address = (addr & 0xFF00) | (address & 0xFF);
    page_crossed = 1;
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
  // struct timespec ts;
  // ts.tv_sec = 0;
  // ts.tv_nsec = 558 * cycle; // Approx. 558 nanoseconds
  // nanosleep(&ts, NULL);
}

void check_page_cross(uint16_t base, uint8_t index) {
  uint16_t result = base + index;
  if ((base & 0xFF00) != (result & 0xFF00)) {
    cyc += 1;
  }
}

/* PPU Functions */

void cpu_ppu_write(Cpu6502 *cpu, uint16_t addr, uint8_t val) {
  LOG("PPU MMIO WRITE: $%X; val: %x\n", addr, val);
  // sleep(1);

  // if ((addr == 0x2000 || addr == 0x2001 || addr == 0x2005 || addr == 0x2006)
  // && cpu->cycles <= 29657) return;
  ppu_registers_write(cpu->ppu, addr, val);
}

uint8_t cpu_ppu_read(Cpu6502 *cpu, uint16_t addr) {
  return ppu_registers_read(cpu->ppu, addr);
}

/* CTRL Functions */
void ctrl1_write(Cpu6502 *cpu, uint8_t val) {
  if ((val & 1) == 1) {
    cpu->strobe = 1;
  } else {
    if (cpu->strobe) {
      cpu->ctrl_bit_index = 0;
    }
    cpu->strobe = 0;
  }
}

uint8_t ctrl1_read(Cpu6502 *cpu) {
  uint8_t bit = (cpu->ctrl_latch_state >> cpu->ctrl_bit_index) & 1;
  if (!cpu->strobe && cpu->ctrl_bit_index < 8)
    cpu->ctrl_bit_index++;

  return bit | 0x40;
}

// Core functions

void cpu_init(Cpu6502 *cpu) {

  // Initialize stack pointer to the top;
  cpu->S = 0xFD;
  cpu->A = 0x0;
  cpu->X = 0x0;
  cpu->Y = 0x0;

  cpu->nmi_state = 0;
  cpu->PC = (memory[0xFFFD] << 8) | memory[0xFFFC];

  // #if NES_TEST_ROM == 1
  //   cpu->PC = 0xC000;
  // #endif

  printf("ADDR: %X\n", cpu->PC);
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

  // memset(memory, 0, sizeof(memory));

  for (int i = 0; i < 25; i++) {
    ppu_exec(cpu);
  }

  log_file = fopen("log.txt", "w");
  fclose(log_file);
  log_file = fopen("log.txt", "a");

#if NES_TEST_ROM
  push_stack(0100 | cpu->S, 0x70);
  cpu->S--;

  push_stack(0100 | cpu->S, 0x00);
  cpu->S--;
#endif
}

void load_cpu_memory(Cpu6502 *cpu, unsigned char *prg_rom, int prg_size) {
  // Clear memory
  memset(memory, 0, CPU_MEMORY_SIZE);

  // Load PRG ROM into 0x8000 - 0xBFFF
  memcpy(&memory[0x8000], prg_rom, prg_size);

  // If PRG ROM is 16KB, mirror it into $C000-$FFFF (NROM-128 behavior)
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

  fprintf(log_file, "\n");

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
  } else if (addr == 0x4016) {
    cpu->A = ctrl1_read(cpu);
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
    dma_cycles = cpu->cycles % 2 == 0 ? 513 : 514;
    uint8_t page_mem[0x100];
    memcpy(page_mem, &memory[cpu->A << 8], 0x100);
    load_ppu_oam_mem(cpu->ppu, page_mem);
  } else if (addr == 0x4016) {
    ctrl1_write(cpu, cpu->A);
  }

  memory[addr] = cpu->A;

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
    dma_cycles = cpu->cycles % 2 == 0 ? 513 : 514;

    uint8_t page_mem[0x100];
    memcpy(page_mem, &memory[cpu->X << 8], 0x100);
    load_ppu_oam_mem(cpu->ppu, page_mem);
  } else if (addr == 0x4016) {
    ctrl1_write(cpu, cpu->X);
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
    dma_cycles = cpu->cycles % 2 == 0 ? 513 : 514;

    uint8_t page_mem[0x100];
    memcpy(page_mem, &memory[cpu->Y << 8], 0x100);
    load_ppu_oam_mem(cpu->ppu, page_mem);
  } else if (addr == 0x4016) {
    ctrl1_write(cpu, cpu->Y);
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
}

void instr_TXA(Cpu6502 *cpu) {
  cpu->A = cpu->X;

  cpu->P[1] = cpu->A == 0;
  cpu->P[7] = (cpu->A & 0x80) == 0x80;
  cpu->PC++;
}
void instr_TAY(Cpu6502 *cpu) {
  cpu->Y = cpu->A;

  cpu->P[1] = cpu->Y == 0;
  cpu->P[7] = (cpu->Y & 0x80) == 0x80;
  cpu->PC++;
}
void instr_TYA(Cpu6502 *cpu) {
  cpu->A = cpu->Y;

  cpu->P[1] = cpu->A == 0;
  cpu->P[7] = (cpu->A & 0x80) == 0x80;
  cpu->PC++;
}

// Arithmetic
void instr_ADC(Cpu6502 *cpu, uint8_t oper) {

  uint16_t result = 0;
  uint8_t new_A = cpu->A;

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

  cpu->PC++;
}
void instr_SBC(Cpu6502 *cpu, uint8_t oper) {
  uint16_t result = 0;
  uint8_t new_A = cpu->A;

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

  cpu->PC++;
}

void instr_INC(Cpu6502 *cpu, uint8_t *M) {
  *M = *M + 1;

  cpu->P[1] = *M == 0;
  cpu->P[7] = (*M & 0x80) == 0x80;
  cpu->PC++;
}

void instr_DEC(Cpu6502 *cpu, uint8_t *M) {
  *M = *M - 1;

  cpu->P[1] = *M == 0;
  cpu->P[7] = (*M & 0x80) == 0x80;
  cpu->PC++;
}

void instr_INX(Cpu6502 *cpu) {
  cpu->X++;

  cpu->P[1] = cpu->X == 0;
  cpu->P[7] = (cpu->X & 0x80) == 0x80;
  cpu->PC++;
}

void instr_DEX(Cpu6502 *cpu) {
  cpu->X--;

  cpu->P[1] = cpu->X == 0;
  cpu->P[7] = (cpu->X & 0x80) == 0x80;
  cpu->PC++;
}

void instr_INY(Cpu6502 *cpu) {
  cpu->Y++;

  cpu->P[1] = cpu->Y == 0;
  cpu->P[7] = (cpu->Y & 0x80) == 0x80;
  cpu->PC++;
}

void instr_DEY(Cpu6502 *cpu) {
  cpu->Y--;

  cpu->P[1] = cpu->Y == 0;
  cpu->P[7] = (cpu->Y & 0x80) == 0x80;
  cpu->PC++;
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
  cpu->P[1] = (cpu->A & M) == 0;

  cpu->PC++;
}

// Compare
void instr_CMP(Cpu6502 *cpu, uint8_t M) {

  // Carry Flag
  cpu->P[0] = cpu->A >= M;

  // Zero flag
  cpu->P[1] = cpu->A == M;

  // Negative Flag
  cpu->P[7] = ((cpu->A - M) & 0x80) == 0x80;

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

  // Branch if flag condition is met
  if (flag) {

    if ((signed_offset & 0x80) == 0x80) {
      // Negative
      // Convert from 2's complement to regular by negation + 1
      cpu->PC += 1 - ((~signed_offset & 0xFF) + 1);
    } else {
      cpu->PC += 1 + signed_offset;
    }

    if ((cpu->PC & 0xFF00) != ((old_addr + 2) & 0xFF00)) {
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

void instr_BCC(Cpu6502 *cpu) { instr_branch(cpu, !cpu->P[0]); }

void instr_BCS(Cpu6502 *cpu) { instr_branch(cpu, cpu->P[0]); }

void instr_BEQ(Cpu6502 *cpu) { instr_branch(cpu, cpu->P[1]); }

void instr_BNE(Cpu6502 *cpu) { instr_branch(cpu, !cpu->P[1]); }

void instr_BPL(Cpu6502 *cpu) { instr_branch(cpu, !cpu->P[7]); }

void instr_BMI(Cpu6502 *cpu) { instr_branch(cpu, cpu->P[7]); }

void instr_BVC(Cpu6502 *cpu) { instr_branch(cpu, !cpu->P[6]); }

void instr_BVS(Cpu6502 *cpu) { instr_branch(cpu, cpu->P[6]); }

void instr_JMP(Cpu6502 *cpu, uint16_t addr) { cpu->PC = addr; }

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
  cpu->S -= 1;

  cpu->PC = memory[0xFFFF] << 8 | memory[0xFFFE];
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
}

// Stack
void instr_PHA(Cpu6502 *cpu) {

  // Push accumulator on stack
  push_stack(cpu->S, cpu->A);

  cpu->S--;
  cpu->PC++;
}

void instr_PLA(Cpu6502 *cpu) {

  // Pull accumulator from stack
  cpu->S++;
  cpu->A = memory[0x100 | cpu->S];
  cpu->PC++;

  cpu->P[1] = cpu->A == 0;
  cpu->P[7] = (cpu->A & 0x80) == 0x80;
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
}

void instr_PLP(Cpu6502 *cpu) {
  // Pull Processor Status from Stack
  cpu->S++;

  uint8_to_char_array(memory[0x0100 | cpu->S], cpu->P);

  // These bits are ignored, just set to 1
  cpu->P[5] = 1;
  cpu->P[4] = 1;

  cpu->PC++;
}

void instr_TXS(Cpu6502 *cpu) {
  cpu->S = cpu->X;
  cpu->PC++;
}

void instr_TSX(Cpu6502 *cpu) {
  cpu->X = cpu->S;
  cpu->PC++;

  cpu->P[1] = cpu->X == 0;
  cpu->P[7] = (cpu->X & 0x80) == 0x80;
}

// Flags
void instr_CLC(Cpu6502 *cpu) {
  // clear carry flag
  cpu->P[0] = 0;

  cpu->PC++;
}

void instr_SEC(Cpu6502 *cpu) {
  // set carry flag
  cpu->P[0] = 1;

  cpu->PC++;
}

void instr_CLI(Cpu6502 *cpu) {
  // clear interrupt flag
  cpu->P[2] = 0;

  cpu->PC++;
}

void instr_SEI(Cpu6502 *cpu) {
  cpu->P[2] = 1;

  cpu->PC++;
}

void instr_CLD(Cpu6502 *cpu) {
  cpu->P[3] = 0;

  cpu->PC++;
}

void instr_SED(Cpu6502 *cpu) {
  cpu->P[3] = 1;

  cpu->PC++;
}

void instr_CLV(Cpu6502 *cpu) {
  cpu->P[6] = 0;

  cpu->PC++;
}

// Other
void instr_NOP(Cpu6502 *cpu) { cpu->PC++; }

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
  cpu->P[7] = ((cpu->A - *M) & 0x80) == 0x80;

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

  *M = *M << 1;

  cpu->A |= *M;

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
  cpu->S -= 1;

  cpu->PC = (memory[0xFFFB] << 8) | memory[0xFFFA];
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

uint16_t addr_ind_jmp(Cpu6502 *cpu) {
  cpu->PC++;
  uint8_t LB = memory[cpu->PC];

  cpu->PC++;
  uint8_t HB = memory[cpu->PC];

  uint16_t addr = (HB << 8) | LB;

  if (LB == 0xFF) {
    return (memory[addr & 0xFF00] << 8) | memory[addr];
  } else {
    return (memory[addr + 1] << 8) | memory[addr];
  }
}

uint16_t addr_abs_X(Cpu6502 *cpu) {
  // Increment to get the lower byte
  cpu->PC++;
  uint16_t addr;

  uint8_t LB = memory[cpu->PC];
  cpu->PC++;
  uint8_t HB = memory[cpu->PC];

  // addr = (memory[cpu->PC] << 8 | LB) + cpu->X;
  addr = (HB << 8 | LB) + cpu->X;
  if ((addr & 0xFF00) != ((addr - cpu->X) & 0xFF00)) {
    page_crossed = 1;
  } else {
    page_crossed = 0;
  }

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
  if ((addr & 0xFF00) != ((addr - cpu->Y) & 0xFF00)) {
    page_crossed = 1;
  } else {
    page_crossed = 0;
  }

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

  uint8_t LB = memory[BB];
  uint8_t HB = memory[page_crossing(BB, 1)];

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

  // addr = page_crossing(HB << 8 | LB, cpu->Y);
  addr = (HB << 8 | LB) + cpu->Y;
  if ((addr & 0xFF00) != ((addr - cpu->Y) & 0xFF00)) {
    page_crossed = 1;
  } else {
    page_crossed = 0;
  }
  return addr;
}

uint16_t addr_zpg(Cpu6502 *cpu) {
  cpu->PC++;
  uint16_t addr;
  LB = memory[cpu->PC];

  addr = (uint16_t)LB;
  return addr;
}

uint16_t addr_zpg_X(Cpu6502 *cpu) {
  cpu->PC++;
  uint16_t addr;
  LB = memory[cpu->PC];

  // Discard carry, zpg should not exceed 0x00FF
  addr = (uint16_t)((LB + cpu->X) & 0xFF);
  return addr;
}

uint16_t addr_zpg_Y(Cpu6502 *cpu) {
  cpu->PC++;
  uint16_t addr;
  LB = memory[cpu->PC];

  addr = (LB + cpu->Y) & 0xFF;
  return addr;
}
Opcode lookup_table[256] =
    {
        [0x00] =
            {
                .addr_mode = NULL,
                .instr_none = instr_BRK,
                .cycles = 7,
                .page_cycles = 0,
                .mnemonic = "BRK",
                INSTR_NONE,
            },
        [0x01] =
            {
                .addr_mode = addr_X_ind,
                .instr_val = instr_ORA,
                .cycles = 6,
                .page_cycles = 0,
                .mnemonic = "ORA ind,X",
                INSTR_VAL,
            },
        [0x03] =
            {
                .addr_mode = addr_X_ind,
                .instr_mem = instr_SLO,
                .cycles = 8,
                .page_cycles = 0,
                .mnemonic = "SLO ind,X",
                INSTR_MEM,
            },
        [0x04] =
            {
                .addr_mode = addr_zpg,
                .instr_addr = NULL,
                .cycles = 3,
                .page_cycles = 0,
                .mnemonic = "NOP zpg",
                INSTR_ADDR,
            },
        [0x05] =
            {
                .addr_mode = addr_zpg,
                .instr_val = instr_ORA,
                .cycles = 3,
                .page_cycles = 0,
                .mnemonic = "ORA zpg",
                INSTR_VAL,
            },
        [0x06] =
            {
                .addr_mode = addr_zpg,
                .instr_mem = instr_ASL,
                .cycles = 5,
                .page_cycles = 0,
                .mnemonic = "ASL zpg",
                INSTR_MEM,
            },
        [0x07] =
            {
                .addr_mode = addr_zpg,
                .instr_mem = instr_SLO,
                .cycles = 5,
                .page_cycles = 0,
                .mnemonic = "SLO zpg",
                INSTR_MEM,
            },
        [0x08] =
            {
                .addr_mode = NULL,
                .instr_none = instr_PHP,
                .cycles = 3,
                .page_cycles = 0,
                .mnemonic = "PHP impl",
                INSTR_NONE,
            },
        [0x09] =
            {
                .addr_mode = addr_imm,
                .instr_val = instr_ORA,
                .cycles = 2,
                .page_cycles = 0,
                .mnemonic = "ORA #",
                INSTR_VAL,
            },
        [0x0A] =
            {
                .addr_mode = NULL,
                .instr_mem = instr_ASL,
                .cycles = 2,
                .page_cycles = 0,
                .mnemonic = "ASL A",
                .instr_type = INSTR_ACC,
            },
        [0x0C] =
            {
                .addr_mode = addr_abs,
                .instr_addr = NULL,
                .cycles = 4,
                .page_cycles = 0,
                .mnemonic = "NOP abs",
                INSTR_ADDR,
            },
        [0x0D] =
            {
                .addr_mode = addr_abs,
                .instr_val = instr_ORA,
                .cycles = 4,
                .page_cycles = 0,
                .mnemonic = "ORA abs",
                INSTR_VAL,
            },
        [0x0E] =
            {
                .addr_mode = addr_abs,
                .instr_mem = instr_ASL,
                .cycles = 6,
                .page_cycles = 0,
                .mnemonic = "ASL abs",
                INSTR_MEM,
            },
        [0x0F] =
            {
                .addr_mode = addr_abs,
                .instr_mem = instr_SLO,
                .cycles = 6,
                .page_cycles = 0,
                .mnemonic = "SLO abs",
                INSTR_MEM,
            },

        [0x10] =
            {
                .addr_mode = NULL,
                .instr_none = instr_BPL,
                .cycles = 2,
                .page_cycles = 1,
                .mnemonic = "BPL rel",
                INSTR_NONE,
            },
        [0x11] =
            {
                .addr_mode = addr_ind_Y,
                .instr_val = instr_ORA,
                .cycles = 5,
                .page_cycles = 1,
                .mnemonic = "ORA (ind),Y",
                INSTR_VAL,
            },
        [0x13] =
            {
                .addr_mode = addr_ind_Y,
                .instr_mem = instr_SLO,
                .cycles = 8,
                .page_cycles = 0,
                .mnemonic = "SLO (ind),Y",
                INSTR_MEM,
            },
        [0x14] =
            {
                .addr_mode = addr_zpg_X,
                .instr_addr = NULL,
                .cycles = 4,
                .page_cycles = 0,
                .mnemonic = "NOP zpg,X",
                INSTR_ADDR,
            },
        [0x15] =
            {
                .addr_mode = addr_zpg_X,
                .instr_val = instr_ORA,
                .cycles = 4,
                .page_cycles = 0,
                .mnemonic = "ORA zpg,X",
                INSTR_VAL,
            },
        [0x16] =
            {
                .addr_mode = addr_zpg_X,
                .instr_mem = instr_ASL,
                .cycles = 6,
                .page_cycles = 0,
                .mnemonic = "ASL zpg,X",
                INSTR_MEM,
            },
        [0x17] =
            {
                .addr_mode = addr_zpg_X,
                .instr_mem = instr_SLO,
                .cycles = 6,
                .page_cycles = 0,
                .mnemonic = "SLO zpg,X",
                INSTR_MEM,
            },
        [0x18] =
            {
                .addr_mode = NULL,
                .instr_none = instr_CLC,
                .cycles = 2,
                .page_cycles = 0,
                .mnemonic = "CLC",
                INSTR_NONE,
            },
        [0x19] =
            {
                .addr_mode = addr_abs_Y,
                .instr_val = instr_ORA,
                .cycles = 4,
                .page_cycles = 1,
                .mnemonic = "ORA abs,Y",
                INSTR_VAL,
            },
        [0x1A] =
            {
                .addr_mode = NULL,
                .instr_none = instr_NOP,
                .cycles = 2,
                .page_cycles = 0,
                .mnemonic = "NOP",
                INSTR_NONE,
            },
        [0x1B] =
            {
                .addr_mode = addr_abs_Y,
                .instr_mem = instr_SLO,
                .cycles = 7,
                .page_cycles = 0,
                .mnemonic = "SLO abs,Y",
                INSTR_MEM,
            },
        [0x1C] =
            {
                .addr_mode = addr_abs_X,
                .instr_addr = NULL,
                .cycles = 4,
                .page_cycles = 1,
                .mnemonic = "NOP abs,X",
                INSTR_ADDR,
            },
        [0x1D] =
            {
                .addr_mode = addr_abs_X,
                .instr_val = instr_ORA,
                .cycles = 4,
                .page_cycles = 1,
                .mnemonic = "ORA abs,X",
                INSTR_VAL,
            },
        [0x1E] =
            {
                .addr_mode = addr_abs_X,
                .instr_mem = instr_ASL,
                .cycles = 7,
                .page_cycles = 0,
                .mnemonic = "ASL abs,X",
                INSTR_MEM,
            },
        [0x1F] =
            {
                .addr_mode = addr_abs_X,
                .instr_mem = instr_SLO,
                .cycles = 7,
                .page_cycles = 0,
                .mnemonic = "SLO abs,X",
                INSTR_MEM,
            },
        [0x20] =
            {
                .addr_mode = addr_abs,
                .instr_addr = instr_JSR,
                .cycles = 6,
                .page_cycles = 0,
                .mnemonic = "JSR",
                INSTR_ADDR,
            },
        [0x21] =
            {
                .addr_mode = addr_X_ind,
                .instr_val = instr_AND,
                .cycles = 6,
                .page_cycles = 0,
                .mnemonic = "AND",
                INSTR_VAL,
            },
        [0x23] =
            {
                .addr_mode = addr_X_ind,
                .instr_mem = instr_RLA,
                .cycles = 8,
                .page_cycles = 0,
                .mnemonic = "RLA",
                INSTR_MEM,
            },
        [0x24] =
            {
                .addr_mode = addr_zpg,
                .instr_val = instr_BIT,
                .cycles = 3,
                .page_cycles = 0,
                .mnemonic = "BIT",
                INSTR_VAL,
            },
        [0x25] =
            {
                .addr_mode = addr_zpg,
                .instr_val = instr_AND,
                .cycles = 3,
                .page_cycles = 0,
                .mnemonic = "AND",
                INSTR_VAL,
            },
        [0x26] =
            {
                .addr_mode = addr_zpg,
                .instr_mem = instr_ROL,
                .cycles = 5,
                .page_cycles = 0,
                .mnemonic = "ROL",
                INSTR_MEM,
            },
        [0x27] =
            {
                .addr_mode = addr_zpg,
                .instr_mem = instr_RLA,
                .cycles = 5,
                .page_cycles = 0,
                .mnemonic = "RLA",
                INSTR_MEM,
            },
        [0x28] =
            {
                .addr_mode = NULL,
                .instr_none = instr_PLP,
                .cycles = 4,
                .page_cycles = 0,
                .mnemonic = "PLP",
                INSTR_NONE,
            },
        [0x29] =
            {
                .addr_mode = addr_imm,
                .instr_val = instr_AND,
                .cycles = 2,
                .page_cycles = 0,
                .mnemonic = "AND",
                INSTR_VAL,
            },
        [0x2A] =
            {
                .addr_mode = NULL,
                .instr_mem = instr_ROL,
                .cycles = 2,
                .page_cycles = 0,
                .mnemonic = "ROL A",
                INSTR_ACC,
            },
        [0x2C] =
            {
                .addr_mode = addr_abs,
                .instr_val = instr_BIT,
                .cycles = 4,
                .page_cycles = 0,
                .mnemonic = "BIT",
                INSTR_VAL,
            },
        [0x2D] =
            {
                .addr_mode = addr_abs,
                .instr_val = instr_AND,
                .cycles = 4,
                .page_cycles = 0,
                .mnemonic = "AND",
                INSTR_VAL,
            },
        [0x2E] =
            {
                .addr_mode = addr_abs,
                .instr_mem = instr_ROL,
                .cycles = 6,
                .page_cycles = 0,
                .mnemonic = "ROL",
                INSTR_MEM,
            },
        [0x2F] =
            {
                .addr_mode = addr_abs,
                .instr_mem = instr_RLA,
                .cycles = 6,
                .page_cycles = 0,
                .mnemonic = "RLA",
                INSTR_MEM,
            },

        [0x30] =
            {
                .addr_mode = NULL,
                .instr_none = instr_BMI,
                .cycles = 2,
                .page_cycles = 1,
                .mnemonic = "BMI rel",
                INSTR_NONE,
            },
        [0x31] =
            {
                .addr_mode = addr_ind_Y,
                .instr_val = instr_AND,
                .cycles = 5,
                .page_cycles = 1,
                .mnemonic = "AND (ind),Y",
                INSTR_VAL,
            },
        [0x33] =
            {
                .addr_mode = addr_ind_Y,
                .instr_mem = instr_RLA,
                .cycles = 8,
                .page_cycles = 0,
                .mnemonic = "RLA (ind),Y",
                INSTR_MEM,
            },
        [0x34] =
            {
                .addr_mode = addr_zpg_X,
                .instr_addr = NULL,
                .cycles = 4,
                .page_cycles = 0,
                .mnemonic = "NOP zpg,X",
                INSTR_ADDR,
            },
        [0x35] =
            {
                .addr_mode = addr_zpg_X,
                .instr_val = instr_AND,
                .cycles = 4,
                .page_cycles = 0,
                .mnemonic = "AND zpg,X",
                INSTR_VAL,
            },
        [0x36] =
            {
                .addr_mode = addr_zpg_X,
                .instr_mem = instr_ROL,
                .cycles = 6,
                .page_cycles = 0,
                .mnemonic = "ROL zpg,X",
                INSTR_MEM,
            },
        [0x37] =
            {
                .addr_mode = addr_zpg_X,
                .instr_mem = instr_RLA,
                .cycles = 6,
                .page_cycles = 0,
                .mnemonic = "RLA zpg",
                INSTR_MEM,
            },
        [0x38] =
            {
                .addr_mode = NULL,
                .instr_none = instr_SEC,
                .cycles = 2,
                .page_cycles = 0,
                .mnemonic = "SEC",
                INSTR_NONE,
            },
        [0x39] =
            {
                .addr_mode = addr_abs_Y,
                .instr_val = instr_AND,
                .cycles = 4,
                .page_cycles = 1,
                .mnemonic = "AND abs,Y",
                INSTR_VAL,
            },
        [0x3A] =
            {
                .addr_mode = NULL,
                .instr_none = instr_NOP,
                .cycles = 2,
                .page_cycles = 0,
                .mnemonic = "NOP",
                INSTR_NONE,
            },
        [0x3B] =
            {
                .addr_mode = addr_abs_Y,
                .instr_mem = instr_RLA,
                .cycles = 7,
                .page_cycles = 0,
                .mnemonic = "RLA abs,Y",
                INSTR_MEM,
            },
        [0x3C] =
            {
                .addr_mode = addr_abs_X,
                .instr_addr = NULL,
                .cycles = 4,
                .page_cycles = 1,
                .mnemonic = "NOP abs,X",
                INSTR_ADDR,
            },
        [0x3D] =
            {
                .addr_mode = addr_abs_X,
                .instr_val = instr_AND,
                .cycles = 4,
                .page_cycles = 1,
                .mnemonic = "AND abs,X",
                INSTR_VAL,
            },
        [0x3E] =
            {
                .addr_mode = addr_abs_X,
                .instr_mem = instr_ROL,
                .cycles = 7,
                .page_cycles = 0,
                .mnemonic = "ROL abs,X",
                INSTR_MEM,
            },
        [0x3F] =
            {
                .addr_mode = addr_abs_X,
                .instr_mem = instr_RLA,
                .cycles = 7,
                .page_cycles = 0,
                .mnemonic = "RLA abs,X",
                INSTR_MEM,
            },

        [0x40] =
            {
                .addr_mode = NULL,
                .instr_none = instr_RTI,
                .cycles = 6,
                .page_cycles = 0,
                .mnemonic = "RTI",
                INSTR_NONE,
            },
        [0x41] =
            {
                .addr_mode = addr_X_ind,
                .instr_val = instr_EOR,
                .cycles = 6,
                .page_cycles = 0,
                .mnemonic = "EOR",
                INSTR_VAL,
            },
        [0x43] =
            {
                .addr_mode = addr_X_ind,
                .instr_mem = instr_SRE,
                .cycles = 8,
                .page_cycles = 0,
                .mnemonic = "SRE",
                INSTR_MEM,
            },
        [0x44] =
            {
                .addr_mode = addr_zpg,
                .instr_addr = NULL,
                .cycles = 3,
                .page_cycles = 0,
                .mnemonic = "NOP",
                INSTR_ADDR,
            },
        [0x45] =
            {
                .addr_mode = addr_zpg,
                .instr_val = instr_EOR,
                .cycles = 3,
                .page_cycles = 0,
                .mnemonic = "EOR",
                INSTR_VAL,
            },
        [0x46] =
            {
                .addr_mode = addr_zpg,
                .instr_mem = instr_LSR,
                .cycles = 5,
                .page_cycles = 0,
                .mnemonic = "LSR",
                INSTR_MEM,
            },
        [0x47] =
            {
                .addr_mode = addr_zpg,
                .instr_mem = instr_SRE,
                .cycles = 5,
                .page_cycles = 0,
                .mnemonic = "SRE",
                INSTR_MEM,
            },
        [0x48] =
            {
                .addr_mode = NULL,
                .instr_none = instr_PHA,
                .cycles = 3,
                .page_cycles = 0,
                .mnemonic = "PHA",
                INSTR_NONE,
            },
        [0x49] =
            {
                .addr_mode = addr_imm,
                .instr_val = instr_EOR,
                .cycles = 2,
                .page_cycles = 0,
                .mnemonic = "EOR",
                INSTR_VAL,
            },
        [0x4A] =
            {
                .addr_mode = NULL,
                .instr_mem = instr_LSR,
                .cycles = 2,
                .page_cycles = 0,
                .mnemonic = "LSR A",
                .instr_type = INSTR_ACC, // Pass accumulator to *M
            },
        [0x4B] =
            {
                .addr_mode = addr_abs_Y,
                .instr_mem = instr_SRE,
                .cycles = 7,
                .page_cycles = 0,
                .mnemonic = "SRE",
                INSTR_MEM,
            },
        [0x4C] =
            {
                .addr_mode = addr_abs,
                .instr_addr = instr_JMP,
                .cycles = 3,
                .page_cycles = 0,
                .mnemonic = "JMP",
                INSTR_ADDR,
            },
        [0x4D] =
            {
                .addr_mode = addr_abs,
                .instr_val = instr_EOR,
                .cycles = 4,
                .page_cycles = 0,
                .mnemonic = "EOR",
                INSTR_VAL,
            },
        [0x4E] =
            {
                .addr_mode = addr_abs,
                .instr_mem = instr_LSR,
                .cycles = 6,
                .page_cycles = 0,
                .mnemonic = "LSR",
                INSTR_MEM,
            },
        [0x4F] =
            {
                .addr_mode = addr_abs,
                .instr_mem = instr_SRE,
                .cycles = 6,
                .page_cycles = 0,
                .mnemonic = "SRE",
                INSTR_MEM,
            },

        [0x50] =
            {
                .addr_mode = NULL,
                .instr_none = instr_BVC,
                .cycles = 2,
                .page_cycles = 0,
                .mnemonic = "BVC",
                INSTR_NONE,
            },
        [0x51] =
            {
                .addr_mode = addr_ind_Y,
                .instr_val = instr_EOR,
                .cycles = 5,
                .page_cycles = 1,
                .mnemonic = "EOR",
                INSTR_VAL,
            },
        [0x53] =
            {
                .addr_mode = addr_ind_Y,
                .instr_mem = instr_SRE,
                .cycles = 8,
                .page_cycles = 0,
                .mnemonic = "SRE",
                INSTR_MEM,
            },
        [0x54] =
            {
                .addr_mode = addr_zpg_X,
                .instr_addr = NULL,
                .cycles = 4,
                .page_cycles = 0,
                .mnemonic = "NOP zpg,X",
                INSTR_ADDR,
            },
        [0x55] =
            {
                .addr_mode = addr_zpg_X,
                .instr_val = instr_EOR,
                .cycles = 4,
                .page_cycles = 0,
                .mnemonic = "EOR",
                INSTR_VAL,
            },
        [0x56] =
            {
                .addr_mode = addr_zpg_X,
                .instr_mem = instr_LSR,
                .cycles = 6,
                .page_cycles = 0,
                .mnemonic = "LSR",
                INSTR_MEM,
            },
        [0x57] =
            {
                .addr_mode = addr_zpg_X,
                .instr_mem = instr_SRE,
                .cycles = 6,
                .page_cycles = 0,
                .mnemonic = "SRE",
                INSTR_MEM,
            },
        [0x58] =
            {
                .addr_mode = NULL,
                .instr_none = instr_CLI,
                .cycles = 2,
                .page_cycles = 0,
                .mnemonic = "CLI",
                INSTR_NONE,
            },
        [0x59] =
            {
                .addr_mode = addr_abs_Y,
                .instr_val = instr_EOR,
                .cycles = 4,
                .page_cycles = 1,
                .mnemonic = "EOR",
                INSTR_VAL,
            },
        [0x5A] =
            {
                .addr_mode = NULL,
                .instr_none = instr_NOP,
                .cycles = 2,
                .page_cycles = 0,
                .mnemonic = "NOP",
                INSTR_NONE,
            }, // NOP implied, PC increment externally
        [0x5B] =
            {
                .addr_mode = addr_abs_Y,
                .instr_mem = instr_SRE,
                .cycles = 7,
                .page_cycles = 0,
                .mnemonic = "SRE",
                INSTR_MEM,
            },
        [0x5C] =
            {
                .addr_mode = addr_abs_X,
                .instr_addr = NULL,
                .cycles = 4,
                .page_cycles = 1,
                .mnemonic = "NOP abs,X",
                INSTR_ADDR,
            },
        [0x5D] =
            {
                .addr_mode = addr_abs_X,
                .instr_val = instr_EOR,
                .cycles = 4,
                .page_cycles = 1,
                .mnemonic = "EOR",
                INSTR_VAL,
            },
        [0x5E] =
            {
                .addr_mode = addr_abs_X,
                .instr_mem = instr_LSR,
                .cycles = 7,
                .page_cycles = 0,
                .mnemonic = "LSR",
                INSTR_MEM,
            },
        [0x5F] =
            {
                .addr_mode = addr_abs_X,
                .instr_mem = instr_SRE,
                .cycles = 7,
                .page_cycles = 0,
                .mnemonic = "SRE",
                INSTR_MEM,
            },

        [0x60] =
            {
                .addr_mode = NULL,
                .instr_none = instr_RTS,
                .cycles = 6,
                .page_cycles = 0,
                .mnemonic = "RTS",
                INSTR_NONE,
            },
        [0x61] =
            {
                .addr_mode = addr_X_ind,
                .instr_val = instr_ADC,
                .cycles = 6,
                .page_cycles = 0,
                .mnemonic = "ADC",
                INSTR_VAL,
            },
        [0x63] =
            {
                .addr_mode = addr_X_ind,
                .instr_mem = instr_RRA,
                .cycles = 8,
                .page_cycles = 0,
                .mnemonic = "RRA",
                INSTR_MEM,
            },
        [0x64] =
            {
                .addr_mode = addr_zpg,
                .instr_addr = NULL,
                .cycles = 3,
                .page_cycles = 0,
                .mnemonic = "NOP",
                INSTR_ADDR,
            }, // NOP zpg, PC increment handled externally
        [0x65] =
            {
                .addr_mode = addr_zpg,
                .instr_val = instr_ADC,
                .cycles = 3,
                .page_cycles = 0,
                .mnemonic = "ADC",
                INSTR_VAL,
            },
        [0x66] =
            {
                .addr_mode = addr_zpg,
                .instr_mem = instr_ROR,
                .cycles = 5,
                .page_cycles = 0,
                .mnemonic = "ROR",
                INSTR_MEM,
            },
        [0x67] =
            {
                .addr_mode = addr_zpg,
                .instr_mem = instr_RRA,
                .cycles = 5,
                .page_cycles = 0,
                .mnemonic = "RRA",
                INSTR_MEM,
            },
        [0x68] =
            {
                .addr_mode = NULL,
                .instr_none = instr_PLA,
                .cycles = 4,
                .page_cycles = 0,
                .mnemonic = "PLA",
                INSTR_NONE,
            },
        [0x69] =
            {
                .addr_mode = addr_imm,
                .instr_val = instr_ADC,
                .cycles = 2,
                .page_cycles = 0,
                .mnemonic = "ADC",
                INSTR_VAL,
            },
        [0x6A] =
            {
                .addr_mode = NULL,
                .instr_mem = instr_ROR,
                .cycles = 2,
                .page_cycles = 0,
                .mnemonic = "ROR",
                INSTR_ACC,
            },
        [0x6C] =
            {
                .addr_mode = addr_ind_jmp,
                .instr_addr = instr_JMP,
                .cycles = 5,
                .page_cycles = 0,
                .mnemonic = "JMP",
                INSTR_ADDR,
            }, // JMP indirect, PC handled in switch
        [0x6D] =
            {
                .addr_mode = addr_abs,
                .instr_val = instr_ADC,
                .cycles = 4,
                .page_cycles = 0,
                .mnemonic = "ADC",
                INSTR_VAL,
            },
        [0x6E] =
            {
                .addr_mode = addr_abs,
                .instr_mem = instr_ROR,
                .cycles = 6,
                .page_cycles = 0,
                .mnemonic = "ROR",
                INSTR_MEM,
            },
        [0x6F] =
            {
                .addr_mode = addr_abs,
                .instr_mem = instr_RRA,
                .cycles = 6,
                .page_cycles = 0,
                .mnemonic = "RRA",
                INSTR_MEM,
            },

        [0x70] =
            {
                .addr_mode = NULL,
                .instr_none = instr_BVS,
                .cycles = 2,
                .page_cycles = 0,
                .mnemonic = "BVS",
                INSTR_NONE,
            },
        [0x71] =
            {
                .addr_mode = addr_ind_Y,
                .instr_val = instr_ADC,
                .cycles = 5,
                .page_cycles = 1,
                .mnemonic = "ADC",
                INSTR_VAL,
            },
        [0x73] =
            {
                .addr_mode = addr_ind_Y,
                .instr_mem = instr_RRA,
                .cycles = 8,
                .page_cycles = 0,
                .mnemonic = "RRA",
                INSTR_MEM,
            },
        [0x74] =
            {
                .addr_mode = addr_zpg_X,
                .instr_addr = NULL,
                .cycles = 4,
                .page_cycles = 0,
                .mnemonic = "NOP",
                INSTR_ADDR,
            }, // NOP zpg,X, PC increment done in switch
        [0x75] =
            {
                .addr_mode = addr_zpg_X,
                .instr_val = instr_ADC,
                .cycles = 4,
                .page_cycles = 0,
                .mnemonic = "ADC",
                INSTR_VAL,
            },
        [0x76] =
            {
                .addr_mode = addr_zpg_X,
                .instr_mem = instr_ROR,
                .cycles = 6,
                .page_cycles = 0,
                .mnemonic = "ROR",
                INSTR_MEM,
            },
        [0x77] =
            {
                .addr_mode = addr_zpg_X,
                .instr_mem = instr_RRA,
                .cycles = 6,
                .page_cycles = 0,
                .mnemonic = "RRA",
                INSTR_MEM,
            },
        [0x78] =
            {
                .addr_mode = NULL,
                .instr_none = instr_SEI,
                .cycles = 2,
                .page_cycles = 0,
                .mnemonic = "SEI",
                INSTR_NONE,
            },
        [0x79] =
            {
                .addr_mode = addr_abs_Y,
                .instr_val = instr_ADC,
                .cycles = 4,
                .page_cycles = 1,
                .mnemonic = "ADC",
                INSTR_VAL,
            },
        [0x7A] =
            {
                .addr_mode = NULL,
                .instr_none = instr_NOP,
                .cycles = 2,
                .page_cycles = 0,
                .mnemonic = "NOP",
                INSTR_NONE,
            }, // NOP implied, PC increment handled externally
        [0x7B] =
            {
                .addr_mode = addr_abs_X,
                .instr_mem = instr_RRA,
                .cycles = 7,
                .page_cycles = 0,
                .mnemonic = "RRA",
                INSTR_MEM,
            },
        [0x7C] =
            {
                .addr_mode = addr_abs_X,
                .instr_addr = NULL,
                .cycles = 4,
                .page_cycles = 1,
                .mnemonic = "NOP",
                INSTR_ADDR,
            }, // Unofficial NOP or placeholder
        [0x7D] =
            {
                .addr_mode = addr_abs_X,
                .instr_val = instr_ADC,
                .cycles = 4,
                .page_cycles = 1,
                .mnemonic = "ADC",
                INSTR_VAL,
            },
        [0x7E] =
            {
                .addr_mode = addr_abs_X,
                .instr_mem = instr_ROR,
                .cycles = 7,
                .page_cycles = 0,
                .mnemonic = "ROR",
                INSTR_MEM,
            },
        [0x7F] =
            {
                .addr_mode = addr_abs_X,
                .instr_mem = instr_RRA,
                .cycles = 7,
                .page_cycles = 0,
                .mnemonic = "RRA",
                INSTR_MEM,
            },

        [0x80] =
            {
                .addr_mode = addr_imm,
                .instr_addr = NULL,
                .cycles = 2,
                .page_cycles = 0,
                .mnemonic = "NOP",
                INSTR_ADDR,
            },
        [0x81] =
            {
                .addr_mode = addr_X_ind,
                .instr_addr = instr_STA,
                .cycles = 6,
                .page_cycles = 0,
                .mnemonic = "STA",
                .instr_type = INSTR_ADDR,
            },
        [0x82] =
            {
                .addr_mode = addr_imm,
                .instr_none = NULL,
                .cycles = 2,
                .page_cycles = 0,
                .mnemonic = "NOP",
                INSTR_NONE,
            },
        [0x83] =
            {
                .addr_mode = addr_X_ind,
                .instr_addr = instr_SAX,
                .cycles = 6,
                .page_cycles = 0,
                .mnemonic = "SAX",
                INSTR_ADDR,
            },
        [0x84] =
            {
                .addr_mode = addr_zpg,
                .instr_addr = instr_STY,
                .cycles = 3,
                .page_cycles = 0,
                .mnemonic = "STY",
                .instr_type = INSTR_ADDR,
            },
        [0x85] =
            {
                .addr_mode = addr_zpg,
                .instr_addr = instr_STA,
                .cycles = 3,
                .page_cycles = 0,
                .mnemonic = "STA",
                INSTR_ADDR,
            },
        [0x86] =
            {
                .addr_mode = addr_zpg,
                .instr_addr = instr_STX,
                .cycles = 3,
                .page_cycles = 0,
                .mnemonic = "STX",
                INSTR_ADDR,
            },
        [0x87] =
            {
                .addr_mode = addr_zpg,
                .instr_addr = instr_SAX,
                .cycles = 3,
                .page_cycles = 0,
                .mnemonic = "SAX",
                INSTR_ADDR,
            },
        [0x88] =
            {
                .addr_mode = NULL,
                .instr_none = instr_DEY,
                .cycles = 2,
                .page_cycles = 0,
                .mnemonic = "DEY",
                INSTR_NONE,
            },
        [0x89] =
            {
                .addr_mode = addr_imm,
                .instr_none = NULL,
                .cycles = 2,
                .page_cycles = 0,
                .mnemonic = "NOP #",
                INSTR_NONE,
            },
        [0x8A] =
            {
                .addr_mode = NULL,
                .instr_none = instr_TXA,
                .cycles = 2,
                .page_cycles = 0,
                .mnemonic = "TXA",
                INSTR_NONE,
            },
        [0x8C] =
            {
                .addr_mode = addr_abs,
                .instr_addr = instr_STY,
                .cycles = 4,
                .page_cycles = 0,
                .mnemonic = "STY abs",
                INSTR_ADDR,
            },
        [0x8D] =
            {
                .addr_mode = addr_abs,
                .instr_addr = instr_STA,
                .cycles = 4,
                .page_cycles = 0,
                .mnemonic = "STA abs",
                INSTR_ADDR,
            },
        [0x8E] =
            {
                .addr_mode = addr_abs,
                .instr_addr = instr_STX,
                .cycles = 4,
                .page_cycles = 0,
                .mnemonic = "STX abs",
                INSTR_ADDR,
            },
        [0x8F] =
            {
                .addr_mode = addr_abs,
                .instr_addr = instr_SAX,
                .cycles = 4,
                .page_cycles = 0,
                .mnemonic = "SAX abs",
                INSTR_ADDR,
            },

        [0x90] =
            {
                .addr_mode = NULL,
                .instr_none = instr_BCC,
                .cycles = 2,
                .page_cycles = 1,
                .mnemonic = "BCC rel",
                INSTR_NONE,
            },
        [0x91] =
            {
                .addr_mode = addr_ind_Y,
                .instr_addr = instr_STA,
                .cycles = 6,
                .page_cycles = 0,
                .mnemonic = "STA ind,Y",
                INSTR_ADDR,
            },
        [0x94] =
            {
                .addr_mode = addr_zpg_X,
                .instr_addr = instr_STY,
                .cycles = 4,
                .page_cycles = 0,
                .mnemonic = "STY zpg,X",
                INSTR_ADDR,
            },
        [0x95] =
            {
                .addr_mode = addr_zpg_X,
                .instr_addr = instr_STA,
                .cycles = 4,
                .page_cycles = 0,
                .mnemonic = "STA zpg,X",
                INSTR_ADDR,
            },
        [0x96] =
            {
                .addr_mode = addr_zpg_Y,
                .instr_addr = instr_STX,
                .cycles = 4,
                .page_cycles = 0,
                .mnemonic = "STX zpg,Y",
                INSTR_ADDR,
            },
        [0x97] =
            {
                .addr_mode = addr_zpg_Y,
                .instr_addr = instr_SAX,
                .cycles = 4,
                .page_cycles = 0,
                .mnemonic = "SAX zpg,Y",
                INSTR_ADDR,
            },
        [0x98] =
            {
                .addr_mode = NULL,
                .instr_none = instr_TYA,
                .cycles = 2,
                .page_cycles = 0,
                .mnemonic = "TYA",
                INSTR_NONE,
            },
        [0x99] =
            {
                .addr_mode = addr_abs_Y,
                .instr_addr = instr_STA,
                .cycles = 5,
                .page_cycles = 0,
                .mnemonic = "STA abs,Y",
                INSTR_ADDR,
            },
        [0x9A] =
            {
                .addr_mode = NULL,
                .instr_none = instr_TXS,
                .cycles = 2,
                .page_cycles = 0,
                .mnemonic = "TXS",
                INSTR_NONE,
            },
        [0x9D] =
            {
                .addr_mode = addr_abs_X,
                .instr_addr = instr_STA,
                .cycles = 5,
                .page_cycles = 0,
                .mnemonic = "STA abs,X",
                INSTR_ADDR,
            },
        [0xA0] =
            {
                addr_imm,
                .instr_addr = instr_LDY,
                2,
                0,
                "LDY #",
                INSTR_ADDR,
            },
        [0xA1] =
            {
                addr_X_ind,
                .instr_addr = instr_LDA,
                6,
                0,
                "LDA ind,X",
                INSTR_ADDR,
            },
        [0xA2] =
            {
                addr_imm,
                .instr_addr = instr_LDX,
                2,
                0,
                "LDX #",
                INSTR_ADDR,
            },
        [0xA3] =
            {
                addr_X_ind,
                .instr_val = instr_LAX,
                6,
                0,
                "LAX ind,X",
                INSTR_VAL,
            },
        [0xA4] =
            {
                addr_zpg,
                .instr_addr = instr_LDY,
                3,
                0,
                "LDY zpg",
                INSTR_ADDR,
            },
        [0xA5] =
            {
                addr_zpg,
                .instr_addr = instr_LDA,
                3,
                0,
                "LDA zpg",
                INSTR_ADDR,
            },
        [0xA6] =
            {
                addr_zpg,
                .instr_addr = instr_LDX,
                3,
                0,
                "LDX zpg",
                INSTR_ADDR,
            },
        [0xA7] =
            {
                addr_zpg,
                .instr_val = instr_LAX,
                3,
                0,
                "LAX zpg",
                INSTR_VAL,
            },
        [0xA8] =
            {
                NULL,
                .instr_none = instr_TAY,
                2,
                0,
                "TAY impl",
                INSTR_NONE,
            },
        [0xA9] =
            {
                addr_imm,
                .instr_addr = instr_LDA,
                2,
                0,
                "LDA #",
                INSTR_ADDR,
            },
        [0xAA] =
            {
                NULL,
                .instr_none = instr_TAX,
                2,
                0,
                "TAX impl",
                INSTR_NONE,
            },
        [0xAC] =
            {
                addr_abs,
                .instr_addr = instr_LDY,
                4,
                1,
                "LDY abs",
                INSTR_ADDR,
            },
        [0xAD] =
            {
                addr_abs,
                .instr_addr = instr_LDA,
                4,
                1,
                "LDA abs",
                INSTR_ADDR,
            },
        [0xAE] =
            {
                addr_abs,
                .instr_addr = instr_LDX,
                4,
                1,
                "LDX abs",
                INSTR_ADDR,
            },
        [0xAF] =
            {
                addr_abs,
                .instr_val = instr_LAX,
                4,
                1,
                "LAX abs",
                INSTR_VAL,
            },
        [0xB0] =
            {
                NULL,
                .instr_none = instr_BCS,
                2,
                0,
                "BCS",
                INSTR_NONE,
            },
        [0xB1] =
            {
                addr_ind_Y,
                .instr_addr = instr_LDA,
                5,
                1,
                "LDA (ind),Y",
                INSTR_ADDR,
            },
        [0xB3] =
            {
                addr_ind_Y,
                .instr_val = instr_LAX,
                5,
                1,
                "LAX (ind),Y",
                INSTR_VAL,
            },
        [0xB4] =
            {
                addr_zpg_X,
                .instr_addr = instr_LDY,
                4,
                0,
                "LDY zpg,X",
                INSTR_ADDR,
            },
        [0xB5] =
            {
                addr_zpg_X,
                .instr_addr = instr_LDA,
                4,
                0,
                "LDA zpg,X",
                INSTR_ADDR,
            },
        [0xB6] =
            {
                addr_zpg_Y,
                .instr_addr = instr_LDX,
                4,
                0,
                "LDX zpg,Y",
                INSTR_ADDR,
            },
        [0xB7] =
            {
                addr_zpg_Y,
                .instr_val = instr_LAX,
                4,
                0,
                "LAX zpg,Y",
                INSTR_VAL,
            },
        [0xB8] =
            {
                NULL,
                .instr_none = instr_CLV,
                2,
                0,
                "CLV",
                INSTR_NONE,
            },
        [0xB9] =
            {
                addr_abs_Y,
                .instr_addr = instr_LDA,
                4,
                1,
                "LDA abs,Y",
                INSTR_ADDR,
            },
        [0xBA] =
            {
                NULL,
                .instr_none = instr_TSX,
                2,
                0,
                "TSX",
                INSTR_NONE,
            },
        [0xBC] =
            {
                addr_abs_X,
                .instr_addr = instr_LDY,
                4,
                1,
                "LDY abs,X",
                INSTR_ADDR,
            },
        [0xBD] =
            {
                addr_abs_X,
                .instr_addr = instr_LDA,
                4,
                1,
                "LDA abs,X",
                INSTR_ADDR,
            },
        [0xBE] =
            {
                addr_abs_Y,
                .instr_addr = instr_LDX,
                4,
                1,
                "LDX abs,Y",
                INSTR_ADDR,
            },
        [0xBF] =
            {
                addr_abs_Y,
                .instr_val = instr_LAX,
                4,
                1,
                "LAX abs,Y",
                INSTR_VAL,
            },
        [0xC0] =
            {
                addr_imm,
                .instr_val = instr_CPY,
                2,
                0,
                "CPY #",
                INSTR_VAL,
            },
        [0xC1] =
            {
                addr_X_ind,
                .instr_val = instr_CMP,
                6,
                0,
                "CMP X,ind",
                INSTR_VAL,
            },
        [0xC2] =
            {
                addr_imm,
                .instr_none = instr_NOP,
                2,
                0,
                "NOP #",
                INSTR_NONE,
            },
        [0xC3] =
            {
                addr_X_ind,
                .instr_mem = instr_DCP,
                8,
                0,
                "DCP X,ind",
                INSTR_MEM,
            },
        [0xC4] =
            {
                addr_zpg,
                .instr_val = instr_CPY,
                3,
                0,
                "CPY zpg",
                INSTR_VAL,
            },
        [0xC5] =
            {
                addr_zpg,
                .instr_val = instr_CMP,
                3,
                0,
                "CMP zpg",
                INSTR_VAL,
            },
        [0xC6] =
            {
                addr_zpg,
                .instr_mem = instr_DEC,
                5,
                0,
                "DEC zpg",
                INSTR_MEM,
            },
        [0xC7] =
            {
                addr_zpg,
                .instr_mem = instr_DCP,
                5,
                0,
                "DCP zpg",
                INSTR_MEM,
            },
        [0xC8] =
            {
                NULL,
                .instr_none = instr_INY,
                2,
                0,
                "INY",
                INSTR_NONE,
            },
        [0xC9] =
            {
                addr_imm,
                .instr_val = instr_CMP,
                2,
                0,
                "CMP #",
                INSTR_VAL,
            },
        [0xCA] =
            {
                NULL,
                .instr_none = instr_DEX,
                2,
                0,
                "DEX",
                INSTR_NONE,
            },
        [0xCC] =
            {
                addr_abs,
                .instr_val = instr_CPY,
                4,
                0,
                "CPY abs",
                INSTR_VAL,
            },
        [0xCD] =
            {
                addr_abs,
                .instr_val = instr_CMP,
                4,
                0,
                "CMP abs",
                INSTR_VAL,
            },
        [0xCE] =
            {
                addr_abs,
                .instr_mem = instr_DEC,
                6,
                0,
                "DEC abs",
                INSTR_MEM,
            },
        [0xCF] =
            {
                addr_abs,
                .instr_mem = instr_DCP,
                6,
                0,
                "DCP abs",
                INSTR_MEM,
            },

        [0xD0] =
            {
                NULL,
                .instr_none = instr_BNE,
                2,
                0,
                "BNE rel",
                INSTR_NONE,
            },
        [0xD1] =
            {
                addr_ind_Y,
                .instr_val = instr_CMP,
                5,
                1,
                "CMP ind,Y",
                INSTR_VAL,
            },
        [0xD3] =
            {
                addr_ind_Y,
                .instr_mem = instr_DCP,
                8,
                0,
                "DCP ind,Y",
                INSTR_MEM,
            },
        [0xD4] =
            {
                .addr_mode = addr_zpg_X,
                .instr_addr = NULL,
                .cycles = 4,
                .page_cycles = 0,
                .mnemonic = "NOP",
                INSTR_ADDR,
            }, // NOP zpg,X, PC increment done in switch
        [0xD5] =
            {
                addr_zpg_X,
                .instr_val = instr_CMP,
                4,
                0,
                "CMP zpg,X",
                INSTR_VAL,
            },
        [0xD6] =
            {
                addr_zpg_X,
                .instr_mem = instr_DEC,
                6,
                0,
                "DEC zpg,X",
                INSTR_MEM,
            },
        [0xD7] =
            {
                addr_zpg_X,
                .instr_mem = instr_DCP,
                6,
                0,
                "DCP zpg,X",
                INSTR_MEM,
            },
        [0xD8] =
            {
                NULL,
                .instr_none = instr_CLD,
                2,
                0,
                "CLD",
                INSTR_NONE,
            },
        [0xD9] =
            {
                addr_abs_Y,
                .instr_val = instr_CMP,
                4,
                1,
                "CMP abs,Y",
                INSTR_VAL,
            },
        [0xDA] =
            {
                .addr_mode = NULL,
                .instr_none = instr_NOP,
                .cycles = 2,
                .page_cycles = 0,
                .mnemonic = "NOP",
                INSTR_NONE,
            }, // NOP implied, PC increment handled externally
        [0xDB] =
            {
                addr_abs_Y,
                .instr_mem = instr_DCP,
                7,
                0,
                "DCP abs,Y",
                INSTR_MEM,
            },
        [0xDC] =
            {
                addr_abs_X,
                .instr_addr = NULL,
                4,
                1,
                "NOP abs,X",
                INSTR_ADDR,
            }, // This one exists in your code: cpu->PC++;
               // emulate_6502_cycle(cyc);
        [0xDD] =
            {
                addr_abs_X,
                .instr_val = instr_CMP,
                4,
                1,
                "CMP abs,X",
                INSTR_VAL,
            },
        [0xDE] =
            {
                addr_abs_X,
                .instr_mem = instr_DEC,
                7,
                0,
                "DEC abs,X",
                INSTR_MEM,
            },
        [0xDF] =
            {
                addr_abs_X,
                .instr_mem = instr_DCP,
                7,
                0,
                "DCP abs,X",
                INSTR_MEM,
            },

        [0xE0] =
            {
                addr_imm,
                .instr_val = instr_CPX,
                2,
                0,
                "CPX #",
                INSTR_VAL,
            },
        [0xE1] =
            {
                addr_X_ind,
                .instr_val = instr_SBC,
                6,
                0,
                "SBC X,ind",
                INSTR_VAL,
            },
        [0xE2] =
            {
                NULL,
                .instr_none = NULL,
                2,
                0,
                "NOP imm",
                INSTR_NONE,
            },
        [0xE3] =
            {
                addr_X_ind,
                .instr_mem = instr_ISC,
                8,
                0,
                "ISC X,ind",
                INSTR_MEM,
            },
        [0xE4] =
            {
                addr_zpg,
                .instr_val = instr_CPX,
                3,
                0,
                "CPX zpg",
                INSTR_VAL,
            },
        [0xE5] =
            {
                addr_zpg,
                .instr_val = instr_SBC,
                3,
                0,
                "SBC zpg",
                INSTR_VAL,
            },
        [0xE6] =
            {
                addr_zpg,
                .instr_mem = instr_INC,
                5,
                0,
                "INC zpg",
                INSTR_MEM,
            },
        [0xE7] =
            {
                addr_zpg,
                .instr_mem = instr_ISC,
                5,
                0,
                "ISC zpg",
                INSTR_MEM,
            },
        [0xE8] =
            {
                NULL,
                .instr_none = instr_INX,
                2,
                0,
                "INX",
                INSTR_NONE,
            },
        [0xE9] =
            {
                addr_imm,
                .instr_val = instr_SBC,
                2,
                0,
                "SBC #",
                INSTR_VAL,
            },
        [0xEA] =
            {
                NULL,
                .instr_none = instr_NOP,
                2,
                0,
                "NOP",
                INSTR_NONE,
            },
        [0xEB] =
            {
                addr_imm,
                .instr_val = instr_SBC,
                2,
                0,
                "USBC #",
                INSTR_VAL,
            },
        [0xEC] =
            {
                addr_abs,
                .instr_val = instr_CPX,
                4,
                0,
                "CPX abs",
                INSTR_VAL,
            },
        [0xED] =
            {
                addr_abs,
                .instr_val = instr_SBC,
                4,
                0,
                "SBC abs",
                INSTR_VAL,
            },
        [0xEE] =
            {
                addr_abs,
                .instr_mem = instr_INC,
                6,
                0,
                "INC abs",
                INSTR_MEM,
            },
        [0xEF] =
            {
                addr_abs,
                .instr_mem = instr_ISC,
                6,
                0,
                "ISC abs",
                INSTR_MEM,
            },

        [0xF0] =
            {
                NULL,
                .instr_none = instr_BEQ,
                2,
                0,
                "BEQ rel",
                INSTR_NONE,
            },
        [0xF1] =
            {
                addr_ind_Y,
                .instr_val = instr_SBC,
                5,
                1,
                "SBC ind,Y",
                INSTR_VAL,
            },
        [0xF3] =
            {
                addr_ind_Y,
                .instr_mem = instr_ISC,
                8,
                0,
                "ISC ind,Y",
                INSTR_MEM,
            },
        [0xF4] =
            {
                addr_zpg_X,
                .instr_addr = NULL,
                4,
                0,
                "NOP zpg,X",
                INSTR_ADDR,
            },
        [0xF5] =
            {
                addr_zpg_X,
                .instr_val = instr_SBC,
                4,
                0,
                "SBC zpg,X",
                INSTR_VAL,
            },
        [0xF6] =
            {
                addr_zpg_X,
                .instr_mem = instr_INC,
                6,
                0,
                "INC zpg,X",
                INSTR_MEM,
            },
        [0xF7] =
            {
                addr_zpg_X,
                .instr_mem = instr_ISC,
                6,
                0,
                "ISC zpg,X",
                INSTR_MEM,
            },
        [0xF8] =
            {
                NULL,
                .instr_none = instr_SED,
                2,
                0,
                "SED",
                INSTR_NONE,
            },
        [0xF9] =
            {
                addr_abs_Y,
                .instr_val = instr_SBC,
                4,
                1,
                "SBC abs,Y",
                INSTR_VAL,
            },
        [0xFA] =
            {
                NULL,
                .instr_none = instr_NOP,
                2,
                0,
                "NOP imp",
                INSTR_NONE,
            },
        [0xFB] =
            {
                addr_abs_Y,
                .instr_mem = instr_ISC,
                7,
                0,
                "ISC abs,Y",
                INSTR_MEM,
            },
        [0xFC] =
            {
                addr_abs_X,
                .instr_addr = NULL,
                4,
                1,
                "NOP abs,X",
                INSTR_ADDR,
            },
        [0xFD] =
            {
                addr_abs_X,
                .instr_val = instr_SBC,
                4,
                1,
                "SBC abs,X",
                INSTR_VAL,
            },
        [0xFE] =
            {
                addr_abs_X,
                .instr_mem = instr_INC,
                7,
                0,
                "INC abs,X",
                INSTR_MEM,
            },
        [0xFF] =
            {
                addr_abs_X,
                .instr_mem = instr_ISC,
                7,
                0,
                "ISC abs,X",
                INSTR_MEM,
            },
};

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
  LOG("A: %x\n", cpu->A);
  LOG("X: %x\n", cpu->X);
  LOG("Y: %x\n", cpu->Y);
  LOG("Cycle: %d\n\n", cpu->cycles);

  // dump_log_file(cpu);

#if NES_TEST_ROM == 1
  if (cpu->PC == 0x7001) {
    printf("Reached addressed originally pushed to stack!");
    exit(0);
  }
#endif

  if (dma_active_flag) {
    if (dma_cycles > 0) {
      dma_cycles--;
      cpu->cycles++;
      ppu_exec(cpu);
      return;
    } else {
      dma_active_flag = 0;
    }
  }
  Opcode opcode = lookup_table[instr];
  cyc = opcode.cycles;
  uint8_t val;
  uint16_t addr;

  switch (opcode.instr_type) {
  case INSTR_NONE:
    opcode.instr_none(cpu);
    break;
  case INSTR_VAL:
    val = memory[opcode.addr_mode(cpu)];
    opcode.instr_val(cpu, val);
    break;
  case INSTR_MEM:
    addr = opcode.addr_mode(cpu);
    opcode.instr_mem(cpu, &memory[addr]);
    break;
  case INSTR_ADDR:
    addr = opcode.addr_mode(cpu);
    if (opcode.instr_addr == NULL) {
      cpu->PC++;
    } else {
      opcode.instr_addr(cpu, addr);
    }
    break;
  case INSTR_ACC:
    opcode.instr_mem(cpu, &cpu->A);
  }

  cyc += page_crossed ? opcode.page_cycles : 0;

  for (int i = 0; i < cyc; i++) {
    ppu_exec(cpu);
  }
  cpu->cycles += cyc;
  cyc = 0;
  page_crossed = 0;

  // PPU set NMI flag
  if (cpu->ppu->nmi_flag) {

    // Execute NMI subroutine
    cpu_nmi_triggered(cpu);

    // Reset nmi flag
    cpu->ppu->nmi_flag = 0;
    return;
  }

  // print("}\n");
}
