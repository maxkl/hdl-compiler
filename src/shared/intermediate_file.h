
#pragma once

#include <stdlib.h>

#include <shared/intermediate.h>

#define INTERMEDIATE_FILE_VERSION 2

struct intermediate_file {
	struct intermediate_block **blocks;
	uint32_t block_count;
};

int intermediate_file_read(const char *filename, struct intermediate_file *intermediate_file);
int intermediate_file_write(const char *filename, struct intermediate_file *intermediate_file);
