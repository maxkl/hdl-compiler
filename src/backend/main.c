
#include <stdio.h>
#include <string.h>

#include <shared/helper/log.h>
#include <shared/helper/mem.h>
#include <shared/intermediate.h>
#include <shared/intermediate_file.h>

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s [write|read] <file>\n", argv[0]);
        return -1;
    }

    const char *cmd = argv[1];
    const char *filename = argv[2];

    int ret;
    if (strcmp(cmd, "read") == 0) {
    	ret = intermediate_file_read(filename);
    } else if (strcmp(cmd, "write") == 0) {
    	struct intermediate_block **blocks = xcalloc(2, sizeof(struct intermediate_block *));

    	{
	    	struct intermediate_block *block = intermediate_block_create("peter");
	    	intermediate_block_add_input(block, "a", 1);
	    	intermediate_block_add_input(block, "b", 1);
	    	intermediate_block_add_output(block, "q", 1);
	    	struct intermediate_statement *stmt = intermediate_block_add_statement(block, INTERMEDIATE_OP_AND, 2);
	    	intermediate_statement_set_input(stmt, 0, 0);
	    	intermediate_statement_set_input(stmt, 1, 1);
	    	intermediate_statement_set_output(stmt, 0, 2);
	    	blocks[0] = block;
	    }

    	{
	    	struct intermediate_block *block = intermediate_block_create("horst");
	    	intermediate_block_add_input(block, "s0", 1);
	    	intermediate_block_add_input(block, "s1", 1);
	    	intermediate_block_add_input(block, "a", 1);
	    	intermediate_block_add_input(block, "b", 1);
	    	intermediate_block_add_input(block, "c", 1);
	    	intermediate_block_add_input(block, "d", 1);
	    	intermediate_block_add_output(block, "q", 1);
	    	struct intermediate_statement *stmt = intermediate_block_add_statement(block, INTERMEDIATE_OP_MUX, 2);
	    	intermediate_statement_set_input(stmt, 0, 0);
	    	intermediate_statement_set_input(stmt, 1, 1);
	    	intermediate_statement_set_input(stmt, 2, 2);
	    	intermediate_statement_set_input(stmt, 3, 3);
	    	intermediate_statement_set_input(stmt, 4, 4);
	    	intermediate_statement_set_input(stmt, 5, 5);
	    	intermediate_statement_set_output(stmt, 0, 6);
	    	blocks[1] = block;
	    }

    	ret = intermediate_file_write(filename, blocks, 2);
    } else {
    	log_error("Error: Invalid command \"%s\"\n", cmd);
    	return -1;
    }

    return ret;
}