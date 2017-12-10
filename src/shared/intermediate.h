
#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

enum intermediate_op {
	INTERMEDIATE_OP_CONNECT = 1,

	INTERMEDIATE_OP_CONST_0,
	INTERMEDIATE_OP_CONST_1,

	INTERMEDIATE_OP_AND,
	INTERMEDIATE_OP_OR,
	INTERMEDIATE_OP_XOR,

	INTERMEDIATE_OP_NOT,

	INTERMEDIATE_OP_MUX
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
	uint32_t name_index;

	uint32_t input_signals;
	uint32_t output_signals;
	uint32_t block_count;
	struct intermediate_block **blocks;
	uint32_t statement_count;
	struct intermediate_statement *statements;
	uint32_t next_signal;
};

struct intermediate_block *intermediate_block_create(const char *name);
void intermediate_block_destroy(struct intermediate_block *block);
uint32_t intermediate_block_allocate_signals(struct intermediate_block *block, uint32_t count);
uint32_t intermediate_block_allocate_input_signals(struct intermediate_block *block, uint32_t count);
uint32_t intermediate_block_allocate_output_signals(struct intermediate_block *block, uint32_t count);
uint32_t intermediate_block_add_block(struct intermediate_block *block, struct intermediate_block *added_block);
struct intermediate_statement *intermediate_block_add_statement(struct intermediate_block *block, uint16_t op, uint16_t size);

void intermediate_statement_init(struct intermediate_statement *statement, uint16_t op, uint16_t size);
void intermediate_statement_destroy(struct intermediate_statement *statement);
void intermediate_statement_set_input(struct intermediate_statement *statement, uint32_t index, uint32_t signal_id);
void intermediate_statement_set_output(struct intermediate_statement *statement, uint32_t index, uint32_t signal_id);

void intermediate_print(FILE *stream, struct intermediate_block **blocks, uint32_t block_count);
