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
 * Chapter 6 os MOS
 * INDEX REGISTERS AND INDEX ADDRESSING CONCEPTS
 */
static inline addr get_address_zero()
{
	union {
		byte b[2];
		addr full;
	} address;
	address.b[0] = cpu_memload(cpustate.PC++);
	address.b[1] = 0x00;
	return address.full;
};

static inline addr get_address_zero_x()
{
	union {
		byte b[2];
		addr full;
	} address;
	address.b[0] = cpu_memload(cpustate.PC++) + cpustate.X;
	address.b[1] = 0x00;
	return address.full;
};

static inline addr get_address_zero_y()
{
	union {
		byte b[2];
		addr full;
	} address;
	address.b[0] = cpu_memload(cpustate.PC++) + cpustate.Y;
	address.b[1] = 0x00;
	return address.full;
};

static inline addr get_address_absolute()
{
	union {
		byte b[2];
		addr full;
	} address;
	address.b[0] = cpu_memload(cpustate.PC++);
	address.b[1] = cpu_memload(cpustate.PC++);
	return address.full;
};

static inline addr get_address_absolute_x()
{
	union {
		byte b[2];
		addr full;
	} address;
	address.b[0] = cpu_memload(cpustate.PC++);
	address.b[1] = cpu_memload(cpustate.PC++);
	return address.full + (addr) cpustate.X;
};

static inline addr get_address_absolute_y()
{
	union {
		byte b[2];
		addr full;
	} address;
	address.b[0] = cpu_memload(cpustate.PC++);
	address.b[1] = cpu_memload(cpustate.PC++);
	return address.full + (addr) cpustate.Y;
};

static inline addr get_address_indirect_x()
{
	union {
		byte b[2];
		addr full;
	} address;
	byte baseaddress = cpu_memload(cpustate.PC++) + cpustate.X;
	address.b[0] = cpu_memload((addr)baseaddress);
	address.b[1] = 0x00;
	return address.full;
};

static inline addr get_address_indirect_y()
{
	union {
		byte b[2];
		addr full;
	} address;
	byte baseaddress = cpu_memload(cpustate.PC++);
	address.b[0] = cpu_memload((addr)baseaddress);
	address.b[1] = cpu_memload(((addr)baseaddress) + 1);;
	return address.full + (addr) cpustate.Y;
};

/*
 * Chapter 2 of MOS
 * THE DATA BUS, ACCUMULATOR AND ARITHMETIC UNIT
 */
static void op_lda(void) /* page 4 MOS */
{
	cpustate.A = cpu_memload(cpustate.PC++);
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_lda_absolute(void) /* page 4 MOS */
{
	addr memaddr = get_address_absolute();
	cpustate.A = cpu_memload(memaddr);
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_lda_absolute_x(void) /* page 4 MOS */
{
	addr memaddr = get_address_absolute_x();
	cpustate.A = cpu_memload(memaddr);
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_lda_absolute_y(void) /* page 4 MOS */
{
	addr memaddr = get_address_absolute_y();
	cpustate.A = cpu_memload(memaddr);
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_lda_zero(void) /* page 4 MOS */
{
	addr memaddr = get_address_zero();
	cpustate.A = cpu_memload(memaddr);
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_lda_zero_x(void) /* page 4 MOS */
{
	addr memaddr = get_address_zero_x();
	cpustate.A = cpu_memload(memaddr);
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_lda_indirect_x(void) /* page 4 MOS */
{
	addr memaddr = get_address_indirect_x();
	cpustate.A = cpu_memload(memaddr);
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_lda_indirect_y(void) /* page 4 MOS */
{
	addr memaddr = get_address_indirect_y();
	cpustate.A = cpu_memload(memaddr);
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_sta_absolute(void) /* page 5 MOS */
{
	addr memaddr = get_address_absolute();
	cpu_memstore(memaddr, cpustate.A);
};

static void op_sta_absolute_x(void) /* page 5 MOS */
{
	addr memaddr = get_address_absolute_x();
	cpu_memstore(memaddr, cpustate.A);
};

static void op_sta_absolute_y(void) /* page 5 MOS */
{
	addr memaddr = get_address_absolute_y();
	cpu_memstore(memaddr, cpustate.A);
};

static void op_sta_zero(void) /* page 5 MOS */
{
	addr memaddr = get_address_zero();
	cpu_memstore(memaddr, cpustate.A);
};

static void op_sta_zero_x(void) /* page 5 MOS */
{
	addr memaddr = get_address_zero_x();
	cpu_memstore(memaddr, cpustate.A);
};

static void op_sta_indirect_x(void) /* page 5 MOS */
{
	addr memaddr = get_address_indirect_x();
	cpu_memstore(memaddr, cpustate.A);
};

static void op_sta_indirect_y(void) /* page 5 MOS */
{
	addr memaddr = get_address_indirect_y();
	cpu_memstore(memaddr, cpustate.A);
};

static void op_adc(void) /* page 7 MOS */
{
	byte value = cpu_memload(cpustate.PC++);
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
	byte value = cpu_memload(memaddr);
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
	byte value = cpu_memload(memaddr);
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
	byte value = cpu_memload(memaddr);
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
	byte value = cpu_memload(memaddr);
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
	byte value = cpu_memload(memaddr);
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
	byte value = cpu_memload(memaddr);
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
	byte value = cpu_memload(memaddr);
	uint16_t sum = (uint16_t) cpustate.A + (uint16_t) value + cpustate.C;
	cpustate.V = (((cpustate.A ^ value) & 0x80) == 0x00) && ((sum & 0x80) != (value & 0x80));
	cpustate.A = sum;
	cpustate.C = (sum & 0x0100) != 0x0000;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_sbc(void) /* page 14 MOS */
{
	byte value = cpu_memload(cpustate.PC++);
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
	byte value = cpu_memload(memaddr);
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
	byte value = cpu_memload(memaddr);
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
	byte value = cpu_memload(memaddr);
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
	byte value = cpu_memload(memaddr);
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
	byte value = cpu_memload(memaddr);
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
	byte value = cpu_memload(memaddr);
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
	byte value = cpu_memload(memaddr);
	uint16_t sum = (uint16_t) cpustate.A - (uint16_t) value - (1 - cpustate.C);
	cpustate.V = (((cpustate.A ^ value) & 0x80) == 0x00) && ((sum & 0x80) != (value & 0x80));
	cpustate.A = sum;
	cpustate.C = (sum & 0x0100) != 0x0000;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_and(void) /* page 20 MOS */
{
	byte value = cpu_memload(cpustate.PC++);
	cpustate.A &= value;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_and_absolute(void) /* page 20 MOS */
{
	addr memaddr = get_address_absolute();
	byte value = cpu_memload(memaddr);
	cpustate.A &= value;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_and_absolute_x(void) /* page 20 MOS */
{
	addr memaddr = get_address_absolute_x();
	byte value = cpu_memload(memaddr);
	cpustate.A &= value;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_and_absolute_y(void) /* page 20 MOS */
{
	addr memaddr = get_address_absolute_y();
	byte value = cpu_memload(memaddr);
	cpustate.A &= value;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_and_zero(void) /* page 20 MOS */
{
	addr memaddr = get_address_zero();
	byte value = cpu_memload(memaddr);
	cpustate.A &= value;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_and_zero_x(void) /* page 20 MOS */
{
	addr memaddr = get_address_zero_x();
	byte value = cpu_memload(memaddr);
	cpustate.A &= value;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_and_indirect_x(void) /* page 20 MOS */
{
	addr memaddr = get_address_indirect_x();
	byte value = cpu_memload(memaddr);
	cpustate.A &= value;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_and_indirect_y(void) /* page 20 MOS */
{
	addr memaddr = get_address_indirect_y();
	byte value = cpu_memload(memaddr);
	cpustate.A &= value;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_ora(void) /* page 21 MOS */
{
	byte value = cpu_memload(cpustate.PC++);
	cpustate.A |= value;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_ora_absolute(void) /* page 21 MOS */
{
	addr memaddr = get_address_absolute();
	byte value = cpu_memload(memaddr);
	cpustate.A |= value;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_ora_absolute_x(void) /* page 21 MOS */
{
	addr memaddr = get_address_absolute_x();
	byte value = cpu_memload(memaddr);
	cpustate.A |= value;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_ora_absolute_y(void) /* page 21 MOS */
{
	addr memaddr = get_address_absolute_y();
	byte value = cpu_memload(memaddr);
	cpustate.A |= value;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_ora_zero(void) /* page 21 MOS */
{
	addr memaddr = get_address_zero();
	byte value = cpu_memload(memaddr);
	cpustate.A |= value;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_ora_zero_x(void) /* page 21 MOS */
{
	addr memaddr = get_address_zero_x();
	byte value = cpu_memload(memaddr);
	cpustate.A |= value;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_ora_indirect_x(void) /* page 21 MOS */
{
	addr memaddr = get_address_indirect_x();
	byte value = cpu_memload(memaddr);
	cpustate.A |= value;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_ora_indirect_y(void) /* page 21 MOS */
{
	addr memaddr = get_address_indirect_y();
	byte value = cpu_memload(memaddr);
	cpustate.A |= value;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_eor(void) /* page 21 MOS */
{
	byte value = cpu_memload(cpustate.PC++);
	cpustate.A ^= value;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_eor_absolute(void) /* page 21 MOS */
{
	addr memaddr = get_address_absolute();
	byte value = cpu_memload(memaddr);
	cpustate.A ^= value;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_eor_absolute_x(void) /* page 21 MOS */
{
	addr memaddr = get_address_absolute_x();
	byte value = cpu_memload(memaddr);
	cpustate.A ^= value;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_eor_absolute_y(void) /* page 21 MOS */
{
	addr memaddr = get_address_absolute_y();
	byte value = cpu_memload(memaddr);
	cpustate.A ^= value;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_eor_zero(void) /* page 21 MOS */
{
	addr memaddr = get_address_zero();
	byte value = cpu_memload(memaddr);
	cpustate.A ^= value;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_eor_zero_x(void) /* page 21 MOS */
{
	addr memaddr = get_address_zero_x();
	byte value = cpu_memload(memaddr);
	cpustate.A ^= value;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_eor_indirect_x(void) /* page 21 MOS */
{
	addr memaddr = get_address_indirect_x();
	byte value = cpu_memload(memaddr);
	cpustate.A ^= value;
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_eor_indirect_y(void) /* page 21 MOS */
{
	addr memaddr = get_address_indirect_y();
	byte value = cpu_memload(memaddr);
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
	addr memaddr = get_address_zero();
	byte mem = cpu_memload(memaddr);
	byte diff = cpustate.A - mem;
	cpustate.C = cpustate.A >= mem;
	cpustate.N = (diff & 0x80) != 0x00;
	cpustate.Z = diff == 0x00;
};

static void op_cmp_zero_x(void) /* page 45 MOS */
{
	addr memaddr = get_address_zero_x();
	byte mem = cpu_memload(memaddr);
	byte diff = cpustate.A - mem;
	cpustate.C = cpustate.A >= mem;
	cpustate.N = (diff & 0x80) != 0x00;
	cpustate.Z = diff == 0x00;
};

static void op_cmp_absolute(void) /* page 45 MOS */
{
	addr memaddr = get_address_absolute();
	byte mem = cpu_memload(memaddr);
	byte diff = cpustate.A - mem;
	cpustate.C = cpustate.A >= mem;
	cpustate.N = (diff & 0x80) != 0x00;
	cpustate.Z = diff == 0x00;
};

static void op_cmp_absolute_x(void) /* page 45 MOS */
{
	addr memaddr = get_address_absolute_x();
	byte mem = cpu_memload(memaddr);
	byte diff = cpustate.A - mem;
	cpustate.C = cpustate.A >= mem;
	cpustate.N = (diff & 0x80) != 0x00;
	cpustate.Z = diff == 0x00;
};

static void op_cmp_absolute_y(void) /* page 45 MOS */
{
	addr memaddr = get_address_absolute_y();
	byte mem = cpu_memload(memaddr);
	byte diff = cpustate.A - mem;
	cpustate.C = cpustate.A >= mem;
	cpustate.N = (diff & 0x80) != 0x00;
	cpustate.Z = diff == 0x00;
};

static void op_cmp_indirect_x(void) /* page 45 MOS */
{
	addr memaddr = get_address_indirect_x();
	byte mem = cpu_memload(memaddr);
	byte diff = cpustate.A - mem;
	cpustate.C = cpustate.A >= mem;
	cpustate.N = (diff & 0x80) != 0x00;
	cpustate.Z = diff == 0x00;
};

static void op_cmp_indirect_y(void) /* page 45 MOS */
{
	addr memaddr = get_address_indirect_y();
	byte mem = cpu_memload(memaddr);
	byte diff = cpustate.A - mem;
	cpustate.C = cpustate.A >= mem;
	cpustate.N = (diff & 0x80) != 0x00;
	cpustate.Z = diff == 0x00;
};

static void op_bit_zero(void) /* page 47 MOS */
{
	addr memaddr = get_address_zero();
	byte value = cpu_memload(memaddr);
	byte and = cpustate.A & value;
	cpustate.Z = and == 0x00;
	cpustate.N = (and & 0x80) != 0x00;
	cpustate.V = (and & 0x60) != 0x00;
};

static void op_bit_absolute(void) /* page 47 MOS */
{
	addr memaddr = get_address_absolute();
	byte value = cpu_memload(memaddr);
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
	addr memaddr = get_address_absolute();
	cpustate.X = cpu_memload(memaddr);
	cpustate.N = (cpustate.X & 0x80) != 0;
	cpustate.Z = (cpustate.X == 0x00);
};

static void op_ldx_zero(void) /* page 96 MOS */
{
	addr memaddr = get_address_zero();
	cpustate.X = cpu_memload(memaddr);
	cpustate.N = (cpustate.X & 0x80) != 0;
	cpustate.Z = (cpustate.X == 0x00);
};

static void op_ldx_absolute_y(void) /* page 96 MOS */
{
	addr memaddr = get_address_absolute_y();
	cpustate.X = cpu_memload(memaddr);
	cpustate.N = (cpustate.X & 0x80) != 0;
	cpustate.Z = (cpustate.X == 0x00);
};

static void op_ldx_zero_y(void) /* page 96 MOS */
{
	addr memaddr = get_address_zero_y();
	cpustate.X = cpu_memload(memaddr);
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
	addr memaddr = get_address_absolute();
	cpustate.Y = cpu_memload(memaddr);
	cpustate.N = (cpustate.Y & 0x80) != 0;
	cpustate.Z = (cpustate.Y == 0x00);
};

static void op_ldy_zero(void) /* page 96 MOS */
{
	addr memaddr = get_address_zero();
	cpustate.Y = cpu_memload(memaddr);
	cpustate.N = (cpustate.Y & 0x80) != 0;
	cpustate.Z = (cpustate.Y == 0x00);
};

static void op_ldy_absolute_x(void) /* page 96 MOS */
{
	addr memaddr = get_address_absolute_x();
	cpustate.Y = cpu_memload(memaddr);
	cpustate.N = (cpustate.Y & 0x80) != 0;
	cpustate.Z = (cpustate.Y == 0x00);
};

static void op_ldy_zero_x(void) /* page 96 MOS */
{
	addr memaddr = get_address_zero_x();
	cpustate.Y = cpu_memload(memaddr);
	cpustate.N = (cpustate.Y & 0x80) != 0;
	cpustate.Z = (cpustate.Y == 0x00);
};

static void op_stx_absolute(void) /* page 97 MOS */
{
	addr memaddr = get_address_absolute();
	cpu_memstore(memaddr, cpustate.X);
};

static void op_stx_zero(void) /* page 97 MOS */
{
	addr memaddr = get_address_zero();
	cpu_memstore(memaddr, cpustate.X);
};

static void op_stx_zero_y(void) /* page 97 MOS */
{
	addr memaddr = get_address_zero_y();
	cpu_memstore(memaddr, cpustate.X);
};

static void op_sty_absolute(void) /* page 97 MOS */
{
	addr memaddr = get_address_absolute();
	cpu_memstore(memaddr, cpustate.Y);
};

static void op_sty_zero(void) /* page 97 MOS */
{
	addr memaddr = get_address_zero();
	cpu_memstore(memaddr, cpustate.Y);
};

static void op_sty_zero_x(void) /* page 97 MOS */
{
	addr memaddr = get_address_zero_x();
	cpu_memstore(memaddr, cpustate.Y);
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
	addr memaddr = get_address_zero();
	byte value = cpu_memload(memaddr);
	byte sub = cpustate.X - value;
	cpustate.Z = sub == 0x00;
	cpustate.N = (sub & 0x80) != 0;
	cpustate.C = cpustate.X > value;
};

static void op_cpx_absolute(void) /* page 99 MOS */
{
	addr memaddr = get_address_absolute();
	byte value = cpu_memload(memaddr);
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
	addr memaddr = get_address_zero();
	byte value = cpu_memload(memaddr);
	byte sub = cpustate.Y - value;
	cpustate.Z = sub == 0x00;
	cpustate.N = (sub & 0x80) != 0;
	cpustate.C = cpustate.Y > value;
};

static void op_cpy_absolute(void) /* page 99 MOS */
{
	addr memaddr = get_address_absolute();
	byte value = cpu_memload(memaddr);
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
	destination.b[0] = cpu_memload(cpustate.PC++);
	destination.b[1] = cpu_memload(cpustate.PC++);
	source.offset = cpustate.PC;
	cpu_memstore(0x0100 + cpustate.SP--, source.b[0]);
	cpu_memstore(0x0100 + cpustate.SP--, source.b[1]);
	cpustate.PC = destination.offset;
};

static void op_rts(void) /* page 108 MOS */
{
	union {
		byte b[2];
		addr offset;
	} source;
	source.b[1] = cpu_memload(0x0100 + ++cpustate.SP);
	source.b[0] = cpu_memload(0x0100 + ++cpustate.SP);
	cpustate.PC = source.offset;
};

static void op_pha(void) /* page 117 MOS */
{
	cpu_memstore(0x0100 + cpustate.SP--, cpustate.A);
};

static void op_pla(void) /* page 118 MOS */
{
	cpustate.A = cpu_memload(0x0100 + ++cpustate.SP);
	cpustate.Z = cpustate.A == 0x00;
	cpustate.N = (cpustate.A & 0x80) != 0x00;
};

static void op_txs(void) /* page 120 MOS */
{
	cpu_memstore(0x0100 + cpustate.SP--, cpustate.X);
};

static void op_tsx(void) /* page 122 MOS */
{
	cpustate.X = cpu_memload(0x0100 + ++cpustate.SP);
	cpustate.Z = cpustate.X == 0x00;
	cpustate.N = (cpustate.X & 0x80) != 0x00;
};

static void op_php(void) /* page 122 MOS */
{
	cpu_memstore(0x0100 + cpustate.SP--, cpustate.P);
};

static void op_plp(void) /* page 123 MOS */
{
	cpustate.P = cpu_memload(0x0100 + ++cpustate.SP);
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
	byte value = cpu_memload(memaddr);
	cpustate.C = value & 0x01;
	value >>= 1;
	cpustate.N = 0;
	cpustate.Z = value != 0x00;
	cpu_memstore(memaddr, value);
};

static void op_lsr_zero_x(void) /* page 148 MOS */
{
	addr memaddr = get_address_zero_x();
	byte value = cpu_memload(memaddr);
	cpustate.C = value & 0x01;
	value >>= 1;
	cpustate.N = 0;
	cpustate.Z = value != 0x00;
	cpu_memstore(memaddr, value);
};

static void op_lsr_absolute(void) /* page 148 MOS */
{
	addr memaddr = get_address_absolute();
	byte value = cpu_memload(memaddr);
	value >>= 1;
	cpustate.C = value & 0x01;
	cpustate.N = 0;
	cpustate.Z = value != 0x00;
	cpu_memstore(memaddr, value);
};

static void op_lsr_absolute_x(void) /* page 148 MOS */
{
	addr memaddr = get_address_absolute_x();
	byte value = cpu_memload(memaddr);
	value >>= 1;
	cpustate.C = value & 0x01;
	cpustate.N = 0;
	cpustate.Z = value != 0x00;
	cpu_memstore(memaddr, value);
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
	byte value = cpu_memload(memaddr);
	cpustate.C = (value & 0x80) != 0x00;
	value <<= 1;
	cpustate.N = (value & 0x80) != 0x00;
	cpustate.Z = value != 0x00;
	cpu_memstore(memaddr, value);
};

static void op_asl_zero_x(void) /* page 149 MOS */
{
	addr memaddr = get_address_zero_x();
	byte value = cpu_memload(memaddr);
	cpustate.C = (value & 0x80) != 0x00;
	value <<= 1;
	cpustate.N = (value & 0x80) != 0x00;
	cpustate.Z = value != 0x00;
	cpu_memstore(memaddr, value);
};

static void op_asl_absolute(void) /* page 149 MOS */
{
	addr memaddr = get_address_absolute();
	byte value = cpu_memload(memaddr);
	cpustate.C = (value & 0x80) != 0x00;
	value <<= 1;
	cpustate.N = (value & 0x80) != 0x00;
	cpustate.Z = value != 0x00;
	cpu_memstore(memaddr, value);
};

static void op_asl_absolute_x(void) /* page 149 MOS */
{
	addr memaddr = get_address_absolute_x();
	byte value = cpu_memload(memaddr);
	cpustate.C = (value & 0x80) != 0x00;
	value <<= 1;
	cpustate.N = (value & 0x80) != 0x00;
	cpustate.Z = value != 0x00;
	cpu_memstore(memaddr, value);
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
	byte value = cpu_memload(memaddr);
	byte oldc = cpustate.C;
	cpustate.C = (value & 0x80) != 0x00;
	value <<= 1;
	value += oldc;
	cpustate.N = (value & 0x80) != 0x00;
	cpustate.Z = value != 0x00;
	cpu_memstore(memaddr, value);
};

static void op_rol_zero_x(void) /* page 149 MOS */
{
	addr memaddr = get_address_zero_x();
	byte value = cpu_memload(memaddr);
	byte oldc = cpustate.C;
	cpustate.C = (value & 0x80) != 0x00;
	value <<= 1;
	value += oldc;
	cpustate.N = (value & 0x80) != 0x00;
	cpustate.Z = value != 0x00;
	cpu_memstore(memaddr, value);
};

static void op_rol_absolute(void) /* page 149 MOS */
{
	addr memaddr = get_address_absolute();
	byte value = cpu_memload(memaddr);
	byte oldc = cpustate.C;
	cpustate.C = (value & 0x80) != 0x00;
	value <<= 1;
	value += oldc;
	cpustate.N = (value & 0x80) != 0x00;
	cpustate.Z = value != 0x00;
	cpu_memstore(memaddr, value);
};

static void op_rol_absolute_x(void) /* page 149 MOS */
{
	addr memaddr = get_address_absolute_x();
	byte value = cpu_memload(memaddr);
	byte oldc = cpustate.C;
	cpustate.C = (value & 0x80) != 0x00;
	value <<= 1;
	value += oldc;
	cpustate.N = (value & 0x80) != 0x00;
	cpustate.Z = value != 0x00;
	cpu_memstore(memaddr, value);
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
	byte value = cpu_memload(memaddr);
	byte oldc = cpustate.C;
	cpustate.C = value & 0x01;
	value >>= 1;
	value += 0x80 & oldc;
	cpustate.N = (value & 0x80) != 0x00;
	cpustate.Z = value != 0x00;
	cpu_memstore(memaddr, value);
};

static void op_ror_zero_x(void) /* page 149 MOS */
{
	addr memaddr = get_address_zero_x();
	byte value = cpu_memload(memaddr);
	byte oldc = cpustate.C;
	cpustate.C = value & 0x01;
	value >>= 1;
	value += 0x80 & oldc;
	cpustate.N = (value & 0x80) != 0x00;
	cpustate.Z = value != 0x00;
	cpu_memstore(memaddr, value);
};

static void op_ror_absolute(void) /* page 149 MOS */
{
	addr memaddr = get_address_absolute();
	byte value = cpu_memload(memaddr);
	byte oldc = cpustate.C;
	cpustate.C = value & 0x01;
	value >>= 1;
	value += 0x80 & oldc;
	cpustate.N = (value & 0x80) != 0x00;
	cpustate.Z = value != 0x00;
	cpu_memstore(memaddr, value);
};

static void op_ror_absolute_x(void) /* page 149 MOS */
{
	addr memaddr = get_address_absolute_x();
	byte value = cpu_memload(memaddr);
	byte oldc = cpustate.C;
	cpustate.C = value & 0x01;
	value >>= 1;
	value += 0x80 & oldc;
	cpustate.N = (value & 0x80) != 0x00;
	cpustate.Z = value != 0x00;
	cpu_memstore(memaddr, value);
};

static void op_inc_absolute(void) /* page 155 MOS */
{
	addr memaddr = get_address_absolute();
	byte value = cpu_memload(memaddr) + 1;
	cpustate.Z = value == 0x00;
	cpustate.N = (value & 0x80) != 0x00;
	cpu_memstore(memaddr, value);
};

static void op_inc_absolute_x(void) /* page 155 MOS */
{
	addr memaddr = get_address_absolute_x();
	byte value = cpu_memload(memaddr) + 1;
	cpustate.Z = value == 0x00;
	cpustate.N = (value & 0x80) != 0x00;
	cpu_memstore(memaddr, value);
};

static void op_inc_zero(void) /* page 155 MOS */
{
	addr memaddr = get_address_zero();
	byte value = cpu_memload(memaddr) + 1;
	cpustate.Z = value == 0x00;
	cpustate.N = (value & 0x80) != 0x00;
	cpu_memstore(memaddr, value);
};

static void op_inc_zero_x(void) /* page 155 MOS */
{
	addr memaddr = get_address_zero_x();
	byte value = cpu_memload(memaddr) + 1;
	cpustate.Z = value == 0x00;
	cpustate.N = (value & 0x80) != 0x00;
	cpu_memstore(memaddr, value);
};

static void op_dec_absolute(void) /* page 155 MOS */
{
	addr memaddr = get_address_absolute();
	byte value = cpu_memload(memaddr) - 1;
	cpustate.Z = value == 0x00;
	cpustate.N = (value & 0x80) != 0x00;
	cpu_memstore(memaddr, value);
};

static void op_dec_absolute_x(void) /* page 155 MOS */
{
	addr memaddr = get_address_absolute_x();
	byte value = cpu_memload(memaddr) - 1;
	cpustate.Z = value == 0x00;
	cpustate.N = (value & 0x80) != 0x00;
	cpu_memstore(memaddr, value);
};

static void op_dec_zero(void) /* page 155 MOS */
{
	addr memaddr = get_address_zero();
	byte value = cpu_memload(memaddr) - 1;
	cpustate.Z = value == 0x00;
	cpustate.N = (value & 0x80) != 0x00;
	cpu_memstore(memaddr, value);
};

static void op_dec_zero_x(void) /* page 155 MOS */
{
	addr memaddr = get_address_zero_x();
	byte value = cpu_memload(memaddr) - 1;
	cpustate.Z = value == 0x00;
	cpustate.N = (value & 0x80) != 0x00;
	cpu_memstore(memaddr, value);
};

opfunct opcode_map[] = {
	/* 00 */ &op_brk,
	/* 01 */ &op_ora_indirect_x,
	/* 02 */ NULL,
	/* 03 */ NULL,
	/* 04 */ NULL,
	/* 05 */ &op_ora_zero,
	/* 06 */ &op_asl_zero,
	/* 07 */ NULL,
	/* 08 */ &op_php,
	/* 09 */ &op_ora,
	/* 0a */ &op_asl,
	/* 0b */ NULL,
	/* 0c */ NULL,
	/* 0d */ &op_ora_absolute,
	/* 0e */ &op_asl_absolute,
	/* 0f */ NULL,
	/* 10 */ &op_bpl,
	/* 11 */ &op_ora_indirect_y,
	/* 12 */ NULL,
	/* 13 */ NULL,
	/* 14 */ NULL,
	/* 15 */ &op_ora_zero_x,
	/* 16 */ &op_asl_zero_x,
	/* 17 */ NULL,
	/* 18 */ &op_clc,
	/* 19 */ &op_ora_absolute_y,
	/* 1a */ NULL,
	/* 1b */ NULL,
	/* 1c */ NULL,
	/* 1d */ &op_ora_absolute_x,
	/* 1e */ &op_asl_absolute_x,
	/* 1f */ NULL,
	/* 20 */ &op_jsr,
	/* 21 */ &op_and_indirect_x,
	/* 22 */ NULL,
	/* 23 */ NULL,
	/* 24 */ &op_bit_zero,
	/* 25 */ &op_and_zero,
	/* 26 */ &op_rol_zero,
	/* 27 */ NULL,
	/* 28 */ &op_plp,
	/* 29 */ &op_and,
	/* 2a */ &op_rol,
	/* 2b */ NULL,
	/* 2c */ &op_bit_absolute,
	/* 2d */ &op_and_absolute,
	/* 2e */ &op_rol_absolute,
	/* 2f */ NULL,
	/* 30 */ &op_bmi,
	/* 31 */ &op_and_indirect_y,
	/* 32 */ NULL,
	/* 33 */ NULL,
	/* 34 */ NULL,
	/* 35 */ &op_and_zero_x,
	/* 36 */ &op_rol_zero_x,
	/* 37 */ NULL,
	/* 38 */ &op_sec,
	/* 39 */ &op_and_absolute_y,
	/* 3a */ NULL,
	/* 3b */ NULL,
	/* 3c */ NULL,
	/* 3d */ &op_and_absolute_x,
	/* 3e */ &op_rol_absolute_x,
	/* 3f */ NULL,
	/* 40 */ NULL,
	/* 41 */ &op_eor_indirect_x,
	/* 42 */ NULL,
	/* 43 */ NULL,
	/* 44 */ NULL,
	/* 45 */ &op_eor_zero,
	/* 46 */ &op_lsr_zero,
	/* 47 */ NULL,
	/* 48 */ &op_pha,
	/* 49 */ &op_eor,
	/* 4a */ &op_lsr,
	/* 4b */ NULL,
	/* 4c */ &op_jmp,
	/* 4d */ &op_eor_absolute,
	/* 4e */ &op_lsr_absolute,
	/* 4f */ NULL,
	/* 50 */ &op_bvc,
	/* 51 */ &op_eor_indirect_y,
	/* 52 */ NULL,
	/* 53 */ NULL,
	/* 54 */ NULL,
	/* 55 */ &op_eor_zero_x,
	/* 56 */ &op_lsr_zero_x,
	/* 57 */ NULL,
	/* 58 */ &op_cli,
	/* 59 */ &op_eor_absolute_y,
	/* 5a */ NULL,
	/* 5b */ NULL,
	/* 5c */ NULL,
	/* 5d */ &op_eor_absolute_x,
	/* 5e */ &op_lsr_absolute_x,
	/* 5f */ NULL,
	/* 60 */ &op_rts,
	/* 61 */ &op_adc_indirect_x,
	/* 62 */ NULL,
	/* 63 */ NULL,
	/* 64 */ NULL,
	/* 65 */ &op_adc_zero,
	/* 66 */ &op_ror_zero,
	/* 67 */ NULL,
	/* 68 */ &op_pla,
	/* 69 */ &op_adc,
	/* 6a */ &op_ror,
	/* 6b */ NULL,
	/* 6c */ &op_jmp_indirect,
	/* 6d */ &op_adc_absolute,
	/* 6e */ &op_ror_absolute,
	/* 6f */ NULL,
	/* 70 */ &op_bvs,
	/* 71 */ &op_adc_indirect_y,
	/* 72 */ NULL,
	/* 73 */ NULL,
	/* 74 */ NULL,
	/* 75 */ &op_adc_zero_x,
	/* 76 */ &op_ror_zero_x,
	/* 77 */ NULL,
	/* 78 */ &op_sei,
	/* 79 */ &op_adc_absolute_y,
	/* 7a */ NULL,
	/* 7b */ NULL,
	/* 7c */ NULL,
	/* 7d */ &op_adc_absolute_x,
	/* 7e */ &op_ror_absolute_x,
	/* 7f */ NULL,
	/* 80 */ NULL,
	/* 81 */ &op_sta_indirect_x,
	/* 82 */ NULL,
	/* 83 */ NULL,
	/* 84 */ &op_sty_zero,
	/* 85 */ &op_sta_zero,
	/* 86 */ &op_stx_zero,
	/* 87 */ NULL,
	/* 88 */ &op_dey,
	/* 89 */ NULL,
	/* 8a */ &op_txa,
	/* 8b */ NULL,
	/* 8c */ &op_sty_absolute,
	/* 8d */ &op_sta_absolute,
	/* 8e */ &op_stx_absolute,
	/* 8f */ NULL,
	/* 90 */ &op_bcc,
	/* 91 */ &op_sta_indirect_y,
	/* 92 */ NULL,
	/* 93 */ NULL,
	/* 94 */ &op_sty_zero_x,
	/* 95 */ &op_sta_zero_x,
	/* 96 */ &op_stx_zero_y,
	/* 97 */ NULL,
	/* 98 */ &op_tya,
	/* 99 */ &op_sta_absolute_y,
	/* 9a */ &op_txs,
	/* 9b */ NULL,
	/* 9c */ NULL,
	/* 9d */ &op_sta_absolute_x,
	/* 9e */ NULL,
	/* 9f */ NULL,
	/* a0 */ &op_ldy,
	/* a1 */ &op_lda_indirect_x,
	/* a2 */ &op_ldx,
	/* a3 */ NULL,
	/* a4 */ &op_ldy_zero,
	/* a5 */ &op_lda_zero,
	/* a6 */ &op_ldx_zero,
	/* a7 */ NULL,
	/* a8 */ &op_tay,
	/* a9 */ &op_lda,
	/* aa */ &op_tax,
	/* ab */ NULL,
	/* ac */ &op_ldy_absolute,
	/* ad */ &op_lda_absolute,
	/* ae */ &op_ldx_absolute,
	/* af */ NULL,
	/* b0 */ &op_bcs,
	/* b1 */ &op_lda_indirect_y,
	/* b2 */ NULL,
	/* b3 */ NULL,
	/* b4 */ &op_ldy_zero_x,
	/* b5 */ &op_lda_zero_x,
	/* b6 */ &op_ldx_zero_y,
	/* b7 */ NULL,
	/* b8 */ &op_clv,
	/* b9 */ &op_lda_absolute_y,
	/* ba */ &op_tsx,
	/* bb */ NULL,
	/* bc */ &op_ldy_absolute_x,
	/* bd */ &op_lda_absolute_x,
	/* be */ &op_ldx_absolute_y,
	/* bf */ NULL,
	/* c0 */ &op_cpy,
	/* c1 */ &op_cmp_indirect_x,
	/* c2 */ NULL,
	/* c3 */ NULL,
	/* c4 */ &op_cpy_zero,
	/* c5 */ &op_cmp_zero,
	/* c6 */ &op_dec_zero,
	/* c7 */ NULL,
	/* c8 */ &op_incy,
	/* c9 */ &op_cmp,
	/* ca */ &op_dex,
	/* cb */ NULL,
	/* cc */ &op_cpy_absolute,
	/* cd */ &op_cmp_absolute,
	/* ce */ &op_dec_absolute,
	/* cf */ NULL,
	/* d0 */ &op_bne,
	/* d1 */ &op_cmp_indirect_y,
	/* d2 */ NULL,
	/* d3 */ NULL,
	/* d4 */ NULL,
	/* d5 */ &op_cmp_zero_x,
	/* d6 */ &op_dec_zero_x,
	/* d7 */ NULL,
	/* d8 */ &op_cld,
	/* d9 */ &op_cmp_absolute_y,
	/* da */ NULL,
	/* db */ NULL,
	/* dc */ NULL,
	/* dd */ &op_cmp_absolute_x,
	/* de */ &op_dec_absolute_x,
	/* df */ NULL,
	/* e0 */ &op_cpx,
	/* e1 */ &op_sbc_indirect_x,
	/* e2 */ NULL,
	/* e3 */ NULL,
	/* e4 */ &op_cpx_zero,
	/* e5 */ &op_sbc_zero,
	/* e6 */ &op_inc_zero,
	/* e7 */ NULL,
	/* e8 */ &op_incx,
	/* e9 */ &op_sbc,
	/* ea */ NULL,
	/* eb */ NULL,
	/* ec */ &op_cpx_absolute,
	/* ed */ &op_sbc_absolute,
	/* ee */ &op_inc_absolute,
	/* ef */ NULL,
	/* f0 */ &op_beq,
	/* f1 */ &op_sbc_indirect_y,
	/* f2 */ NULL,
	/* f3 */ NULL,
	/* f4 */ NULL,
	/* f5 */ &op_sbc_zero_x,
	/* f6 */ &op_inc_zero_x,
	/* f7 */ NULL,
	/* f8 */ &op_sed,
	/* f9 */ &op_sbc_absolute_y,
	/* fa */ NULL,
	/* fb */ NULL,
	/* fc */ NULL,
	/* fd */ &op_sbc_absolute_x,
	/* fe */ &op_inc_absolute_x,
	/* ff */ NULL,
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
	printf("OP: %02x\n", cpu_memload(cpustate.PC));

	if (cpustate.SP != 0xff) {
		printf("Stack: ");
		for (off = 0xff; off > cpustate.SP; off--) {
			printf("%02x ", cpu_memload(0x0100 + off));
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
	pcaddress.b[0] = cpu_memload(0xfffc);
	pcaddress.b[1] = cpu_memload(0xfffd);
	cpustate.PC = pcaddress.full;
};

void cpu_run()
{
	byte op;
	opfunct func;

	printf("Running program at: 0x%04x\n", cpustate.PC);

	do {
		print_cpustate();
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
