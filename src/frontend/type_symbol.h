
#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include "ast.h"

enum type_symbol_type {
	TYPE_SYMBOL_BLOCK
};

struct type_symbol {
	char *name;
	enum type_symbol_type type;
	union {
		struct ast_node *block;
	} data;
};

struct type_symbol *type_symbol_create(char *name, enum type_symbol_type type);
void type_symbol_destroy(struct type_symbol *type_symbol);
void type_symbol_print(FILE *stream, struct type_symbol *type_symbol);
