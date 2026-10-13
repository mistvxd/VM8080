#pragma once

#include <stddef.h>
#include <stdint.h>

#include "cpu.h"

typedef enum {
	LOADER_OK = 0,
	LOADER_INVALID_ARGUMENT,
	LOADER_OPEN_FAILED,
	LOADER_READ_FAILED,
	LOADER_TOO_LARGE,
	LOADER_EMPTY,
} LoaderResult;

LoaderResult loader_load_binary(CPU *cpu, const char *path, uint16_t load_address,
				size_t *loaded_size);
const char *loader_result_string(LoaderResult result);
