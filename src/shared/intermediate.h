
#pragma once

#include <stdint.h>
#include <stdlib.h>

#define INTERMEDIATE_OP_AND 1
#define INTERMEDIATE_OP_MUX 2

struct intermediate_input {
	char *name;
	uint32_t width;
};

struct intermediate_output {
	char *name;
	uint32_t width;
};

struct intermediate_statement {
	uint16_t op;
	uint16_t size;
	uint32_t input_count;
	uint32_t *inputs;
	uint32_t output_count;
	uint32_t *outputs;
};

struct intermediate_block {
	char *name;
	uint32_t input_count;
	struct intermediate_input *inputs;
	uint32_t output_count;
	struct intermediate_output *outputs;
	uint32_t statement_count;
	struct intermediate_statement *statements;
};

struct intermediate_block *intermediate_block_create(const char *name);
void intermediate_block_destroy(struct intermediate_block *block);
void intermediate_block_add_input(struct intermediate_block *block, const char *name, uint32_t width);
void intermediate_block_add_output(struct intermediate_block *block, const char *name, uint32_t width);
struct intermediate_statement *intermediate_block_add_statement(struct intermediate_block *block, uint16_t op, uint16_t size);

void intermediate_statement_init(struct intermediate_statement *statement, uint16_t op, uint16_t size);
void intermediate_statement_destroy(struct intermediate_statement *statement);
void intermediate_statement_set_input(struct intermediate_statement *statement, uint32_t index, uint32_t signal_id);
void intermediate_statement_set_output(struct intermediate_statement *statement, uint32_t index, uint32_t signal_id);
