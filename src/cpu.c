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
 * Chapter 4 of MOS
 * TEST BRANCH AND JUMP INSTRUCTIONS
 */
static void op_jmp(void) /* page 36 MOS */
{
	union {
		byte b[2];
		addr offset;
	} address;
	address.b[0] = cpu_memload(cpustate.PC++);
	address.b[1] = cpu_memload(cpustate.PC++);
	cpustate.PC = address.offset;
};

static void op_jmp_indirect(void) /* page 36 MOS */
{
	union {
		byte b[2];
		addr offset;
	} address;
	address.b[0] = cpu_memload(cpustate.PC++);
	address.b[1] = cpu_memload(cpustate.PC++);
	union {
		byte b[2];
		addr offset;
	} newpc;
	newpc.b[0] = cpu_memload(address.offset);
	newpc.b[1] = cpu_memload(address.offset + 1);
	cpustate.PC = newpc.offset;
};

static void op_bmi(void) /* page 40 MOS */
{
	byte diff = cpu_memload(cpustate.PC++);
	if (cpustate.N) {
		if (diff & 0x80)
			cpustate.PC += (diff ^ 0xff) + 0x01;
		else
			cpustate.PC += diff;
	}
};

static void op_bpl(void) /* page 40 MOS */
{
	byte diff = cpu_memload(cpustate.PC++);
	if (cpustate.N == 0) {
		if (diff & 0x80)
			cpustate.PC += (diff ^ 0xff) + 0x01;
		else
			cpustate.PC += diff;
	}
};

static void op_bcc(void) /* page 40 MOS */
{
	byte diff = cpu_memload(cpustate.PC++);
	if (cpustate.C == 0) {
		if (diff & 0x80)
			cpustate.PC += (diff ^ 0xff) + 0x01;
		else
			cpustate.PC += diff;
	}
};

static void op_bcs(void) /* page 40 MOS */
{
	byte diff = cpu_memload(cpustate.PC++);
	if (cpustate.C) {
		if (diff & 0x80)
			cpustate.PC += (diff ^ 0xff) + 0x01;
		else
			cpustate.PC += diff;
	}
};

static void op_beq(void) /* page 41 MOS */
{
	byte diff = cpu_memload(cpustate.PC++);
	if (cpustate.Z) {
		if (diff & 0x80)
			cpustate.PC += (diff ^ 0xff) + 0x01;
		else
			cpustate.PC += diff;
	}
};

static void op_bne(void) /* page 41 MOS */
{
	byte diff = cpu_memload(cpustate.PC++);
	if (cpustate.Z == 0) {
		if (diff & 0x80)
			cpustate.PC -= (diff ^ 0xff) + 0x01;
		else
			cpustate.PC += diff;
	}
};

static void op_bvs(void) /* page 41 MOS */
{
	byte diff = cpu_memload(cpustate.PC++);
	if (cpustate.V) {
		if (diff & 0x80)
			cpustate.PC += (diff ^ 0xff) + 0x01;
		else
			cpustate.PC += diff;
	}
};

static void op_bvc(void) /* page 41 MOS */
{
	byte diff = cpu_memload(cpustate.PC++);
	if (cpustate.V == 0) {
		if (diff & 0x80)
			cpustate.PC += (diff ^ 0xff) + 0x01;
		else
			cpustate.PC += diff;
	}
};

static void op_cmp(void) /* page 45 MOS */
{
	byte mem = cpu_memload(cpustate.PC++);
	byte diff = cpustate.A - mem;
	cpustate.C = cpustate.A >= mem;
	cpustate.N = (diff & 0x80) != 0x00;
	cpustate.Z = diff == 0x00;
};

static void op_cmp_zero(void) /* page 45 MOS */
{
	union {
		byte b[2];
		addr offset;
	} address;
	address.b[0] = cpu_memload(cpustate.PC++);
	address.b[1] = 0x00;
	byte mem = cpu_memload(address.offset);
	byte diff = cpustate.A - mem;
	cpustate.C = cpustate.A >= mem;
	cpustate.N = (diff & 0x80) != 0x00;
	cpustate.Z = diff == 0x00;
};

static void op_cmp_zero_x(void) /* page 45 MOS */
{
	union {
		byte b[2];
		addr offset;
	} address;
	address.b[0] = cpu_memload(cpustate.PC++);
	address.b[1] = 0x00;
	byte mem = cpu_memload(address.offset + (addr) cpustate.X);
	byte diff = cpustate.A - mem;
	cpustate.C = cpustate.A >= mem;
	cpustate.N = (diff & 0x80) != 0x00;
	cpustate.Z = diff == 0x00;
};

static void op_cmp_absolute(void) /* page 45 MOS */
{
	union {
		byte b[2];
		addr offset;
	} address;
	address.b[0] = cpu_memload(cpustate.PC++);
	address.b[1] = cpu_memload(cpustate.PC++);
	byte mem = cpu_memload(address.offset);
	byte diff = cpustate.A - mem;
	cpustate.C = cpustate.A >= mem;
	cpustate.N = (diff & 0x80) != 0x00;
	cpustate.Z = diff == 0x00;
};

static void op_cmp_absolute_x(void) /* page 45 MOS */
{
	union {
		byte b[2];
		addr offset;
	} address;
	address.b[0] = cpu_memload(cpustate.PC++);
	address.b[1] = cpu_memload(cpustate.PC++);
	byte mem = cpu_memload(address.offset + (addr) cpustate.X);
	byte diff = cpustate.A - mem;
	cpustate.C = cpustate.A >= mem;
	cpustate.N = (diff & 0x80) != 0x00;
	cpustate.Z = diff == 0x00;
};

static void op_cmp_absolute_y(void) /* page 45 MOS */
{
	union {
		byte b[2];
		addr offset;
	} address;
	address.b[0] = cpu_memload(cpustate.PC++);
	address.b[1] = cpu_memload(cpustate.PC++);
	byte mem = cpu_memload(address.offset + (addr) cpustate.Y);
	byte diff = cpustate.A - mem;
	cpustate.C = cpustate.A >= mem;
	cpustate.N = (diff & 0x80) != 0x00;
	cpustate.Z = diff == 0x00;
};

static void op_cmp_indirect_x(void) /* page 45 MOS */
{
	union {
		byte b[2];
		addr offset;
	} address;
	byte baseaddress = cpu_memload(cpustate.PC++) + cpustate.X;
	address.b[0] = cpu_memload((addr)baseaddress);
	address.b[1] = 0x00;
	byte mem = cpu_memload(address.offset);
	byte diff = cpustate.A - mem;
	cpustate.C = cpustate.A >= mem;
	cpustate.N = (diff & 0x80) != 0x00;
	cpustate.Z = diff == 0x00;
};

static void op_cmp_indirect_y(void) /* page 45 MOS */
{
	union {
		byte b[2];
		addr offset;
	} address;
	byte baseaddress = cpu_memload(cpustate.PC++);
	address.b[0] = cpu_memload((addr)baseaddress);
	address.b[1] = cpu_memload(((addr)baseaddress) + 1);;
	byte mem = cpu_memload(address.offset + (addr) cpustate.Y);
	byte diff = cpustate.A - mem;
	cpustate.C = cpustate.A >= mem;
	cpustate.N = (diff & 0x80) != 0x00;
	cpustate.Z = diff == 0x00;
};

static void op_bit_zero(void) /* page 47 MOS */
{
	byte dir = cpu_memload(cpustate.PC++);
	byte value = cpu_memload((addr)dir);
	byte and = cpustate.A & value;
	cpustate.Z = and == 0x00;
	cpustate.N = (and & 0x80) != 0x00;
	cpustate.V = (and & 0x60) != 0x00;
};

static void op_bit_absolute(void) /* page 47 MOS */
{
	union {
		byte b[2];
		addr offset;
	} address;
	address.b[0] = cpu_memload(cpustate.PC++);
	address.b[1] = cpu_memload(cpustate.PC++);
	byte value = cpu_memload(address.offset);
	byte and = cpustate.A & value;
	cpustate.Z = and == 0x00;
	cpustate.N = (and & 0x80) != 0x00;
	cpustate.V = (and & 0x60) != 0x00;
};

/*
 * Chapter 7 of MOS
 * INDEX REGISTER INSTRUCTIONS
 */
static void op_ldx(void) /* page 96 MOS */
{
	cpustate.X = cpu_memload(cpustate.PC++);
	cpustate.N = (cpustate.X & 0x80) != 0;
	cpustate.Z = (cpustate.X == 0x00);
};

static void op_ldx_absolute(void) /* page 96 MOS */
{
	union {
		byte b[2];
		addr offset;
	} address;
	address.b[0] = cpu_memload(cpustate.PC++);
	address.b[1] = cpu_memload(cpustate.PC++);
	cpustate.X = cpu_memload(address.offset);
	cpustate.N = (cpustate.X & 0x80) != 0;
	cpustate.Z = (cpustate.X == 0x00);
};

static void op_ldx_zero(void) /* page 96 MOS */
{
	union {
		byte b[2];
		addr offset;
	} address;
	address.b[0] = cpu_memload(cpustate.PC++);
	address.b[1] = 0x00;
	cpustate.X = cpu_memload(address.offset);
	cpustate.N = (cpustate.X & 0x80) != 0;
	cpustate.Z = (cpustate.X == 0x00);
};

static void op_ldx_absolute_y(void) /* page 96 MOS */
{
	union {
		byte b[2];
		addr offset;
	} address;
	address.b[0] = cpu_memload(cpustate.PC++);
	address.b[1] = cpu_memload(cpustate.PC++);
	addr yoffset = (addr) cpustate.Y;
	cpustate.X = cpu_memload(address.offset + yoffset);
	cpustate.N = (cpustate.X & 0x80) != 0;
	cpustate.Z = (cpustate.X == 0x00);
};

static void op_ldx_zero_y(void) /* page 96 MOS */
{
	union {
		byte b[2];
		addr offset;
	} address;
	address.b[0] = cpu_memload(cpustate.PC++);
	address.b[1] = 0x00;
	addr yoffset = (addr) cpustate.Y;
	cpustate.X = cpu_memload(address.offset + yoffset);
	cpustate.N = (cpustate.X & 0x80) != 0;
	cpustate.Z = (cpustate.X == 0x00);
};

static void op_ldy(void) /* page 96 MOS */
{
	cpustate.Y = cpu_memload(cpustate.PC++);
	cpustate.N = (cpustate.Y & 0x80) != 0;
	cpustate.Z = (cpustate.Y == 0x00);
};

static void op_ldy_absolute(void) /* page 96 MOS */
{
	union {
		byte b[2];
		addr offset;
	} address;
	address.b[0] = cpu_memload(cpustate.PC++);
	address.b[1] = cpu_memload(cpustate.PC++);
	cpustate.Y = cpu_memload(address.offset);
	cpustate.N = (cpustate.Y & 0x80) != 0;
	cpustate.Z = (cpustate.Y == 0x00);
};

static void op_ldy_zero(void) /* page 96 MOS */
{
	union {
		byte b[2];
		addr offset;
	} address;
	address.b[0] = cpu_memload(cpustate.PC++);
	address.b[1] = 0x00;
	cpustate.Y = cpu_memload(address.offset);
	cpustate.N = (cpustate.Y & 0x80) != 0;
	cpustate.Z = (cpustate.Y == 0x00);
};

static void op_ldy_absolute_x(void) /* page 96 MOS */
{
	union {
		byte b[2];
		addr offset;
	} address;
	address.b[0] = cpu_memload(cpustate.PC++);
	address.b[1] = cpu_memload(cpustate.PC++);
	addr xoffset = (addr) cpustate.X;
	cpustate.Y = cpu_memload(address.offset + xoffset);
	cpustate.N = (cpustate.Y & 0x80) != 0;
	cpustate.Z = (cpustate.Y == 0x00);
};

static void op_ldy_zero_x(void) /* page 96 MOS */
{
	union {
		byte b[2];
		addr offset;
	} address;
	address.b[0] = cpu_memload(cpustate.PC++);
	address.b[1] = 0x00;
	addr xoffset = (addr) cpustate.X;
	cpustate.Y = cpu_memload(address.offset + xoffset);
	cpustate.N = (cpustate.Y & 0x80) != 0;
	cpustate.Z = (cpustate.Y == 0x00);
};

static void op_stx_absolute(void) /* page 97 MOS */
{
	union {
		byte b[2];
		addr offset;
	} address;
	address.b[0] = cpu_memload(cpustate.PC++);
	address.b[1] = cpu_memload(cpustate.PC++);
	cpu_memstore(address.offset, cpustate.X);
};

static void op_stx_zero(void) /* page 97 MOS */
{
	union {
		byte b[2];
		addr offset;
	} address;
	address.b[0] = cpu_memload(cpustate.PC++);
	address.b[1] = 0x00;
	cpu_memstore(address.offset, cpustate.X);
};

static void op_stx_zero_y(void) /* page 97 MOS */
{
	union {
		byte b[2];
		addr offset;
	} address;
	address.b[0] = cpu_memload(cpustate.PC++);
	address.b[1] = 0x00;
	addr yoffset = (addr) cpustate.Y;
	cpu_memstore(address.offset + yoffset, cpustate.X);
};

static void op_sty_absolute(void) /* page 97 MOS */
{
	union {
		byte b[2];
		addr offset;
	} address;
	address.b[0] = cpu_memload(cpustate.PC++);
	address.b[1] = cpu_memload(cpustate.PC++);
	cpu_memstore(address.offset, cpustate.Y);
};

static void op_sty_zero(void) /* page 97 MOS */
{
	union {
		byte b[2];
		addr offset;
	} address;
	address.b[0] = cpu_memload(cpustate.PC++);
	address.b[1] = 0x00;
	cpu_memstore(address.offset, cpustate.Y);
};

static void op_sty_zero_x(void) /* page 97 MOS */
{
	union {
		byte b[2];
		addr offset;
	} address;
	address.b[0] = cpu_memload(cpustate.PC++);
	address.b[1] = 0x00;
	addr xoffset = (addr) cpustate.X;
	cpu_memstore(address.offset + xoffset, cpustate.Y);
};

static void op_incx(void) /* page 97 MOS */
{
	cpustate.X += 0x01;
	cpustate.N = (cpustate.X & 0x80) != 0;
	cpustate.Z = (cpustate.X == 0x00);
};

static void op_incy(void) /* page 97 MOS */
{
	cpustate.Y += 0x01;
	cpustate.N = (cpustate.Y & 0x80) != 0;
	cpustate.Z = (cpustate.Y == 0x00);
};

static void op_dex(void) /* page 98 MOS */
{
	cpustate.X -= 0x01;
	cpustate.N = (cpustate.X & 0x80) != 0;
	cpustate.Z = (cpustate.X == 0x00);
};

static void op_dey(void) /* page 98 MOS */
{
	cpustate.Y -= 0x01;
	cpustate.N = (cpustate.Y & 0x80) != 0;
	cpustate.Z = (cpustate.Y == 0x00);
};

static void op_cpx(void) /* page 99 MOS */
{
	byte value = cpu_memload(cpustate.PC++);
	byte sub = cpustate.X - value;
	cpustate.Z = sub == 0x00;
	cpustate.N = (sub & 0x80) != 0;
	cpustate.C = cpustate.X > value;
};

static void op_cpx_zero(void) /* page 99 MOS */
{
	union {
		byte b[2];
		addr offset;
	} address;
	address.b[0] = cpu_memload(cpustate.PC++);
	address.b[1] = 0x00;
	byte value = cpu_memload(address.offset);
	byte sub = cpustate.X - value;
	cpustate.Z = sub == 0x00;
	cpustate.N = (sub & 0x80) != 0;
	cpustate.C = cpustate.X > value;
};

static void op_cpx_absolute(void) /* page 99 MOS */
{
	union {
		byte b[2];
		addr offset;
	} address;
	address.b[0] = cpu_memload(cpustate.PC++);
	address.b[1] = cpu_memload(cpustate.PC++);
	byte value = cpu_memload(address.offset);
	byte sub = cpustate.X - value;
	cpustate.Z = sub == 0x00;
	cpustate.N = (sub & 0x80) != 0;
	cpustate.C = cpustate.X > value;
};

static void op_cpy(void) /* page 99 MOS */
{
	byte value = cpu_memload(cpustate.PC++);
	byte sub = cpustate.Y - value;
	cpustate.Z = sub == 0x00;
	cpustate.N = (sub & 0x80) != 0;
	cpustate.C = cpustate.Y > value;
};

static void op_cpy_zero(void) /* page 99 MOS */
{
	union {
		byte b[2];
		addr offset;
	} address;
	address.b[0] = cpu_memload(cpustate.PC++);
	address.b[1] = 0x00;
	byte value = cpu_memload(address.offset);
	byte sub = cpustate.Y - value;
	cpustate.Z = sub == 0x00;
	cpustate.N = (sub & 0x80) != 0;
	cpustate.C = cpustate.Y > value;
};

static void op_cpy_absolute(void) /* page 99 MOS */
{
	union {
		byte b[2];
		addr offset;
	} address;
	address.b[0] = cpu_memload(cpustate.PC++);
	address.b[1] = cpu_memload(cpustate.PC++);
	byte value = cpu_memload(address.offset);
	byte sub = cpustate.Y - value;
	cpustate.Z = sub == 0x00;
	cpustate.N = (sub & 0x80) != 0;
	cpustate.C = cpustate.Y > value;
};

static void op_tax(void) /* page 100 MOS */
{
	cpustate.X = cpustate.A;
	cpustate.N = (cpustate.X & 0x80) != 0;
	cpustate.Z = (cpustate.X == 0x00);
};

static void op_tay(void) /* page 101 MOS */
{
	cpustate.Y = cpustate.A;
	cpustate.N = (cpustate.Y & 0x80) != 0;
	cpustate.Z = (cpustate.Y == 0x00);
};

static void op_txa(void) /* page 100 MOS */
{
	cpustate.A = cpustate.X;
	cpustate.N = (cpustate.X & 0x80) != 0;
	cpustate.Z = (cpustate.X == 0x00);
};

static void op_tya(void) /* page 101 MOS */
{
	cpustate.A = cpustate.Y;
	cpustate.N = (cpustate.Y & 0x80) != 0;
	cpustate.Z = (cpustate.Y == 0x00);
};

static void op_brk(void) /* page 144 MOS */
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
	/* 10 */ &op_bpl,
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
	/* 24 */ &op_bit_zero,
	/* 25 */ NULL,
	/* 26 */ NULL,
	/* 27 */ NULL,
	/* 28 */ NULL,
	/* 29 */ NULL,
	/* 2a */ NULL,
	/* 2b */ NULL,
	/* 2c */ &op_bit_absolute,
	/* 2d */ NULL,
	/* 2e */ NULL,
	/* 2f */ NULL,
	/* 30 */ &op_bmi,
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
	/* 4c */ &op_jmp,
	/* 4d */ NULL,
	/* 4e */ NULL,
	/* 4f */ NULL,
	/* 50 */ &op_bvc,
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
	/* 6c */ &op_jmp_indirect,
	/* 6d */ NULL,
	/* 6e */ NULL,
	/* 6f */ NULL,
	/* 70 */ &op_bvs,
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
	/* 8a */ &op_txa,
	/* 8b */ NULL,
	/* 8c */ &op_sty_absolute,
	/* 8d */ NULL,
	/* 8e */ &op_stx_absolute,
	/* 8f */ NULL,
	/* 90 */ &op_bcc,
	/* 91 */ NULL,
	/* 92 */ NULL,
	/* 93 */ NULL,
	/* 94 */ &op_sty_zero_x,
	/* 95 */ NULL,
	/* 96 */ &op_stx_zero_y,
	/* 97 */ NULL,
	/* 98 */ &op_tya,
	/* 99 */ NULL,
	/* 9a */ NULL,
	/* 9b */ NULL,
	/* 9c */ NULL,
	/* 9d */ NULL,
	/* 9e */ NULL,
	/* 9f */ NULL,
	/* a0 */ &op_ldy,
	/* a1 */ NULL,
	/* a2 */ &op_ldx,
	/* a3 */ NULL,
	/* a4 */ &op_ldy_zero,
	/* a5 */ NULL,
	/* a6 */ &op_ldx_zero,
	/* a7 */ NULL,
	/* a8 */ &op_tay,
	/* a9 */ NULL,
	/* aa */ &op_tax,
	/* ab */ NULL,
	/* ac */ &op_ldy_absolute,
	/* ad */ NULL,
	/* ae */ &op_ldx_absolute,
	/* af */ NULL,
	/* b0 */ &op_bcs,
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
	/* c1 */ &op_cmp_indirect_x,
	/* c2 */ NULL,
	/* c3 */ NULL,
	/* c4 */ &op_cpy_zero,
	/* c5 */ &op_cmp_zero,
	/* c6 */ NULL,
	/* c7 */ NULL,
	/* c8 */ &op_incy,
	/* c9 */ &op_cmp,
	/* ca */ &op_dex,
	/* cb */ NULL,
	/* cc */ &op_cpy_absolute,
	/* cd */ &op_cmp_absolute,
	/* ce */ NULL,
	/* cf */ NULL,
	/* d0 */ &op_bne,
	/* d1 */ &op_cmp_indirect_y,
	/* d2 */ NULL,
	/* d3 */ NULL,
	/* d4 */ NULL,
	/* d5 */ &op_cmp_zero_x,
	/* d6 */ NULL,
	/* d7 */ NULL,
	/* d8 */ NULL,
	/* d9 */ &op_cmp_absolute_y,
	/* da */ NULL,
	/* db */ NULL,
	/* dc */ NULL,
	/* dd */ &op_cmp_absolute_x,
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
	/* f0 */ &op_beq,
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

		/* advance */
		cpustate.PC++;

		/* execute */
		func();

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
