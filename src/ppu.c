/*
 * Ricoh 2C02
 */
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <SDL/SDL.h>

#define SCR_WIDTH 256
#define SCR_HEIGHT 240
#define SCR_BPP 8

typedef uint8_t byte;
typedef uint16_t addr;

SDL_Surface * screen;

extern struct st_cpustate {
	addr PC; /* program counter */
	byte SP; /* stack counter */
	byte A; /* accumulator register */
	byte X; /* x register */
	byte Y; /* y register */
	byte NMI; /* non masked interrupt */
	byte IRQ; /* maskeable interrupt */
	union {
		byte P; /* processor state flags */
		struct {
			byte C:1; /* carry */
			byte Z:1; /* zero */
			byte I:1; /* interrupt disable */
			byte D:1; /* decimal mode (not in 2A03) */
			byte B:1; /* break */
			byte _unused_:1;
			byte V:1; /* overflow */
			byte N:1; /* negative */
		};
	};
} cpustate;


byte ppumemory[0x10000];

void ppu_init()
{
	flockfile(stdout);
	printf("ppu: Booting PPU...\n");
	printf("ppu: Initializing screen...\n");
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
		exit(1);

	screen = SDL_SetVideoMode(SCR_WIDTH, SCR_HEIGHT, SCR_BPP, SDL_HWSURFACE);
	if (!screen) {
		SDL_Quit();
		exit(1);
	}
	funlockfile(stdout);
};

void show_sprite(byte number)
{
	int i, j, color, row, column;
	Uint32 *pixmem32;
	Uint32 colour;

	if(SDL_MUSTLOCK(screen)) {
		if(SDL_LockSurface(screen) < 0) return;
	}

	for (i=0 + number*8; i<8 + number*8; i++) {
		for (j=0; j<8; j++) {
			color = ((ppumemory[i]&(1<<j))!=0x0) + 2*((ppumemory[i+0x08]&(1<<j))!=0x0);
			color *= 75;
			colour = SDL_MapRGB(screen->format, color, color, color);
			row = i%240;
			column = j + 8 * (i/240);
			pixmem32 = (Uint32*)(screen->pixels + column + row * screen->w);
			*pixmem32 = colour;
		}
	};

	if(SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);
};

void ppu_run()
{
	//const struct timespec vblank_period = {0, 16666666};
	const struct timespec vblank_period = {2, 16666666};

	while (1) {
		int i;
		for(i=0; i<512; i++)
			show_sprite(i);
		SDL_Flip(screen);
		printf("ppu: Vblank draw\n");
		nanosleep(&vblank_period, NULL);
		cpustate.NMI = 1;
	};
};

void ppu_load(byte * prg, size_t size)
{
	memcpy(ppumemory, prg, size);
};
