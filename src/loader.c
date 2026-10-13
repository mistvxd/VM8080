#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "loader.h"
#include "memory.h"

LoaderResult loader_load_binary(CPU *cpu, const char *path, uint16_t load_address,
				size_t *loaded_size)
{
	if (cpu == NULL || path == NULL || loaded_size == NULL)
		return LOADER_INVALID_ARGUMENT;

	*loaded_size = 0;

	FILE *file = fopen(path, "rb");

	if (file == NULL)
		return LOADER_OPEN_FAILED;

	if (fseek(file, 0, SEEK_END) != 0) {
		fclose(file);
		return LOADER_READ_FAILED;
	}

	long file_size = ftell(file);

	if (file_size == 0) {
		fclose(file);
		return LOADER_EMPTY;
	}

	if (file_size < 0 || (size_t)file_size > sizeof(cpu->memory) - load_address) {
		fclose(file);
		return LOADER_TOO_LARGE;
	}

	if (fseek(file, 0, SEEK_SET) != 0) {
		fclose(file);
		return LOADER_READ_FAILED;
	}

	size_t size = (size_t)file_size;
	uint8_t *buffer = malloc(size);
	if (buffer == NULL) {
		fclose(file);
		return LOADER_READ_FAILED;
	}

	if (fread(buffer, 1, size, file) != size) {
		free(buffer);
		fclose(file);
		return LOADER_READ_FAILED;
	}

	fclose(file);
	int load_result = cpu_load_mem(cpu, buffer, size, load_address);
	free(buffer);
	if (load_result != 0)
		return LOADER_TOO_LARGE;

	*loaded_size = size;
	return LOADER_OK;
}

const char *loader_result_string(LoaderResult result)
{
	switch (result) {
	case LOADER_OK:
		return "ok";
	case LOADER_INVALID_ARGUMENT:
		return "argumento inválido";
	case LOADER_OPEN_FAILED:
		return "não foi possível abrir o arquivo";
	case LOADER_READ_FAILED:
		return "erro ao ler o arquivo";
	case LOADER_TOO_LARGE:
		return "o binário não cabe na memória";
	case LOADER_EMPTY:
		return "o arquivo está vazio";
	}
	return "erro desconhecido";
}
