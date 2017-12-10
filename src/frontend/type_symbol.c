
#include "type_symbol.h"

#include <string.h>

#include <shared/helper/mem.h>

static const char *type_symbol_type_names[] = {
	"block"
};

struct type_symbol *type_symbol_create(char *name, enum type_symbol_type type) {
	struct type_symbol *type_symbol = xmalloc(sizeof(struct type_symbol));
	type_symbol->name = xstrdup(name);
	type_symbol->type = type;
	return type_symbol;
}

void type_symbol_destroy(struct type_symbol *type_symbol) {
	xfree(type_symbol->name);
	xfree(type_symbol);
}

void type_symbol_print(FILE *stream, struct type_symbol *type_symbol) {
	fprintf(stream, "%s %s", type_symbol_type_names[type_symbol->type], type_symbol->name);
}
