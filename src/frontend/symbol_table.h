
#pragma once

#include <stdlib.h>
#include <stdio.h>

#include "symbol.h"

struct symbol_table {
	struct symbol_table *parent;
	struct hashtable *symbols;
};

struct symbol_table *symbol_table_create();
void symbol_table_destroy(struct symbol_table *symbol_table);
void symbol_table_print(FILE *stream, struct symbol_table *symbol_table);

void symbol_table_add(struct symbol_table *symbol_table, struct symbol *symbol);
struct symbol *symbol_table_find(struct symbol_table *symbol_table, char *name);
struct symbol *symbol_table_find_recursive(struct symbol_table *symbol_table, char *name);
