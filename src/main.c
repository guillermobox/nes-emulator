/*
 * Guillermo Indalecio Fernandez
 *
 * NES-Emulator
 */

#include <stdint.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include "ines.h"
#include "cpu.h"
#include "ppu.h"

pthread_t cpu_thread, ppu_thread;

void stop_emulation()
{
	pthread_kill(cpu_thread, SIGKILL);
	pthread_join(cpu_thread, NULL);
	cpu_dump();
	/* better handling of interrupts would be a good idea */
	exit(EXIT_FAILURE);
};

void sig_interrupt(int sig)
{
	(void) sig;
	fprintf(stderr, "Captured interrupt signal!\n");
	stop_emulation();
};

void *cpu_thread_function(void *input)
{
	(void) input;
	cpu_run();
	return NULL;
};

void *ppu_thread_function(void *input)
{
	(void) input;
	ppu_run();
	return NULL;
};

int main(int argc, char *argv[])
{
	(void) argc;
	(void) argv;

	signal(SIGINT, sig_interrupt);

	cpu_init();
	ppu_init();
	read_ines(argv[1]);

	pthread_create(&cpu_thread, NULL, cpu_thread_function, NULL);
	pthread_create(&ppu_thread, NULL, ppu_thread_function, NULL);
	
	pthread_join(cpu_thread, NULL);
	pthread_join(ppu_thread, NULL);

	cpu_dump();

	return EXIT_SUCCESS;
};
