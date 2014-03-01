/*
 * Ricoh 2A03 based on MOS 6502
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef uint8_t byte;
typedef uint16_t addr;
typedef void (*opfunct)(void);

/*
 * I'm mapping the addresses above 0x8000 directly onto the
 * memory bank of the application
 */
byte memory[0x8000];
byte * prgmem;

struct st_cpustate {
	addr PC; /* program counter */
	byte SP; /* stack counter */
	byte A; /* accumulator register */
	byte X; /* x register */
	byte Y; /* y register */
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

static inline void cpu_memstore(addr address, byte data)
{
	if (address < 0x8000)
		*(memory + address) = data;
	else
		*(prgmem + address - 0x8000) = data;
};

static inline byte cpu_memload(addr address)
{
	if (address < 0x8000)
		return *(memory + address);
	else
		return *(prgmem + address - 0x8000);
};

static inline void cpu_setx(byte value)
{
	cpustate.X = value;
	cpustate.N = (cpustate.X & 0x80) != 0;
	cpustate.Z = (cpustate.X == 0x00);
};

static inline void cpu_sety(byte value)
{
	cpustate.Y = value;
	cpustate.N = (cpustate.Y & 0x80) != 0;
	cpustate.Z = (cpustate.Y == 0x00);
};

/*
 * Chapter 7 of MOS
 * INDEX REGISTER INSTRUCTIONS
 */
void op_ldx_inmediate(void) /* page 96 MOS */
{
	cpustate.PC += 1;
	cpustate.X = cpu_memload(cpustate.PC);
	cpustate.N = (cpustate.X & 0x80) != 0;
	cpustate.Z = (cpustate.X == 0x00);
};

void op_ldx_absolute(void) /* page 96 MOS */
{
	union {
		byte b[2];
		addr offset;
	} address;
	address.b[0] = cpu_memload(++cpustate.PC);
	address.b[1] = cpu_memload(++cpustate.PC);
	cpustate.X = cpu_memload(address.offset);
	cpustate.N = (cpustate.X & 0x80) != 0;
	cpustate.Z = (cpustate.X == 0x00);
};

void op_ldx_zero(void) /* page 96 MOS */
{
	union {
		byte b[2];
		addr offset;
	} address;
	address.b[0] = cpu_memload(++cpustate.PC);
	address.b[1] = 0x00;
	cpustate.X = cpu_memload(address.offset);
	cpustate.N = (cpustate.X & 0x80) != 0;
	cpustate.Z = (cpustate.X == 0x00);
};

void op_ldx_absolute_y(void) /* page 96 MOS */
{
	union {
		byte b[2];
		addr offset;
	} address;
	address.b[0] = cpu_memload(++cpustate.PC);
	address.b[1] = cpu_memload(++cpustate.PC);
	addr yoffset = (addr) cpustate.Y;
	cpustate.X = cpu_memload(address.offset + yoffset);
	cpustate.N = (cpustate.X & 0x80) != 0;
	cpustate.Z = (cpustate.X == 0x00);
};

void op_ldx_zero_y(void) /* page 96 MOS */
{
	union {
		byte b[2];
		addr offset;
	} address;
	address.b[0] = cpu_memload(++cpustate.PC);
	address.b[1] = 0x00;
	addr yoffset = (addr) cpustate.Y;
	cpustate.X = cpu_memload(address.offset + yoffset);
	cpustate.N = (cpustate.X & 0x80) != 0;
	cpustate.Z = (cpustate.X == 0x00);
};

void op_ldy_inmediate(void) /* page 96 MOS */
{
	cpustate.PC += 1;
	cpustate.Y = cpu_memload(cpustate.PC);
	cpustate.N = (cpustate.Y & 0x80) != 0;
	cpustate.Z = (cpustate.Y == 0x00);
};

void op_ldy_absolute(void) /* page 96 MOS */
{
	union {
		byte b[2];
		addr offset;
	} address;
	address.b[0] = cpu_memload(++cpustate.PC);
	address.b[1] = cpu_memload(++cpustate.PC);
	cpustate.Y = cpu_memload(address.offset);
	cpustate.N = (cpustate.Y & 0x80) != 0;
	cpustate.Z = (cpustate.Y == 0x00);
};

void op_ldy_zero(void) /* page 96 MOS */
{
	union {
		byte b[2];
		addr offset;
	} address;
	address.b[0] = cpu_memload(++cpustate.PC);
	address.b[1] = 0x00;
	cpustate.Y = cpu_memload(address.offset);
	cpustate.N = (cpustate.Y & 0x80) != 0;
	cpustate.Z = (cpustate.Y == 0x00);
};

void op_ldy_absolute_x(void) /* page 96 MOS */
{
	union {
		byte b[2];
		addr offset;
	} address;
	address.b[0] = cpu_memload(++cpustate.PC);
	address.b[1] = cpu_memload(++cpustate.PC);
	addr xoffset = (addr) cpustate.X;
	cpustate.Y = cpu_memload(address.offset + xoffset);
	cpustate.N = (cpustate.Y & 0x80) != 0;
	cpustate.Z = (cpustate.Y == 0x00);
};

void op_ldy_zero_x(void) /* page 96 MOS */
{
	union {
		byte b[2];
		addr offset;
	} address;
	address.b[0] = cpu_memload(++cpustate.PC);
	address.b[1] = 0x00;
	addr xoffset = (addr) cpustate.X;
	cpustate.Y = cpu_memload(address.offset + xoffset);
	cpustate.N = (cpustate.Y & 0x80) != 0;
	cpustate.Z = (cpustate.Y == 0x00);
};

void op_stx_absolute(void) /* page 97 MOS */
{
	union {
		byte b[2];
		addr offset;
	} address;
	address.b[0] = cpu_memload(++cpustate.PC);
	address.b[1] = cpu_memload(++cpustate.PC);
	cpu_memstore(address.offset, cpustate.X);
};

void op_stx_zero(void) /* page 97 MOS */
{
	union {
		byte b[2];
		addr offset;
	} address;
	address.b[0] = cpu_memload(++cpustate.PC);
	address.b[1] = 0x00;
	cpu_memstore(address.offset, cpustate.X);
};

void op_stx_zero_y(void) /* page 97 MOS */
{
	union {
		byte b[2];
		addr offset;
	} address;
	address.b[0] = cpu_memload(++cpustate.PC);
	address.b[1] = 0x00;
	addr yoffset = (addr) cpustate.Y;
	cpu_memstore(address.offset + yoffset, cpustate.X);
};

void op_sty_absolute(void) /* page 97 MOS */
{
	union {
		byte b[2];
		addr offset;
	} address;
	address.b[0] = cpu_memload(++cpustate.PC);
	address.b[1] = cpu_memload(++cpustate.PC);
	cpu_memstore(address.offset, cpustate.Y);
};

void op_sty_zero(void) /* page 97 MOS */
{
	union {
		byte b[2];
		addr offset;
	} address;
	address.b[0] = cpu_memload(++cpustate.PC);
	address.b[1] = 0x00;
	cpu_memstore(address.offset, cpustate.Y);
};

void op_sty_zero_x(void) /* page 97 MOS */
{
	union {
		byte b[2];
		addr offset;
	} address;
	address.b[0] = cpu_memload(++cpustate.PC);
	address.b[1] = 0x00;
	addr xoffset = (addr) cpustate.X;
	cpu_memstore(address.offset + xoffset, cpustate.Y);
};

void op_incx(void) /* page 97 MOS */
{
	cpustate.X += 0x01;
	cpustate.N = (cpustate.X & 0x80) != 0;
	cpustate.Z = (cpustate.X == 0x00);
};

void op_incy(void) /* page 97 MOS */
{
	cpustate.Y += 0x01;
	cpustate.N = (cpustate.Y & 0x80) != 0;
	cpustate.Z = (cpustate.Y == 0x00);
};

void op_dex(void) /* page 98 MOS */
{
	cpustate.X -= 0x01;
	cpustate.N = (cpustate.X & 0x80) != 0;
	cpustate.Z = (cpustate.X == 0x00);
};

void op_dey(void) /* page 98 MOS */
{
	cpustate.Y -= 0x01;
	cpustate.N = (cpustate.Y & 0x80) != 0;
	cpustate.Z = (cpustate.Y == 0x00);
};

void op_cpx(void) /* page 99 MOS */
{
	byte value = cpu_memload(++cpustate.PC);
	byte sub = cpustate.X - value;
	cpustate.Z = sub == 0x00;
	cpustate.N = (sub & 0x80) != 0;
	cpustate.C = cpustate.X > value;
};

void op_cpx_zero(void) /* page 99 MOS */
{
	union {
		byte b[2];
		addr offset;
	} address;
	address.b[0] = cpu_memload(++cpustate.PC);
	address.b[1] = 0x00;
	byte value = cpu_memload(address.offset);
	byte sub = cpustate.X - value;
	cpustate.Z = sub == 0x00;
	cpustate.N = (sub & 0x80) != 0;
	cpustate.C = cpustate.X > value;
};

void op_cpx_absolute(void) /* page 99 MOS */
{
	union {
		byte b[2];
		addr offset;
	} address;
	address.b[0] = cpu_memload(++cpustate.PC);
	address.b[1] = cpu_memload(++cpustate.PC);
	byte value = cpu_memload(address.offset);
	byte sub = cpustate.X - value;
	cpustate.Z = sub == 0x00;
	cpustate.N = (sub & 0x80) != 0;
	cpustate.C = cpustate.X > value;
};

void op_cpy(void) /* page 99 MOS */
{
	byte value = cpu_memload(++cpustate.PC);
	byte sub = cpustate.Y - value;
	cpustate.Z = sub == 0x00;
	cpustate.N = (sub & 0x80) != 0;
	cpustate.C = cpustate.Y > value;
};

void op_cpy_zero(void) /* page 99 MOS */
{
	union {
		byte b[2];
		addr offset;
	} address;
	address.b[0] = cpu_memload(++cpustate.PC);
	address.b[1] = 0x00;
	byte value = cpu_memload(address.offset);
	byte sub = cpustate.Y - value;
	cpustate.Z = sub == 0x00;
	cpustate.N = (sub & 0x80) != 0;
	cpustate.C = cpustate.Y > value;
};

void op_cpy_absolute(void) /* page 99 MOS */
{
	union {
		byte b[2];
		addr offset;
	} address;
	address.b[0] = cpu_memload(++cpustate.PC);
	address.b[1] = cpu_memload(++cpustate.PC);
	byte value = cpu_memload(address.offset);
	byte sub = cpustate.Y - value;
	cpustate.Z = sub == 0x00;
	cpustate.N = (sub & 0x80) != 0;
	cpustate.C = cpustate.Y > value;
};

void op_tax(void) /* page 100 MOS */
{
	cpustate.X = cpustate.A;
	cpustate.N = (cpustate.X & 0x80) != 0;
	cpustate.Z = (cpustate.X == 0x00);
};

void op_tay(void) /* page 101 MOS */
{
	cpustate.Y = cpustate.A;
	cpustate.N = (cpustate.Y & 0x80) != 0;
	cpustate.Z = (cpustate.Y == 0x00);
};

void op_txa(void) /* page 100 MOS */
{
	cpustate.A = cpustate.X;
	cpustate.N = (cpustate.X & 0x80) != 0;
	cpustate.Z = (cpustate.X == 0x00);
};

void op_tya(void) /* page 101 MOS */
{
	cpustate.A = cpustate.Y;
	cpustate.N = (cpustate.Y & 0x80) != 0;
	cpustate.Z = (cpustate.Y == 0x00);
};

void op_brk(void) /* page 144 MOS */
{
	/* interrupt to vector IRQ address */
};

opfunct opcode_map[] = {
	/* 00 */ &op_brk,
	/* 01 */ NULL,
	/* 02 */ NULL,
	/* 03 */ NULL,
	/* 04 */ NULL,
	/* 05 */ NULL,
	/* 06 */ NULL,
	/* 07 */ NULL,
	/* 08 */ NULL,
	/* 09 */ NULL,
	/* 0a */ NULL,
	/* 0b */ NULL,
	/* 0c */ NULL,
	/* 0d */ NULL,
	/* 0e */ NULL,
	/* 0f */ NULL,
	/* 10 */ NULL,
	/* 11 */ NULL,
	/* 12 */ NULL,
	/* 13 */ NULL,
	/* 14 */ NULL,
	/* 15 */ NULL,
	/* 16 */ NULL,
	/* 17 */ NULL,
	/* 18 */ NULL,
	/* 19 */ NULL,
	/* 1a */ NULL,
	/* 1b */ NULL,
	/* 1c */ NULL,
	/* 1d */ NULL,
	/* 1e */ NULL,
	/* 1f */ NULL,
	/* 20 */ NULL,
	/* 21 */ NULL,
	/* 22 */ NULL,
	/* 23 */ NULL,
	/* 24 */ NULL,
	/* 25 */ NULL,
	/* 26 */ NULL,
	/* 27 */ NULL,
	/* 28 */ NULL,
	/* 29 */ NULL,
	/* 2a */ NULL,
	/* 2b */ NULL,
	/* 2c */ NULL,
	/* 2d */ NULL,
	/* 2e */ NULL,
	/* 2f */ NULL,
	/* 30 */ NULL,
	/* 31 */ NULL,
	/* 32 */ NULL,
	/* 33 */ NULL,
	/* 34 */ NULL,
	/* 35 */ NULL,
	/* 36 */ NULL,
	/* 37 */ NULL,
	/* 38 */ NULL,
	/* 39 */ NULL,
	/* 3a */ NULL,
	/* 3b */ NULL,
	/* 3c */ NULL,
	/* 3d */ NULL,
	/* 3e */ NULL,
	/* 3f */ NULL,
	/* 40 */ NULL,
	/* 41 */ NULL,
	/* 42 */ NULL,
	/* 43 */ NULL,
	/* 44 */ NULL,
	/* 45 */ NULL,
	/* 46 */ NULL,
	/* 47 */ NULL,
	/* 48 */ NULL,
	/* 49 */ NULL,
	/* 4a */ NULL,
	/* 4b */ NULL,
	/* 4c */ NULL,
	/* 4d */ NULL,
	/* 4e */ NULL,
	/* 4f */ NULL,
	/* 50 */ NULL,
	/* 51 */ NULL,
	/* 52 */ NULL,
	/* 53 */ NULL,
	/* 54 */ NULL,
	/* 55 */ NULL,
	/* 56 */ NULL,
	/* 57 */ NULL,
	/* 58 */ NULL,
	/* 59 */ NULL,
	/* 5a */ NULL,
	/* 5b */ NULL,
	/* 5c */ NULL,
	/* 5d */ NULL,
	/* 5e */ NULL,
	/* 5f */ NULL,
	/* 60 */ NULL,
	/* 61 */ NULL,
	/* 62 */ NULL,
	/* 63 */ NULL,
	/* 64 */ NULL,
	/* 65 */ NULL,
	/* 66 */ NULL,
	/* 67 */ NULL,
	/* 68 */ NULL,
	/* 69 */ NULL,
	/* 6a */ NULL,
	/* 6b */ NULL,
	/* 6c */ NULL,
	/* 6d */ NULL,
	/* 6e */ NULL,
	/* 6f */ NULL,
	/* 70 */ NULL,
	/* 71 */ NULL,
	/* 72 */ NULL,
	/* 73 */ NULL,
	/* 74 */ NULL,
	/* 75 */ NULL,
	/* 76 */ NULL,
	/* 77 */ NULL,
	/* 78 */ NULL,
	/* 79 */ NULL,
	/* 7a */ NULL,
	/* 7b */ NULL,
	/* 7c */ NULL,
	/* 7d */ NULL,
	/* 7e */ NULL,
	/* 7f */ NULL,
	/* 80 */ NULL,
	/* 81 */ NULL,
	/* 82 */ NULL,
	/* 83 */ NULL,
	/* 84 */ &op_sty_zero,
	/* 85 */ NULL,
	/* 86 */ &op_stx_zero,
	/* 87 */ NULL,
	/* 88 */ &op_dey,
	/* 89 */ NULL,
	/* 8a */ NULL,
	/* 8b */ NULL,
	/* 8c */ &op_sty_absolute,
	/* 8d */ NULL,
	/* 8e */ &op_stx_absolute,
	/* 8f */ NULL,
	/* 90 */ NULL,
	/* 91 */ NULL,
	/* 92 */ NULL,
	/* 93 */ NULL,
	/* 94 */ &op_sty_zero_x,
	/* 95 */ NULL,
	/* 96 */ &op_stx_zero_y,
	/* 97 */ NULL,
	/* 98 */ NULL,
	/* 99 */ NULL,
	/* 9a */ NULL,
	/* 9b */ NULL,
	/* 9c */ NULL,
	/* 9d */ NULL,
	/* 9e */ NULL,
	/* 9f */ NULL,
	/* a0 */ &op_ldy_inmediate,
	/* a1 */ NULL,
	/* a2 */ &op_ldx_inmediate,
	/* a3 */ NULL,
	/* a4 */ &op_ldy_zero,
	/* a5 */ NULL,
	/* a6 */ &op_ldx_zero,
	/* a7 */ NULL,
	/* a8 */ NULL,
	/* a9 */ NULL,
	/* aa */ NULL,
	/* ab */ NULL,
	/* ac */ &op_ldy_absolute,
	/* ad */ NULL,
	/* ae */ &op_ldx_absolute,
	/* af */ NULL,
	/* b0 */ NULL,
	/* b1 */ NULL,
	/* b2 */ NULL,
	/* b3 */ NULL,
	/* b4 */ &op_ldy_zero_x,
	/* b5 */ NULL,
	/* b6 */ &op_ldx_zero_y,
	/* b7 */ NULL,
	/* b8 */ NULL,
	/* b9 */ NULL,
	/* ba */ NULL,
	/* bb */ NULL,
	/* bc */ &op_ldy_absolute_x,
	/* bd */ NULL,
	/* be */ &op_ldx_absolute_y,
	/* bf */ NULL,
	/* c0 */ &op_cpy,
	/* c1 */ NULL,
	/* c2 */ NULL,
	/* c3 */ NULL,
	/* c4 */ &op_cpy_zero,
	/* c5 */ NULL,
	/* c6 */ NULL,
	/* c7 */ NULL,
	/* c8 */ &op_incy,
	/* c9 */ NULL,
	/* ca */ &op_dex,
	/* cb */ NULL,
	/* cc */ &op_cpy_absolute,
	/* cd */ NULL,
	/* ce */ NULL,
	/* cf */ NULL,
	/* d0 */ NULL,
	/* d1 */ NULL,
	/* d2 */ NULL,
	/* d3 */ NULL,
	/* d4 */ NULL,
	/* d5 */ NULL,
	/* d6 */ NULL,
	/* d7 */ NULL,
	/* d8 */ NULL,
	/* d9 */ NULL,
	/* da */ NULL,
	/* db */ NULL,
	/* dc */ NULL,
	/* dd */ NULL,
	/* de */ NULL,
	/* df */ NULL,
	/* e0 */ &op_cpx,
	/* e1 */ NULL,
	/* e2 */ NULL,
	/* e3 */ NULL,
	/* e4 */ &op_cpx_zero,
	/* e5 */ NULL,
	/* e6 */ NULL,
	/* e7 */ NULL,
	/* e8 */ &op_incx,
	/* e9 */ NULL,
	/* ea */ NULL,
	/* eb */ NULL,
	/* ec */ &op_cpx_absolute,
	/* ed */ NULL,
	/* ee */ NULL,
	/* ef */ NULL,
	/* f0 */ NULL,
	/* f1 */ NULL,
	/* f2 */ NULL,
	/* f3 */ NULL,
	/* f4 */ NULL,
	/* f5 */ NULL,
	/* f6 */ NULL,
	/* f7 */ NULL,
	/* f8 */ NULL,
	/* f9 */ NULL,
	/* fa */ NULL,
	/* fb */ NULL,
	/* fc */ NULL,
	/* fd */ NULL,
	/* fe */ NULL,
	/* ff */ NULL,
};

void print_cpustate()
{
	byte off;

	printf("PC: 0x%02x SP: 0x%02x A: 0x%02x X: 0x%02x Y: 0x%02x ",
			cpustate.PC, cpustate.SP, cpustate.A,
			cpustate.X, cpustate.Y);
	printf("CZIDBVN: %d%d%d%d%d%d%db\n",
			cpustate.C, cpustate.Z, cpustate.I,
			cpustate.D, cpustate.B, cpustate.V,
			cpustate.N);

	if (cpustate.SP != 0xff) {
		printf("Stack: ");
		for (off = 0xff; off > cpustate.SP; off--) {
			printf("%02x ", cpu_memload(0x0100 + off));
		}
	}
};

static void cpu_boot()
{
	printf("Booting CPU...\n");
	printf("Starting memory...\n");
	memset(memory, 0, sizeof(memory));
	prgmem = memory;

	printf("Starting CPU state...\n");
	memset(&cpustate, 0, sizeof(cpustate));

	printf("Starting stack...\n");
	cpustate.SP = 0xff;
}

void cpu_init()
{
	cpu_boot();
};

void cpu_load(byte *prg)
{
	prgmem = prg;
	cpustate.PC = 0x8000;
};

void cpu_run()
{
	byte op;
	opfunct func;

	printf("Running program at: 0x%04x\n", cpustate.PC);

	do {
		/* fetch */
		op = cpu_memload(cpustate.PC);

		/* decode */
		func = opcode_map[op];
		if (func == NULL) {
			fprintf(stderr, "Unrecognized instruction: %02x\n", op);
			fprintf(stderr, "  At position: %04x\n", cpustate.PC);
			exit(1);
		}

		/* execute */
		func();

		/* advance */
		cpustate.PC ++;

	} while (op != 0x00);
	
	printf("Program stopped at: 0x%04x\n", cpustate.PC-1);
};

void cpu_dump()
{
	FILE * f;
	size_t size;

	printf("Dumping memory\n");
	printf("CPU State:\n");
	print_cpustate();

	f = fopen("core.dump", "w");

	if (f == NULL) {
		fprintf(stderr, "Memory dump impossible: file opening error\n");
		return;
	};

	size = fwrite(memory, sizeof(byte), sizeof(memory) / sizeof(byte), f);

	if (size != sizeof(memory) / sizeof(byte)) {
		fprintf(stderr, "Memory dump not complete!\n");
	}

	fclose(f);
};
