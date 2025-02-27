#include "cpu.h"
#include <stdint.h>

uint8_t stack[0xFF] = {0}; 

void cpu_init(Cpu6502 *cpu) {

    // Initialize stack pointer to the top;
    cpu->S = 0xFF;

    cpu->A = 0x0;
    cpu->XR = 0x0;
    cpu->YR = 0x0;
    cpu->PC = 0x0;
    cpu->P = 0x0;
} 



void cpu_execute(Cpu6502 *cpu) {

    // Placeholder for instruction
    uint8_t instr;

    // Compare the upper 4 bits
    switch ((instr >> 4) & 0x0F) {
        
        // 0x0X
        case 0x0:

            switch (instr & 0x0F) {
                // 0x00
                // BRK impl
                case 0x0:
                    break;

                // 0x01
                // ORA X,ind
                case 0x1:
                    break;

                // 0x05
                // ORA zpg
                case 0x5:
                    break;

                // 0x06
                // ASL zpg
                case 0x6:
                    break;
                
                // 0x08
                // PHP impl
                case 0x8:
                    break;

                // 0x09
                // ORA #
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