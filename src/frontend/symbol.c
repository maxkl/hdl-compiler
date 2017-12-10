
#include "symbol.h"

#include <string.h>

#include <shared/helper/mem.h>

struct symbol_type *symbol_type_create(enum symbol_type_type type, uint64_t width) {
	struct symbol_type *symbol_type = xcalloc(1, sizeof(struct symbol_type));
	symbol_type->type = type;
	symbol_type->width = width;
	return symbol_type;
}

void symbol_type_destroy(struct symbol_type *symbol_type) {
	xfree(symbol_type);
}

void symbol_type_print(FILE *stream, struct symbol_type *symbol_type) {
	switch (symbol_type->type) {
		case SYMBOL_TYPE_IN:
			fprintf(stream, "in");
			break;
		case SYMBOL_TYPE_OUT:
			fprintf(stream, "out");
			break;
		case SYMBOL_TYPE_BLOCK:
			fprintf(stream, "block %s", symbol_type->data.block->data.identifier);
			break;
	}

	if (symbol_type->width != 1) {
		fprintf(stream, "[%lu]", symbol_type->width);
	}
}

struct symbol *symbol_create(char *name, struct symbol_type *type) {
	struct symbol *symbol = xmalloc(sizeof(struct symbol));
	symbol->name = xstrdup(name);
	symbol->type = xmalloc(sizeof(struct symbol_type));
	memcpy(symbol->type, type, sizeof(struct symbol_type));
	return symbol;
}

void symbol_destroy(struct symbol *symbol) {
	xfree(symbol->name);
	xfree(symbol->type);
	xfree(symbol);
}

void symbol_print(FILE *stream, struct symbol *symbol) {
	symbol_type_print(stream, symbol->type);
	fprintf(stream, " %s", symbol->name);
}
