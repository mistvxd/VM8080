#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "memory.h"

int cpu_load_mem(CPU *cpu, const uint8_t *data, size_t size, uint16_t address)
{
	if (cpu == NULL || data == NULL || size > sizeof(cpu->memory) - address)
		return -1;

	memcpy(&cpu->memory[address], data, size);
	return 0;
}
