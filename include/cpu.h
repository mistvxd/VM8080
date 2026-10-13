#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define CPU_MEM_SIZE (UINT16_MAX + 1u)

typedef uint8_t MemoryEntry;

typedef union {
	uint16_t w;
	struct {
		uint8_t lo, hi;
	};
} Reg;

typedef struct {
	Reg af, bc, de, hl, sp, pc;
} CPURegs;

typedef struct {
	CPURegs regs;
	MemoryEntry memory[CPU_MEM_SIZE];
	bool halted;
	uint8_t ports[256];
} CPU;

typedef enum {
	CPU_RUN_HALTED,
	CPU_RUN_LIMIT_REACHED,
} CpuRunResult;

typedef enum {
	CPU_REG_B,
	CPU_REG_C,
	CPU_REG_D,
	CPU_REG_E,
	CPU_REG_H,
	CPU_REG_L,
	CPU_REG_M,
	CPU_REG_A
} CPURegister;

typedef enum {
	CPU_PAIR_BC,
	CPU_PAIR_DE,
	CPU_PAIR_HL,
	CPU_PAIR_SP
} CPURegisterPair;

uint8_t cpu_reg_read(CPU *cpu, CPURegister reg);
void cpu_reg_write(CPU *cpu, CPURegister reg, uint8_t value);

uint16_t cpu_pair_read(CPU *cpu, CPURegisterPair pair);
void cpu_pair_write(CPU *cpu, CPURegisterPair pair, uint16_t value);

uint8_t cpu_fetch8(CPU *cpu);
uint16_t cpu_fetch16(CPU *cpu);

void cpu_reset(CPU *cpu);
void cpu_step(CPU *cpu);
void cpu_dump(CPU *cpu);
void mem_dump(CPU *cpu, size_t threshold);
CpuRunResult cpu_run_steps(CPU *cpu, size_t max_steps, size_t *steps_executed);
void cpu_run(CPU *cpu);
