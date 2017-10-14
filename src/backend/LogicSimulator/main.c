
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <shared/helper/log.h>
#include <shared/helper/mem.h>
#include <shared/intermediate.h>
#include <shared/intermediate_file.h>

int generate_circuit(FILE *f, struct intermediate_block **blocks, uint32_t block_count) {
	if (block_count == 0) {
		fprintf(stderr, "Need at least one block\n");
		return -1;
	}

	struct intermediate_block *block = blocks[0];

	uint32_t signal_count = 0;

	for (uint32_t i = 0; i < block->input_count; i++) {
		signal_count += block->inputs[0].width;
	}

	for (uint32_t i = 0; i < block->output_count; i++) {
		signal_count += block->outputs[0].width;
	}

	for (uint32_t i = 0; i < block->statement_count; i++) {
		struct intermediate_statement *stmt = &block->statements[i];

		for (uint32_t j = 0; j < stmt->input_count; j++) {
			uint32_t input_signal_id = stmt->inputs[j];
			if (input_signal_id >= signal_count) {
				signal_count = input_signal_id + 1;
			}
		}

		for (uint32_t j = 0; j < stmt->output_count; j++) {
			uint32_t output_signal_id = stmt->outputs[j];
			if (output_signal_id >= signal_count) {
				signal_count = output_signal_id + 1;
			}
		}
	}

	size_t component_offset = (signal_count - 1) * 2 + 2;
	size_t output_connection_offset = (signal_count - 1) * 2 + 2 + 7 + 2;

	size_t top_offset = 0;

	fprintf(f, "{");
    fprintf(f, "\"components\":");
    fprintf(f, "[");

	bool added_component = false;

	for (uint32_t i = 0; i < block->statement_count; i++) {
		struct intermediate_statement *stmt = &block->statements[i];

		top_offset += 1;

		switch (stmt->op) {
			case INTERMEDIATE_OP_CONNECT:
				break;
			case INTERMEDIATE_OP_AND:
				if (added_component) {
					fprintf(f, ",");
				}
				fprintf(f, "{\"type\":\"and\",\"x\":%lu,\"y\":%lu,\"inputs\":%u}", component_offset + 1, top_offset, stmt->input_count);
				added_component = true;
				top_offset += 1 + (stmt->input_count - 1) * 2 + 1;
				break;
			case INTERMEDIATE_OP_OR:
				if (added_component) {
					fprintf(f, ",");
				}
				fprintf(f, "{\"type\":\"or\",\"x\":%lu,\"y\":%lu,\"inputs\":%u}", component_offset + 1, top_offset, stmt->input_count);
				added_component = true;
				top_offset += 1 + (stmt->input_count - 1) * 2 + 1;
				break;
			case INTERMEDIATE_OP_XOR:
				if (added_component) {
					fprintf(f, ",");
				}
				fprintf(f, "{\"type\":\"xor\",\"x\":%lu,\"y\":%lu,\"inputs\":%u}", component_offset + 1, top_offset, stmt->input_count);
				added_component = true;
				top_offset += 1 + (stmt->input_count - 1) * 2 + 1;
				break;
			case INTERMEDIATE_OP_NOT:
				if (added_component) {
					fprintf(f, ",");
				}
				fprintf(f, "{\"type\":\"not\",\"x\":%lu,\"y\":%lu}", component_offset + 1, top_offset);
				added_component = true;
				top_offset += 4;
				break;
			default:
				fprintf(stderr, "Unsupported op %u\n", stmt->op);
				return -1;
		}

		top_offset += 1;
	}

	fprintf(f, "]");
    fprintf(f, ",");
    fprintf(f, "\"connections\":");
    fprintf(f, "[");

	top_offset = 0;

	bool added_connection = false;

	for (uint32_t i = 0; i < block->statement_count; i++) {
		struct intermediate_statement *stmt = &block->statements[i];

		bool skip_connections = false;

		top_offset += 1;

		switch (stmt->op) {
			case INTERMEDIATE_OP_CONNECT:
				if (added_connection) {
					fprintf(f, ",");
				}
				fprintf(f, "{\"x1\":%lu,\"y1\":%lu,\"x2\":%lu,\"y2\":%lu}", (size_t) stmt->inputs[0] * 2, top_offset, output_connection_offset + stmt->outputs[0] * 2, top_offset);
				added_connection = true;
				skip_connections = true;
				break;
			case INTERMEDIATE_OP_AND:
				top_offset += 1 + (stmt->input_count - 1) * 2 + 1;
				break;
			case INTERMEDIATE_OP_OR:
				top_offset += 1 + (stmt->input_count - 1) * 2 + 1;
				break;
			case INTERMEDIATE_OP_XOR:
				top_offset += 1 + (stmt->input_count - 1) * 2 + 1;
				break;
			case INTERMEDIATE_OP_NOT:
				top_offset += 4;
				break;
			default:
				fprintf(stderr, "Unsupported op %u\n", stmt->op);
				return -1;
		}

		top_offset += 1;

		if (skip_connections) {
			continue;
		}

		uint32_t max_pins = stmt->input_count > stmt->output_count ? stmt->input_count : stmt->output_count;
		size_t height = (1 + (max_pins - 1) * 2 + 1);
		if (height < 4) {
			height = 4;
		}
		size_t mid = height / 2;

		size_t component_top_offset = top_offset - 1 - height;
		size_t input_top_offset = component_top_offset + 1 + mid - stmt->input_count;
		size_t output_top_offset = component_top_offset + 1 + mid - stmt->output_count;

		for (uint32_t j = 0; j < stmt->input_count; j++) {
			uint32_t input_signal_id = stmt->inputs[j];

			size_t y = input_top_offset + j * 2;

			if (added_connection) {
				fprintf(f, ",");
			}
			fprintf(f, "{\"x1\":%lu,\"y1\":%lu,\"x2\":%lu,\"y2\":%lu}", (size_t) input_signal_id * 2, y, component_offset, y);
			added_connection = true;
		}

		for (uint32_t j = 0; j < stmt->output_count; j++) {
			uint32_t output_signal_id = stmt->outputs[j];

			size_t y = output_top_offset + j * 2;

			if (added_connection) {
				fprintf(f, ",");
			}
			fprintf(f, "{\"x1\":%lu,\"y1\":%lu,\"x2\":%lu,\"y2\":%lu}", output_connection_offset - 2, y, output_connection_offset + output_signal_id * 2, y);
			added_connection = true;
		}
	}

	for (uint32_t i = 0; i < signal_count; i++) {
		top_offset += 1;

		if (added_connection) {
			fprintf(f, ",");
		}
		fprintf(f, "{\"x1\":%lu,\"y1\":%lu,\"x2\":%lu,\"y2\":%lu}", (size_t) i * 2, top_offset, output_connection_offset + i * 2, top_offset);
		added_connection = true;

		top_offset += 1;
	}

	for (uint32_t i = 0; i < signal_count; i++) {
		if (added_connection) {
			fprintf(f, ",");
		}
		fprintf(f, "{\"x1\":%lu,\"y1\":%lu,\"x2\":%lu,\"y2\":%lu}", (size_t) i * 2, (size_t) 0, (size_t) i * 2, top_offset);
		added_connection = true;

		if (added_connection) {
			fprintf(f, ",");
		}
		fprintf(f, "{\"x1\":%lu,\"y1\":%lu,\"x2\":%lu,\"y2\":%lu}", output_connection_offset + i * 2, (size_t) 0, output_connection_offset + i * 2, top_offset);
		added_connection = true;
	}

    fprintf(f, "]");
    fprintf(f, "}");

    fprintf(f, "\n");

    return 0;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <input file> <output file>\n", argv[0]);
        return -1;
    }

    const char *input_filename = argv[1];
    const char *output_filename = argv[2];

    int ret;

    struct intermediate_block **blocks;
    uint32_t block_count;

    ret = intermediate_file_read(input_filename, &blocks, &block_count);
    if (ret) {
    	return ret;
    }

    FILE *output_file;

    if (strcmp(output_filename, "-") == 0) {
    	output_file = stdout;
    } else {
	    output_file = fopen(output_filename, "wb");
	    if (output_file == NULL) {
	    	return -1;
	    }
	}

    ret = generate_circuit(output_file, blocks, block_count);
    if (ret) {
    	return ret;
    }

    if (output_file != stdout) {
	    fclose(output_file);
	}

    return 0;
}