#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "cpu.h"

#define FLAG_S 0x80
#define FLAG_Z 0x40
#define FLAG_AC 0x10
#define FLAG_P 0x04
#define FLAG_CY 0x01

typedef enum {
	/* Data transfer */
	INS_MOV,
	INS_MVI,
	INS_LXI,
	INS_LDA,
	INS_STA,
	INS_LHLD,
	INS_SHLD,
	INS_LDAX,
	INS_STAX,
	INS_XCHG,
	INS_XTHL,

	/* Arithmetic */
	INS_ADD,
	INS_ADC,
	INS_ADI,
	INS_ACI,
	INS_SUB,
	INS_SBB,
	INS_SUI,
	INS_SBI,
	INS_INR,
	INS_DCR,
	INS_INX,
	INS_DCX,
	INS_DAD,
	INS_DAA,

	/* Logical */
	INS_ANA,
	INS_ANI,
	INS_XRA,
	INS_XRI,
	INS_ORA,
	INS_ORI,
	INS_CMP,
	INS_CPI,
	INS_CMA,

	/* Rotate and carry */
	INS_RLC,
	INS_RRC,
	INS_RAL,
	INS_RAR,
	INS_STC,
	INS_CMC,

	/* Branch */
	INS_JMP,
	INS_JNZ,
	INS_JZ,
	INS_JNC,
	INS_JC,
	INS_JPO,
	INS_JPE,
	INS_JP,
	INS_JM,

	/* Call */
	INS_CALL,
	INS_CNZ,
	INS_CZ,
	INS_CNC,
	INS_CC,
	INS_CPO,
	INS_CPE,
	INS_CP,
	INS_CM,

	/* Return */
	INS_RET,
	INS_RNZ,
	INS_RZ,
	INS_RNC,
	INS_RC,
	INS_RPO,
	INS_RPE,
	INS_RP,
	INS_RM,

	INS_RST,
	INS_PCHL,

	/* Stack */
	INS_PUSH,
	INS_POP,
	INS_SPHL,

	/* Input and output */
	INS_IN,
	INS_OUT,

	/* Interrupt control */
	INS_DI,
	INS_EI,

	/* Processor control */
	INS_NOP,
	INS_HLT,

	INS_INVALID
} Instruction;

static const char *const instruction_names[] =
	{"MOV",	   "MVI",  "LXI",  "LDA", "STA", "LHLD", "SHLD", "LDAX", "STAX", "XCHG", "XTHL",

	 "ADD",	   "ADC",  "ADI",  "ACI", "SUB", "SBB",	 "SUI",	 "SBI",	 "INR",	 "DCR",	 "INX",
	 "DCX",	   "DAD",  "DAA",

	 "ANA",	   "ANI",  "XRA",  "XRI", "ORA", "ORI",	 "CMP",	 "CPI",	 "CMA",

	 "RLC",	   "RRC",  "RAL",  "RAR", "STC", "CMC",

	 "JMP",	   "JNZ",  "JZ",   "JNC", "JC",	 "JPO",	 "JPE",	 "JP",	 "JM",

	 "CALL",   "CNZ",  "CZ",   "CNC", "CC",	 "CPO",	 "CPE",	 "CP",	 "CM",

	 "RET",	   "RNZ",  "RZ",   "RNC", "RC",	 "RPO",	 "RPE",	 "RP",	 "RM",

	 "RST",	   "PCHL",

	 "PUSH",   "POP",  "SPHL",

	 "IN",	   "OUT",

	 "DI",	   "EI",

	 "NOP",	   "HLT",

	 "INVALID"};

typedef struct {
	Instruction instruction;
	const char *mnemonic;
	uint8_t size;
	uint8_t cycles;
} InstructionInfo;

Instruction instruction_decode(uint8_t opcode);
void instruction_execute(CPU *cpu, uint8_t opcode, uint16_t pc);
void instruction_set_trace(bool enabled);

const InstructionInfo *instruction_get_info(uint8_t opcode);
