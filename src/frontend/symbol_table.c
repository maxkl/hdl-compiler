
#include "symbol_table.h"

#include <shared/helper/mem.h>
#include <shared/helper/hashtable.h>

struct symbol_table *symbol_table_create(struct symbol_table *parent) {
	struct symbol_table *symbol_table = malloc(sizeof(struct symbol_table));
	symbol_table->parent = parent;
	symbol_table->symbols = hashtable_create(hashtable_hash_string, hashtable_compare_string, NULL);
	symbol_table->types = hashtable_create(hashtable_hash_string, hashtable_compare_string, NULL);
	return symbol_table;
}

void symbol_table_destroy(struct symbol_table *symbol_table) {
	(void) symbol_table;
}

void symbol_table_add(struct symbol_table *symbol_table, struct symbol *symbol) {
	hashtable_set(symbol_table->symbols, symbol->name, symbol);
}

void symbol_table_add_type(struct symbol_table *symbol_table, struct type_symbol *symbol) {
	hashtable_set(symbol_table->types, symbol->name, symbol);
}

struct symbol *symbol_table_find(struct symbol_table *symbol_table, char *name) {
	return hashtable_get(symbol_table->symbols, name);
}

struct symbol *symbol_table_find_recursive(struct symbol_table *symbol_table, char *name) {
	struct symbol *symbol = NULL;
	while (symbol == NULL && symbol_table != NULL) {
		symbol = symbol_table_find(symbol_table, name);
		symbol_table = symbol_table->parent;
	}
	return symbol;
}

struct type_symbol *symbol_table_find_type(struct symbol_table *symbol_table, char *name) {
	return hashtable_get(symbol_table->types, name);
}

struct type_symbol *symbol_table_find_type_recursive(struct symbol_table *symbol_table, char *name) {
	struct type_symbol *symbol = NULL;
	while (symbol == NULL && symbol_table != NULL) {
		symbol = symbol_table_find_type(symbol_table, name);
		symbol_table = symbol_table->parent;
	}
	return symbol;
}

void symbol_table_print(FILE *stream, struct symbol_table *symbol_table) {
	struct hashtable_iterator it;
	for (hashtable_iterator_init(&it, symbol_table->symbols); !hashtable_iterator_finished(&it); hashtable_iterator_next(&it)) {
		struct symbol *symbol = hashtable_iterator_value(&it);

		symbol_print(stream, symbol);
		fprintf(stream, "\n");
	}
}
