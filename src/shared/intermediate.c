
#include "intermediate.h"

#include <string.h>

#include <shared/helper/log.h>
#include <shared/helper/mem.h>

struct intermediate_block *intermediate_block_create(const char *name) {
	struct intermediate_block *block = xcalloc(1, sizeof(struct intermediate_block));
	block->name = xstrdup(name);
	return block;
}

void intermediate_block_destroy(struct intermediate_block *block) {
	xfree(block->name);
	xfree(block->inputs);
	xfree(block->outputs);
	xfree(block->statements);
	xfree(block);
}

void intermediate_block_add_input(struct intermediate_block *block, const char *name, uint32_t width) {
	block->input_count++;
	block->inputs = xrealloc(block->inputs, sizeof(struct intermediate_input) * block->input_count);
	struct intermediate_input *input = &block->inputs[block->input_count - 1];
	input->name = xstrdup(name);
	input->width = width;
}

void intermediate_block_add_output(struct intermediate_block *block, const char *name, uint32_t width) {
	block->output_count++;
	block->outputs = xrealloc(block->outputs, sizeof(struct intermediate_output) * block->output_count);
	struct intermediate_output *output = &block->outputs[block->output_count - 1];
	output->name = xstrdup(name);
	output->width = width;
}

struct intermediate_statement *intermediate_block_add_statement(struct intermediate_block *block, uint16_t op, uint16_t size) {
	block->statement_count++;
	block->statements = xrealloc(block->statements, sizeof(struct intermediate_statement) * block->statement_count);
	struct intermediate_statement *statement = &block->statements[block->statement_count - 1];
	intermediate_statement_init(statement, op, size);
	return statement;
}

void intermediate_statement_init(struct intermediate_statement *statement, uint16_t op, uint16_t size) {
	statement->op = op;
	statement->size = size;
	size_t input_count, output_count;
	switch (op) {
		case INTERMEDIATE_OP_AND:
			input_count = size;
			output_count = 1;
			break;
		case INTERMEDIATE_OP_MUX:
			input_count = size + (1 << size);
			output_count = 1;
			break;
		default:
			log_error("Error: intermediate_statement_create() called with invalid op\n");
			return;
	}
	statement->input_count = input_count;
	statement->inputs = xcalloc(input_count, sizeof(uint32_t));
	statement->output_count = output_count;
	statement->outputs = xcalloc(output_count, sizeof(uint32_t));
}

void intermediate_statement_destroy(struct intermediate_statement *statement) {
	xfree(statement->inputs);
	xfree(statement->outputs);
	xfree(statement);
}

void intermediate_statement_set_input(struct intermediate_statement *statement, uint32_t index, uint32_t signal_id) {
	statement->inputs[index] = signal_id;
}

void intermediate_statement_set_output(struct intermediate_statement *statement, uint32_t index, uint32_t signal_id) {
	statement->outputs[index] = signal_id;
}
