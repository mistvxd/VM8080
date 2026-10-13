#pragma once

#include <stdint.h>
#include <stddef.h>
#include "cpu.h"

int cpu_load_mem(CPU *cpu, const uint8_t *data, size_t size, uint16_t address);
