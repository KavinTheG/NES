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
void cpu_execute(Cpu6502 *cpu);

void push_stack(uint8_t lower_addr, uint8_t val);
//void dump_test_output();

void dump_log(Cpu6502 *cpu, FILE *log);
