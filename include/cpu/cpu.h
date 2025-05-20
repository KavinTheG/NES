#ifndef CPU_H
#define CPU_H

#include "ppu.h"
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>

typedef struct Cpu6502 {

  int cycles;

  uint8_t instr;

  // Accumulator
  uint8_t A;

  // X and Y register
  uint8_t X;
  uint8_t Y;

  // Program Counter
  uint16_t PC;

  // Stack pointer
  // Initialize it to the top
  uint8_t S;

  /** Status register
  Bit 7 - N: Negative
  Bit 6 - V: Overflow
  Bit 5 - n/a
  Bit 4 - B: Break
  Bit 3 - D: Decimal
  Bit 2 - I: Interrupt
  Bit 1 - Z: Zero
  Bit 0 - C: Carry
  **/
  unsigned char P[8];

  PPU *ppu;

  uint8_t ctrl_latch_state;
  int ctrl_bit_index;

  unsigned char nmi_state;
  unsigned char strobe;
} Cpu6502;

void cpu_init(Cpu6502 *cpu);

void load_cpu_memory(Cpu6502 *cpu, unsigned char *prg_rom, int prg_size);

// void load_cpu_mem(Cpu6502 *cpu, char *filename);
void load_test_rom(Cpu6502 *cpu);
void cpu_execute(Cpu6502 *cpu);

void push_stack(uint8_t lower_addr, uint8_t val);
void dump_log(Cpu6502 *cpu, FILE *log);

// Address modes
uint16_t addr_abs(Cpu6502 *cpu);
uint16_t addr_abs_X(Cpu6502 *cpu);
uint16_t addr_abs_Y(Cpu6502 *cpu);
uint16_t addr_imm(Cpu6502 *cpu);
uint16_t addr_ind(Cpu6502 *cpu);
uint16_t addr_X_ind(Cpu6502 *cpu);
uint16_t addr_ind_Y(Cpu6502 *cpu);
uint16_t addr_zpg(Cpu6502 *cpu);
uint16_t addr_zpg_X(Cpu6502 *cpu);
uint16_t addr_zpg_Y(Cpu6502 *cpu);

// Access
void instr_LDA(Cpu6502 *cpu, uint16_t addr);
void instr_STA(Cpu6502 *cpu, uint16_t addr);
void instr_LDX(Cpu6502 *cpu, uint16_t addr);
void instr_STX(Cpu6502 *cpu, uint16_t addr);
void instr_LDY(Cpu6502 *cpu, uint16_t addr);
void instr_STY(Cpu6502 *cpu, uint16_t addr);

// Transfer
void instr_TAX(Cpu6502 *cpu);
void instr_TXA(Cpu6502 *cpu);
void instr_TAY(Cpu6502 *cpu);
void instr_TYA(Cpu6502 *cpu);

// Arithmetic
void instr_ADC(Cpu6502 *cpu, uint8_t oper);
void instr_SBC(Cpu6502 *cpu, uint8_t oper);
void instr_INC(Cpu6502 *cpu, uint8_t *M);
void instr_DEC(Cpu6502 *cpu, uint8_t *M);
void instr_INX(Cpu6502 *cpu);
void instr_DEX(Cpu6502 *cpu);
void instr_INY(Cpu6502 *cpu);
void instr_DEY(Cpu6502 *cpu);

// Shift
void instr_ASL(Cpu6502 *cpu, uint8_t *M);
void instr_LSR(Cpu6502 *cpu, uint8_t *M);
void instr_ROL(Cpu6502 *cpu, uint8_t *M);
void instr_ROR(Cpu6502 *cpu, uint8_t *M);

// Bitwise
void instr_AND(Cpu6502 *cpu, uint8_t M);
void instr_ORA(Cpu6502 *cpu, uint8_t M);
void instr_EOR(Cpu6502 *cpu, uint8_t M);
void instr_BIT(Cpu6502 *cpu, uint8_t M);

// Compare
void instr_CMP(Cpu6502 *cpu, uint8_t M);
void instr_CPX(Cpu6502 *cpu, uint8_t M);
void instr_CPY(Cpu6502 *cpu, uint8_t M);

// Branch
void instr_BCC(Cpu6502 *cpu);
void instr_BCS(Cpu6502 *cpu);
void instr_BEQ(Cpu6502 *cpu);
void instr_BNE(Cpu6502 *cpu);
void instr_BPL(Cpu6502 *cpu);
void instr_BMI(Cpu6502 *cpu);
void instr_BVC(Cpu6502 *cpu);
void instr_BVS(Cpu6502 *cpu);

// Jump
void instr_JMP(Cpu6502 *cpu, uint16_t addr);
void instr_JSR(Cpu6502 *cpu, uint16_t addr);
void instr_RTS(Cpu6502 *cpu);
void instr_BRK(Cpu6502 *cpu);
void instr_RTI(Cpu6502 *cpu);

// Stack
void instr_PHA(Cpu6502 *cpu);
void instr_PLA(Cpu6502 *cpu);
void instr_PHP(Cpu6502 *cpu);
void instr_PLP(Cpu6502 *cpu);
void instr_TXS(Cpu6502 *cpu);
void instr_TSX(Cpu6502 *cpu);

// Flags
void instr_CLC(Cpu6502 *cpu);
void instr_SEC(Cpu6502 *cpu);
void instr_CLI(Cpu6502 *cpu);
void instr_SEI(Cpu6502 *cpu);
void instr_CLD(Cpu6502 *cpu);
void instr_SED(Cpu6502 *cpu);
void instr_CLV(Cpu6502 *cpu);

// Other
void instr_NOP(Cpu6502 *cpu);

// illegal opcodes
void instr_LAX(Cpu6502 *cpu, uint8_t val);
void instr_SAX(Cpu6502 *cpu, uint16_t addr);

void instr_DCP(Cpu6502 *cpu, uint8_t *M);
void instr_ISC(Cpu6502 *cpu, uint8_t *M);

void instr_RLA(Cpu6502 *cpu, uint8_t *M);
void instr_RRA(Cpu6502 *cpu, uint8_t *M);
void instr_SLO(Cpu6502 *cpu, uint8_t *M);
void instr_SRE(Cpu6502 *cpu, uint8_t *M);

// PPU Functions
void set_ppu_w_reg(unsigned char ppu_w);
unsigned char get_ppu_w_reg();
uint16_t get_ppu_PPUADDR(Cpu6502 *cpu);
unsigned char get_ppu_PPUADDR_completed();
unsigned char get_ppu_PPUDATA_write();
unsigned char get_ppu_OAM_write();
unsigned char get_ppu_OAMDMA_write();

void get_ppu_dma_page(Cpu6502 *cpu, uint8_t *page_mem);

typedef uint16_t (*AddrMode)(Cpu6502 *cpu);
typedef void (*InstrAddr)(Cpu6502 *cpu, uint16_t addr);
typedef void (*InstrVal)(Cpu6502 *cpu, uint8_t val);
typedef void (*InstrMem)(Cpu6502 *cpu, uint8_t *M);
typedef void (*InstrNone)(Cpu6502 *cpu);

typedef enum { INSTR_NONE, INSTR_ADDR, INSTR_VAL, INSTR_MEM } InstrType;

typedef struct Opcode {
  AddrMode addr_mode;
  union {
    InstrAddr instr_addr;
    InstrVal instr_val;
    InstrMem instr_mem;
    InstrNone instr_none;
  };
  uint8_t cycles;
  uint8_t page_cycles;
  const char *mnemonic;
  InstrType instr_type;
} Opcode;

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
                .instr_none = instr_NOP,
                .cycles = 3,
                .page_cycles = 0,
                .mnemonic = "NOP zpg",
                INSTR_NONE,
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
                INSTR_MEM,
            },
        [0x0C] =
            {
                .addr_mode = addr_abs,
                .instr_none = instr_NOP,
                .cycles = 4,
                .page_cycles = 0,
                .mnemonic = "NOP abs",
                INSTR_NONE,
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
                .instr_none = instr_NOP,
                .cycles = 4,
                .page_cycles = 0,
                .mnemonic = "NOP zpg,X",
                INSTR_NONE,
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
                .instr_none = instr_NOP,
                .cycles = 4,
                .page_cycles = 1,
                .mnemonic = "NOP abs,X",
                INSTR_NONE,
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
                INSTR_MEM,
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
                .instr_none = NULL,
                .cycles = 4,
                .page_cycles = 0,
                .mnemonic = "NOP zpg,X",
                INSTR_NONE,
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
                .instr_none = instr_NOP,
                .cycles = 4,
                .page_cycles = 1,
                .mnemonic = "NOP abs,X",
                INSTR_NONE,
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
                .addr_mode = NULL,
                .instr_none = NULL,
                .cycles = 3,
                .page_cycles = 0,
                .mnemonic = "NOP",
                INSTR_NONE,
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
                INSTR_MEM,
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
                .addr_mode = NULL,
                .instr_none = NULL,
                .cycles = 4,
                .page_cycles = 0,
                .mnemonic = "NOP",
                INSTR_NONE,
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
                .instr_none = NULL,
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
                .instr_none = NULL,
                .cycles = 4,
                .page_cycles = 1,
                .mnemonic = "NOP",
                INSTR_NONE,
            }, // NOP abs,X with PC increment externally
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
                .addr_mode = NULL,
                .instr_none = NULL,
                .cycles = 3,
                .page_cycles = 0,
                .mnemonic = "NOP",
                INSTR_NONE,
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
                INSTR_MEM,
            },
        [0x6C] =
            {
                .addr_mode = NULL,
                .instr_none = NULL,
                .cycles = 5,
                .page_cycles = 0,
                .mnemonic = "JMP",
                INSTR_NONE,
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
                .addr_mode = NULL,
                .instr_none = NULL,
                .cycles = 4,
                .page_cycles = 0,
                .mnemonic = "NOP",
                INSTR_NONE,
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
                .instr_none = NULL,
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
                .instr_none = NULL,
                .cycles = 4,
                .page_cycles = 1,
                .mnemonic = "NOP",
                INSTR_NONE,
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
                .instr_none = NULL,
                .cycles = 2,
                .page_cycles = 0,
                .mnemonic = "NOP",
                INSTR_NONE,
            },
        [0x81] =
            {
                .addr_mode = addr_X_ind,
                .instr_addr = instr_STA,
                .cycles = 6,
                .page_cycles = 0,
                .mnemonic = "STA",
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
            },
        [0xB6] =
            {
                addr_zpg_Y,
                .instr_addr = instr_LDX,
                4,
                0,
                "LDX zpg,Y",
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
                .instr_none = NULL,
                4,
                1,
                "NOP abs,X",
                INSTR_NONE,
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
                .instr_none = NULL,
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
                NULL,
                .instr_none = NULL,
                4,
                0,
                "NOP zpg,X",
                INSTR_NONE,
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
                .instr_none = NULL,
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
                .instr_none = NULL,
                4,
                1,
                "NOP abs,X",
                INSTR_NONE,
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
};

#endif
