
#include <stdio.h>
#include <string.h>

#include <shared/helper/log.h>
#include <shared/helper/mem.h>
#include <shared/intermediate.h>
#include <shared/intermediate_file.h>

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file>\n", argv[0]);
        return -1;
    }

    const char *filename = argv[1];

    int ret;

	struct intermediate_block **blocks;
	uint32_t block_count;

	ret = intermediate_file_read(filename, &blocks, &block_count);
	if (ret) {
		return ret;
	}

	intermediate_print(stdout, blocks, block_count);

    return 0;
}