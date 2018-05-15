/*
 * Ricoh 2C02
 */
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <SDL/SDL.h>
#include "input.h"

#define SCR_WIDTH 256
#define SCR_HEIGHT 240
#define SCR_BPP 8
#define SCR_SCALE 3

typedef uint8_t byte;
typedef uint16_t addr;

SDL_Surface * screen = NULL;

SDL_Color sdlpalette[64];
byte palette[64][3] = {
	{0x75, 0x75, 0x75},
	{0x27, 0x1B, 0x8F},
	{0x00, 0x00, 0xAB},
	{0x47, 0x00, 0x9F},
	{0x8F, 0x00, 0x77},
	{0xAB, 0x00, 0x13},
	{0xA7, 0x00, 0x00},
	{0x7F, 0x0B, 0x00},
	{0x43, 0x2F, 0x00},
	{0x00, 0x47, 0x00},
	{0x00, 0x51, 0x00},
	{0x00, 0x3F, 0x17},
	{0x1B, 0x3F, 0x5F},
	{0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00},
	{0xBC, 0xBC, 0xBC},
	{0x00, 0x73, 0xEF},
	{0x23, 0x3B, 0xEF},
	{0x83, 0x00, 0xF3},
	{0xBF, 0x00, 0xBF},
	{0xE7, 0x00, 0x5B},
	{0xDB, 0x2B, 0x00},
	{0xCB, 0x4F, 0x0F},
	{0x8B, 0x73, 0x00},
	{0x00, 0x97, 0x00},
	{0x00, 0xAB, 0x00},
	{0x00, 0x93, 0x3B},
	{0x00, 0x83, 0x8B},
	{0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00},
	{0xFF, 0xFF, 0xFF},
	{0x3F, 0xBF, 0xFF},
	{0x5F, 0x97, 0xFF},
	{0xA7, 0x8B, 0xFD},
	{0xF7, 0x7B, 0xFF},
	{0xFF, 0x77, 0xB7},
	{0xFF, 0x77, 0x63},
	{0xFF, 0x9B, 0x3B},
	{0xF3, 0xBF, 0x3F},
	{0x83, 0xD3, 0x13},
	{0x4F, 0xDF, 0x4B},
	{0x58, 0xF8, 0x98},
	{0x00, 0xEB, 0xDB},
	{0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00},
	{0xFF, 0xFF, 0xFF},
	{0xAB, 0xE7, 0xFF},
	{0xC7, 0xD7, 0xFF},
	{0xD7, 0xCB, 0xFF},
	{0xFF, 0xC7, 0xFF},
	{0xFF, 0xC7, 0xDB},
	{0xFF, 0xBF, 0xB3},
	{0xFF, 0xDB, 0xAB},
	{0xFF, 0xE7, 0xA3},
	{0xE3, 0xFF, 0xA3},
	{0xAB, 0xF3, 0xBF},
	{0xB3, 0xFF, 0xCF},
	{0x9F, 0xFF, 0xF3},
	{0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00}
};

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

byte oam[0x100];
byte ppumemory[0x4000];
extern byte memory[0x10000];

void ppu_dump()
{
	printf("PPU: Dumping memory\n");
	FILE * f = fopen("oam.dump", "w");
	fwrite(oam, sizeof(byte), 0x100, f);
	fclose(f);

	f = fopen("ppu.dump", "w");
	fwrite(ppumemory, sizeof(byte), 0x10000, f);
	fclose(f);
}

struct {
	union {
		byte ctr1;
		struct {
			byte NT:2;
			byte INC:1;
			byte PATFG:1;
			byte PATBG:1;
			byte SPR:1;
			byte _unused_:1;
			byte NMI:1;
		};
	};
	union {
		byte ctr2;
		struct {
			byte BW:1;
			byte BG:1;
			byte FG:1;
			byte SBG:1;
			byte SFG:1;
			byte COL:3;
		};
	};
	union {
		byte ctr;
		struct {
			byte _unused2_:4;
			byte VRAM:1;
			byte SCAN:1;
			byte HIT:1;
			byte BLANK:1;
		};
	};
	union {
		addr scroll;
		struct {
			byte scrolly;
			byte scrollx;
		};
	};
	addr ppuaddress;
	byte oamaddress;
} state;


void ppu_dmatransfer(byte data)
{
	memcpy(oam, memory + (((addr) data) << 8), sizeof(oam));
	//printf("Doing a DMA transfer from 0x%04x to OAM\n", ((addr)data) << 8);
};

void ppu_write_oam(byte data)
{
	oam[state.oamaddress] = data;
};

void ppu_set_oam(byte data)
{
	state.oamaddress = data;
	//printf("PPU: Setting OAM address: 0x%02x\n", state.oamaddress);
};

byte firstread = 1;
byte ppumask = 8;
byte scrollmask = 8;
byte ppu_get_control()
{
	//printf("Cleaning ppu address\n");
	ppumask = 8;
	scrollmask = 8;
	state.ppuaddress = 0x0000;
	byte ans = state.ctr;
	state.BLANK = 0;
	return ans;
};

void ppu_set_control2(byte data)
{
	state.ctr2 = data;

	/*
	{
		printf("PPU: System in %s\n", (state.BW)?"Monochrome":"Color");
		printf("PPU: Background-clip: %d\n", state.BG);
		printf("PPU: Sprite-clip: %d\n", state.FG);
		printf("PPU: Show background: %d\n", state.SBG);
		printf("PPU: Show sprite: %d\n", state.SFG);
		printf("PPU: Background-color: %d\n", state.COL);
	}
	*/
};

void ppu_set_control1(byte data)
{
	state.ctr1 = data;
	
	/*
	{
		if (state.NT == 0)
			printf("PPU: Name table selected: 0x2000\n");
		if (state.NT == 1)
			printf("PPU: Name table selected: 0x2400\n");
		if (state.NT == 2)
			printf("PPU: Name table selected: 0x2800\n");
		if (state.NT == 3)
			printf("PPU: Name table selected: 0x2C00\n");
		printf("PPU: Increment data by: %d\n", (state.INC)?32:1);
		printf("PPU: Sprites pattern table: 0x%d000\n", (state.PATFG) != 0);
		printf("PPU: Background pattern table: 0x%d000\n", (state.PATBG) != 0);
		printf("PPU: Sprites size: %s\n", (state.SPR)?"8x16":"8x8");
		printf("PPU: NMI will occur with VBlank: %s\n", (state.NMI)?"yes":"no");
	}
	*/
};

addr ppudemirror(addr address)
{
	if (address >= 0x2800 && address < 0x2C00)
		address -= 0x0800;
	else if (address >= 0x2C00 && address < 0x3000)
		address -= 0x0800;

	if (address == 0x3F10)
		return 0x3F00;
	else if (address == 0x3F14)
		return 0x3F04;
	else if (address == 0x3F18)
		return 0x3F08;
	else if (address == 0x3F1c)
		return 0x3F0c;

	return address;
};


byte ppu_read_data()
{
	addr address = ppudemirror(state.ppuaddress);

	if (firstread) {
		firstread = 0;
		return 0;
	}

	if (state.INC == 1)
		state.ppuaddress += 32;
	else
		state.ppuaddress += 1;

	return ppumemory[address];
};

void ppu_write_data(byte data)
{
	addr address = ppudemirror(state.ppuaddress);

	if (state.INC == 1)
		state.ppuaddress += 32;
	else
		state.ppuaddress += 1;

	ppumemory[address] = data;
};


void ppu_set_address(byte data)
{
	firstread = 1;
	state.ppuaddress |= ((addr) data) << ppumask;
	if (ppumask == 8)
		ppumask = 0;
//	state.ppuaddress <<= 8;
//	state.ppuaddress |= (addr) data;
	//printf("Setting ppu address: %04x\n", state.ppuaddress);
};

void ppu_set_scroll(byte data)
{
	addr obj = state.scroll;
	obj &= (0x00FF) << (8 - scrollmask);
	obj |= ((addr) data) << scrollmask;

	state.scroll = obj;

	if (scrollmask == 8)
		scrollmask = 0;
//	state.scroll <<= 8;
//	state.scroll |= (addr) data;
	//printf("Setting scroll to: %02x %02x\n", state.scrollx, state.scrolly);
};

void ppu_init()
{
	flockfile(stdout);
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
		exit(1);

	screen = SDL_SetVideoMode(SCR_SCALE*SCR_WIDTH, SCR_SCALE*SCR_HEIGHT, SCR_BPP, SDL_HWPALETTE);
	if (!screen) {
		SDL_Quit();
		exit(1);
	}
	int i;
	for (i = 0; i < 64; i++) {
		sdlpalette[i].r = palette[i][0];
		sdlpalette[i].g = palette[i][1];
		sdlpalette[i].b = palette[i][2];
	}
        SDL_SetPalette(screen, SDL_LOGPAL|SDL_PHYSPAL, sdlpalette, 0, 64);
	SDL_memset(screen->pixels, 0, screen->h * screen->pitch);

	funlockfile(stdout);
};

struct st_sprite {
	byte y, index;
	union {
		byte attr;
		struct {
			byte pal:2;
			byte _nothing_:3;
			byte priority:1;
			byte xflip:1;
			byte yflip:1;
		};
	};
	byte x;
};

void paintline(byte line)
{
	addr pos, nametable, attrtable, pattable;
	byte scroll = state.scrollx;
	byte i;
	byte pixel, pal, tile, lowtile, hightile, color, x, y;
	Uint8 *pixelp;
	struct st_sprite *sprites[8];
	byte spritecount = 0;

	if (state.NT == 0) {
		nametable = 0x2000;
		attrtable = 0x23C0;
	} else {
		printf("Oh waddup! %d\n", state.NT);
		exit(1);
	}

	// look for sprites to display
	for (i = 0; i < 64; i++) {
		struct st_sprite * sprite = (struct st_sprite *) &oam[i * 4];

		if (sprite->y + 8 > line && sprite->y <= line && sprite->y != 0) {
			if (i == 0)
				state.HIT = 1;
			sprites[spritecount++] = sprite;
		}
	}


	if (state.PATBG == 1)
		pattable = 0x1000;
	else
		pattable = 0x0000;

	y = line;

	// for each pixel
	for (x = 0; x < 255; x++) {
		byte tilex, tiley, attr;

		tilex = (x + scroll) / 8;
		tiley = y / 8;

		pos = nametable + tilex + tiley * 32;
		tile = ppumemory[pos];
		pos = attrtable + (tilex/4) + (tiley/4)*8;
		attr = ppumemory[pos];

		int k = 0;
		if (tilex % 4 >= 2)
			k += 1;
		if (tiley % 4 >= 2)
			k += 2;
		color = (attr >> (k * 2)) & 0x03;

		/* get the 8 pixel slice of the tile to show */
		lowtile = ppumemory[pattable + 16*tile + (y % 8)];
		hightile = ppumemory[pattable + 16*tile + 8 + (y % 8)];

		i = 7 - ((x + scroll) % 8);
		pixel = (lowtile >> i) & 1;
		pixel += 2 * ((hightile >> i) & 1);

		if (pixel == 0)
			pal = ppumemory[0x3F00];
		else
			pal = ppumemory[0x3F00 + 4 * color + pixel];

		if (state.SBG) {
			int z1, z2;
			for (z1 = 0; z1 < SCR_SCALE; z1++)
				for (z2 = 0; z2 < SCR_SCALE; z2++) {
					pixelp = (screen->pixels + (z1 + SCR_SCALE*x) + (z2 + SCR_SCALE*y) * screen->pitch);
					*pixelp = pal;
				}
		}
	}

	if (state.PATFG == 1)
		pattable = 0x1000;
	else
		pattable = 0x0000;

	for (i = 0; i < spritecount; i++) {
		struct st_sprite * sprite = sprites[i];

		lowtile = ppumemory[pattable + 16*sprite->index + ((y - sprite->y)%8)];
		hightile = ppumemory[pattable + 16*sprite->index + 8 + ((y - sprite->y)%8)];

		for (x = 0; x < 8; x++) {
			if (sprite->xflip) {
				pixel = (lowtile >> x) & 1;
				pixel += 2 * ((hightile >> x) & 1);
			} else {
				pixel = (lowtile >> (7 - x)) & 1;
				pixel += 2 * ((hightile >> (7 - x)) & 1);
			}

			if (pixel == 0)
				continue;
			else
				pal = ppumemory[0x3F10 + 4 * sprite->pal + pixel];

			if (state.SFG) {
			int z1, z2;
			for (z1 = 0; z1 < SCR_SCALE; z1++)
			for (z2 = 0; z2 < SCR_SCALE; z2++) {
				pixelp = (screen->pixels +
						(z1 + SCR_SCALE*(sprite->x + x)) +
						(z2 + SCR_SCALE * y) * screen->pitch);

				*pixelp = pal;
			}
			}
		}
	}
}

static long timediff(struct timespec from, struct timespec to)
{
	return (to.tv_sec - from.tv_sec) * 1000000000 + to.tv_nsec - from.tv_nsec;
};

void paintframe()
{
	struct timespec start, end, remain;
	long elapsed;
	long frame = 16666666 / 24;
	byte line = 0x00;

	for (line = 0; line < 240; line += 10) {
		clock_gettime(CLOCK_REALTIME, &start);
		paintline(line);
		paintline(line + 1);
		paintline(line + 2);
		paintline(line + 3);
		paintline(line + 4);
		paintline(line + 5);
		paintline(line + 6);
		paintline(line + 7);
		paintline(line + 8);
		paintline(line + 9);
		clock_gettime(CLOCK_REALTIME, &end);

		elapsed = timediff(start, end);

		if (elapsed < frame) {
			struct timespec sleepage ={.tv_sec=0, .tv_nsec=frame - elapsed};
			nanosleep(&sleepage, &remain);
		} else {
			printf("Line overload!\n");
		}
	}

}

void ppu_run()
{
	long count = 1000;
	SDL_Event event;

	while (count) {
		count--;

		state.HIT = 0;
		state.BLANK = 0;
		paintframe();
		SDL_Flip(screen);

//		while (SDL_PollEvent(&event)) {
//			switch (event.type) {
//				case SDL_KEYDOWN:
//				case SDL_KEYUP:
//					keychange(&event.key);
//					break;
//			}
//		}

	//	elapsed = timediff(start, end);

	//	if (elapsed < frame) {
	//		struct timespec sleepage ={.tv_sec=0, .tv_nsec=frame - elapsed};
	//		nanosleep(&sleepage, &remain);
	//	} else {
	//		printf("Frame overload!\n");
	//	}

		/* Send vblank signals */
		state.BLANK = 1;
		if (state.NMI)
			cpustate.NMI = 1;
	};
	exit(0);
};

void ppu_load(byte * prg, size_t size)
{
	memcpy(ppumemory, prg, size);
};
