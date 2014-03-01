/*
 * Guillermo Indalecio Fernandez
 *
 * NES-Emulator
 */

#include "ines.h"
#include "cpu.h"
#include <stdint.h>

typedef uint8_t byte;

byte prg_inc[] = {
	0xe8, 0xc8, 0xe8, 0x00
};

byte prg_load[] = {
	0xa2, 0xf5, 0xae, 0x00, 0x80, 0xc8, 0xc8, 0xbe, 0x00, 0x80, 0xb6, 0x01, 0x00
};

byte prg_store[] = {
	0xa2, 0xf0, 0x86, 0x14, 0x00
};

int main(int argc, char *argv[])
{
	(void) argc;
	(void) argv;

	cpu_init();
	cpu_load(prg_store);
	cpu_run();
	cpu_dump();

	return 0;
};
