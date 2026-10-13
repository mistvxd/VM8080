#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "cpu.h"
#include "instruction.h"
#include "loader.h"
#include "vm.h"

static void usage(const char *program)
{
	fprintf(stderr, "Usage: %s [--trace] [--steps N] arquivo.bin\n", program);
}

int main(int argc, char **argv)
{
	const char *path = NULL;
	size_t max_steps = 1024;
	bool trace = false;

	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--trace") == 0) {
			trace = true;
		} else if (strcmp(argv[i], "--steps") == 0 && i + 1 < argc) {
			char *end = NULL;
			unsigned long value = strtoul(argv[++i], &end, 10);
			if (*end != '\0' || value == 0) {
				usage(argv[0]);
				return EXIT_FAILURE;
			}
			max_steps = (size_t)value;
		} else if (path == NULL) {
			path = argv[i];
		} else {
			usage(argv[0]);
			return EXIT_FAILURE;
		}
	}

	if (path == NULL) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}

	CPU cpu;
	vm_init(&cpu);
	instruction_set_trace(trace);

	size_t loaded_size;
	LoaderResult result = loader_load_binary(&cpu, path, 0, &loaded_size);
	if (result != LOADER_OK) {
		fprintf(stderr, "Error: %s (%s)\n", path, loader_result_string(result));
		return EXIT_FAILURE;
	}

	printf("Loaded %zu bytes\n", loaded_size);
	size_t steps;
	CpuRunResult run_result = cpu_run_steps(&cpu, max_steps, &steps);
	if (run_result == CPU_RUN_LIMIT_REACHED)
		fprintf(stderr, "Warning: limit of %zu instructions reached\n", max_steps);

	printf("%zu instructions executed\n", steps);
	cpu_dump(&cpu);

	return EXIT_SUCCESS;
}
