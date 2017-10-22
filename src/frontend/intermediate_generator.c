
#include "intermediate_generator.h"

#include <stdio.h>
#include <stdbool.h>

#include "ast.h"
#include "symbol_table.h"
#include "expression_type.h"

#include <shared/helper/mem.h>
#include <shared/helper/hashtable.h>

static int generate_expression(struct ast_node *expression, struct symbol_table *symbol_table, uint32_t *signal_out_out, struct intermediate_block *block);
static int generate_behaviour_identifier(struct ast_node *behaviour_identifier, struct symbol_table *symbol_table, uint32_t *signal_out_out);

static int generate_binary_expression(struct ast_node *expression, struct symbol_table *symbol_table, uint32_t *signal_out_out, struct intermediate_block *block) {
	int ret;

    struct expression_type *expression_type = expression->semantic_data.expression_type;
    uint64_t expression_width = expression_type->width;

	uint32_t signal_a, signal_b;

	ret = generate_expression(expression->children[0], symbol_table, &signal_a, block);
	if (ret) {
		return ret;
	}

	ret = generate_expression(expression->children[1], symbol_table, &signal_b, block);
	if (ret) {
		return ret;
	}

    uint32_t signal_out = intermediate_block_allocate_signals(block, expression_width);

    switch (expression->data.op) {
        case AST_OP_AND:
            for (uint32_t i = 0; i < expression_width; i++) {
                struct intermediate_statement *stmt = intermediate_block_add_statement(block, INTERMEDIATE_OP_AND, 2);
                intermediate_statement_set_input(stmt, 0, signal_a + i);
                intermediate_statement_set_input(stmt, 1, signal_b + i);
                intermediate_statement_set_output(stmt, 0, signal_out + i);
            }
            break;
        case AST_OP_OR:
            for (uint32_t i = 0; i < expression_width; i++) {
                struct intermediate_statement *stmt = intermediate_block_add_statement(block, INTERMEDIATE_OP_OR, 2);
                intermediate_statement_set_input(stmt, 0, signal_a + i);
                intermediate_statement_set_input(stmt, 1, signal_b + i);
                intermediate_statement_set_output(stmt, 0, signal_out + i);
            }
            break;
        case AST_OP_XOR:
            for (uint32_t i = 0; i < expression_width; i++) {
                struct intermediate_statement *stmt = intermediate_block_add_statement(block, INTERMEDIATE_OP_XOR, 2);
                intermediate_statement_set_input(stmt, 0, signal_a + i);
                intermediate_statement_set_input(stmt, 1, signal_b + i);
                intermediate_statement_set_output(stmt, 0, signal_out + i);
            }
            break;
        default:
            return -1;
    }

	*signal_out_out = signal_out;

	return 0;
}

static int generate_unary_expression(struct ast_node *expression, struct symbol_table *symbol_table, uint32_t *signal_out_out, struct intermediate_block *block) {
	int ret;

    struct expression_type *expression_type = expression->semantic_data.expression_type;
    uint64_t expression_width = expression_type->width;

    uint32_t signal;

	ret = generate_expression(expression->children[0], symbol_table, &signal, block);
	if (ret) {
		return ret;
	}

    uint32_t signal_out = intermediate_block_allocate_signals(block, expression_width);

    switch (expression->data.op) {
        case AST_OP_NOT:
            for (uint32_t i = 0; i < expression_width; i++) {
                struct intermediate_statement *stmt = intermediate_block_add_statement(block, INTERMEDIATE_OP_NOT, 1);
                intermediate_statement_set_input(stmt, 0, signal + i);
                intermediate_statement_set_output(stmt, 0, signal_out + i);
            }
            break;
        default:
            return -1;
    }

    *signal_out_out = signal_out;

	return 0;
}

static int generate_constant(uint64_t value, uint64_t width, uint32_t *signal_out_out, struct intermediate_block *block) {
    int ret;

    uint32_t signal_out = intermediate_block_allocate_signals(block, width);

    for (uint32_t i = 0; i < width; i++) {
        bool bit = value & (1 << i);
        struct intermediate_statement *stmt = intermediate_block_add_statement(block, bit ? INTERMEDIATE_OP_CONST_1 : INTERMEDIATE_OP_CONST_0, 1);
        intermediate_statement_set_output(stmt, 0, signal_out + i);
    }

    *signal_out_out = signal_out;

    return 0;
}

static int generate_expression(struct ast_node *expression, struct symbol_table *symbol_table, uint32_t *signal_out_out, struct intermediate_block *block) {
	int ret;

	switch (expression->type) {
		case AST_BINARY_EXPRESSION:
			return generate_binary_expression(expression, symbol_table, signal_out_out, block);
		case AST_UNARY_EXPRESSION:
			return generate_unary_expression(expression, symbol_table, signal_out_out, block);
		case AST_BEHAVIOUR_IDENTIFIER:
			return generate_behaviour_identifier(expression, symbol_table, signal_out_out);
		case AST_NUMBER:
            return generate_constant(expression->data.number.value, expression->data.number.width, signal_out_out, block);
		default:
			return -1;
	}
}

static int generate_behaviour_identifier(struct ast_node *behaviour_identifier, struct symbol_table *symbol_table, uint32_t *signal_out) {
	int ret;

    struct ast_node *identifier = behaviour_identifier->children[0];

    char *signal_name = identifier->data.identifier;
    struct symbol *symbol = symbol_table_find_recursive(symbol_table, signal_name);
    struct symbol_type *symbol_type = symbol->type;

    uint32_t signal = symbol->signal;

    struct ast_node *property_identifier = behaviour_identifier->children[1];
    if (property_identifier == NULL) {
    	//
    } else {
    	if (property_identifier->type != AST_IDENTIFIER) {
        	return -1;
        }

    	fprintf(stderr, "Property access not supported\n");
    	return -1;
    }

    struct ast_node *subscript = behaviour_identifier->children[2];
    if (subscript == NULL) {
    	//
    } else {
        signal += subscript->semantic_data.subscript.end_index;
	}

    *signal_out = signal;

	return 0;
}

static int generate_behaviour_statement(struct ast_node *behaviour_statement, struct symbol_table *symbol_table, struct intermediate_block *block) {
    int ret;

    struct expression_type *expression_type = behaviour_statement->semantic_data.expression_type;
    uint64_t expression_width = expression_type->width;

    uint32_t target_signal;

    ret = generate_behaviour_identifier(behaviour_statement->children[0], symbol_table, &target_signal);
    if (ret) {
        return ret;
    }

    uint32_t source_signal;

    ret = generate_expression(behaviour_statement->children[1], symbol_table, &source_signal, block);
    if (ret) {
        return ret;
    }

    for (uint32_t i = 0; i < expression_width; i++) {
        struct intermediate_statement *stmt = intermediate_block_add_statement(block, INTERMEDIATE_OP_CONNECT, 1);
        intermediate_statement_set_input(stmt, 0, source_signal + i);
        intermediate_statement_set_output(stmt, 0, target_signal + i);
    }

    return 0;
}

static int generate_behaviour_statements(struct ast_node *behaviour_statements, struct symbol_table *symbol_table, struct intermediate_block *block) {
    int ret;

    if (behaviour_statements->type == AST_BEHAVIOUR_STATEMENT) {
        return generate_behaviour_statement(behaviour_statements, symbol_table, block);
    }

    ret = generate_behaviour_statements(behaviour_statements->children[0], symbol_table, block);
    if (ret) {
        return ret;
    }

    ret = generate_behaviour_statement(behaviour_statements->children[1], symbol_table, block);
    if (ret) {
        return ret;
    }

    return 0;
}

static int generate_block(struct ast_node *block, struct intermediate_block ***blocks, size_t *block_count) {
    int ret;

    struct symbol_table *symbol_table = block->semantic_data.symbol_table;

    struct ast_node *identifier = block->children[0];

    struct intermediate_block *intermediate_block = intermediate_block_create(identifier->data.identifier);

    struct hashtable_iterator it;
    for (hashtable_iterator_init(&it, symbol_table->symbols); !hashtable_iterator_finished(&it); hashtable_iterator_next(&it)) {
        struct symbol *symbol = hashtable_iterator_value(&it);
        struct symbol_type *symbol_type = symbol->type;

        if (symbol_type->type == SYMBOL_TYPE_IN) {
            symbol->signal = intermediate_block_add_input(intermediate_block, symbol->name, symbol_type->width);
        }
    }

    for (hashtable_iterator_init(&it, symbol_table->symbols); !hashtable_iterator_finished(&it); hashtable_iterator_next(&it)) {
        struct symbol *symbol = hashtable_iterator_value(&it);
        struct symbol_type *symbol_type = symbol->type;

        if (symbol_type->type == SYMBOL_TYPE_OUT) {
            symbol->signal = intermediate_block_add_output(intermediate_block, symbol->name, symbol_type->width);
        }
    }

    ret = generate_behaviour_statements(block->children[2], symbol_table, intermediate_block);
    if (ret) {
        intermediate_block_destroy(intermediate_block);
        return ret;
    }

    (*block_count)++;
    *blocks = xrealloc(*blocks, sizeof(struct intermediate_block *) * *block_count);
    (*blocks)[*block_count - 1] = intermediate_block;

    return 0;
}

static int generate_blocks(struct ast_node *blocks, struct intermediate_block ***intermediate_blocks, size_t *block_count) {
    int ret;

    if (blocks->type == AST_BLOCK) {
        return generate_block(blocks, intermediate_blocks, block_count);
    }

    ret = generate_blocks(blocks->children[0], intermediate_blocks, block_count);
    if (ret) {
        return ret;
    }

    ret = generate_block(blocks->children[1], intermediate_blocks, block_count);
    if (ret) {
        return ret;
    }

    return 0;
}

int intermediate_generator_generate(struct ast_node *ast_root, struct intermediate_file *intermediate_file) {
	int ret;

    struct intermediate_block **blocks = NULL;
    size_t block_count = 0;
    
    ret = generate_blocks(ast_root, &blocks, &block_count);
    if (ret) {
    	return -1;
    }

    // struct intermediate_block *block = intermediate_block_create("peter");
    // intermediate_block_add_input(block, "a", 1);
    // intermediate_block_add_input(block, "b", 1);
    // intermediate_block_add_output(block, "q", 1);
    // uint32_t index;
    // struct intermediate_statement *stmt = intermediate_block_add_statement(block, INTERMEDIATE_OP_AND, 2, &index);
    // intermediate_statement_set_input(stmt, 0, 0);
    // intermediate_statement_set_input(stmt, 1, 1);
    // intermediate_statement_set_output(stmt, 0, 2);

    // *blocks = xmalloc(sizeof(struct intermediate_block *) * 1);
    // (*blocks)[0] = block;
    // *block_count = 1;

    intermediate_file->blocks = blocks;
    intermediate_file->block_count = block_count;

    return 0;
}
