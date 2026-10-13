#pragma once

#include <stdint.h>

#include "cpu.h"

enum {
	ALU_FLAG_CY = 0x01,
	ALU_FLAG_FIXED = 0x02,
	ALU_FLAG_P = 0x04,
	ALU_FLAG_AC = 0x10,
	ALU_FLAG_Z = 0x40,
	ALU_FLAG_S = 0x80
};

void alu_add(CPU *cpu, uint8_t value);
void alu_adc(CPU *cpu, uint8_t value);
void alu_sub(CPU *cpu, uint8_t value);
void alu_sbb(CPU *cpu, uint8_t value);
void alu_cmp(CPU *cpu, uint8_t value);

uint8_t alu_inr(CPU *cpu, uint8_t value);
uint8_t alu_dcr(CPU *cpu, uint8_t value);

void alu_dad(CPU *cpu, uint16_t value);
uint16_t alu_inx(uint16_t value);
uint16_t alu_dcx(uint16_t value);

void alu_ana(CPU *cpu, uint8_t value);
void alu_xra(CPU *cpu, uint8_t value);
void alu_ora(CPU *cpu, uint8_t value);

void alu_daa(CPU *cpu);
void alu_cma(CPU *cpu);
void alu_stc(CPU *cpu);
void alu_cmc(CPU *cpu);

void alu_rlc(CPU *cpu);
void alu_rrc(CPU *cpu);
void alu_ral(CPU *cpu);
void alu_rar(CPU *cpu);
