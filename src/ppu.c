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
#define SCR_BPP 32
#define SCR_SCALE 3

typedef uint8_t byte;
typedef uint16_t addr;

SDL_Surface * screen = NULL;

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
byte ppumemory[0x10000];
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

	screen = SDL_SetVideoMode(SCR_SCALE*SCR_WIDTH, SCR_SCALE*SCR_HEIGHT, SCR_BPP, SDL_HWSURFACE);
	if (!screen) {
		SDL_Quit();
		exit(1);
	}
	SDL_memset(screen->pixels, 0, screen->h * screen->pitch);

	funlockfile(stdout);
};

void show_sprite(byte number, byte mirrorx, byte mirrory, byte color, byte x, byte y)
{
	int i, j, row, column;
	int z1, z2;
	int pal;
	Uint32 *pixmem32;
	Uint32 colour;
	addr offset;

	if (state.PATFG == 1)
		offset = 0x1000;
	else
		offset = 0x0000;

	byte * tile;
	tile = &ppumemory[offset + number*16];
	int pixel;

	for (i=0; i<8; i++) {
		for (j=0; j<8; j++) {
			pixel = (tile[j] >> (7-i) ) & 1;
			pixel += 2 * ((tile[j + 8] >> (7-i)) & 1);
			if (pixel == 0)
				continue;
			else
				pal = ppumemory[0x3F10 + 4 * color + pixel];

			colour = SDL_MapRGB(screen->format,
				palette[pal][0],
				palette[pal][1],
				palette[pal][2]
			);

			if (mirrorx)
				column = SCR_SCALE * (x + 7 - i);
			else
				column = SCR_SCALE * (x + i);
			if (mirrory)
				row = SCR_SCALE * (y + 7 - j);
			else
				row = SCR_SCALE * (y + j);

			for (z1 = 0; z1 < SCR_SCALE; z1++) {
				for (z2 = 0; z2 < SCR_SCALE; z2++) {
					pixmem32 = (Uint32*)(screen->pixels + (column + z1)*4 + (row + z2) * screen->pitch);
					*pixmem32 = colour;
				}
			}
		}
	};
};


void show_background(byte number, byte color, int ioff, int joff, byte scroll)
{
	int i, j, row, column;
	int z1, z2;
	int pal;
	Uint32 *pixmem32;
	Uint32 colour;
	addr offset;

	if (state.PATBG == 1)
		offset = 0x1000;
	else
		offset = 0x0000;

	byte * tile;
	tile = &ppumemory[offset + number*16];
	int pixel;

	for (i=0; i<8; i++) {
		for (j=0; j<8; j++) {
			pixel = (tile[j] >> (7 - i)) & 1;
			pixel += 2 * ((tile[j + 8] >> (7 - i)) & 1);
			if (pixel == 0)
				pal = ppumemory[0x3F00];
			else
				pal = ppumemory[0x3F00 + 4 * color + pixel];

			colour = SDL_MapRGB(screen->format,
				palette[pal][0],
				palette[pal][1],
				palette[pal][2]
			);

			row = SCR_SCALE * (joff * 8 + j);
			column = SCR_SCALE * (ioff * 8 + i);

			if (row < SCR_SCALE * 8)
				continue;
			
//			if (column < scroll * SCR_SCALE)
//				continue;
			if (column - scroll * SCR_SCALE >= screen->w)
				continue;

			for (z1 = 0; z1 < SCR_SCALE; z1++) {
				for (z2 = 0; z2 < SCR_SCALE; z2++) {
	pixmem32 = (Uint32*)(screen->pixels + (column+z1 - scroll * SCR_SCALE)*4 + (row+z2) * screen->pitch);
	*pixmem32 = colour;
				}
			}
		}
	};

};

void clearbackground()
{
	Uint32 color;
//	byte backdrop;
//
//	backdrop = ppumemory[0x3F00];
	color = SDL_MapRGB(screen->format,
			0,
			0,
			0);

	memset(screen->pixels, color, sizeof(Uint32) * screen->h * screen->w);

};

void paintsprites()
{
	int i;

	if (state.SFG == 0) {
		return;
	}

	for (i = 0; i < 64; i++) {
		struct {
			byte y, index, attribute, x;
		} * sprite = (void*) &oam[i * 4];

		show_sprite(sprite->index,
				sprite->attribute & (1<<6),
				sprite->attribute & (1<<7),
				sprite->attribute & 3,
				sprite->x,
				sprite->y);
	}
};

void paintbackground()
{
	addr base, basemax;

	if (state.SBG == 0) {
		return;
		clearbackground();
	}

	if (state.NT == 0) {
		base = 0x2000;
		basemax = 0x23C0;
	} else {
		printf("Oh waddup! %d\n", state.NT);
		exit(1);
	}

	byte scroll = state.scrollx;
	byte i, j, I, J, imin, xpos, ypos;
	byte tile, attr, color;
	addr pos;

	imin = scroll / 8;
	i = imin;
	j = 0;

	for (xpos = 0; xpos <= 32; xpos++) {

		for (j = 0; j < 30; j++) {

			ypos = j;

			J = ypos / 4;

			if (i < 32) {
				I = i / 4;
				pos = 0x2000 + i + j * 32;
				attr = ppumemory[0x23C0 + I + J * 8];
			} else {
				I = (i - 32) / 4;
				pos = 0x2400 + (i - 32) + j * 32;
				attr = ppumemory[0x27C0 + I + J * 8];
			}

			tile = ppumemory[pos];

			int k = 0;
			if (i % 4 >= 2)
				k += 1;
			if (j % 4 >= 2)
				k += 2;
			color = (attr >> (k * 2)) & 0x03;

			show_background(tile, color, xpos, ypos, scroll % 8);
		}

		i = (i + 1) % 64;
	}

//	for (i = 0; base < basemax; i++, base++) {
//		byte tile, attribute, color;
//		tile = ppumemory[base];
//
//		int I, J;
//		int j;
//
//		j = i / 32;
//
//		I = (i % 32) / 4;
//		J = j / 4;
//
//		attribute = ppumemory[basemax + I + J * 8];
//		
//
//		show_background(tile, color, i % 32, i / 32, scroll);
//	}
//
//	base = 0x2400;
//	basemax = 0x27C0;
//	for (i = 0; base < basemax; i++, base++) {
//		byte tile, attribute, color;
//		tile = ppumemory[base];
//
//		int I, J;
//		int j;
//
//		j = i / 32;
//
//		I = (i % 32) / 4;
//		if (i%32 > extratiles) continue;
//		byte xpos, ypos;
//
//		xpos = 32 - extratiles + (i%32);
//		ypos = i / 32;
//
//		if (xpos == 32 && scroll % 8 == 0) continue;
//
//		J = j / 4;
//
//		attribute = ppumemory[basemax + I + J * 8];
//		
//		int k = 0;
//		if (i % 4 >= 2)
//			k += 1;
//		if (j % 4 >= 2)
//			k += 2;
//
//		color = (attribute >> (k * 2)) & 0x03;
//
//		show_background(tile, color, xpos, ypos, scroll % 8);
//	}


};

static long timediff(struct timespec from, struct timespec to)
{
	return (to.tv_sec - from.tv_sec) * 1000000000 + to.tv_nsec - from.tv_nsec;
};

void ppu_run()
{
	struct timespec start, end, remain;
	long elapsed;
	long frame = 16000000 / 2;

	while (1) {

		clock_gettime(CLOCK_REALTIME, &start);

		/* Print the screen, by scanlines */
		state.HIT = 0;

		paintbackground();
		state.HIT = 1;
		paintsprites();

		SDL_Flip(screen);

		clock_gettime(CLOCK_REALTIME, &end);

		elapsed = timediff(start, end);

		if (elapsed < frame) {
			struct timespec sleepage ={.tv_sec=0, .tv_nsec=frame - elapsed};
			nanosleep(&sleepage, &remain);
		}

		/* Send vblank signals */
		state.BLANK = 1;
		if (state.NMI)
			cpustate.NMI = 1;
	};
};

void ppu_load(byte * prg, size_t size)
{
	memcpy(ppumemory, prg, size);
};
