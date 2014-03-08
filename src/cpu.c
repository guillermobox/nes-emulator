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
 * memory bank of the application.
 */
byte memory[0x10000];
byte * prgmem = memory + 0x8000;
addr address;

struct st_cpustate {
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

static inline void memstore(addr address, byte data)
{
	if (address < 0x8000)
		*(memory + address) = data;
	else
		*(prgmem + address - 0x8000) = data;
};

static inline byte memload(addr address)
{
	if (address < 0x8000)
		return *(memory + address);
	else
		return *(prgmem + address - 0x8000);
};

static void stack_push(byte data)
{
};

static byte stack_pull()
{

};

/*
 * Chapter 6 os MOS
 * INDEX REGISTERS AND INDEX ADDRESSING CONCEPTS
 */
static void zer()
{
	address = (addr) memload(cpustate.PC++);
};

static void zex()
{
	address = (addr) (memload(cpustate.PC++) + cpustate.X);
};

static void zey()
{
	address = (addr) (memload(cpustate.PC++) + cpustate.Y);
};

static void abs()
{
	address = (addr) memload(cpustate.PC++) |
		((addr) memload(cpustate.PC)++ << 8)
};

static void abx()
{
	address = (addr) memload(cpustate.PC++) |
		((addr) memload(cpustate.PC)++ <<8) +
		(addr) cpustate.X;
};

static void aby()
{
	address = (addr) memload(cpustate.PC++) |
		((addr) memload(cpustate.PC)++ <<8) +
		(addr) cpustate.Y;
};

static void aix()
{
	address = memload( (addr)(memload(cpustate.PC++) + cpustate.X));
};

static void aiy()
{
	address = memload( (addr)(memload(cpustate.PC++) + cpustate.Y));
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

static void op_adc(void) /* page 7 MOS */
{
	byte value = memload(cpustate.PC++);
	uint16_t sum = (uint16_t) cpustate.A + (uint16_t) value + cpustate.C;
	cpustate.V = (((cpustate.A ^ value) & 0x80) == 0x00) && ((sum & 0x80) != (value & 0x80));
	cpustate.A = sum;
	cpustate.C = (sum & 0x0100) != 0x0000;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_adc_absolute(void) /* page 7 MOS */
{
	addr memaddr = get_address_absolute();
	byte value = memload(memaddr);
	uint16_t sum = (uint16_t) cpustate.A + (uint16_t) value + cpustate.C;
	cpustate.V = (((cpustate.A ^ value) & 0x80) == 0x00) && ((sum & 0x80) != (value & 0x80));
	cpustate.A = sum;
	cpustate.C = (sum & 0x0100) != 0x0000;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_adc_absolute_x(void) /* page 7 MOS */
{
	addr memaddr = get_address_absolute_x();
	byte value = memload(memaddr);
	uint16_t sum = (uint16_t) cpustate.A + (uint16_t) value + cpustate.C;
	cpustate.V = (((cpustate.A ^ value) & 0x80) == 0x00) && ((sum & 0x80) != (value & 0x80));
	cpustate.A = sum;
	cpustate.C = (sum & 0x0100) != 0x0000;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_adc_absolute_y(void) /* page 7 MOS */
{
	addr memaddr = get_address_absolute_y();
	byte value = memload(memaddr);
	uint16_t sum = (uint16_t) cpustate.A + (uint16_t) value + cpustate.C;
	cpustate.V = (((cpustate.A ^ value) & 0x80) == 0x00) && ((sum & 0x80) != (value & 0x80));
	cpustate.A = sum;
	cpustate.C = (sum & 0x0100) != 0x0000;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_adc_zero(void) /* page 7 MOS */
{
	addr memaddr = get_address_zero();
	byte value = memload(memaddr);
	uint16_t sum = (uint16_t) cpustate.A + (uint16_t) value + cpustate.C;
	cpustate.V = (((cpustate.A ^ value) & 0x80) == 0x00) && ((sum & 0x80) != (value & 0x80));
	cpustate.A = sum;
	cpustate.C = (sum & 0x0100) != 0x0000;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_adc_zero_x(void) /* page 7 MOS */
{
	addr memaddr = get_address_zero_x();
	byte value = memload(memaddr);
	uint16_t sum = (uint16_t) cpustate.A + (uint16_t) value + cpustate.C;
	cpustate.V = (((cpustate.A ^ value) & 0x80) == 0x00) && ((sum & 0x80) != (value & 0x80));
	cpustate.A = sum;
	cpustate.C = (sum & 0x0100) != 0x0000;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_adc_indirect_x(void) /* page 7 MOS */
{
	addr memaddr = get_address_indirect_x();
	byte value = memload(memaddr);
	uint16_t sum = (uint16_t) cpustate.A + (uint16_t) value + cpustate.C;
	cpustate.V = (((cpustate.A ^ value) & 0x80) == 0x00) && ((sum & 0x80) != (value & 0x80));
	cpustate.A = sum;
	cpustate.C = (sum & 0x0100) != 0x0000;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_adc_indirect_y(void) /* page 7 MOS */
{
	addr memaddr = get_address_indirect_y();
	byte value = memload(memaddr);
	uint16_t sum = (uint16_t) cpustate.A + (uint16_t) value + cpustate.C;
	cpustate.V = (((cpustate.A ^ value) & 0x80) == 0x00) && ((sum & 0x80) != (value & 0x80));
	cpustate.A = sum;
	cpustate.C = (sum & 0x0100) != 0x0000;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_sbc(void) /* page 14 MOS */
{
	byte value = memload(cpustate.PC++);
	uint16_t sum = (uint16_t) cpustate.A - (uint16_t) value - (1 - cpustate.C);
	cpustate.V = (((cpustate.A ^ value) & 0x80) == 0x00) && ((sum & 0x80) != (value & 0x80));
	cpustate.A = sum;
	cpustate.C = (sum & 0x0100) != 0x0000;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_sbc_absolute(void) /* page 14 MOS */
{
	addr memaddr = get_address_absolute();
	byte value = memload(memaddr);
	uint16_t sum = (uint16_t) cpustate.A - (uint16_t) value - (1 - cpustate.C);
	cpustate.V = (((cpustate.A ^ value) & 0x80) == 0x00) && ((sum & 0x80) != (value & 0x80));
	cpustate.A = sum;
	cpustate.C = (sum & 0x0100) != 0x0000;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_sbc_absolute_x(void) /* page 14 MOS */
{
	addr memaddr = get_address_absolute_x();
	byte value = memload(memaddr);
	uint16_t sum = (uint16_t) cpustate.A - (uint16_t) value - (1 - cpustate.C);
	cpustate.V = (((cpustate.A ^ value) & 0x80) == 0x00) && ((sum & 0x80) != (value & 0x80));
	cpustate.A = sum;
	cpustate.C = (sum & 0x0100) != 0x0000;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_sbc_absolute_y(void) /* page 14 MOS */
{
	addr memaddr = get_address_absolute_y();
	byte value = memload(memaddr);
	uint16_t sum = (uint16_t) cpustate.A - (uint16_t) value - (1 - cpustate.C);
	cpustate.V = (((cpustate.A ^ value) & 0x80) == 0x00) && ((sum & 0x80) != (value & 0x80));
	cpustate.A = sum;
	cpustate.C = (sum & 0x0100) != 0x0000;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_sbc_zero(void) /* page 14 MOS */
{
	addr memaddr = get_address_zero();
	byte value = memload(memaddr);
	uint16_t sum = (uint16_t) cpustate.A - (uint16_t) value - (1 - cpustate.C);
	cpustate.V = (((cpustate.A ^ value) & 0x80) == 0x00) && ((sum & 0x80) != (value & 0x80));
	cpustate.A = sum;
	cpustate.C = (sum & 0x0100) != 0x0000;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_sbc_zero_x(void) /* page 14 MOS */
{
	addr memaddr = get_address_zero_x();
	byte value = memload(memaddr);
	uint16_t sum = (uint16_t) cpustate.A - (uint16_t) value - (1 - cpustate.C);
	cpustate.V = (((cpustate.A ^ value) & 0x80) == 0x00) && ((sum & 0x80) != (value & 0x80));
	cpustate.A = sum;
	cpustate.C = (sum & 0x0100) != 0x0000;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_sbc_indirect_x(void) /* page 14 MOS */
{
	addr memaddr = get_address_indirect_x();
	byte value = memload(memaddr);
	uint16_t sum = (uint16_t) cpustate.A - (uint16_t) value - (1 - cpustate.C);
	cpustate.V = (((cpustate.A ^ value) & 0x80) == 0x00) && ((sum & 0x80) != (value & 0x80));
	cpustate.A = sum;
	cpustate.C = (sum & 0x0100) != 0x0000;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_sbc_indirect_y(void) /* page 14 MOS */
{
	addr memaddr = get_address_indirect_y();
	byte value = memload(memaddr);
	uint16_t sum = (uint16_t) cpustate.A - (uint16_t) value - (1 - cpustate.C);
	cpustate.V = (((cpustate.A ^ value) & 0x80) == 0x00) && ((sum & 0x80) != (value & 0x80));
	cpustate.A = sum;
	cpustate.C = (sum & 0x0100) != 0x0000;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_and(void) /* page 20 MOS */
{
	byte value = memload(cpustate.PC++);
	cpustate.A &= value;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_and_absolute(void) /* page 20 MOS */
{
	addr memaddr = get_address_absolute();
	byte value = memload(memaddr);
	cpustate.A &= value;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_and_absolute_x(void) /* page 20 MOS */
{
	addr memaddr = get_address_absolute_x();
	byte value = memload(memaddr);
	cpustate.A &= value;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_and_absolute_y(void) /* page 20 MOS */
{
	addr memaddr = get_address_absolute_y();
	byte value = memload(memaddr);
	cpustate.A &= value;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_and_zero(void) /* page 20 MOS */
{
	addr memaddr = get_address_zero();
	byte value = memload(memaddr);
	cpustate.A &= value;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_and_zero_x(void) /* page 20 MOS */
{
	addr memaddr = get_address_zero_x();
	byte value = memload(memaddr);
	cpustate.A &= value;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_and_indirect_x(void) /* page 20 MOS */
{
	addr memaddr = get_address_indirect_x();
	byte value = memload(memaddr);
	cpustate.A &= value;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_and_indirect_y(void) /* page 20 MOS */
{
	addr memaddr = get_address_indirect_y();
	byte value = memload(memaddr);
	cpustate.A &= value;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_ora(void) /* page 21 MOS */
{
	byte value = memload(cpustate.PC++);
	cpustate.A |= value;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_ora_absolute(void) /* page 21 MOS */
{
	addr memaddr = get_address_absolute();
	byte value = memload(memaddr);
	cpustate.A |= value;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_ora_absolute_x(void) /* page 21 MOS */
{
	addr memaddr = get_address_absolute_x();
	byte value = memload(memaddr);
	cpustate.A |= value;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_ora_absolute_y(void) /* page 21 MOS */
{
	addr memaddr = get_address_absolute_y();
	byte value = memload(memaddr);
	cpustate.A |= value;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_ora_zero(void) /* page 21 MOS */
{
	addr memaddr = get_address_zero();
	byte value = memload(memaddr);
	cpustate.A |= value;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_ora_zero_x(void) /* page 21 MOS */
{
	addr memaddr = get_address_zero_x();
	byte value = memload(memaddr);
	cpustate.A |= value;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_ora_indirect_x(void) /* page 21 MOS */
{
	addr memaddr = get_address_indirect_x();
	byte value = memload(memaddr);
	cpustate.A |= value;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_ora_indirect_y(void) /* page 21 MOS */
{
	addr memaddr = get_address_indirect_y();
	byte value = memload(memaddr);
	cpustate.A |= value;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_eor(void) /* page 21 MOS */
{
	byte value = memload(cpustate.PC++);
	cpustate.A ^= value;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_eor_absolute(void) /* page 21 MOS */
{
	addr memaddr = get_address_absolute();
	byte value = memload(memaddr);
	cpustate.A ^= value;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_eor_absolute_x(void) /* page 21 MOS */
{
	addr memaddr = get_address_absolute_x();
	byte value = memload(memaddr);
	cpustate.A ^= value;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_eor_absolute_y(void) /* page 21 MOS */
{
	addr memaddr = get_address_absolute_y();
	byte value = memload(memaddr);
	cpustate.A ^= value;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_eor_zero(void) /* page 21 MOS */
{
	addr memaddr = get_address_zero();
	byte value = memload(memaddr);
	cpustate.A ^= value;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_eor_zero_x(void) /* page 21 MOS */
{
	addr memaddr = get_address_zero_x();
	byte value = memload(memaddr);
	cpustate.A ^= value;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_eor_indirect_x(void) /* page 21 MOS */
{
	addr memaddr = get_address_indirect_x();
	byte value = memload(memaddr);
	cpustate.A ^= value;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_eor_indirect_y(void) /* page 21 MOS */
{
	addr memaddr = get_address_indirect_y();
	byte value = memload(memaddr);
	cpustate.A ^= value;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};


/*
 * Chapter 3 of MOS
 * CONCEPTS OF FLAGS AND STATUS REGISTER
 */
static void op_sec(void) /* page 24 MOS */
{
	cpustate.C = 1;
};

static void op_clc(void) /* page 25 MOS */
{
	cpustate.C = 0;
};

static void op_sei(void) /* page 26 MOS */
{
	cpustate.I = 1;
};

static void op_cli(void) /* page 26 MOS */
{
	cpustate.I = 0;
};

static void op_sed(void) /* page 26 MOS */
{
	cpustate.D = 1;
};

static void op_cld(void) /* page 27 MOS */
{
	cpustate.D = 0;
};

static void op_clv(void) /* page 28 MOS */
{
	cpustate.V = 0;
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
	address.b[0] = memload(cpustate.PC++);
	address.b[1] = memload(cpustate.PC++);
	cpustate.PC = address.offset;
};

static void op_jmp_indirect(void) /* page 36 MOS */
{
	union {
		byte b[2];
		addr offset;
	} address;
	address.b[0] = memload(cpustate.PC++);
	address.b[1] = memload(cpustate.PC++);
	union {
		byte b[2];
		addr offset;
	} newpc;
	newpc.b[0] = memload(address.offset);
	newpc.b[1] = memload(address.offset + 1);
	cpustate.PC = newpc.offset;
};

static void op_bmi(void) /* page 40 MOS */
{
	byte diff = memload(cpustate.PC++);
	if (cpustate.N) {
		if (diff & 0x80)
			cpustate.PC -= (diff ^ 0xff) + 0x01;
		else
			cpustate.PC += diff;
	}
};

static void op_bpl(void) /* page 40 MOS */
{
	byte diff = memload(cpustate.PC++);
	if (cpustate.N == 0) {
		if (diff & 0x80)
			cpustate.PC -= (diff ^ 0xff) + 0x01;
		else
			cpustate.PC += diff;
	}
};

static void op_bcc(void) /* page 40 MOS */
{
	byte diff = memload(cpustate.PC++);
	if (cpustate.C == 0) {
		if (diff & 0x80)
			cpustate.PC -= (diff ^ 0xff) + 0x01;
		else
			cpustate.PC += diff;
	}
};

static void op_bcs(void) /* page 40 MOS */
{
	byte diff = memload(cpustate.PC++);
	if (cpustate.C) {
		if (diff & 0x80)
			cpustate.PC -= (diff ^ 0xff) + 0x01;
		else
			cpustate.PC += diff;
	}
};

static void op_beq(void) /* page 41 MOS */
{
	byte diff = memload(cpustate.PC++);
	if (cpustate.Z) {
		if (diff & 0x80)
			cpustate.PC -= (diff ^ 0xff) + 0x01;
		else
			cpustate.PC += diff;
	}
};

static void op_bne(void) /* page 41 MOS */
{
	byte diff = memload(cpustate.PC++);
	if (cpustate.Z == 0) {
		if (diff & 0x80)
			cpustate.PC -= (diff ^ 0xff) + 0x01;
		else
			cpustate.PC += diff;
	}
};

static void op_bvs(void) /* page 41 MOS */
{
	byte diff = memload(cpustate.PC++);
	if (cpustate.V) {
		if (diff & 0x80)
			cpustate.PC -= (diff ^ 0xff) + 0x01;
		else
			cpustate.PC += diff;
	}
};

static void op_bvc(void) /* page 41 MOS */
{
	byte diff = memload(cpustate.PC++);
	if (cpustate.V == 0) {
		if (diff & 0x80)
			cpustate.PC -= (diff ^ 0xff) + 0x01;
		else
			cpustate.PC += diff;
	}
};

static void op_cmp(void) /* page 45 MOS */
{
	byte mem = memload(cpustate.PC++);
	byte diff = cpustate.A - mem;
	cpustate.C = cpustate.A >= mem;
	cpustate.N = (diff & 0x80) != 0x00;
	cpustate.Z = diff == 0x00;
};

static void op_cmp_zero(void) /* page 45 MOS */
{
	addr memaddr = get_address_zero();
	byte mem = memload(memaddr);
	byte diff = cpustate.A - mem;
	cpustate.C = cpustate.A >= mem;
	cpustate.N = (diff & 0x80) != 0x00;
	cpustate.Z = diff == 0x00;
};

static void op_cmp_zero_x(void) /* page 45 MOS */
{
	addr memaddr = get_address_zero_x();
	byte mem = memload(memaddr);
	byte diff = cpustate.A - mem;
	cpustate.C = cpustate.A >= mem;
	cpustate.N = (diff & 0x80) != 0x00;
	cpustate.Z = diff == 0x00;
};

static void op_cmp_absolute(void) /* page 45 MOS */
{
	addr memaddr = get_address_absolute();
	byte mem = memload(memaddr);
	byte diff = cpustate.A - mem;
	cpustate.C = cpustate.A >= mem;
	cpustate.N = (diff & 0x80) != 0x00;
	cpustate.Z = diff == 0x00;
};

static void op_cmp_absolute_x(void) /* page 45 MOS */
{
	addr memaddr = get_address_absolute_x();
	byte mem = memload(memaddr);
	byte diff = cpustate.A - mem;
	cpustate.C = cpustate.A >= mem;
	cpustate.N = (diff & 0x80) != 0x00;
	cpustate.Z = diff == 0x00;
};

static void op_cmp_absolute_y(void) /* page 45 MOS */
{
	addr memaddr = get_address_absolute_y();
	byte mem = memload(memaddr);
	byte diff = cpustate.A - mem;
	cpustate.C = cpustate.A >= mem;
	cpustate.N = (diff & 0x80) != 0x00;
	cpustate.Z = diff == 0x00;
};

static void op_cmp_indirect_x(void) /* page 45 MOS */
{
	addr memaddr = get_address_indirect_x();
	byte mem = memload(memaddr);
	byte diff = cpustate.A - mem;
	cpustate.C = cpustate.A >= mem;
	cpustate.N = (diff & 0x80) != 0x00;
	cpustate.Z = diff == 0x00;
};

static void op_cmp_indirect_y(void) /* page 45 MOS */
{
	addr memaddr = get_address_indirect_y();
	byte mem = memload(memaddr);
	byte diff = cpustate.A - mem;
	cpustate.C = cpustate.A >= mem;
	cpustate.N = (diff & 0x80) != 0x00;
	cpustate.Z = diff == 0x00;
};

static void op_bit_zero(void) /* page 47 MOS */
{
	addr memaddr = get_address_zero();
	byte value = memload(memaddr);
	byte and = cpustate.A & value;
	cpustate.Z = and == 0x00;
	cpustate.N = (and & 0x80) != 0x00;
	cpustate.V = (and & 0x60) != 0x00;
};

static void op_bit_absolute(void) /* page 47 MOS */
{
	addr memaddr = get_address_absolute();
	byte value = memload(memaddr);
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
	cpustate.X = memload(cpustate.PC++);
	cpustate.N = (cpustate.X & 0x80) != 0;
	cpustate.Z = (cpustate.X == 0x00);
};

static void op_ldx_absolute(void) /* page 96 MOS */
{
	addr memaddr = get_address_absolute();
	cpustate.X = memload(memaddr);
	cpustate.N = (cpustate.X & 0x80) != 0;
	cpustate.Z = (cpustate.X == 0x00);
};

static void op_ldx_zero(void) /* page 96 MOS */
{
	addr memaddr = get_address_zero();
	cpustate.X = memload(memaddr);
	cpustate.N = (cpustate.X & 0x80) != 0;
	cpustate.Z = (cpustate.X == 0x00);
};

static void op_ldx_absolute_y(void) /* page 96 MOS */
{
	addr memaddr = get_address_absolute_y();
	cpustate.X = memload(memaddr);
	cpustate.N = (cpustate.X & 0x80) != 0;
	cpustate.Z = (cpustate.X == 0x00);
};

static void op_ldx_zero_y(void) /* page 96 MOS */
{
	addr memaddr = get_address_zero_y();
	cpustate.X = memload(memaddr);
	cpustate.N = (cpustate.X & 0x80) != 0;
	cpustate.Z = (cpustate.X == 0x00);
};

static void op_ldy(void) /* page 96 MOS */
{
	cpustate.Y = memload(cpustate.PC++);
	cpustate.N = (cpustate.Y & 0x80) != 0;
	cpustate.Z = (cpustate.Y == 0x00);
};

static void op_ldy_absolute(void) /* page 96 MOS */
{
	addr memaddr = get_address_absolute();
	cpustate.Y = memload(memaddr);
	cpustate.N = (cpustate.Y & 0x80) != 0;
	cpustate.Z = (cpustate.Y == 0x00);
};

static void op_ldy_zero(void) /* page 96 MOS */
{
	addr memaddr = get_address_zero();
	cpustate.Y = memload(memaddr);
	cpustate.N = (cpustate.Y & 0x80) != 0;
	cpustate.Z = (cpustate.Y == 0x00);
};

static void op_ldy_absolute_x(void) /* page 96 MOS */
{
	addr memaddr = get_address_absolute_x();
	cpustate.Y = memload(memaddr);
	cpustate.N = (cpustate.Y & 0x80) != 0;
	cpustate.Z = (cpustate.Y == 0x00);
};

static void op_ldy_zero_x(void) /* page 96 MOS */
{
	addr memaddr = get_address_zero_x();
	cpustate.Y = memload(memaddr);
	cpustate.N = (cpustate.Y & 0x80) != 0;
	cpustate.Z = (cpustate.Y == 0x00);
};

static void op_stx_absolute(void) /* page 97 MOS */
{
	addr memaddr = get_address_absolute();
	memstore(memaddr, cpustate.X);
};

static void op_stx_zero(void) /* page 97 MOS */
{
	addr memaddr = get_address_zero();
	memstore(memaddr, cpustate.X);
};

static void op_stx_zero_y(void) /* page 97 MOS */
{
	addr memaddr = get_address_zero_y();
	memstore(memaddr, cpustate.X);
};

static void op_sty_absolute(void) /* page 97 MOS */
{
	addr memaddr = get_address_absolute();
	memstore(memaddr, cpustate.Y);
};

static void op_sty_zero(void) /* page 97 MOS */
{
	addr memaddr = get_address_zero();
	memstore(memaddr, cpustate.Y);
};

static void op_sty_zero_x(void) /* page 97 MOS */
{
	addr memaddr = get_address_zero_x();
	memstore(memaddr, cpustate.Y);
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
	byte value = memload(cpustate.PC++);
	byte sub = cpustate.X - value;
	cpustate.Z = sub == 0x00;
	cpustate.N = (sub & 0x80) != 0;
	cpustate.C = cpustate.X > value;
};

static void op_cpx_zero(void) /* page 99 MOS */
{
	addr memaddr = get_address_zero();
	byte value = memload(memaddr);
	byte sub = cpustate.X - value;
	cpustate.Z = sub == 0x00;
	cpustate.N = (sub & 0x80) != 0;
	cpustate.C = cpustate.X > value;
};

static void op_cpx_absolute(void) /* page 99 MOS */
{
	addr memaddr = get_address_absolute();
	byte value = memload(memaddr);
	byte sub = cpustate.X - value;
	cpustate.Z = sub == 0x00;
	cpustate.N = (sub & 0x80) != 0;
	cpustate.C = cpustate.X > value;
};

static void op_cpy(void) /* page 99 MOS */
{
	byte value = memload(cpustate.PC++);
	byte sub = cpustate.Y - value;
	cpustate.Z = sub == 0x00;
	cpustate.N = (sub & 0x80) != 0;
	cpustate.C = cpustate.Y > value;
};

static void op_cpy_zero(void) /* page 99 MOS */
{
	addr memaddr = get_address_zero();
	byte value = memload(memaddr);
	byte sub = cpustate.Y - value;
	cpustate.Z = sub == 0x00;
	cpustate.N = (sub & 0x80) != 0;
	cpustate.C = cpustate.Y > value;
};

static void op_cpy_absolute(void) /* page 99 MOS */
{
	addr memaddr = get_address_absolute();
	byte value = memload(memaddr);
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

/*
 * Chapter 8 of MOS
 * STACK PROCESSING
 */
static void op_jsr(void) /* page 106 MOS */
{
	union {
		byte b[2];
		addr offset;
	} destination, source;
	destination.b[0] = memload(cpustate.PC++);
	destination.b[1] = memload(cpustate.PC++);
	source.offset = cpustate.PC;
	memstore(0x0100 + cpustate.SP--, source.b[0]);
	memstore(0x0100 + cpustate.SP--, source.b[1]);
	cpustate.PC = destination.offset;
};

static void op_rts(void) /* page 108 MOS */
{
	union {
		byte b[2];
		addr offset;
	} source;
	source.b[1] = memload(0x0100 + ++cpustate.SP);
	source.b[0] = memload(0x0100 + ++cpustate.SP);
	cpustate.PC = source.offset;
};

static void op_pha(void) /* page 117 MOS */
{
	memstore(0x0100 + cpustate.SP--, cpustate.A);
};

static void op_pla(void) /* page 118 MOS */
{
	cpustate.A = memload(0x0100 + ++cpustate.SP);
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_txs(void) /* page 120 MOS */
{
	memstore(0x0100 + cpustate.SP--, cpustate.X);
};

static void op_tsx(void) /* page 122 MOS */
{
	cpustate.X = memload(0x0100 + ++cpustate.SP);
	cpustate.Z = cpustate.X == 0x00;
	cpustate.N = (cpustate.X & 0x80) != 0x00;
};

static void op_php(void) /* page 122 MOS */
{
	memstore(0x0100 + cpustate.SP--, cpustate.P);
};

static void op_plp(void) /* page 123 MOS */
{
	cpustate.P = memload(0x0100 + ++cpustate.SP);
};

static void op_rti(void) /* page 132 MOS */
{
	cpustate.NMI = 0;
	union {
		byte b[2];
		addr full;
	} oldaddress;
	oldaddress.b[1] = memload(0x0100 + ++cpustate.SP);
	oldaddress.b[0] = memload(0x0100 + ++cpustate.SP);
	cpustate.P = memload(0x0100 + ++cpustate.SP);
	cpustate.PC = oldaddress.full;
};

static void op_brk(void) /* page 144 MOS */
{
	/* interrupt to vector IRQ address */
};

/*
 * Chapter 10
 * SHIFT AND MEMORY MODIFY INSTRUCTIONS
 */
static void op_lsr(void) /* page 148 MOS */
{
	cpustate.C = cpustate.A & 0x01;
	cpustate.A >>= 1;
	cpustate.N = 0;
	cpustate.Z = cpustate.A != 0x00;
};

static void op_lsr_zero(void) /* page 148 MOS */
{
	addr memaddr = get_address_zero();
	byte value = memload(memaddr);
	cpustate.C = value & 0x01;
	value >>= 1;
	cpustate.N = 0;
	cpustate.Z = value != 0x00;
	memstore(memaddr, value);
};

static void op_lsr_zero_x(void) /* page 148 MOS */
{
	addr memaddr = get_address_zero_x();
	byte value = memload(memaddr);
	cpustate.C = value & 0x01;
	value >>= 1;
	cpustate.N = 0;
	cpustate.Z = value != 0x00;
	memstore(memaddr, value);
};

static void op_lsr_absolute(void) /* page 148 MOS */
{
	addr memaddr = get_address_absolute();
	byte value = memload(memaddr);
	value >>= 1;
	cpustate.C = value & 0x01;
	cpustate.N = 0;
	cpustate.Z = value != 0x00;
	memstore(memaddr, value);
};

static void op_lsr_absolute_x(void) /* page 148 MOS */
{
	addr memaddr = get_address_absolute_x();
	byte value = memload(memaddr);
	value >>= 1;
	cpustate.C = value & 0x01;
	cpustate.N = 0;
	cpustate.Z = value != 0x00;
	memstore(memaddr, value);
};

static void op_asl(void) /* page 149 MOS */
{
	cpustate.C = (cpustate.A & 0x80) != 0x00;
	cpustate.A <<= 1;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
	cpustate.Z = cpustate.A != 0x00;
};

static void op_asl_zero(void) /* page 149 MOS */
{
	addr memaddr = get_address_zero();
	byte value = memload(memaddr);
	cpustate.C = (value & 0x80) != 0x00;
	value <<= 1;
	cpustate.N = (value & 0x80) != 0x00;
	cpustate.Z = value != 0x00;
	memstore(memaddr, value);
};

static void op_asl_zero_x(void) /* page 149 MOS */
{
	addr memaddr = get_address_zero_x();
	byte value = memload(memaddr);
	cpustate.C = (value & 0x80) != 0x00;
	value <<= 1;
	cpustate.N = (value & 0x80) != 0x00;
	cpustate.Z = value != 0x00;
	memstore(memaddr, value);
};

static void op_asl_absolute(void) /* page 149 MOS */
{
	addr memaddr = get_address_absolute();
	byte value = memload(memaddr);
	cpustate.C = (value & 0x80) != 0x00;
	value <<= 1;
	cpustate.N = (value & 0x80) != 0x00;
	cpustate.Z = value != 0x00;
	memstore(memaddr, value);
};

static void op_asl_absolute_x(void) /* page 149 MOS */
{
	addr memaddr = get_address_absolute_x();
	byte value = memload(memaddr);
	cpustate.C = (value & 0x80) != 0x00;
	value <<= 1;
	cpustate.N = (value & 0x80) != 0x00;
	cpustate.Z = value != 0x00;
	memstore(memaddr, value);
};

static void op_rol(void) /* page 149 MOS */
{
	byte oldc = cpustate.C;
	cpustate.C = (cpustate.A & 0x80) != 0x00;
	cpustate.A <<= 1;
	cpustate.A += oldc;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
	cpustate.Z = cpustate.A != 0x00;
};

static void op_rol_zero(void) /* page 149 MOS */
{
	addr memaddr = get_address_zero();
	byte value = memload(memaddr);
	byte oldc = cpustate.C;
	cpustate.C = (value & 0x80) != 0x00;
	value <<= 1;
	value += oldc;
	cpustate.N = (value & 0x80) != 0x00;
	cpustate.Z = value != 0x00;
	memstore(memaddr, value);
};

static void op_rol_zero_x(void) /* page 149 MOS */
{
	addr memaddr = get_address_zero_x();
	byte value = memload(memaddr);
	byte oldc = cpustate.C;
	cpustate.C = (value & 0x80) != 0x00;
	value <<= 1;
	value += oldc;
	cpustate.N = (value & 0x80) != 0x00;
	cpustate.Z = value != 0x00;
	memstore(memaddr, value);
};

static void op_rol_absolute(void) /* page 149 MOS */
{
	addr memaddr = get_address_absolute();
	byte value = memload(memaddr);
	byte oldc = cpustate.C;
	cpustate.C = (value & 0x80) != 0x00;
	value <<= 1;
	value += oldc;
	cpustate.N = (value & 0x80) != 0x00;
	cpustate.Z = value != 0x00;
	memstore(memaddr, value);
};

static void op_rol_absolute_x(void) /* page 149 MOS */
{
	addr memaddr = get_address_absolute_x();
	byte value = memload(memaddr);
	byte oldc = cpustate.C;
	cpustate.C = (value & 0x80) != 0x00;
	value <<= 1;
	value += oldc;
	cpustate.N = (value & 0x80) != 0x00;
	cpustate.Z = value != 0x00;
	memstore(memaddr, value);
};

static void op_ror(void) /* page 150 MOS */
{
	byte oldc = cpustate.C;
	cpustate.C = cpustate.A & 0x01;
	cpustate.A >>= 1;
	cpustate.A += 0x80 & oldc ;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
	cpustate.Z = cpustate.A != 0x00;
};

static void op_ror_zero(void) /* page 149 MOS */
{
	addr memaddr = get_address_zero();
	byte value = memload(memaddr);
	byte oldc = cpustate.C;
	cpustate.C = value & 0x01;
	value >>= 1;
	value += 0x80 & oldc;
	cpustate.N = (value & 0x80) != 0x00;
	cpustate.Z = value != 0x00;
	memstore(memaddr, value);
};

static void op_ror_zero_x(void) /* page 149 MOS */
{
	addr memaddr = get_address_zero_x();
	byte value = memload(memaddr);
	byte oldc = cpustate.C;
	cpustate.C = value & 0x01;
	value >>= 1;
	value += 0x80 & oldc;
	cpustate.N = (value & 0x80) != 0x00;
	cpustate.Z = value != 0x00;
	memstore(memaddr, value);
};

static void op_ror_absolute(void) /* page 149 MOS */
{
	addr memaddr = get_address_absolute();
	byte value = memload(memaddr);
	byte oldc = cpustate.C;
	cpustate.C = value & 0x01;
	value >>= 1;
	value += 0x80 & oldc;
	cpustate.N = (value & 0x80) != 0x00;
	cpustate.Z = value != 0x00;
	memstore(memaddr, value);
};

static void op_ror_absolute_x(void) /* page 149 MOS */
{
	addr memaddr = get_address_absolute_x();
	byte value = memload(memaddr);
	byte oldc = cpustate.C;
	cpustate.C = value & 0x01;
	value >>= 1;
	value += 0x80 & oldc;
	cpustate.N = (value & 0x80) != 0x00;
	cpustate.Z = value != 0x00;
	memstore(memaddr, value);
};

static void op_inc_absolute(void) /* page 155 MOS */
{
	addr memaddr = get_address_absolute();
	byte value = memload(memaddr) + 1;
	cpustate.Z = value == 0x00;
	cpustate.N = (value & 0x80) != 0x00;
	memstore(memaddr, value);
};

static void op_inc_absolute_x(void) /* page 155 MOS */
{
	addr memaddr = get_address_absolute_x();
	byte value = memload(memaddr) + 1;
	cpustate.Z = value == 0x00;
	cpustate.N = (value & 0x80) != 0x00;
	memstore(memaddr, value);
};

static void op_inc_zero(void) /* page 155 MOS */
{
	addr memaddr = get_address_zero();
	byte value = memload(memaddr) + 1;
	cpustate.Z = value == 0x00;
	cpustate.N = (value & 0x80) != 0x00;
	memstore(memaddr, value);
};

static void op_inc_zero_x(void) /* page 155 MOS */
{
	addr memaddr = get_address_zero_x();
	byte value = memload(memaddr) + 1;
	cpustate.Z = value == 0x00;
	cpustate.N = (value & 0x80) != 0x00;
	memstore(memaddr, value);
};

static void op_dec_absolute(void) /* page 155 MOS */
{
	addr memaddr = get_address_absolute();
	byte value = memload(memaddr) - 1;
	cpustate.Z = value == 0x00;
	cpustate.N = (value & 0x80) != 0x00;
	memstore(memaddr, value);
};

static void op_dec_absolute_x(void) /* page 155 MOS */
{
	addr memaddr = get_address_absolute_x();
	byte value = memload(memaddr) - 1;
	cpustate.Z = value == 0x00;
	cpustate.N = (value & 0x80) != 0x00;
	memstore(memaddr, value);
};

static void op_dec_zero(void) /* page 155 MOS */
{
	addr memaddr = get_address_zero();
	byte value = memload(memaddr) - 1;
	cpustate.Z = value == 0x00;
	cpustate.N = (value & 0x80) != 0x00;
	memstore(memaddr, value);
};

static void op_dec_zero_x(void) /* page 155 MOS */
{
	addr memaddr = get_address_zero_x();
	byte value = memload(memaddr) - 1;
	cpustate.Z = value == 0x00;
	cpustate.N = (value & 0x80) != 0x00;
	memstore(memaddr, value);
};

#define NUL NULL

opfunct addressing_map[] = {
       /* 0   1    2    3    4    5    6    7    8    9    a    b    c    d    e    f  */
/* 0 */ imp, inx, NUL, NUL, NUL, zer, zer, NUL, imp, imp, imp, NUL, NUL, abs, abs, NUL,
/* 1 */
/* 2 */
/* 3 */
/* 4 */
/* 5 */
/* 6 */
/* 7 */
/* 8 */
/* 9 */
/* a */
/* b */
/* c */
/* d */
/* e */
/* f */
};

opfunct instruction_map[] = {
       /* 0   1    2    3    4    5    6    7    8    9    a    b    c    d    e    f  */
/* 0 */ brk, ora, NUL, NUL, NUL, ora, asl, NUL, php, ora, asl, NUL, NUL, ora, asl, NUL,
/* 1 */
/* 2 */
/* 3 */
/* 4 */
/* 5 */
/* 6 */
/* 7 */
/* 8 */
/* 9 */
/* a */
/* b */
/* c */
/* d */
/* e */
/* f */
};

void print_cpustate()
{
	byte off;

	flockfile(stdout);

	printf("PC: 0x%02x SP: 0x%02x A: 0x%02x X: 0x%02x Y: 0x%02x ",
			cpustate.PC, cpustate.SP, cpustate.A,
			cpustate.X, cpustate.Y);
	printf("CZIDBVN: %d%d%d%d%d%d%db ",
			cpustate.C, cpustate.Z, cpustate.I,
			cpustate.D, cpustate.B, cpustate.V,
			cpustate.N);
	printf("OP: %02x\n", memload(cpustate.PC));

	if (cpustate.SP != 0xff) {
		printf("Stack: ");
		for (off = 0xff; off > cpustate.SP; off--) {
			printf("%02x ", memload(0x0100 + off));
		}
	}
	printf("\n");


	funlockfile(stdout);
};

static void cpu_boot()
{
	printf("Booting CPU...\n");
	printf("Starting memory...\n");
	memset(memory, 0, sizeof(memory));

	printf("Starting CPU state...\n");
	memset(&cpustate, 0, sizeof(cpustate));

	printf("Starting stack...\n");
	cpustate.SP = 0xff;
}

void cpu_init()
{
	cpu_boot();
};

void cpu_load(byte *prg, size_t size)
{
	union {
		byte b[2];
		addr full;
	} pcaddress;
	memcpy(prgmem, prg, size);
	pcaddress.b[0] = memload(0xfffc);
	pcaddress.b[1] = memload(0xfffd);
	cpustate.PC = pcaddress.full;
};

void check_interrupts()
{
	if (cpustate.NMI) {
		cpustate.NMI = 0;
		union {
			byte b[2];
			addr full;
		} pcaddress, source;
		pcaddress.b[0] = memload(0xfffa);
		pcaddress.b[1] = memload(0xfffb);
		source.full = cpustate.PC;
		memstore(0x0100 + cpustate.SP--, cpustate.P);
		memstore(0x0100 + cpustate.SP--, source.b[0]);
		memstore(0x0100 + cpustate.SP--, source.b[1]);
		cpustate.PC = pcaddress.full;
	}
};

void cpu_run()
{
	byte op;
	opfunct addressing, instruction;

	printf("Running program at: 0x%04x\n", cpustate.PC);

	do {
		print_cpustate();
		/* fetch */
		op = memload(cpustate.PC);

		/* decode */
		addressing = addressing_map[op];
		instruction = instruction_map[op];

		if (func == NULL) {
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
