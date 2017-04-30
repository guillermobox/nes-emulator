/*
 * Ricoh 2A03 based on MOS 6502
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "ppu.h"

typedef uint8_t byte;
typedef uint16_t addr;
typedef void (*opfunct)(void);

/*
 * I'm mapping the addresses above 0x8000 directly onto the
 * memory bank of the application.
 */
byte memory[0x10000];
byte * prgmem = memory + 0x8000;
addr address; /* address used for memory addressing in the functions */

struct st_cpustate {
	union {
		addr PC; /* program counter */
		struct {
			byte PCH;
			byte PCL;
		};
	};
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

int gamepad_state = 0;
byte gamepad_mask = 0x01;
byte gamepad_value = 0x00;

void gamepad_write(byte data)
{
	(void) data;
	gamepad_state = data;
	gamepad_mask = 0x01;
}

byte gamepad_read()
{
	byte ret;
	if (gamepad_state == 0)
		ret = (gamepad_value & gamepad_mask) != 0;
	else
		ret = (gamepad_value & 0x01);
	gamepad_mask <<= 1;
	return ret;
}

addr demirror(addr address)
{
	if (address < 0x2000) {
		return address & 0x07FF;
	}
	if (address < 0x4000){
		return 0x2000 + (address & 0x0007);
	}
	return address;
}

static inline void memstore(addr address, byte data)
{
	address = demirror(address);

	if (address == 0x2000) {
		ppu_set_control1(data);
		return;
	}
	if (address == 0x2001) {
		ppu_set_control2(data);
		return;
	}
	if (address == 0x2006) {
		ppu_set_address(data);
		return;
	}
	if (address == 0x2007) {
		ppu_write_data(data);
		return;
	}
	if (address == 0x2005) {
		ppu_set_scroll(data);
		return ;
	}
	if (address == 0x2003) {
		ppu_set_oam(data);
		return;
	}
	if (address == 0x2004) {
		ppu_write_oam(data);
		return;
	}
	if (address == 0x4014) {
		ppu_dmatransfer(data);
		return;
	}
	if (address == 0x4016) {
		gamepad_write(data);
		return;
	}
	if (address >= 0x4000 && address <= 0x4017) {
		//printf("Audio routine write\n");
		return;
	}

	if (address < 0x0800)
		*(memory + address) = data;
	else
		printf("ERROR: You cannot write here: %04x!\n", address);
};

static inline byte memload(addr address)
{
	address = demirror(address);

	if (address == 0x2002) {
		return ppu_get_control();
	}

	if (address == 0x2007) {
		return ppu_read_data();
	}

	if (address == 0x4016) {
		return gamepad_read();
	}

	if (address >= 0x4000 && address <= 0x4017) {
		//printf("Audio routine read\n");
		return 0x00;
	}

	if (address < 0x8000 && address > 0x07FF) {
		printf("ERROR: what are you reading here? %04x\n", address);
	}

	return *(memory + address);
};

static void stack_push(byte data)
{
	memstore(0x0100 + cpustate.SP--, data);
};

static byte stack_pull()
{
	return memload(0x0100 + ++cpustate.SP);
};

/*
 * Chapter 6 os MOS
 * INDEX REGISTERS AND INDEX ADDRESSING CONCEPTS
 */

static void imp()
{
};

static void dir()
{
	address = cpustate.PC++;
};

static void zer()
{
	address = (addr) memload(cpustate.PC++);
};

static void zex()
{
	address = (addr) 0xFF & (memload(cpustate.PC++) + cpustate.X);
};

static void zey()
{
	address = (addr) 0xFF & (memload(cpustate.PC++) + cpustate.Y);
};

static void aba()
{
	address  =  memload(cpustate.PC++);
	address |= memload(cpustate.PC++) << 8;
};

static void abx()
{
	address  = memload(cpustate.PC++);
	address |= memload(cpustate.PC++) << 8;
	address += cpustate.X;
};

static void aby()
{
	address  = memload(cpustate.PC++);
	address |= memload(cpustate.PC++) << 8;
	address += cpustate.Y;
};

static void ind()
{
	byte off = memload(cpustate.PC++);
	address  = memload(off);
	address |= memload((off + 1) & 0xff) << 8;
};

static void aix()
{
	addr off;
	off = 0xFF & ((memload(cpustate.PC++) + cpustate.X));
	address  = memload(off & 0xff);
	address |= memload((off + 1) & 0xff) << 8;
};

static void aiy()
{
	addr off;
	off = memload(cpustate.PC++);
	address = memload(off);
	address += memload((off + 1) & 0xff) << 8;
	address += cpustate.Y;
};

static void rel()
{
	addr original = cpustate.PC + 0x01;
	byte offset = memload(cpustate.PC++);
	if (offset & 0x80)
		address = original + (0xff00 | offset);
	else
		address = original + offset;
};

/*
 * Chapter 2 of MOS
 * THE DATA BUS, ACCUMULATOR AND ARITHMETIC UNIT
 */

static void lda(void) /* page 4 MOS */
{
	cpustate.A = memload(address);
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void sta(void) /* page 5 MOS */
{
	memstore(address, cpustate.A);
};

static void adc(void) /* page 7 MOS */
{
	byte value = memload(address);
	uint16_t sum = (uint16_t) cpustate.A + (uint16_t) value + cpustate.C;
	if (cpustate.D) {
		if (sum & 0x0100) {
			cpustate.C = 1;
			sum = sum + (6<<4);
		}
		if ((sum & 0x0f) > 0x09)
			sum = sum + 6;
		if ((sum & 0xf0) > 0x90)
			sum = sum + (6<<4);
	}
	cpustate.V = (((cpustate.A ^ value) & 0x80) == 0x00) && ((sum & 0x80) != (value & 0x80));
	cpustate.A = sum;
	cpustate.C = (sum & 0x0100) != 0x0000;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void sbc(void) /* page 14 MOS */
{
	byte value = memload(address);
	byte value2 = (value ^ 0xFF) + cpustate.C;
	uint16_t sum = (uint16_t) cpustate.A + (uint16_t) value2;
	if (cpustate.D) {
		if ((sum & 0x0f) > 0x09)
			sum = sum - 6;
		if ((sum & 0xf0) > 0x90)
			sum = sum - (6<<4);
	}
	cpustate.V = (((cpustate.A ^ value) & 0x80) == 0x00) && ((sum & 0x80) != (value & 0x80));
	cpustate.A = sum;
	if (value!=0)
		cpustate.C = (sum & 0x0100) != 0x0000;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void and(void) /* page 20 MOS */
{
	byte value = memload(address);
	cpustate.A &= value;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void ora(void) /* page 21 MOS */
{
	byte value = memload(address);
	cpustate.A |= value;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void eor(void) /* page 21 MOS */
{
	byte value = memload(address);
	cpustate.A ^= value;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};


/*
 * Chapter 3 of MOS
 * CONCEPTS OF FLAGS AND STATUS REGISTER
 */
static void sec(void) /* page 24 MOS */
{
	cpustate.C = 1;
};

static void clc(void) /* page 25 MOS */
{
	cpustate.C = 0;
};

static void sei(void) /* page 26 MOS */
{
	cpustate.I = 1;
};

static void cli(void) /* page 26 MOS */
{
	cpustate.I = 0;
};

static void sed(void) /* page 26 MOS */
{
	cpustate.D = 1;
};

static void cld(void) /* page 27 MOS */
{
	cpustate.D = 0;
};

static void clv(void) /* page 28 MOS */
{
	cpustate.V = 0;
};

/*
 * Chapter 4 of MOS
 * TEST BRANCH AND JUMP INSTRUCTIONS
 */
static void jmp(void) /* page 36 MOS */
{
	cpustate.PC = address;
};

static void bmi(void) /* page 40 MOS */
{
	if (cpustate.N) {
		cpustate.PC = address;
	}
};

static void bpl(void) /* page 40 MOS */
{
	if (cpustate.N == 0) {
		cpustate.PC = address;
	}
};

static void bcc(void) /* page 40 MOS */
{
	if (cpustate.C == 0) {
		cpustate.PC = address;
	}
};

static void bcs(void) /* page 40 MOS */
{
	if (cpustate.C) {
		cpustate.PC = address;
	}
};

static void beq(void) /* page 41 MOS */
{
	if (cpustate.Z) {
		cpustate.PC = address;
	}
};

static void bne(void) /* page 41 MOS */
{
	if (cpustate.Z == 0) {
		cpustate.PC = address;
	}
};

static void bvs(void) /* page 41 MOS */
{
	if (cpustate.V) {
		cpustate.PC = address;
	}
};

static void bvc(void) /* page 41 MOS */
{
	if (cpustate.V == 0) {
		cpustate.PC = address;
	}
};

static void cmp(void) /* page 45 MOS */
{
	byte mem = memload(address);
	cpustate.C = (mem > cpustate.A)? 0 : 1;
	cpustate.N = ((cpustate.A - mem) & 0x80) != 0x00;
	cpustate.Z = cpustate.A == mem;
};

static void bit(void) /* page 47 MOS */
{
	byte value = memload(address);
	byte and = cpustate.A & value;
	cpustate.Z = and == 0x00;
	cpustate.N = (value & 0x80) != 0x00;
	cpustate.V = (value & 0x40) != 0x00;
};

/*
 * Chapter 7 of MOS
 * INDEX REGISTER INSTRUCTIONS
 */
static void ldx(void) /* page 96 MOS */
{
	cpustate.X = memload(address);
	cpustate.N = (cpustate.X & 0x80) != 0;
	cpustate.Z = (cpustate.X == 0x00);
};

static void ldy(void) /* page 96 MOS */
{
	cpustate.Y = memload(address);
	cpustate.N = (cpustate.Y & 0x80) != 0;
	cpustate.Z = (cpustate.Y == 0x00);
};

static void stx(void) /* page 97 MOS */
{
	memstore(address, cpustate.X);
};

static void sty(void) /* page 97 MOS */
{
	memstore(address, cpustate.Y);
};

static void inx(void) /* page 97 MOS */
{
	cpustate.X += 0x01;
	cpustate.N = (cpustate.X & 0x80) != 0;
	cpustate.Z = (cpustate.X == 0x00);
};

static void iny(void) /* page 97 MOS */
{
	cpustate.Y += 0x01;
	cpustate.N = (cpustate.Y & 0x80) != 0;
	cpustate.Z = (cpustate.Y == 0x00);
};

static void dex(void) /* page 98 MOS */
{
	cpustate.X -= 0x01;
	cpustate.N = (cpustate.X & 0x80) != 0;
	cpustate.Z = (cpustate.X == 0x00);
};

static void dey(void) /* page 98 MOS */
{
	cpustate.Y -= 0x01;
	cpustate.N = (cpustate.Y & 0x80) != 0;
	cpustate.Z = (cpustate.Y == 0x00);
};

static void cpx(void) /* page 99 MOS */
{
	byte value = memload(address);
	byte sub = cpustate.X - value;
	cpustate.Z = value == cpustate.X;
	cpustate.N = (sub & 0x80) != 0;
	cpustate.C = (value > cpustate.X)? 0 : 1;
};

static void cpy(void) /* page 99 MOS */
{
	byte value = memload(address);
	byte sub = cpustate.Y - value;
	cpustate.Z = value == cpustate.Y;
	cpustate.N = (sub & 0x80) != 0;
	cpustate.C = (value > cpustate.Y)? 0 : 1;
};

static void tax(void) /* page 100 MOS */
{
	cpustate.X = cpustate.A;
	cpustate.N = (cpustate.X & 0x80) != 0;
	cpustate.Z = (cpustate.X == 0x00);
};

static void tay(void) /* page 101 MOS */
{
	cpustate.Y = cpustate.A;
	cpustate.N = (cpustate.Y & 0x80) != 0;
	cpustate.Z = (cpustate.Y == 0x00);
};

static void txa(void) /* page 100 MOS */
{
	cpustate.A = cpustate.X;
	cpustate.N = (cpustate.X & 0x80) != 0;
	cpustate.Z = (cpustate.X == 0x00);
};

static void tya(void) /* page 101 MOS */
{
	cpustate.A = cpustate.Y;
	cpustate.N = (cpustate.Y & 0x80) != 0;
	cpustate.Z = (cpustate.Y == 0x00);
};

/*
 * Chapter 8 of MOS
 * STACK PROCESSING
 */
static void jsr(void) /* page 106 MOS */
{
	cpustate.PC--;
	stack_push(cpustate.PCL);
	stack_push(cpustate.PCH);
	cpustate.PC = address;
};

static void rts(void) /* page 108 MOS */
{
	cpustate.PCH = stack_pull();
	cpustate.PCL = stack_pull();
	cpustate.PC += 1;
};

static void pha(void) /* page 117 MOS */
{
	stack_push(cpustate.A);
};

static void pla(void) /* page 118 MOS */
{
	cpustate.A = stack_pull();
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void txs(void) /* page 120 MOS */
{
	cpustate.SP = cpustate.X;
};

static void tsx(void) /* page 122 MOS */
{
	cpustate.X = cpustate.SP;
	cpustate.Z = cpustate.X == 0x00;
	cpustate.N = (cpustate.X & 0x80) != 0x00;
};

static void php(void) /* page 122 MOS */
{
	stack_push(cpustate.P);
};

static void plp(void) /* page 123 MOS */
{
	cpustate.P = stack_pull();
};

static void rti(void) /* page 132 MOS */
{
	cpustate.PC = stack_pull() << 8 | stack_pull();
	cpustate.P = stack_pull();
};

static void brk(void) /* page 144 MOS */
{
	cpustate.PC = (addr)memload(0xFFFE) | ((addr)memload(0xFFFF) << 8);
};

/*
 * Chapter 10
 * SHIFT AND MEMORY MODIFY INSTRUCTIONS
 */
static void lsra(void) /* page 148 MOS */
{
	cpustate.C = cpustate.A & 0x01;
	cpustate.A >>= 1;
	cpustate.N = 0;
	cpustate.Z = cpustate.A == 0x00;
};

static void lsr(void) /* page 148 MOS */
{
	byte value = memload(address);
	cpustate.C = value & 0x01;
	value >>= 1;
	cpustate.N = 0;
	cpustate.Z = value == 0x00;
	memstore(address, value);
};

static void asla(void) /* page 149 MOS */
{
	cpustate.C = (cpustate.A & 0x80) != 0x00;
	cpustate.A <<= 1;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
	cpustate.Z = cpustate.A == 0x00;
};

static void asl(void) /* page 149 MOS */
{
	byte value = memload(address);
	cpustate.C = (value & 0x80) != 0x00;
	value <<= 1;
	cpustate.N = (value & 0x80) != 0x00;
	cpustate.Z = value == 0x00;
	memstore(address, value);
};

static void rola(void) /* page 149 MOS */
{
	byte oldc = cpustate.C;
	cpustate.C = (cpustate.A & 0x80) != 0x00;
	cpustate.A <<= 1;
	if (oldc) cpustate.A += 1;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
	cpustate.Z = cpustate.A == 0x00;
};

static void rol(void) /* page 149 MOS */
{
	byte value = memload(address);
	byte oldc = cpustate.C;
	cpustate.C = (value & 0x80) != 0x00;
	value <<= 1;
	if (oldc) value += 1;
	cpustate.N = (value & 0x80) != 0x00;
	cpustate.Z = value == 0x00;
	memstore(address, value);
};

static void rora(void) /* page 150 MOS */
{
	byte oldc = cpustate.C;
	cpustate.C = cpustate.A & 0x01;
	cpustate.A >>= 1;
	if (oldc)
		cpustate.A += 0x80;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
	cpustate.Z = cpustate.A == 0x00;
};

static void ror(void) /* page 149 MOS */
{
	byte value = memload(address);
	byte oldc = cpustate.C;
	cpustate.C = value & 0x01;
	value >>= 1;
	if (oldc)
		value += 0x80;
	cpustate.N = (value & 0x80) != 0x00;
	cpustate.Z = value == 0x00;
	memstore(address, value);
};

static void inc(void) /* page 155 MOS */
{
	byte value = memload(address) + 1;
	cpustate.Z = value == 0x00;
	cpustate.N = (value & 0x80) != 0x00;
	memstore(address, value);
};

static void dec(void) /* page 155 MOS */
{
	byte value = memload(address) + 0xff;
	cpustate.Z = value == 0x00;
	cpustate.N = (value & 0x80) != 0x00;
	memstore(address, value);
};

static void nop(void)
{
};

#define NUL NULL

opfunct addressing_map[] = {
       /* 0   1    2    3    4    5    6    7    8    9    a    b    c    d    e    f  */
/* 0 */ imp, aix, NUL, NUL, NUL, zer, zer, NUL, imp, dir, imp, NUL, NUL, aba, aba, NUL,
/* 1 */ rel, aiy, NUL, NUL, NUL, zex, zex, NUL, imp, aby, NUL, NUL, NUL, abx, abx, NUL,
/* 2 */ aba, aix, NUL, NUL, zer, zer, zer, NUL, imp, dir, imp, NUL, aba, aba, aba, NUL,
/* 3 */ rel, aiy, NUL, NUL, NUL, zex, zex, NUL, imp, aby, NUL, NUL, NUL, abx, abx, NUL,
/* 4 */ imp, aix, NUL, NUL, NUL, zer, zer, NUL, imp, dir, imp, NUL, aba, aba, aba, NUL,
/* 5 */ rel, aiy, NUL, NUL, NUL, zex, zex, NUL, imp, aby, NUL, NUL, NUL, abx, abx, NUL,
/* 6 */ imp, aix, NUL, NUL, NUL, zer, zer, NUL, imp, dir, imp, NUL, ind, aba, aba, NUL,
/* 7 */ rel, aiy, NUL, NUL, NUL, zex, zex, NUL, imp, aby, NUL, NUL, NUL, abx, abx, NUL,
/* 8 */ NUL, aix, NUL, NUL, zer, zer, zer, NUL, imp, NUL, imp, NUL, aba, aba, aba, NUL,
/* 9 */ rel, aiy, NUL, NUL, zex, zex, zey, NUL, imp, aby, imp, NUL, NUL, abx, NUL, NUL,
/* a */ dir, aix, dir, NUL, zer, zer, zer, NUL, imp, dir, imp, NUL, aba, aba, aba, NUL,
/* b */ rel, aiy, NUL, NUL, zex, zex, zey, NUL, imp, aby, imp, NUL, abx, abx, aby, NUL,
/* c */ dir, aix, NUL, NUL, zer, zer, zer, NUL, imp, dir, imp, NUL, aba, aba, aba, NUL,
/* d */ rel, aiy, NUL, NUL, NUL, zex, zex, NUL, imp, aby, NUL, NUL, NUL, abx, abx, NUL,
/* e */ dir, aix, NUL, NUL, zer, zer, zer, NUL, imp, dir, imp, NUL, aba, aba, aba, NUL,
/* f */ rel, aiy, NUL, NUL, NUL, zex, zex, NUL, imp, aby, NUL, NUL, NUL, abx, abx, NUL,
};

opfunct instruction_map[] = {
       /* 0   1    2    3    4    5    6    7    8    9    a    b    c    d    e    f  */
/* 0 */ brk, ora, NUL, NUL, NUL, ora, asl, NUL, php, ora,asla, NUL, NUL, ora, asl, NUL,
/* 1 */ bpl, ora, NUL, NUL, NUL, ora, asl, NUL, clc, ora, NUL, NUL, NUL, ora, asl, NUL,
/* 2 */ jsr, and, NUL, NUL, bit, and, rol, NUL, plp, and,rola, NUL, bit, and, rol, NUL,
/* 3 */ bmi, and, NUL, NUL, NUL, and, rol, NUL, sec, and, NUL, NUL, NUL, and, rol, NUL,
/* 4 */ rti, eor, NUL, NUL, NUL, eor, lsr, NUL, pha, eor,lsra, NUL, jmp, eor, lsr, NUL,
/* 5 */ bvc, eor, NUL, NUL, NUL, eor, lsr, NUL, cli, eor, NUL, NUL, NUL, eor, lsr, NUL,
/* 6 */ rts, adc, NUL, NUL, NUL, adc, ror, NUL, pla, adc,rora, NUL, jmp, adc, ror, NUL,
/* 7 */ bvs, adc, NUL, NUL, NUL, adc, ror, NUL, sei, adc, NUL, NUL, NUL, adc, ror, NUL,
/* 8 */ NUL, sta, NUL, NUL, sty, sta, stx, NUL, dey, NUL, txa, NUL, sty, sta, stx, NUL,
/* 9 */ bcc, sta, NUL, NUL, sty, sta, stx, NUL, tya, sta, txs, NUL, NUL, sta, NUL, NUL,
/* a */ ldy, lda, ldx, NUL, ldy, lda, ldx, NUL, tay, lda, tax, NUL, ldy, lda, ldx, NUL,
/* b */ bcs, lda, NUL, NUL, ldy, lda, ldx, NUL, clv, lda, tsx, NUL, ldy, lda, ldx, NUL,
/* c */ cpy, cmp, NUL, NUL, cpy, cmp, dec, NUL, iny, cmp, dex, NUL, cpy, cmp, dec, NUL,
/* d */ bne, cmp, NUL, NUL, NUL, cmp, dec, NUL, cld, cmp, NUL, NUL, NUL, cmp, dec, NUL,
/* e */ cpx, sbc, NUL, NUL, cpx, sbc, inc, NUL, inx, sbc, nop, NUL, cpx, sbc, inc, NUL,
/* f */ beq, sbc, NUL, NUL, NUL, sbc, inc, NUL, sed, sbc, NUL, NUL, NUL, sbc, inc, NUL,
};

void print_op(addr address, char buffer[16])
{
	byte op;
	opfunct addressing, instruction;

	op = memload(address);
	addressing = addressing_map[op];
	instruction = instruction_map[op];

	buffer[0] = '\0';

#define CHECK(op,rep)\
	if (instruction == op)\
		strcat(buffer, #rep );\
	else

	CHECK(adc, ADC) CHECK(and, AND) CHECK(asl, ASL) CHECK(bcc, BCC)
	CHECK(bcs, BCS) CHECK(beq, BEQ) CHECK(bit, BIT) CHECK(bmi, BMI)
	CHECK(bne, BNE) CHECK(bpl, BPL) CHECK(brk, BRK) CHECK(bvc, BVC)
	CHECK(bvs, BVS) CHECK(clc, CLC) CHECK(cld, CLD) CHECK(cli, CLI)
	CHECK(clv, CLV) CHECK(cmp, CMP) CHECK(cpx, CPX) CHECK(cpy, CPY)
	CHECK(dec, DEC) CHECK(dex, DEX) CHECK(dey, DEY) CHECK(eor, EOR)
	CHECK(inc, INC) CHECK(inx, INX) CHECK(iny, INY) CHECK(jmp, JMP)
	CHECK(jsr, JSR) CHECK(lda, LDA) CHECK(ldx, LDX) CHECK(ldy, LDY)
	CHECK(lsr, LSR) CHECK(nop, NOP) CHECK(ora, ORA) CHECK(pha, PHA)
	CHECK(php, PHP) CHECK(pla, PLA) CHECK(plp, PLP) CHECK(rol, ROL)
	CHECK(ror, ROR) CHECK(rti, RTI) CHECK(rts, RTS) CHECK(sbc, SBC)
	CHECK(sec, SEC) CHECK(sed, SED) CHECK(sei, SEI) CHECK(sta, STA)
	CHECK(stx, STX) CHECK(sty, STY) CHECK(tax, TAX) CHECK(tay, TAY)
	CHECK(tsx, TSX) CHECK(txa, TXA) CHECK(txs, TXS) CHECK(tya, TYA)
	CHECK(asla, ASL A) CHECK(rola, ROL A) CHECK(rora, ROR A)
	CHECK(lsra, LSR A)
	sprintf(buffer, " %02X", op);
#undef CHECK

	if (addressing == imp)
		return;
	else if (addressing == dir || addressing == rel)
		sprintf(buffer + 3, " #$%02X", memload(address + 0x01));
	else if (addressing == zer)
		sprintf(buffer + 3, " $%02X", memload(address + 0x01));
	else if (addressing == zex)
		sprintf(buffer + 3, " $%02X,X", memload(address + 0x01));
	else if (addressing == zey)
		sprintf(buffer + 3, " $%02X,Y", memload(address + 0x01));
	else if (addressing == aba)
		sprintf(buffer + 3, " $%02X%02X", memload(address + 0x02), memload(address + 0x01));
	else if (addressing == abx)
		sprintf(buffer + 3, " $%02X%02X,X", memload(address + 0x02), memload(address + 0x01));
	else if (addressing == aby)
		sprintf(buffer + 3, " $%02X%02X,Y", memload(address + 0x02), memload(address + 0x01));
	else if (addressing == ind)
		sprintf(buffer + 3, " ($%02X%02X)", memload(address + 0x02), memload(address + 0x01));
	else if (addressing == aix)
		sprintf(buffer + 3, " ($%02X,X)", memload(address + 0x01));
	else if (addressing == aiy)
		sprintf(buffer + 3, " ($%02X),Y", memload(address + 0x01));

};



void print_cpustate()
{
	flockfile(stdout);

	printf("PC: 0x%02x SP: 0x%02x A: 0x%02x X: 0x%02x Y: 0x%02x ",
			cpustate.PC, cpustate.SP, cpustate.A,
			cpustate.X, cpustate.Y);
	printf("CZIDBVN: %d%d%d%d%d%d%db ",
			cpustate.C, cpustate.Z, cpustate.I,
			cpustate.D, cpustate.B, cpustate.V,
			cpustate.N);
	//printf("OP: %02x\n", memload(cpustate.PC));
	char buffer[16];
	print_op(cpustate.PC, buffer);
	printf("| %s\n", buffer);

	//if (cpustate.SP != 0xff) {
	//	printf("Stack: ");
	//	for (off = 0xff; off > cpustate.SP; off--) {
	//		printf("%02x ", memload(0x0100 + off));
	//	}
	//}
	//printf("\n");


	funlockfile(stdout);
};

static void cpu_boot()
{
	//printf("Booting CPU...\n");
	//printf("Starting memory...\n");
	memset(memory, 0, sizeof(memory));

	//printf("Starting CPU state...\n");
	memset(&cpustate, 0, sizeof(cpustate));

	//printf("Starting stack...\n");
	cpustate.SP = 0xFF;
}

void cpu_init()
{
	cpu_boot();
};

void cpu_load(byte *prg, size_t size)
{
	memcpy(prgmem, prg, size);
	cpustate.PC = (addr)memload(0xfffc) | ((addr)memload(0xfffd) << 8);
};

void check_interrupts()
{
	if (cpustate.NMI) {
		//printf("CPU: In NMI routine\n");
		cpustate.NMI = 0;
		addr newpc = (addr) memload(0xfffa) | ((addr) memload(0xfffb) << 8);
		stack_push(cpustate.P);
		stack_push((byte)cpustate.PC);
		stack_push((byte)(cpustate.PC >> 8));
		cpustate.PC = newpc;
	}
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

static long timediff(struct timespec from, struct timespec to)
{
	return (to.tv_sec - from.tv_sec) * 1000000000 + to.tv_nsec - from.tv_nsec;
};

void cpu_run()
{
	byte op;
	opfunct addressing, instruction;
	struct timespec start_cycle, end_cycle, remaining;
	long elapsed;
	long step = 10;

	do {
		//print_cpustate();

		/* fetch */
		clock_gettime(CLOCK_REALTIME, &start_cycle);
		op = memload(cpustate.PC);
		if (op == 0x00) {
			printf("Landed in BRK instruction, at 0x%04x\n", cpustate.PC);
			cpu_dump();
			ppu_dump();
		}


		/* decode */
		addressing = addressing_map[op];
		instruction = instruction_map[op];

		if (instruction == NULL) {
			fprintf(stderr, "Unrecognized instruction: %02x\n", op);
			fprintf(stderr, "  At position: %04x\n", cpustate.PC);
			cpu_dump(0);
			exit(1);
		}

		/* advance */
		cpustate.PC++;

		/* execute */
		addressing();
		instruction();
		check_interrupts();

		clock_gettime(CLOCK_REALTIME, &end_cycle);

		elapsed = timediff(start_cycle, end_cycle);
		if (elapsed < step) {
			struct timespec sleepage = {.tv_sec=0, .tv_nsec=step-elapsed};
			nanosleep(&sleepage, &remaining);
		}


	} while (op != 0x00); /* Im stopping on brk */

	exit(1);
	
};

