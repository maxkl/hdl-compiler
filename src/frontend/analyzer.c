
#include "analyzer.h"

#include <stdio.h>
#include <stdbool.h>

#include "ast.h"
#include "symbol_table.h"
#include "expression_type.h"

#include <shared/helper/mem.h>

static int analyze_expression(struct symbol_table *symbol_table, struct ast_node *expression, struct expression_type *type);
static int analyze_behaviour_identifier(struct symbol_table *symbol_table, struct ast_node *behaviour_identifier, struct expression_type *type);

static int analyze_binary_expression(struct symbol_table *symbol_table, struct ast_node *expression, struct expression_type *type) {
	int ret;

	if (expression->type != AST_BINARY_EXPRESSION) {
		return -1;
	}

	if (expression->child_count != 2) {
		return -1;
	}

	struct expression_type type_a, type_b;

	//printf("(");

	ret = analyze_expression(symbol_table, expression->children[0], &type_a);
	if (ret) {
		return ret;
	}

	if (type_a.access_type != EXPRESSION_TYPE_READ) {
		fprintf(stderr, "Write-only signal used as source operand\n");
		return -1;
	}

	switch (expression->data.op) {
		case AST_OP_AND:
			//printf(" & ");
			break;
		case AST_OP_OR:
			//printf(" | ");
			break;
		case AST_OP_XOR:
			//printf(" ^ ");
			break;
		default:
			return -1;
	}

	ret = analyze_expression(symbol_table, expression->children[1], &type_b);
	if (ret) {
		return ret;
	}

	if (type_b.access_type != EXPRESSION_TYPE_READ) {
		fprintf(stderr, "Write-only signal used as source operand\n");
		return -1;
	}

	//printf(")");

	if (!expression_type_matches(&type_a, &type_b)) {
		fprintf(stderr, "Operand types of binary expression don't match\n");
		return -1;
	}

	expression_type_copy(type, &type_a);

	return 0;
}

static int analyze_unary_expression(struct symbol_table *symbol_table, struct ast_node *expression, struct expression_type *type) {
		int ret;

	if (expression->type != AST_UNARY_EXPRESSION) {
		return -1;
	}

	if (expression->child_count != 1) {
		return -1;
	}

	//printf("(");

	switch (expression->data.op) {
		case AST_OP_NOT:
			//printf("~");
			break;
		default:
			return -1;
	}

	ret = analyze_expression(symbol_table, expression->children[0], type);
	if (ret) {
		return ret;
	}

	if (type->access_type != EXPRESSION_TYPE_READ) {
		fprintf(stderr, "Write-only signal used as source operand\n");
		return -1;
	}

	//printf(")");

	return 0;
}

static int analyze_expression(struct symbol_table *symbol_table, struct ast_node *expression, struct expression_type *type) {
	int ret;

	switch (expression->type) {
		case AST_BINARY_EXPRESSION:
            ret = analyze_binary_expression(symbol_table, expression, type);
            if (ret) {
                return ret;
            }

            break;
		case AST_UNARY_EXPRESSION:
        ret = analyze_unary_expression(symbol_table, expression, type);
            if (ret) {
                return ret;
            }

            break;
		case AST_BEHAVIOUR_IDENTIFIER:
			ret = analyze_behaviour_identifier(symbol_table, expression, type);
			if (ret) {
				return ret;
			}

			if (type->access_type != EXPRESSION_TYPE_READ) {
				fprintf(stderr, "Write-only signal used as source operand\n");
				return -1;
			}

			break;
		case AST_NUMBER:
			type->access_type = EXPRESSION_TYPE_READ;
			type->width = 64;
			//printf("%lu", expression->data.number);
			break;
		default:
			return -1;
	}

    struct expression_type *expression_type = xmalloc(sizeof(struct expression_type));
    expression_type_copy(expression_type, type);
    expression->semantic_data.expression_type = expression_type;

    return 0;
}

static int analyze_subscript(struct symbol_table *symbol_table, struct ast_node *subscript, uint64_t *start_index_out, uint64_t *end_index_out) {
	int ret;

	if (subscript->type != AST_SUBSCRIPT) {
		return -1;
	}

	if (subscript->child_count != 2) {
		return -1;
	}

	uint64_t start_index, end_index;

	struct ast_node *start_number = subscript->children[0];
    if (start_number->type != AST_NUMBER) {
        return -1;
    }

    start_index = start_number->data.number;

    struct ast_node *end_number = subscript->children[1];
    if (end_number == NULL) {
    	end_index = start_index;
    } else {
	    if (end_number->type != AST_NUMBER) {
	        return -1;
	    }

    	end_index = end_number->data.number;
	}

	if (end_index > start_index) {
		fprintf(stderr, "Invalid subscript range: end before start\n");
		return -1;
	}

    subscript->semantic_data.subscript.start_index = start_index;
    subscript->semantic_data.subscript.end_index = end_index;

    *start_index_out = start_index;
    *end_index_out = end_index;

	return 0;
}

static int analyze_behaviour_identifier(struct symbol_table *symbol_table, struct ast_node *behaviour_identifier, struct expression_type *type) {
	int ret;

    if (behaviour_identifier->type != AST_BEHAVIOUR_IDENTIFIER) {
        return -1;
    }

    if (behaviour_identifier->child_count != 3) {
        return -1;
    }

    struct ast_node *identifier = behaviour_identifier->children[0];
    if (identifier->type != AST_IDENTIFIER) {
        return -1;
    }

    char *signal_name = identifier->data.identifier;
    struct symbol *symbol = symbol_table_find_recursive(symbol_table, signal_name);

    if (symbol == NULL) {
    	fprintf(stderr, "Use of undeclared identifier \"%s\"\n", signal_name);
    	return -1;
    }

    struct symbol_type *symbol_type = symbol->type;

    //printf("((");
    //symbol_type_print(stdout, symbol_type);
    //printf(") %s)", signal_name);

    struct ast_node *property_identifier = behaviour_identifier->children[1];
    if (property_identifier == NULL) {
    	//
    } else {
    	if (property_identifier->type != AST_IDENTIFIER) {
        	return -1;
        }

    	//printf(".%s", property_identifier->data.identifier);

    	if (symbol_type->type != SYMBOL_TYPE_BLOCK) {
    		fprintf(stderr, "Property access on signal\n");
    		return -1;
    	}

    	fprintf(stderr, "Property access not supported\n");
    	return -1;
    }

    switch (symbol_type->type) {
    	case SYMBOL_TYPE_IN:
    		type->access_type = EXPRESSION_TYPE_READ;
    		break;
    	case SYMBOL_TYPE_OUT:
    		type->access_type = EXPRESSION_TYPE_WRITE;
    		break;
    	case SYMBOL_TYPE_BLOCK:
    		fprintf(stderr, "Block used as signal\n");
    		return -1;
    }

    struct ast_node *subscript = behaviour_identifier->children[2];
    if (subscript == NULL) {
    	type->width = symbol_type->width;
    } else {
        uint64_t start_index, end_index;

	    ret = analyze_subscript(symbol_table, subscript, &start_index, &end_index);
	    if (ret) {
	        return ret;
	    }

        if (start_index >= symbol_type->width) {
            fprintf(stderr, "Subscript index %lu exceeds type width %lu\n", start_index, symbol_type->width);
            return -1;
        }

        type->width = start_index - end_index + 1;
	}

	return 0;
}

static int analyze_behaviour_statement(struct symbol_table *symbol_table, struct ast_node *behaviour_statement) {
    int ret;

    if (behaviour_statement->type != AST_BEHAVIOUR_STATEMENT) {
        return -1;
    }

    if (behaviour_statement->child_count != 2) {
        return -1;
    }

    struct expression_type target_type;

    ret = analyze_behaviour_identifier(symbol_table, behaviour_statement->children[0], &target_type);
    if (ret) {
        return ret;
    }

    if (target_type.access_type != EXPRESSION_TYPE_WRITE) {
    	fprintf(stderr, "Read-only signal used as target operand\n");
    	return -1;
    }

    //printf(" = ");

    struct expression_type source_type;

    ret = analyze_expression(symbol_table, behaviour_statement->children[1], &source_type);
    if (ret) {
        return ret;
    }

    if (source_type.access_type != EXPRESSION_TYPE_READ) {
    	fprintf(stderr, "Write-only signal used as source operand\n");
    	return -1;
    }

    //printf(";\n");

    if (target_type.width != source_type.width) {
		fprintf(stderr, "Operand types of assignment expression don't match\n");
		return -1;
	}

    struct expression_type *expression_type = xmalloc(sizeof(struct expression_type));
    expression_type_copy(expression_type, &source_type);
    behaviour_statement->semantic_data.expression_type = expression_type;

    return 0;
}

static int analyze_behaviour_statements(struct symbol_table *symbol_table, struct ast_node *behaviour_statements) {
    int ret;

    if (behaviour_statements->type == AST_BEHAVIOUR_STATEMENT) {
        return analyze_behaviour_statement(symbol_table, behaviour_statements);
    }

    if (behaviour_statements->type != AST_BEHAVIOUR_STATEMENTS) {
        return -1;
    }

    if (behaviour_statements->child_count != 2) {
        return -1;
    }

    ret = analyze_behaviour_statements(symbol_table, behaviour_statements->children[0]);
    if (ret) {
        return ret;
    }

    ret = analyze_behaviour_statement(symbol_table, behaviour_statements->children[1]);
    if (ret) {
        return ret;
    }

    return 0;
}

static int analyze_declaration_identifier(struct symbol_table *symbol_table, struct ast_node *identifier, struct symbol_type *symbol_type) {
	int ret;

	if (identifier->type != AST_IDENTIFIER) {
		return -1;
	}

	char *name = identifier->data.identifier;

	if (symbol_table_find(symbol_table, name)) {
		fprintf(stderr, "Duplicate declaration of signal \"%s\"\n", name);
		return -1;
	}

	struct symbol *symbol = symbol_create(name, symbol_type);

	//symbol_print(stdout, symbol);
	//printf(";\n");

	symbol_table_add(symbol_table, symbol);

	return 0;
}

static int analyze_declaration_identifier_list(struct symbol_table *symbol_table, struct ast_node *identifier_list, struct symbol_type *symbol_type) {
	int ret;

	if (identifier_list->type == AST_IDENTIFIER) {
		return analyze_declaration_identifier(symbol_table, identifier_list, symbol_type);
	}

	if (identifier_list->type != AST_IDENTIFIER_LIST) {
		return -1;
	}

	if (identifier_list->child_count != 2) {
		return -1;
	}

	ret = analyze_declaration_identifier_list(symbol_table, identifier_list->children[0], symbol_type);
	if (ret) {
		return -1;
	}

	ret = analyze_declaration_identifier(symbol_table, identifier_list->children[1], symbol_type);
	if (ret) {
		return -1;
	}

	return 0;
}

static int analyze_type(struct symbol_table *symbol_table, struct ast_node *type, struct symbol_type **symbol_type_out) {
	int ret;

	if (type->type != AST_TYPE) {
		return -1;
	}

	if (type->child_count != 2) {
		return -1;
	}

	struct symbol_type *symbol_type;

	struct ast_node *type_specifier = type->children[0];
    switch (type_specifier->type) {
    	case AST_TYPE_SPECIFIER_IN:
    		symbol_type = symbol_type_create(SYMBOL_TYPE_IN, 0);
    		break;
		case AST_TYPE_SPECIFIER_OUT:
    		symbol_type = symbol_type_create(SYMBOL_TYPE_OUT, 0);
    		break;
    	case AST_TYPE_SPECIFIER_BLOCK:
    		if (type_specifier->child_count != 1) {
    			return -1;
    		}
    		
    		if (type_specifier->children[0]->type != AST_IDENTIFIER) {
    			return -1;
    		}

    		symbol_type = symbol_type_create(SYMBOL_TYPE_BLOCK, 0);
    		symbol_type->data.block_name = xstrdup(type_specifier->children[0]->data.identifier);

    		break;
    	default:
    		return -1;
    }

    struct ast_node *type_width = type->children[1];
    if (type_width == NULL) {
    	symbol_type->width = 1;
    } else {
	    if (type_width->type != AST_NUMBER) {
	    	return -1;
	    }

	    uint64_t width = type_width->data.number;

	    if (width == 0) {
	    	fprintf(stderr, "Signal declared with invalid width of 0\n");
	    	return -1;
	    }

	    symbol_type->width = width;
	}

    *symbol_type_out = symbol_type;

    return 0;
}

static int analyze_declaration(struct symbol_table *symbol_table, struct ast_node *declaration) {
    int ret;

    if (declaration->type != AST_DECLARATION) {
        return -1;
    }

    if (declaration->child_count != 2) {
    	return -1;
    }

    struct symbol_type *symbol_type;

    ret = analyze_type(symbol_table, declaration->children[0], &symbol_type);
    if (ret) {
    	return ret;
    }

    ret = analyze_declaration_identifier_list(symbol_table, declaration->children[1], symbol_type);
    symbol_type_destroy(symbol_type);
    if (ret) {
    	return ret;
    }

    return 0;
}

static int analyze_declarations(struct symbol_table *symbol_table, struct ast_node *declarations) {
    int ret;

    if (declarations->type == AST_DECLARATION) {
        return analyze_declaration(symbol_table, declarations);
    }

    if (declarations->type != AST_DECLARATIONS) {
        return -1;
    }

    if (declarations->child_count != 2) {
        return -1;
    }

    ret = analyze_declarations(symbol_table, declarations->children[0]);
    if (ret) {
        return ret;
    }

    ret = analyze_declaration(symbol_table, declarations->children[1]);
    if (ret) {
        return ret;
    }

    return 0;
}

static int analyze_block(struct symbol_table *symbol_table, struct ast_node *block) {
    int ret;

    if (block->type != AST_BLOCK) {
        return -1;
    }

    if (block->child_count != 3) {
        return -1;
    }

    if (block->children[0]->type != AST_IDENTIFIER) {
        return -1;
    }

    block->semantic_data.symbol_table = symbol_table_create(symbol_table);

    ret = analyze_declarations(block->semantic_data.symbol_table, block->children[1]);
    if (ret) {
        return ret;
    }

    ret = analyze_behaviour_statements(block->semantic_data.symbol_table, block->children[2]);
    if (ret) {
        return ret;
    }

    return 0;
}

static int analyze_blocks(struct symbol_table *symbol_table, struct ast_node *blocks) {
    int ret;

    if (blocks->type == AST_BLOCK) {
        return analyze_block(symbol_table, blocks);
    }

    if (blocks->type != AST_BLOCKS) {
        return -1;
    }

    if (blocks->child_count != 2) {
        return -1;
    }

    ret = analyze_blocks(symbol_table, blocks->children[0]);
    if (ret) {
        return ret;
    }

    ret = analyze_block(symbol_table, blocks->children[1]);
    if (ret) {
        return ret;
    }

    return 0;
}

int analyzer_analyze(struct ast_node *ast_root) {
	int ret;
    
    ret = analyze_blocks(NULL, ast_root);
    if (ret) {
    	return -1;
    }

    return 0;
}
