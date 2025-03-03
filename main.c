#include <stdio.h>
#include "cpu.h"

int main() {

    Cpu6502 cpu;

    load_rom(&cpu, "test/6502_functional_test.bin");
    cpu_init(&cpu);


    // logging
    const char *filename = "log.txt";

    FILE *log = fopen(filename, "a");

    fprintf(log, "Program started\n");

    while (1) {
         dump_log(&cpu, log);
         cpu_execute(&cpu);

    }

}
