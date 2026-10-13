#include <stdio.h>

#include "alu.h"
#include "cpu.h"
#include "instruction.h"

/* The 8080 encodes most instruction families in regular bit fields. */
static bool trace_enabled;
static const char *const register_names[] = {"B", "C", "D", "E", "H", "L", "M", "A"};
static const char *const pair_names[] = {"B", "D", "H", "SP"};

typedef enum {
	COND_ALWAYS,
	COND_NZ,
	COND_Z,
	COND_NC,
	COND_C,
	COND_PO,
	COND_PE,
	COND_P,
	COND_M
} Condition;

void instruction_set_trace(bool enabled)
{
	trace_enabled = enabled;
}

#define TRACE(...)                                                                                 \
	do {                                                                                       \
		if (trace_enabled)                                                                 \
			printf(__VA_ARGS__);                                                       \
	} while (0)

static CPURegister opcode_register(uint8_t opcode)
{
	return (CPURegister)(opcode & 7);
}
static CPURegister opcode_destination(uint8_t opcode)
{
	return (CPURegister)((opcode >> 3) & 7);
}
static CPURegisterPair opcode_pair(uint8_t opcode)
{
	return (CPURegisterPair)((opcode >> 4) & 3);
}
static Condition opcode_condition(uint8_t opcode)
{
	return (Condition)(((opcode >> 3) & 7) + 1);
}

static bool condition_holds(const CPU *cpu, Condition condition)
{
	uint8_t flags = cpu->regs.af.lo;
	switch (condition) {
	case COND_ALWAYS:
		return true;
	case COND_NZ:
		return !(flags & FLAG_Z);
	case COND_Z:
		return flags & FLAG_Z;
	case COND_NC:
		return !(flags & FLAG_CY);
	case COND_C:
		return flags & FLAG_CY;
	case COND_PO:
		return !(flags & FLAG_P);
	case COND_PE:
		return flags & FLAG_P;
	case COND_P:
		return !(flags & FLAG_S);
	case COND_M:
		return flags & FLAG_S;
	}
	return false;
}

static void push16(CPU *cpu, uint16_t value)
{
	cpu->memory[--cpu->regs.sp.w] = (uint8_t)(value >> 8);
	cpu->memory[--cpu->regs.sp.w] = (uint8_t)value;
}

static uint16_t pop16(CPU *cpu)
{
	uint8_t low = cpu->memory[cpu->regs.sp.w++];
	uint8_t high = cpu->memory[cpu->regs.sp.w++];
	return (uint16_t)(low | ((uint16_t)high << 8));
}

/* Decoder and metadata. */
static Instruction decode_fixed(uint8_t opcode)
{
	switch (opcode) {
	case 0x00:
		return INS_NOP;
	case 0x02:
	case 0x12:
		return INS_STAX;
	case 0x0A:
	case 0x1A:
		return INS_LDAX;
	case 0x07:
		return INS_RLC;
	case 0x0F:
		return INS_RRC;
	case 0x17:
		return INS_RAL;
	case 0x1F:
		return INS_RAR;
	case 0x22:
		return INS_SHLD;
	case 0x27:
		return INS_DAA;
	case 0x2A:
		return INS_LHLD;
	case 0x2F:
		return INS_CMA;
	case 0x32:
		return INS_STA;
	case 0x37:
		return INS_STC;
	case 0x3A:
		return INS_LDA;
	case 0x3F:
		return INS_CMC;
	case 0x76:
		return INS_HLT;
	case 0xC3:
		return INS_JMP;
	case 0xC9:
		return INS_RET;
	case 0xCD:
		return INS_CALL;
	case 0xD3:
		return INS_OUT;
	case 0xDB:
		return INS_IN;
	case 0xE3:
		return INS_XTHL;
	case 0xE9:
		return INS_PCHL;
	case 0xEB:
		return INS_XCHG;
	case 0xF3:
		return INS_DI;
	case 0xF9:
		return INS_SPHL;
	case 0xFB:
		return INS_EI;
	default:
		return INS_INVALID;
	}
}

Instruction instruction_decode(uint8_t opcode)
{
	if (opcode >= 0x40 && opcode <= 0x7F)
		return opcode == 0x76 ? INS_HLT : INS_MOV;
	if (opcode >= 0x80 && opcode <= 0xBF) {
		static const Instruction arithmetic[] = {INS_ADD, INS_ADC, INS_SUB, INS_SBB,
							 INS_ANA, INS_XRA, INS_ORA, INS_CMP};
		return arithmetic[(opcode >> 3) & 7];
	}
	if ((opcode & 0x0F) == 1 && opcode < 0x40)
		return INS_LXI;
	if ((opcode & 0x0F) == 3 && opcode < 0x40)
		return INS_INX;
	if ((opcode & 0x0F) == 0x0B && opcode < 0x40)
		return INS_DCX;
	if ((opcode & 7) == 4 && opcode < 0x40)
		return INS_INR;
	if ((opcode & 7) == 5 && opcode < 0x40)
		return INS_DCR;
	if ((opcode & 7) == 6 && opcode < 0x40)
		return INS_MVI;
	if ((opcode & 0x0F) == 9 && opcode < 0x40)
		return INS_DAD;
	if ((opcode & 0xC7) == 0xC7)
		return INS_RST;
	if ((opcode & 0xC7) == 0xC0)
		return (Instruction)(INS_RNZ + ((opcode >> 3) & 7));
	if ((opcode & 0xC7) == 0xC2)
		return (Instruction)(INS_JNZ + ((opcode >> 3) & 7));
	if ((opcode & 0xC7) == 0xC4)
		return (Instruction)(INS_CNZ + ((opcode >> 3) & 7));
	switch (opcode) {
	case 0xC1:
	case 0xD1:
	case 0xE1:
	case 0xF1:
		return INS_POP;
	case 0xC5:
	case 0xD5:
	case 0xE5:
	case 0xF5:
		return INS_PUSH;
	case 0xC6:
		return INS_ADI;
	case 0xCE:
		return INS_ACI;
	case 0xD6:
		return INS_SUI;
	case 0xDE:
		return INS_SBI;
	case 0xE6:
		return INS_ANI;
	case 0xEE:
		return INS_XRI;
	case 0xF6:
		return INS_ORI;
	case 0xFE:
		return INS_CPI;
	default:
		return decode_fixed(opcode);
	}
}

static uint8_t instruction_size(Instruction instruction)
{
	switch (instruction) {
	case INS_LXI:
	case INS_LDA:
	case INS_STA:
	case INS_LHLD:
	case INS_SHLD:
	case INS_JMP:
	case INS_JNZ:
	case INS_JZ:
	case INS_JNC:
	case INS_JC:
	case INS_JPO:
	case INS_JPE:
	case INS_JP:
	case INS_JM:
	case INS_CALL:
	case INS_CNZ:
	case INS_CZ:
	case INS_CNC:
	case INS_CC:
	case INS_CPO:
	case INS_CPE:
	case INS_CP:
	case INS_CM:
		return 3;
	case INS_MVI:
	case INS_ADI:
	case INS_ACI:
	case INS_SUI:
	case INS_SBI:
	case INS_ANI:
	case INS_XRI:
	case INS_ORI:
	case INS_CPI:
	case INS_IN:
	case INS_OUT:
		return 2;
	default:
		return 1;
	}
}

const InstructionInfo *instruction_get_info(uint8_t opcode)
{
	static InstructionInfo info;
	info.instruction = instruction_decode(opcode);
	info.mnemonic = instruction_names[info.instruction];
	info.size = instruction_size(info.instruction);
	info.cycles = 0;
	return &info;
}

/* Data transfer and arithmetic families. */
static void execute_mov(CPU *cpu, uint8_t opcode, uint16_t pc)
{
	CPURegister destination = opcode_destination(opcode);
	CPURegister source = opcode_register(opcode);
	TRACE("%04X  MOV %s, %s\n", pc, register_names[destination], register_names[source]);
	cpu_reg_write(cpu, destination, cpu_reg_read(cpu, source));
}

static void execute_mvi(CPU *cpu, uint8_t opcode, uint16_t pc)
{
	CPURegister destination = opcode_destination(opcode);
	uint8_t value = cpu_fetch8(cpu);
	TRACE("%04X  MVI %s, 0x%02X\n", pc, register_names[destination], value);
	cpu_reg_write(cpu, destination, value);
}

static void execute_lxi(CPU *cpu, uint8_t opcode, uint16_t pc)
{
	CPURegisterPair pair = opcode_pair(opcode);
	uint16_t value = cpu_fetch16(cpu);
	TRACE("%04X  LXI %s, 0x%04X\n", pc, pair_names[pair], value);
	cpu_pair_write(cpu, pair, value);
}

static void execute_memory_transfer(CPU *cpu, uint8_t opcode, uint16_t pc)
{
	uint16_t address;
	switch (opcode) {
	case 0x02:
	case 0x12:
		address = opcode == 2 ? cpu->regs.bc.w : cpu->regs.de.w;
		TRACE("%04X  STAX %s\n", pc, opcode == 2 ? "B" : "D");
		cpu->memory[address] = cpu->regs.af.hi;
		return;
	case 0x0A:
	case 0x1A:
		address = opcode == 0x0A ? cpu->regs.bc.w : cpu->regs.de.w;
		TRACE("%04X  LDAX %s\n", pc, opcode == 0x0A ? "B" : "D");
		cpu->regs.af.hi = cpu->memory[address];
		return;
	case 0x22:
		address = cpu_fetch16(cpu);
		TRACE("%04X  SHLD 0x%04X\n", pc, address);
		cpu->memory[address] = cpu->regs.hl.lo;
		cpu->memory[(uint16_t)(address + 1)] = cpu->regs.hl.hi;
		return;
	case 0x2A:
		address = cpu_fetch16(cpu);
		TRACE("%04X  LHLD 0x%04X\n", pc, address);
		cpu->regs.hl.lo = cpu->memory[address];
		cpu->regs.hl.hi = cpu->memory[(uint16_t)(address + 1)];
		return;
	case 0x32:
		address = cpu_fetch16(cpu);
		TRACE("%04X  STA 0x%04X\n", pc, address);
		cpu->memory[address] = cpu->regs.af.hi;
		return;
	case 0x3A:
		address = cpu_fetch16(cpu);
		TRACE("%04X  LDA 0x%04X\n", pc, address);
		cpu->regs.af.hi = cpu->memory[address];
		return;
	default:
		return;
	}
}

static void execute_register_arithmetic(CPU *cpu, uint8_t opcode, uint16_t pc)
{
	static const char *const names[] = {"ADD", "ADC", "SUB", "SBB", "ANA", "XRA", "ORA", "CMP"};
	uint8_t operation = (opcode >> 3) & 7;
	uint8_t value = cpu_reg_read(cpu, opcode_register(opcode));
	TRACE("%04X  %s %s\n", pc, names[operation], register_names[opcode_register(opcode)]);
	switch (operation) {
	case 0:
		alu_add(cpu, value);
		break;
	case 1:
		alu_adc(cpu, value);
		break;
	case 2:
		alu_sub(cpu, value);
		break;
	case 3:
		alu_sbb(cpu, value);
		break;
	case 4:
		alu_ana(cpu, value);
		break;
	case 5:
		alu_xra(cpu, value);
		break;
	case 6:
		alu_ora(cpu, value);
		break;
	case 7:
		alu_cmp(cpu, value);
		break;
	}
}

static void execute_register_increment(CPU *cpu, uint8_t opcode, uint16_t pc)
{
	CPURegister reg = opcode_destination(opcode);
	uint8_t value = cpu_reg_read(cpu, reg);
	bool decrement = opcode & 1;
	TRACE("%04X  %s %s\n", pc, decrement ? "DCR" : "INR", register_names[reg]);
	cpu_reg_write(cpu, reg, decrement ? alu_dcr(cpu, value) : alu_inr(cpu, value));
}

static void execute_pair_arithmetic(CPU *cpu, uint8_t opcode, uint16_t pc)
{
	CPURegisterPair pair = opcode_pair(opcode);
	uint8_t operation = opcode & 0x0F;
	const char *name = operation == 9 ? "DAD" : operation == 0x0B ? "DCX" : "INX";
	TRACE("%04X  %s %s\n", pc, name, pair_names[pair]);
	if (operation == 9)
		alu_dad(cpu, cpu_pair_read(cpu, pair));
	else if (operation == 0x0B)
		cpu_pair_write(cpu, pair, alu_dcx(cpu_pair_read(cpu, pair)));
	else
		cpu_pair_write(cpu, pair, alu_inx(cpu_pair_read(cpu, pair)));
}

/* Control flow. */
static void execute_jump(CPU *cpu, uint8_t opcode, uint16_t pc)
{
	static const char *const names[] = {"JMP", "JNZ", "JZ", "JNC", "JC",
					    "JPO", "JPE", "JP", "JM"};
	Condition condition = opcode == 0xC3 ? COND_ALWAYS : opcode_condition(opcode);
	uint16_t target = cpu_fetch16(cpu);
	TRACE("%04X  %s 0x%04X\n", pc, names[condition], target);
	if (condition_holds(cpu, condition))
		cpu->regs.pc.w = target;
}

static void execute_call(CPU *cpu, uint8_t opcode, uint16_t pc)
{
	static const char *const names[] = {"CALL", "CNZ", "CZ", "CNC", "CC",
					    "CPO",  "CPE", "CP", "CM"};
	Condition condition = opcode == 0xCD ? COND_ALWAYS : opcode_condition(opcode);
	uint16_t target = cpu_fetch16(cpu);
	TRACE("%04X  %s 0x%04X\n", pc, names[condition], target);
	if (condition_holds(cpu, condition)) {
		push16(cpu, cpu->regs.pc.w);
		cpu->regs.pc.w = target;
	}
}

static void execute_return(CPU *cpu, uint8_t opcode, uint16_t pc)
{
	static const char *const names[] = {"RET", "RNZ", "RZ", "RNC", "RC",
					    "RPO", "RPE", "RP", "RM"};
	Condition condition = opcode == 0xC9 ? COND_ALWAYS : opcode_condition(opcode);
	TRACE("%04X  %s\n", pc, names[condition]);
	if (condition_holds(cpu, condition))
		cpu->regs.pc.w = pop16(cpu);
}

/* Immediate ALU operations, stack and ports. */
static void execute_immediate(CPU *cpu, uint8_t opcode, uint16_t pc)
{
	static const char *const names[] = {"ADI", "ACI", "SUI", "SBI", "ANI", "XRI", "ORI", "CPI"};
	uint8_t value = cpu_fetch8(cpu);
	uint8_t operation = (uint8_t)((opcode - 0xC6) / 8);
	TRACE("%04X  %s 0x%02X\n", pc, names[operation], value);
	switch (operation) {
	case 0:
		alu_add(cpu, value);
		break;
	case 1:
		alu_adc(cpu, value);
		break;
	case 2:
		alu_sub(cpu, value);
		break;
	case 3:
		alu_sbb(cpu, value);
		break;
	case 4:
		alu_ana(cpu, value);
		break;
	case 5:
		alu_xra(cpu, value);
		break;
	case 6:
		alu_ora(cpu, value);
		break;
	case 7:
		alu_cmp(cpu, value);
		break;
	}
}

static void execute_stack(CPU *cpu, uint8_t opcode, uint16_t pc)
{
	static const char *const names[] = {"B", "D", "H", "PSW"};
	unsigned pair = (opcode >> 4) & 3;
	bool pop = opcode & 8;
	TRACE("%04X  %s %s\n", pc, pop ? "POP" : "PUSH", names[pair]);
	if (!pop) {
		uint16_t value = pair == 0 ? cpu->regs.bc.w
			: pair == 1	   ? cpu->regs.de.w
			: pair == 2
			? cpu->regs.hl.w
			: (uint16_t)(((uint16_t)cpu->regs.af.hi << 8) | cpu->regs.af.lo);
		push16(cpu, value);
		return;
	}
	uint16_t value = pop16(cpu);
	switch (pair) {
	case 0:
		cpu->regs.bc.w = value;
		break;
	case 1:
		cpu->regs.de.w = value;
		break;
	case 2:
		cpu->regs.hl.w = value;
		break;
	case 3:
		cpu->regs.af.hi = value >> 8;
		cpu->regs.af.lo = value;
		break;
	}
}

static void execute_ports(CPU *cpu, uint8_t opcode, uint16_t pc)
{
	uint8_t port = cpu_fetch8(cpu);
	if (opcode == 0xDB) {
		if (port == 0x10) {
			int value = getchar();
			cpu->regs.af.hi = value == EOF ? 0 : (uint8_t)value;
		} else
			cpu->regs.af.hi = cpu->ports[port];
		TRACE("%04X  IN 0x%02X\n", pc, port);
		return;
	}
	if (port == 0x11) {
		putchar(cpu->regs.af.hi);
		fflush(stdout);
	} else
		cpu->ports[port] = cpu->regs.af.hi;
	TRACE("%04X  OUT 0x%02X\n", pc, port);
}

static void execute_accumulator(CPU *cpu, uint8_t opcode, uint16_t pc)
{
	static const char *const names[] = {"RLC", "RRC", "RAL", "RAR"};
	if (opcode == 0x27) {
		TRACE("%04X  DAA\n", pc);
		alu_daa(cpu);
		return;
	}
	if (opcode == 0x2F) {
		TRACE("%04X  CMA\n", pc);
		alu_cma(cpu);
		return;
	}
	unsigned operation = (opcode >> 3) & 3;
	TRACE("%04X  %s\n", pc, names[operation]);
	switch (operation) {
	case 0:
		alu_rlc(cpu);
		break;
	case 1:
		alu_rrc(cpu);
		break;
	case 2:
		alu_ral(cpu);
		break;
	case 3:
		alu_rar(cpu);
		break;
	}
}

static void execute_fixed(CPU *cpu, uint8_t opcode, uint16_t pc)
{
	switch (opcode) {
	case 0x00:
		TRACE("%04X  NOP\n", pc);
		break;
	case 0x22:
	case 0x2A:
	case 0x32:
	case 0x3A:
	case 0x02:
	case 0x0A:
	case 0x12:
	case 0x1A:
		execute_memory_transfer(cpu, opcode, pc);
		break;
	case 0x07:
	case 0x0F:
	case 0x17:
	case 0x1F:
	case 0x27:
	case 0x2F:
		execute_accumulator(cpu, opcode, pc);
		break;
	case 0x37:
		TRACE("%04X  STC\n", pc);
		alu_stc(cpu);
		break;
	case 0x3F:
		TRACE("%04X  CMC\n", pc);
		alu_cmc(cpu);
		break;
	case 0x76:
		TRACE("%04X  HLT\n", pc);
		cpu->halted = true;
		break;
	case 0xE3: {
		uint16_t address = cpu->regs.sp.w;
		uint8_t low = cpu->memory[address];
		uint8_t high = cpu->memory[(uint16_t)(address + 1)];
		TRACE("%04X  XTHL\n", pc);
		cpu->memory[address] = cpu->regs.hl.lo;
		cpu->memory[(uint16_t)(address + 1)] = cpu->regs.hl.hi;
		cpu->regs.hl.lo = low;
		cpu->regs.hl.hi = high;
		break;
	}
	case 0xE9:
		TRACE("%04X  PCHL\n", pc);
		cpu->regs.pc.w = cpu->regs.hl.w;
		break;
	case 0xEB: {
		Reg value = cpu->regs.de;
		TRACE("%04X  XCHG\n", pc);
		cpu->regs.de = cpu->regs.hl;
		cpu->regs.hl = value;
		break;
	}
	case 0xF3:
	case 0xFB:
		TRACE("%04X  %s\n", pc, opcode == 0xF3 ? "DI" : "EI");
		break;
	case 0xF9:
		TRACE("%04X  SPHL\n", pc);
		cpu->regs.sp.w = cpu->regs.hl.w;
		break;
	default:
		TRACE("%04X  Invalid Opcode: %02X\n", pc, opcode);
		break;
	}
}

void instruction_execute(CPU *cpu, uint8_t opcode, uint16_t pc)
{
	if (opcode >= 0x40 && opcode <= 0x7F) {
		if (opcode == 0x76)
			execute_fixed(cpu, opcode, pc);
		else
			execute_mov(cpu, opcode, pc);
		return;
	}
	if (opcode >= 0x80 && opcode <= 0xBF) {
		execute_register_arithmetic(cpu, opcode, pc);
		return;
	}
	if ((opcode & 7) == 4 && opcode < 0x40) {
		execute_register_increment(cpu, opcode, pc);
		return;
	}
	if ((opcode & 7) == 5 && opcode < 0x40) {
		execute_register_increment(cpu, opcode, pc);
		return;
	}
	if ((opcode & 7) == 6 && opcode < 0x40) {
		execute_mvi(cpu, opcode, pc);
		return;
	}
	if ((opcode & 0x0F) == 1 && opcode < 0x40) {
		execute_lxi(cpu, opcode, pc);
		return;
	}
	if (((opcode & 0x0F) == 3 || (opcode & 0x0F) == 0x0B || (opcode & 0x0F) == 9) &&
	    opcode < 0x40) {
		execute_pair_arithmetic(cpu, opcode, pc);
		return;
	}
	if ((opcode & 0xC7) == 0xC7) {
		uint16_t target = opcode & 0x38;
		TRACE("%04X  RST %u\n", pc, target >> 3);
		push16(cpu, cpu->regs.pc.w);
		cpu->regs.pc.w = target;
		return;
	}
	if ((opcode & 0xC7) == 0xC0) {
		execute_return(cpu, opcode, pc);
		return;
	}
	if ((opcode & 0xC7) == 0xC2) {
		execute_jump(cpu, opcode, pc);
		return;
	}
	if ((opcode & 0xC7) == 0xC4) {
		execute_call(cpu, opcode, pc);
		return;
	}
	if (opcode == 0xC3) {
		execute_jump(cpu, opcode, pc);
		return;
	}
	if (opcode == 0xC9) {
		execute_return(cpu, opcode, pc);
		return;
	}
	if (opcode == 0xCD) {
		execute_call(cpu, opcode, pc);
		return;
	}
	if (opcode >= 0xC6 && (opcode & 7) == 6) {
		execute_immediate(cpu, opcode, pc);
		return;
	}
	if (opcode == 0xC1 || opcode == 0xD1 || opcode == 0xE1 || opcode == 0xF1 ||
	    opcode == 0xC5 || opcode == 0xD5 || opcode == 0xE5 || opcode == 0xF5) {
		execute_stack(cpu, opcode, pc);
		return;
	}
	if (opcode == 0xD3 || opcode == 0xDB) {
		execute_ports(cpu, opcode, pc);
		return;
	}
	execute_fixed(cpu, opcode, pc);
}
