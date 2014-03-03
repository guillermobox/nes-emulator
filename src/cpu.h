#ifndef _CPU_H_
#define _CPU_H_

#include <stdint.h>

typedef uint8_t byte;

void cpu_init();
void cpu_load(byte*, size_t);
void cpu_run();
void cpu_dump();

#endif
