
#include "linker.h"

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <shared/helper/log.h>
#include <shared/helper/mem.h>
#include <shared/intermediate.h>
#include <shared/intermediate_file.h>

static int link_two(struct intermediate_file *target, struct intermediate_file *source) {
    uint32_t new_block_count = target->block_count + source->block_count;
    target->blocks = xrealloc(target->blocks, sizeof(struct intermediate_block *) * new_block_count);

	for (uint32_t i = 0; i < source->block_count; i++) {
		struct intermediate_block *source_block = source->blocks[i];

		for (uint32_t j = 0; j < target->block_count; j++) {
			struct intermediate_block *target_block = target->blocks[j];

			if (strcmp(source_block->name, target_block->name) == 0) {
				fprintf(stderr, "linker error: duplicate definition of block \"%s\"\n", source_block->name);
				return 1;
			}
		}

		target->blocks[target->block_count] = source_block;
		target->block_count++;
	}

    return 0;
}

int linker_link(struct intermediate_file *output, struct intermediate_file *inputs, size_t input_count) {
    int ret;

    if (input_count == 0) {
    	output->blocks = NULL;
    	output->block_count = 0;
    	return 0;
    }

    output->blocks = inputs[0].blocks;
    output->block_count = inputs[0].block_count;

    for (size_t i = 1; i < input_count; i++) {
	    struct intermediate_file *input = &inputs[i];

	    ret = link_two(output, input);
	    if (ret) {
	    	return ret;
	    }
	}

    return 0;
}
