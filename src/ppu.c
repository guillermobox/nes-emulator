/*
 * Ricoh 2C02
 */
#include <time.h>
#include <stdio.h>

void ppu_init()
{
	flockfile(stdout);
	printf("ppu: Booting PPU...\n");
	funlockfile(stdout);
};

void ppu_run()
{
	const struct timespec {
		time_t tv_sec;        /* seconds */
		long   tv_nsec;       /* nanoseconds */
	} vblank_period = {0, 16666666};

	while (1) {
		printf("ppu: Vblank draw\n");
		nanosleep(&vblank_period, NULL);
	};
};
