
#include "backend_LogicSimulator.h"

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <shared/helper/log.h>
#include <shared/helper/mem.h>
#include <shared/intermediate.h>
#include <shared/intermediate_file.h>

struct options {
	bool io_components;
};

enum component_type {
	COMPONENT_TYPE_CONNECT,
	COMPONENT_TYPE_CONST_0,
	COMPONENT_TYPE_CONST_1,
	COMPONENT_TYPE_AND,
	COMPONENT_TYPE_OR,
	COMPONENT_TYPE_XOR,
	COMPONENT_TYPE_NOT
};

struct component {
	enum component_type type;
	uint32_t input_count;
	uint32_t *inputs;
	uint32_t output_count;
	uint32_t *outputs;
};

struct circuit {
	uint32_t input_signal_count;
	uint32_t output_signal_count;
	uint32_t signal_count;
	uint32_t component_count;
	struct component **components;
};

static struct component *component_create() {
	return xcalloc(1, sizeof(struct component));
}

static void component_add_input(struct component *component, uint32_t signal) {
	component->input_count++;
	component->inputs = xrealloc(component->inputs, sizeof(uint32_t) * component->input_count);
	component->inputs[component->input_count - 1] = signal;
}

static void component_add_output(struct component *component, uint32_t signal) {
	component->output_count++;
	component->outputs = xrealloc(component->outputs, sizeof(uint32_t) * component->output_count);
	component->outputs[component->output_count - 1] = signal;
}

static struct circuit *circuit_create() {
	return xcalloc(1, sizeof(struct circuit));
}

static uint32_t circuit_allocate_signals(struct circuit *circuit, uint32_t count) {
	uint32_t base_signal = circuit->signal_count;
	circuit->signal_count += count;
	return base_signal;
}

static void circuit_add_component(struct circuit *circuit, struct component *component) {
	circuit->component_count++;
	circuit->components = xrealloc(circuit->components, sizeof(struct component *) * circuit->component_count);
	circuit->components[circuit->component_count - 1] = component;
}

static int output_circuit(FILE *f, struct circuit *circuit, struct options *options) {
	size_t component_offset = (circuit->signal_count - 1) * 2 + 2;
	size_t output_connection_offset = (circuit->signal_count - 1) * 2 + 2 + 7 + 2;

	size_t top_offset = 0;

	fprintf(f, "{");
    fprintf(f, "\"components\":");
    fprintf(f, "[");

	bool added_component = false;

	for (uint32_t i = 0; i < circuit->component_count; i++) {
		struct component *component = circuit->components[i];

		top_offset += 1;

		switch (component->type) {
			case COMPONENT_TYPE_CONNECT:
				break;
			case COMPONENT_TYPE_CONST_0:
			case COMPONENT_TYPE_CONST_1:
				if (added_component) {
					fprintf(f, ",");
				}
				fprintf(f, "{\"type\":\"const\",\"x\":%lu,\"y\":%lu,\"value\":%s}", component_offset + 1, top_offset, component->type == COMPONENT_TYPE_CONST_1 ? "true" : "false");
				added_component = true;
				top_offset += 4;
				break;
			case COMPONENT_TYPE_AND:
				if (added_component) {
					fprintf(f, ",");
				}
				fprintf(f, "{\"type\":\"and\",\"x\":%lu,\"y\":%lu,\"inputs\":%u}", component_offset + 1, top_offset, component->input_count);
				added_component = true;
				top_offset += 1 + (component->input_count - 1) * 2 + 1;
				break;
			case COMPONENT_TYPE_OR:
				if (added_component) {
					fprintf(f, ",");
				}
				fprintf(f, "{\"type\":\"or\",\"x\":%lu,\"y\":%lu,\"inputs\":%u}", component_offset + 1, top_offset, component->input_count);
				added_component = true;
				top_offset += 1 + (component->input_count - 1) * 2 + 1;
				break;
			case COMPONENT_TYPE_XOR:
				if (added_component) {
					fprintf(f, ",");
				}
				fprintf(f, "{\"type\":\"xor\",\"x\":%lu,\"y\":%lu,\"inputs\":%u}", component_offset + 1, top_offset, component->input_count);
				added_component = true;
				top_offset += 1 + (component->input_count - 1) * 2 + 1;
				break;
			case COMPONENT_TYPE_NOT:
				if (added_component) {
					fprintf(f, ",");
				}
				fprintf(f, "{\"type\":\"not\",\"x\":%lu,\"y\":%lu}", component_offset + 1, top_offset);
				added_component = true;
				top_offset += 4;
				break;
		}

		top_offset += 1;
	}

	if (options->io_components) {
		for (uint32_t i = 0; i < circuit->input_signal_count; i++) {
			if (added_component) {
				fprintf(f, ",");
			}
			fprintf(f, "{\"type\":\"togglebutton\",\"x\":%i,\"y\":%i}", -8, i * 6 - circuit->input_signal_count * 6);
			added_component = true;
		}

		for (uint32_t i = 0; i < circuit->output_signal_count; i++) {
			if (added_component) {
				fprintf(f, ",");
			}
			fprintf(f, "{\"type\":\"led\",\"x\":%u,\"y\":%i,\"offColor\":\"#888\",\"onColor\":\"#e00\"}", (circuit->input_signal_count + circuit->output_signal_count) * 2 + 1, i * 6 - circuit->output_signal_count * 6);
			added_component = true;
		}
	}

	fprintf(f, "]");
    fprintf(f, ",");
    fprintf(f, "\"connections\":");
    fprintf(f, "[");

	top_offset = 0;

	bool added_connection = false;

	for (uint32_t i = 0; i < circuit->component_count; i++) {
		struct component *component = circuit->components[i];

		bool skip_connections = false;

		top_offset += 1;

		switch (component->type) {
			case COMPONENT_TYPE_CONNECT:
				if (added_connection) {
					fprintf(f, ",");
				}
				fprintf(f, "{\"x1\":%lu,\"y1\":%lu,\"x2\":%lu,\"y2\":%lu}", (size_t) component->inputs[0] * 2, top_offset, output_connection_offset + component->outputs[0] * 2, top_offset);
				added_connection = true;
				skip_connections = true;
				break;
			case COMPONENT_TYPE_CONST_0:
			case COMPONENT_TYPE_CONST_1:
				top_offset += 4;
				break;
			case COMPONENT_TYPE_AND:
				top_offset += 1 + (component->input_count - 1) * 2 + 1;
				break;
			case COMPONENT_TYPE_OR:
				top_offset += 1 + (component->input_count - 1) * 2 + 1;
				break;
			case COMPONENT_TYPE_XOR:
				top_offset += 1 + (component->input_count - 1) * 2 + 1;
				break;
			case COMPONENT_TYPE_NOT:
				top_offset += 4;
				break;
		}

		top_offset += 1;

		if (skip_connections) {
			continue;
		}

		uint32_t max_pins = component->input_count > component->output_count ? component->input_count : component->output_count;
		size_t height = (1 + (max_pins - 1) * 2 + 1);
		if (height < 4) {
			height = 4;
		}
		size_t mid = height / 2;

		size_t component_top_offset = top_offset - 1 - height;
		size_t input_top_offset = component_top_offset + 1 + mid - component->input_count;
		size_t output_top_offset = component_top_offset + 1 + mid - component->output_count;

		for (uint32_t j = 0; j < component->input_count; j++) {
			uint32_t input_signal_id = component->inputs[j];

			size_t y = input_top_offset + j * 2;

			if (added_connection) {
				fprintf(f, ",");
			}
			fprintf(f, "{\"x1\":%lu,\"y1\":%lu,\"x2\":%lu,\"y2\":%lu}", (size_t) input_signal_id * 2, y, component_offset, y);
			added_connection = true;
		}

		for (uint32_t j = 0; j < component->output_count; j++) {
			uint32_t output_signal_id = component->outputs[j];

			size_t y = output_top_offset + j * 2;

			if (added_connection) {
				fprintf(f, ",");
			}
			fprintf(f, "{\"x1\":%lu,\"y1\":%lu,\"x2\":%lu,\"y2\":%lu}", output_connection_offset - 2, y, output_connection_offset + output_signal_id * 2, y);
			added_connection = true;
		}
	}

	for (uint32_t i = 0; i < circuit->signal_count; i++) {
		top_offset += 1;

		if (added_connection) {
			fprintf(f, ",");
		}
		fprintf(f, "{\"x1\":%lu,\"y1\":%lu,\"x2\":%lu,\"y2\":%lu}", (size_t) i * 2, top_offset, output_connection_offset + i * 2, top_offset);
		added_connection = true;

		top_offset += 1;
	}

	for (uint32_t i = 0; i < circuit->signal_count; i++) {
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

	if (options->io_components) {
		for (uint32_t i = 0; i < circuit->input_signal_count; i++) {
			int x = i * 2;
			int y = i * 6 - circuit->input_signal_count * 6 + 2;

			if (added_connection) {
				fprintf(f, ",");
			}
			fprintf(f, "{\"x1\":%i,\"y1\":%i,\"x2\":%i,\"y2\":%i}", -2, y, x, y);

			fprintf(f, ",");
			fprintf(f, "{\"x1\":%i,\"y1\":%i,\"x2\":%i,\"y2\":%i}", x, y, x, 0);

			added_connection = true;
		}

		for (uint32_t i = 0; i < circuit->output_signal_count; i++) {
			int x = (circuit->input_signal_count + i) * 2;
			int y = i * 6 - circuit->output_signal_count * 6 + 2;

			if (added_connection) {
				fprintf(f, ",");
			}
			fprintf(f, "{\"x1\":%i,\"y1\":%i,\"x2\":%i,\"y2\":%i}", x, y, (circuit->input_signal_count + circuit->output_signal_count) * 2, y);

			fprintf(f, ",");
			fprintf(f, "{\"x1\":%i,\"y1\":%i,\"x2\":%i,\"y2\":%i}", x, y, x, 0);

			added_connection = true;
		}
	}

    fprintf(f, "]");
    fprintf(f, "}");

    fprintf(f, "\n");

    return 0;
}

static int generate_block(struct intermediate_block *block, struct options *options, struct circuit *circuit) {
	int ret;

	uint32_t io_signal_count = block->input_signals + block->output_signals;

	uint32_t public_base_signal = circuit_allocate_signals(circuit, io_signal_count);

	uint32_t *nested_blocks_base_signals = block->block_count > 0 ? xcalloc(block->block_count, sizeof(uint32_t)) : 0;

	for (uint32_t i = 0; i < block->block_count; i++) {
		struct intermediate_block *nested_block = block->blocks[i];

		nested_blocks_base_signals[i] = circuit->signal_count;

		ret = generate_block(nested_block, options, circuit);
		if (ret) {
			return ret;
		}
	}

	uint32_t local_base_signal = circuit->signal_count;

	uint32_t local_signal_count = 0;

	// Inputs and outputs
	for (uint32_t i = 0; i < io_signal_count; i++) {
		struct component *connector = component_create();
		connector->type = COMPONENT_TYPE_CONNECT;
		component_add_input(connector, public_base_signal + i);
		component_add_output(connector, local_base_signal + i);
		circuit_add_component(circuit, connector);
		
		local_signal_count++;
	}

	// Blocks
	for (uint32_t i = 0; i < block->block_count; i++) {
		struct intermediate_block *nested_block = block->blocks[i];
		uint32_t nested_block_io_signal_count = nested_block->input_signals + nested_block->output_signals;

		uint32_t nested_block_base_signal = nested_blocks_base_signals[i];

		for (uint32_t j = 0; j < nested_block_io_signal_count; j++) {
			struct component *connector = component_create();
			connector->type = COMPONENT_TYPE_CONNECT;
			component_add_input(connector, nested_block_base_signal + j);
			component_add_output(connector, local_base_signal + local_signal_count);
			circuit_add_component(circuit, connector);
		
			local_signal_count++;
		}
	}

	xfree(nested_blocks_base_signals);

	// Local signals
	for (uint32_t i = 0; i < block->statement_count; i++) {
		struct intermediate_statement *stmt = &block->statements[i];

		for (uint32_t j = 0; j < stmt->input_count; j++) {
			uint32_t input_signal_id = stmt->inputs[j];
			if (input_signal_id >= local_signal_count) {
				local_signal_count = input_signal_id + 1;
			}
		}

		for (uint32_t j = 0; j < stmt->output_count; j++) {
			uint32_t output_signal_id = stmt->outputs[j];
			if (output_signal_id >= local_signal_count) {
				local_signal_count = output_signal_id + 1;
			}
		}
	}

	circuit_allocate_signals(circuit, local_signal_count);

	for (uint32_t i = 0; i < block->statement_count; i++) {
		struct intermediate_statement *stmt = &block->statements[i];

		struct component *component = component_create();

		switch (stmt->op) {
			case INTERMEDIATE_OP_CONNECT:
				component->type = COMPONENT_TYPE_CONNECT;
				break;
			case INTERMEDIATE_OP_CONST_0:
				component->type = COMPONENT_TYPE_CONST_0;
				break;
			case INTERMEDIATE_OP_CONST_1:
				component->type = COMPONENT_TYPE_CONST_1;
				break;
			case INTERMEDIATE_OP_AND:
				component->type = COMPONENT_TYPE_AND;
				break;
			case INTERMEDIATE_OP_OR:
				component->type = COMPONENT_TYPE_OR;
				break;
			case INTERMEDIATE_OP_XOR:
				component->type = COMPONENT_TYPE_XOR;
				break;
			case INTERMEDIATE_OP_NOT:
				component->type = COMPONENT_TYPE_NOT;
				break;
			default:
				fprintf(stderr, "error: unsupported op 0x%04x in intermediate statement\n", stmt->op);
				return -1;
		}

		for (uint32_t j = 0; j < stmt->input_count; j++) {
			component_add_input(component, local_base_signal + stmt->inputs[j]);
		}

		for (uint32_t j = 0; j < stmt->output_count; j++) {
			component_add_output(component, local_base_signal + stmt->outputs[j]);
		}

		circuit_add_component(circuit, component);
	}

    return 0;
}

static int generate_circuit(FILE *f, struct intermediate_block **blocks, uint32_t block_count, struct options *options) {
	int ret;

	struct intermediate_block *main_block = NULL;

	for (uint32_t i = 0; i < block_count; i++) {
		if (strcmp(blocks[i]->name, "main") == 0) {
			main_block = blocks[i];
			break;
		}
	}

	if (main_block == NULL) {
		fprintf(stderr, "error: cannot find main block\n");
		return 1;
	}

	struct circuit *circuit = circuit_create();

	circuit->input_signal_count = main_block->input_signals;
	circuit->output_signal_count = main_block->output_signals;

	ret = generate_block(main_block, options, circuit);
	if (ret) {
		return ret;
	}

	return output_circuit(f, circuit, options);
}

int backend_LogicSimulator_run(const char *output_filename, struct intermediate_file *intermediate_file, int argc, char **argv) {
	int ret;

	struct options options = {
		.io_components = false
	};

	for (int i = 0; i < argc; i++) {
		char *arg = argv[i];
		if (strcmp(arg, "--generate-io-components") == 0) {
			options.io_components = true;
		} else {
			fprintf(stderr, "error: unrecognized backend option '%s'\n", arg);
			return 1;
		}
	}

	if (output_filename == NULL) {
		output_filename = "circuit.json";
	}

	FILE *output_file = fopen(output_filename, "wb");
	if (output_file == NULL) {
		return 1;
	}

	ret = generate_circuit(output_file, intermediate_file->blocks, intermediate_file->block_count, &options);
	if (ret) {
		return ret;
	}

	fclose(output_file);

	return 0;
}
