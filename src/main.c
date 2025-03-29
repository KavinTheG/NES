#include <stdio.h>
#include <unistd.h>
#include "cpu.h"
#include "config.h"

#define NES_HEADER_SIZE 16


int main() {

    Cpu6502 cpu;

    #if NES_TEST_ROM
    load_cpu_mem(&cpu, "test/nestest.nes");
    #else
    load_test_rom(&cpu);
    #endif
    
    cpu_init(&cpu);

    sleep(1);

    // logging
    const char *filename = "log.txt";

    FILE *log = fopen(filename, "a");

    fprintf(log, "Program started\n");

    while (1) {
        //dump_log(&cpu, log);
        cpu_execute(&cpu);


    }

}






