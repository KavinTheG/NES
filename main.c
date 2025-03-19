#include <stdio.h>
#include <unistd.h>
#include "cpu.h"
#include "config.h"

int main() {

    Cpu6502 cpu;

    #if NES_TEST_ROM
    load_rom(&cpu, "test/nestest.nes");
    #endif
    cpu_init(&cpu);
    //load_test_rom(&cpu);
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
