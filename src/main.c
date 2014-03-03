/*
 * Guillermo Indalecio Fernandez
 *
 * NES-Emulator
 */

#include <stdint.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include "ines.h"
#include "cpu.h"

void sig_interrupt(int sig)
{
	(void) sig;
	fprintf(stderr, "Captured interrupt signal!\n");
	cpu_dump();
	exit(EXIT_FAILURE);
};

int main(int argc, char *argv[])
{
	(void) argc;
	(void) argv;

	signal(SIGINT, sig_interrupt);

	cpu_init();
	read_ines(argv[1]);
	cpu_run();
	cpu_dump();

	return EXIT_SUCCESS;
};
