
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <shared/helper/log.h>
#include <shared/helper/mem.h>
#include <shared/intermediate.h>
#include <shared/intermediate_file.h>

int link_blocks(struct intermediate_block **target_blocks, uint32_t target_block_count, struct intermediate_block **source_blocks, uint32_t source_block_count) {
	int ret;

	for (uint32_t i = 0; i < source_block_count; i++) {
		struct intermediate_block *source_block = source_blocks[i];

		for (uint32_t j = 0; j < target_block_count; j++) {
			struct intermediate_block *target_block = target_blocks[j];

			if (strcmp(source_block->name, target_block->name) == 0) {
				fprintf(stderr, "Duplicate definition of block \"%s\"\n", source_block->name);
				return -1;
			}
		}

		target_blocks[target_block_count] = source_block;
		target_block_count++;
	}

    return 0;
}

static int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <input files>... <output file>\n", argv[0]);
        return -1;
    }

    const char *output_filename = argv[argc - 1];

    int ret;

    struct intermediate_block **linked_blocks = NULL;
	uint32_t linked_block_count = 0;

    for (int i = 1; i < argc - 1; i++) {
    	const char *input_filename = argv[i];

	    struct intermediate_file file;

	    ret = intermediate_file_read(input_filename, &file);
	    if (ret) {
	    	return ret;
	    }

	    uint32_t prev_block_count = linked_block_count;
	    linked_block_count += file.block_count;
	    linked_blocks = xrealloc(linked_blocks, sizeof(struct intermediate_block *) * linked_block_count);

	    ret = link_blocks(linked_blocks, prev_block_count, file.blocks, file.block_count);
	    if (ret) {
	    	return ret;
	    }
	}

 //    FILE *output_file;

 //    if (strcmp(output_filename, "-") == 0) {
 //    	output_file = stdout;
 //    } else {
	//     output_file = fopen(output_filename, "wb");
	//     if (output_file == NULL) {
	//     	return -1;
	//     }
	// }

 //    if (output_file != stdout) {
	//     fclose(output_file);
	// }

	struct intermediate_file file = {
		.blocks = linked_blocks,
		.block_count = linked_block_count
	};

	ret = intermediate_file_write(output_filename, &file);
    if (ret) {
    	return ret;
    }

    return 0;
}