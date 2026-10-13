#include <stdbool.h>
#include <stdint.h>

#include "alu.h"

#define ALU_FLAGS_MASK 0xD7

static uint8_t alu_get_flags(CPU *cpu)
{
	return cpu->regs.af.lo;
}

static void alu_set_flags(CPU *cpu, uint8_t flags)
{
	cpu->regs.af.lo = (flags & ALU_FLAGS_MASK) | ALU_FLAG_FIXED;
}

static void alu_update_flag(CPU *cpu, uint8_t flag, bool set)
{
	uint8_t flags = alu_get_flags(cpu);

	if (set)
		flags |= flag;
	else
		flags &= (uint8_t)~flag;

	alu_set_flags(cpu, flags);
}

static bool alu_get_flag(CPU *cpu, uint8_t flag)
{
	return (alu_get_flags(cpu) & flag) != 0;
}

static bool alu_parity(uint8_t value)
{
	unsigned int count = 0;

	for (unsigned int i = 0; i < 8; i++)
		count += (value >> i) & 1;

	return (count % 2) == 0;
}

static void alu_update_szp(CPU *cpu, uint8_t value)
{
	uint8_t flags = alu_get_flags(cpu);

	flags &= (uint8_t)~(ALU_FLAG_S | ALU_FLAG_Z | ALU_FLAG_P);

	if (value & 0x80)
		flags |= ALU_FLAG_S;

	if (value == 0)
		flags |= ALU_FLAG_Z;

	if (alu_parity(value))
		flags |= ALU_FLAG_P;

	alu_set_flags(cpu, flags);
}

static void alu_add_value(CPU *cpu, uint8_t value, uint8_t carry)
{
	uint8_t a = cpu->regs.af.hi;
	uint16_t result = (uint16_t)a + value + carry;

	bool auxiliary_carry = ((a & 0x0F) + (value & 0x0F) + carry) > 0x0F;

	cpu->regs.af.hi = (uint8_t)result;

	alu_update_szp(cpu, (uint8_t)result);
	alu_update_flag(cpu, ALU_FLAG_AC, auxiliary_carry);
	alu_update_flag(cpu, ALU_FLAG_CY, result > 0xFF);
}

static void alu_sub_value(CPU *cpu, uint8_t value, uint8_t borrow)
{
	uint8_t a = cpu->regs.af.hi;
	uint16_t subtrahend = (uint16_t)value + borrow;
	uint8_t result = (uint8_t)(a - subtrahend);

	bool auxiliary_borrow = (a & 0x0F) < ((value & 0x0F) + borrow);

	cpu->regs.af.hi = result;

	alu_update_szp(cpu, result);
	alu_update_flag(cpu, ALU_FLAG_AC, auxiliary_borrow);
	alu_update_flag(cpu, ALU_FLAG_CY, (uint16_t)a < subtrahend);
}

void alu_add(CPU *cpu, uint8_t value)
{
	alu_add_value(cpu, value, 0);
}

void alu_adc(CPU *cpu, uint8_t value)
{
	uint8_t carry = alu_get_flag(cpu, ALU_FLAG_CY);
	alu_add_value(cpu, value, carry);
}

void alu_sub(CPU *cpu, uint8_t value)
{
	alu_sub_value(cpu, value, 0);
}

void alu_sbb(CPU *cpu, uint8_t value)
{
	uint8_t borrow = alu_get_flag(cpu, ALU_FLAG_CY);
	alu_sub_value(cpu, value, borrow);
}

void alu_cmp(CPU *cpu, uint8_t value)
{
	uint8_t a = cpu->regs.af.hi;
	uint16_t result = (uint16_t)a - value;
	uint8_t result8 = (uint8_t)result;

	bool auxiliary_borrow = (a & 0x0F) < (value & 0x0F);

	alu_update_szp(cpu, result8);
	alu_update_flag(cpu, ALU_FLAG_AC, auxiliary_borrow);
	alu_update_flag(cpu, ALU_FLAG_CY, a < value);
}

uint8_t alu_inr(CPU *cpu, uint8_t value)
{
	uint8_t result = (uint8_t)(value + 1);

	alu_update_szp(cpu, result);
	alu_update_flag(cpu, ALU_FLAG_AC, (value & 0x0F) == 0x0F);

	return result;
}

uint8_t alu_dcr(CPU *cpu, uint8_t value)
{
	uint8_t result = (uint8_t)(value - 1);

	alu_update_szp(cpu, result);
	alu_update_flag(cpu, ALU_FLAG_AC, (value & 0x0F) == 0x00);

	return result;
}

void alu_dad(CPU *cpu, uint16_t value)
{
	uint32_t result = (uint32_t)cpu->regs.hl.w + value;

	cpu->regs.hl.w = (uint16_t)result;
	alu_update_flag(cpu, ALU_FLAG_CY, result > 0xFFFF);
}

uint16_t alu_inx(uint16_t value)
{
	return (uint16_t)(value + 1);
}

uint16_t alu_dcx(uint16_t value)
{
	return (uint16_t)(value - 1);
}

void alu_ana(CPU *cpu, uint8_t value)
{
	cpu->regs.af.hi &= value;

	alu_update_szp(cpu, cpu->regs.af.hi);
	alu_update_flag(cpu, ALU_FLAG_AC, true);
	alu_update_flag(cpu, ALU_FLAG_CY, false);
}

void alu_xra(CPU *cpu, uint8_t value)
{
	cpu->regs.af.hi ^= value;

	alu_update_szp(cpu, cpu->regs.af.hi);
	alu_update_flag(cpu, ALU_FLAG_AC, false);
	alu_update_flag(cpu, ALU_FLAG_CY, false);
}

void alu_ora(CPU *cpu, uint8_t value)
{
	cpu->regs.af.hi |= value;

	alu_update_szp(cpu, cpu->regs.af.hi);
	alu_update_flag(cpu, ALU_FLAG_AC, false);
	alu_update_flag(cpu, ALU_FLAG_CY, false);
}

void alu_daa(CPU *cpu)
{
	uint8_t a = cpu->regs.af.hi;
	uint8_t correction = 0;
	bool old_carry = alu_get_flag(cpu, ALU_FLAG_CY);
	bool old_auxiliary = alu_get_flag(cpu, ALU_FLAG_AC);

	if (old_auxiliary || (a & 0x0F) > 9)
		correction |= 0x06;

	if (old_carry || a > 0x99)
		correction |= 0x60;

	uint16_t result = (uint16_t)a + correction;

	cpu->regs.af.hi = (uint8_t)result;

	alu_update_szp(cpu, (uint8_t)result);
	alu_update_flag(cpu, ALU_FLAG_AC, ((a & 0x0F) + (correction & 0x0F)) > 0x0F);
	alu_update_flag(cpu, ALU_FLAG_CY, old_carry || a > 0x99 || result > 0xFF);
}

void alu_cma(CPU *cpu)
{
	cpu->regs.af.hi = (uint8_t)~cpu->regs.af.hi;
}

void alu_stc(CPU *cpu)
{
	alu_update_flag(cpu, ALU_FLAG_CY, true);
}

void alu_cmc(CPU *cpu)
{
	alu_update_flag(cpu, ALU_FLAG_CY, !alu_get_flag(cpu, ALU_FLAG_CY));
}

void alu_rlc(CPU *cpu)
{
	uint8_t a = cpu->regs.af.hi;
	uint8_t carry = a >> 7;

	cpu->regs.af.hi = (uint8_t)((a << 1) | carry);
	alu_update_flag(cpu, ALU_FLAG_CY, carry != 0);
}

void alu_rrc(CPU *cpu)
{
	uint8_t a = cpu->regs.af.hi;
	uint8_t carry = a & 1;

	cpu->regs.af.hi = (uint8_t)((a >> 1) | (carry << 7));
	alu_update_flag(cpu, ALU_FLAG_CY, carry != 0);
}

void alu_ral(CPU *cpu)
{
	uint8_t a = cpu->regs.af.hi;
	uint8_t carry_in = alu_get_flag(cpu, ALU_FLAG_CY);
	uint8_t carry_out = a >> 7;

	cpu->regs.af.hi = (uint8_t)((a << 1) | carry_in);
	alu_update_flag(cpu, ALU_FLAG_CY, carry_out != 0);
}

void alu_rar(CPU *cpu)
{
	uint8_t a = cpu->regs.af.hi;
	uint8_t carry_in = alu_get_flag(cpu, ALU_FLAG_CY);
	uint8_t carry_out = a & 1;

	cpu->regs.af.hi = (uint8_t)((a >> 1) | (carry_in << 7));
	alu_update_flag(cpu, ALU_FLAG_CY, carry_out != 0);
}
