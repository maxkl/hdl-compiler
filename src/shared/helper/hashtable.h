
#pragma once

#include <stddef.h>
#include <stdbool.h>

#define HASHTABLE_DEFAULT_CAPACITY 64
#define HASHTABLE_DEFAULT_LOAD_FACTOR_NUMERATOR 3
#define HASHTABLE_DEFAULT_LOAD_FACTOR_DENOMINATOR 4

typedef unsigned long (* hashtable_hash_fn_t)(void *key, unsigned long bucket_count);
typedef int (* hashtable_key_compare_fn_t)(void *a, void *b);
typedef void (* hashtable_destroy_fn_t)(void *key, void *value);

struct hashtable_item {
    void *key;
    void *value;
    struct hashtable_item *next;
};

struct hashtable {
    hashtable_hash_fn_t hash_fn;
    hashtable_key_compare_fn_t key_compare_fn;
    hashtable_destroy_fn_t destroy_fn;
    unsigned long bucket_count;
    struct hashtable_item **buckets;
    size_t size;
    size_t load_factor_numerator;
    size_t load_factor_denominator;
};

struct hashtable_iterator {
    struct hashtable *hashtable;
    unsigned long bucket_index;
    struct hashtable_item *item;
};

struct hashtable *hashtable_create(hashtable_hash_fn_t hash_fn, hashtable_key_compare_fn_t key_compare_fn, hashtable_destroy_fn_t destroy_fn);
struct hashtable *hashtable_create_sized(hashtable_hash_fn_t hash_fn, hashtable_key_compare_fn_t key_compare_fn, hashtable_destroy_fn_t destroy_fn, unsigned long capacity);
void hashtable_destroy(struct hashtable *hashtable);

void hashtable_set_load_factor(size_t numerator, size_t denominator);
void hashtable_resize(size_t new_capacity);

void hashtable_set(struct hashtable *hashtable, void *key, void *value);
bool hashtable_has(struct hashtable *hashtable, void *key);
void *hashtable_get(struct hashtable *hashtable, void *key);
bool hashtable_remove(struct hashtable *hashtable, void *key);
void hashtable_clear(struct hashtable *hashtable);

void hashtable_iterator_init(struct hashtable_iterator *iterator, struct hashtable *hashtable);
void hashtable_iterator_next(struct hashtable_iterator *iterator);
bool hashtable_iterator_finished(struct hashtable_iterator *iterator);
void *hashtable_iterator_value(struct hashtable_iterator *iterator);

unsigned long hashtable_hash_string(void *key, unsigned long bucket_count);
int hashtable_compare_string(void *a, void *b);
