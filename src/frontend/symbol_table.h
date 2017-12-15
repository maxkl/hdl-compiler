
#pragma once

#include <stdlib.h>
#include <stdio.h>

#include "symbol.h"
#include "type_symbol.h"

struct symbol_table {
	struct symbol_table *parent;
	struct hashtable *symbols;
	struct hashtable *types;
	unsigned symbol_insertion_index;
};

struct symbol_table *symbol_table_create();
void symbol_table_destroy(struct symbol_table *symbol_table);
void symbol_table_print(FILE *stream, struct symbol_table *symbol_table);

void symbol_table_add(struct symbol_table *symbol_table, struct symbol *symbol);
void symbol_table_add_type(struct symbol_table *symbol_table, struct type_symbol *symbol);
struct symbol *symbol_table_find(struct symbol_table *symbol_table, char *name);
struct symbol *symbol_table_find_recursive(struct symbol_table *symbol_table, char *name);
struct type_symbol *symbol_table_find_type(struct symbol_table *symbol_table, char *name);
struct type_symbol *symbol_table_find_type_recursive(struct symbol_table *symbol_table, char *name);
