
#include <assert.h>
#include <stdio.h>

#include "cpu.h"
#include "instruction.h"

#define MEM_DUMP_WIDTH 16
#define MEM_DUMP_GAP 16
#define MEM_DUMP_CONTEXT 4

uint8_t cpu_reg_read(CPU *cpu, CPURegister reg)
{
	assert(reg >= CPU_REG_B && reg <= CPU_REG_A);

	switch (reg) {
	case CPU_REG_B:
		return cpu->regs.bc.hi;
	case CPU_REG_C:
		return cpu->regs.bc.lo;
	case CPU_REG_D:
		return cpu->regs.de.hi;
	case CPU_REG_E:
		return cpu->regs.de.lo;
	case CPU_REG_H:
		return cpu->regs.hl.hi;
	case CPU_REG_L:
		return cpu->regs.hl.lo;
	case CPU_REG_M:
		return cpu->memory[cpu->regs.hl.w];
	case CPU_REG_A:
		return cpu->regs.af.hi;
	}

	return 0;
}

void cpu_reg_write(CPU *cpu, CPURegister reg, uint8_t value)
{
	assert(reg >= CPU_REG_B && reg <= CPU_REG_A);

	switch (reg) {
	case CPU_REG_B:
		cpu->regs.bc.hi = value;
		break;
	case CPU_REG_C:
		cpu->regs.bc.lo = value;
		break;
	case CPU_REG_D:
		cpu->regs.de.hi = value;
		break;
	case CPU_REG_E:
		cpu->regs.de.lo = value;
		break;
	case CPU_REG_H:
		cpu->regs.hl.hi = value;
		break;
	case CPU_REG_L:
		cpu->regs.hl.lo = value;
		break;
	case CPU_REG_M:
		cpu->memory[cpu->regs.hl.w] = value;
		break;
	case CPU_REG_A:
		cpu->regs.af.hi = value;
		break;
	}
}

uint16_t cpu_pair_read(CPU *cpu, CPURegisterPair pair)
{
	assert(pair >= CPU_PAIR_BC && pair <= CPU_PAIR_SP);

	switch (pair) {
	case CPU_PAIR_BC:
		return cpu->regs.bc.w;
	case CPU_PAIR_DE:
		return cpu->regs.de.w;
	case CPU_PAIR_HL:
		return cpu->regs.hl.w;
	case CPU_PAIR_SP:
		return cpu->regs.sp.w;
	}

	return 0;
}

void cpu_pair_write(CPU *cpu, CPURegisterPair pair, uint16_t value)
{
	assert(pair >= CPU_PAIR_BC && pair <= CPU_PAIR_SP);

	switch (pair) {
	case CPU_PAIR_BC:
		cpu->regs.bc.w = value;
		break;
	case CPU_PAIR_DE:
		cpu->regs.de.w = value;
		break;
	case CPU_PAIR_HL:
		cpu->regs.hl.w = value;
		break;
	case CPU_PAIR_SP:
		cpu->regs.sp.w = value;
		break;
	}
}

uint8_t cpu_fetch8(CPU *cpu)
{
	return cpu->memory[cpu->regs.pc.w++];
}

uint16_t cpu_fetch16(CPU *cpu)
{
	uint16_t lo = cpu_fetch8(cpu);
	uint16_t hi = cpu_fetch8(cpu);

	return (uint16_t)(lo | (hi << 8));
}

void cpu_step(CPU *cpu)
{
	if (cpu->halted)
		return;

	uint16_t pc = cpu->regs.pc.w;
	uint8_t opcode = cpu_fetch8(cpu);

	instruction_execute(cpu, opcode, pc);
}

void cpu_dump(CPU *cpu)
{
	printf("a=%02x ", cpu->regs.af.hi);
	printf("b=%02x ", cpu->regs.bc.hi);
	printf("c=%02x ", cpu->regs.bc.lo);
	printf("d=%02x ", cpu->regs.de.hi);
	printf("e=%02x\n", cpu->regs.de.lo);
	printf("h=%02x ", cpu->regs.hl.hi);
	printf("l=%02x ", cpu->regs.hl.lo);
	printf("f=%02x ", cpu->regs.af.lo);
	printf("sp=%04x ", cpu->regs.sp.w);
	printf("pc=%04x\n", cpu->regs.pc.w);
}

void mem_dump(CPU *cpu, size_t threshold)
{
	const size_t memory_size = sizeof(cpu->memory);
	size_t address = 0;

	while (address < memory_size) {
		while (address < memory_size && cpu->memory[address] == 0)
			address++;

		if (address == memory_size)
			break;

		size_t start = address;
		size_t nonzero = 0;
		size_t end = address;

		while (end < memory_size && end - start < MEM_DUMP_WIDTH && nonzero < threshold) {
			if (cpu->memory[end] != 0)
				nonzero++;

			end++;
		}

		if (nonzero < threshold) {
			address = end;
			continue;
		}

		while (end < memory_size) {
			size_t next_nonzero = end;

			while (next_nonzero < memory_size && cpu->memory[next_nonzero] == 0 &&
			       next_nonzero - end < MEM_DUMP_GAP) {
				next_nonzero++;
			}

			if (next_nonzero == memory_size || next_nonzero - end >= MEM_DUMP_GAP)
				break;

			end = next_nonzero + 1;
		}

		start = start > MEM_DUMP_CONTEXT ? start - MEM_DUMP_CONTEXT : 0;

		if (end + MEM_DUMP_CONTEXT < memory_size)
			end += MEM_DUMP_CONTEXT;
		else
			end = memory_size;

		start -= start % MEM_DUMP_WIDTH;

		printf("\n");

		for (size_t i = start; i < end; i++) {
			if ((i - start) % MEM_DUMP_WIDTH == 0)
				printf("%04zX: ", i);

			printf("%02X ", cpu->memory[i]);

			if ((i - start + 1) % MEM_DUMP_WIDTH == 0 || i + 1 == end)
				printf("\n");
		}

		address = end;
	}
}

void cpu_run(CPU *cpu)
{
	(void)cpu_run_steps(cpu, 1024, NULL);
}

CpuRunResult cpu_run_steps(CPU *cpu, size_t max_steps, size_t *steps_executed)
{
	size_t steps = 0;

	if (cpu != NULL) {
		while (!cpu->halted && steps < max_steps) {
			cpu_step(cpu);
			steps++;
		}
	}

	if (steps_executed != NULL)
		*steps_executed = steps;

	return cpu != NULL && cpu->halted ? CPU_RUN_HALTED : CPU_RUN_LIMIT_REACHED;
}
void cpu_reset(CPU *cpu)
{
	if (cpu == NULL)
		return;

	*cpu = (CPU){0};
	cpu->regs.af.lo = 0x02;
}
