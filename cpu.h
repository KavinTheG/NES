#include <stdint.h>
#include <stdio.h>

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
    //uint8_t P;



} Cpu6502;


void cpu_init(Cpu6502 *cpu);

void load_rom(Cpu6502 *cpu, char *filename);
void load_test_rom(Cpu6502 *cpu);
void cpu_execute(Cpu6502 *cpu);

void push_stack(uint8_t lower_addr, uint8_t val);
//void dump_test_output();

void dump_log(Cpu6502 *cpu, FILE *log);

uint16_t get_PC(Cpu6502 *cpu);

// Access
void instr_LDA(Cpu6502 *cpu, uint8_t val);
void instr_STA(Cpu6502 *cpu, uint16_t addr);
void instr_LDX(Cpu6502 *cpu, uint8_t val);
void instr_STX(Cpu6502 *cpu, uint16_t addr);
void instr_LDY(Cpu6502 *cpu, uint8_t val);
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
void instr_JMP(Cpu6502 *cpu, uint8_t addr);
void instr_JSR(Cpu6502 *cpu, uint8_t addr);
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
