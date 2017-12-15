
#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include "ast.h"

enum symbol_type_type {
	SYMBOL_TYPE_IN,
	SYMBOL_TYPE_OUT,
	SYMBOL_TYPE_BLOCK
};

struct symbol_type {
	enum symbol_type_type type;
	uint64_t width;
	union {
		struct ast_node *block;
	} data;
};

struct symbol {
	char *name;
	struct symbol_type *type;
	uint32_t signal;
	unsigned insertion_order;
};

struct symbol_type *symbol_type_create(enum symbol_type_type type, uint64_t width);
void symbol_type_destroy(struct symbol_type *symbol_type);
void symbol_type_print(FILE *stream, struct symbol_type *symbol_type);

struct symbol *symbol_create(char *name, struct symbol_type *type);
void symbol_destroy(struct symbol *symbol);
void symbol_print(FILE *stream, struct symbol *symbol);
