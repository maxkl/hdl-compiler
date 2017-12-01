
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
	xfree(block->statements);
	xfree(block);
}

uint32_t intermediate_block_allocate_signals(struct intermediate_block *block, uint32_t count) {
	uint32_t base_signal = block->next_signal;
	block->next_signal += count;
	return base_signal;
}

uint32_t intermediate_block_allocate_input_signals(struct intermediate_block *block, uint32_t count) {
	if (block->output_signals > 0 || block->statement_count > 0) {
		fprintf(stderr, "Can't add input signals after adding output signals or statements\n");
		return 0;
	}

	uint32_t base_signal = block->next_signal;
	block->input_signals += count;
	block->next_signal += count;

	return base_signal;
}

uint32_t intermediate_block_allocate_output_signals(struct intermediate_block *block, uint32_t count) {
	if (block->statement_count > 0) {
		fprintf(stderr, "Can't add output signals after adding statements\n");
		return 0;
	}

	uint32_t base_signal = block->next_signal;
	block->output_signals += count;
	block->next_signal += count;

	return base_signal;
}

struct intermediate_statement *intermediate_block_add_statement(struct intermediate_block *block, uint16_t op, uint16_t size) {
	uint32_t index = block->statement_count;
	block->statement_count++;
	block->statements = xrealloc(block->statements, sizeof(struct intermediate_statement) * block->statement_count);
	struct intermediate_statement *statement = &block->statements[index];
	intermediate_statement_init(statement, op, size);
	return statement;
}

void intermediate_statement_init(struct intermediate_statement *statement, uint16_t op, uint16_t size) {
	statement->op = op;
	statement->size = size;
	size_t input_count, output_count;
	switch (op) {
		case INTERMEDIATE_OP_CONNECT:
			if (size != 1) {
				log_error("Error: CONNECT statements with size other than 1 not supported\n");
				return;
			}
			input_count = 1;
			output_count = 1;
			break;
		case INTERMEDIATE_OP_CONST_0:
		case INTERMEDIATE_OP_CONST_1:
			input_count = 0;
			output_count = 1;
			break;
		case INTERMEDIATE_OP_AND:
		case INTERMEDIATE_OP_OR:
		case INTERMEDIATE_OP_XOR:
			input_count = size;
			output_count = 1;
			break;
		case INTERMEDIATE_OP_NOT:
			if (size != 1) {
				log_error("Error: NOT statements with size other than 1 not supported\n");
				return;
			}
			input_count = 1;
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
	statement->inputs = input_count > 0 ? xcalloc(input_count, sizeof(uint32_t)) : NULL;
	statement->output_count = output_count;
	statement->outputs = output_count > 0 ? xcalloc(output_count, sizeof(uint32_t)) : NULL;
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

void intermediate_print(FILE *stream, struct intermediate_block **blocks, uint32_t block_count) {
	for (uint32_t i = 0; i < block_count; i++) {
		struct intermediate_block *block = blocks[i];

		fprintf(stream, "Block: name=\"%s\", input signals=%u, output signals=%u, statements=%u\n", block->name, block->input_signals, block->output_signals, block->statement_count);

		for (size_t j = 0; j < block->statement_count; j++) {
			struct intermediate_statement *stmt = &block->statements[j];

			fprintf(stream, "  Statement: op=");

			switch (stmt->op) {
		        case INTERMEDIATE_OP_CONNECT:
		            fprintf(stream, "CONNECT");
		            break;
		        case INTERMEDIATE_OP_CONST_0:
		            fprintf(stream, "CONST_0");
		            break;
		        case INTERMEDIATE_OP_CONST_1:
		            fprintf(stream, "CONST_1");
		            break;
		        case INTERMEDIATE_OP_AND:
		            fprintf(stream, "AND");
		            break;
		        case INTERMEDIATE_OP_OR:
		            fprintf(stream, "OR");
		            break;
		        case INTERMEDIATE_OP_XOR:
		            fprintf(stream, "XOR");
		            break;
		        case INTERMEDIATE_OP_NOT:
		            fprintf(stream, "NOT");
		            break;
		        case INTERMEDIATE_OP_MUX:
		            fprintf(stream, "MUX");
		            break;
		        default:
		            fprintf(stream, "(invalid)");
		    }

		    fprintf(stream, ", inputs=%u, outputs=%u\n", stmt->input_count, stmt->output_count);

		    for (size_t k = 0; k < stmt->input_count; k++) {
				fprintf(stream, "    Input: id=%u\n", stmt->inputs[k]);
			}

			for (size_t k = 0; k < stmt->output_count; k++) {
				fprintf(stream, "    Output: id=%u\n", stmt->outputs[k]);
			}
		}
	}
}
