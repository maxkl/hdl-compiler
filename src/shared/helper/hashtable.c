
#include "hashtable.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

static struct hashtable *hashtable_create_internal(hashtable_hash_fn_t hash_fn, hashtable_key_compare_fn_t key_compare_fn, hashtable_destroy_fn_t destroy_fn, unsigned long bucket_count, size_t load_factor_numerator, size_t load_factor_denominator) {
    struct hashtable *hashtable = malloc(sizeof(struct hashtable));
    hashtable->hash_fn = hash_fn;
    hashtable->key_compare_fn = key_compare_fn;
    hashtable->destroy_fn = destroy_fn;
    hashtable->bucket_count = bucket_count;
    hashtable->buckets = calloc(bucket_count, sizeof(struct hashtable_item *));
    hashtable->size = 0;
    hashtable->load_factor_numerator = load_factor_numerator;
    hashtable->load_factor_denominator = load_factor_denominator;
    return hashtable;
}

struct hashtable *hashtable_create(hashtable_hash_fn_t hash_fn, hashtable_key_compare_fn_t key_compare_fn, hashtable_destroy_fn_t destroy_fn) {
    return hashtable_create_internal(
        hash_fn, key_compare_fn, destroy_fn,
        HASHTABLE_DEFAULT_CAPACITY,
        HASHTABLE_DEFAULT_LOAD_FACTOR_NUMERATOR,
        HASHTABLE_DEFAULT_LOAD_FACTOR_DENOMINATOR
    );
}

struct hashtable *hashtable_create_sized(hashtable_hash_fn_t hash_fn, hashtable_key_compare_fn_t key_compare_fn, hashtable_destroy_fn_t destroy_fn, unsigned long capacity) {
    return hashtable_create_internal(
        hash_fn, key_compare_fn, destroy_fn,
        capacity,
        HASHTABLE_DEFAULT_LOAD_FACTOR_NUMERATOR,
        HASHTABLE_DEFAULT_LOAD_FACTOR_DENOMINATOR
    );
}

void hashtable_destroy(struct hashtable *hashtable) {
    for (unsigned long i = 0; i < hashtable->bucket_count; i++) {
        struct hashtable_item *item = hashtable->buckets[i];
        while (item != NULL) {
            hashtable->destroy_fn(item->key, item->value);
            struct hashtable_item *next_item = item->next;
            free(item);
            item = next_item;
        }
    }
    free(hashtable->buckets);
    free(hashtable);
}

void hashtable_set_load_factor(size_t numerator, size_t denominator) {
    (void) numerator;
    (void) denominator;
}

void hashtable_resize(size_t new_capacity) {
    (void) new_capacity;
}

void hashtable_set(struct hashtable *hashtable, void *key, void *value) {
    unsigned long hash = hashtable->hash_fn(key, hashtable->bucket_count);
    if (hash >= hashtable->bucket_count) {
        return;
    }

    struct hashtable_item *bucket = hashtable->buckets[hash];
    struct hashtable_item *item = bucket;

    while (item != NULL) {
        if (hashtable->key_compare_fn(item->key, key) == 0) {
            hashtable->destroy_fn(item->key, item->value);
            item->key = key;
            item->value = value;
            return;
        }

        if (item->next == NULL) {
            break;
        } else {
            item = item->next;
        }
    }

    struct hashtable_item *new_item = malloc(sizeof(struct hashtable_item));
    new_item->key = key;
    new_item->value = value;
    new_item->next = NULL;

    if (bucket == NULL) {
        hashtable->buckets[hash] = new_item;
    } else {
        item->next = new_item;
    }

    hashtable->size++;
}

bool hashtable_has(struct hashtable *hashtable, void *key) {
    unsigned long hash = hashtable->hash_fn(key, hashtable->bucket_count);
    if (hash >= hashtable->bucket_count) {
        return false;
    }

    struct hashtable_item *item = hashtable->buckets[hash];

    while (item != NULL) {
        if (hashtable->key_compare_fn(item->key, key) == 0) {
            return true;
        }
        item = item->next;
    }

    return false;
}

void *hashtable_get(struct hashtable *hashtable, void *key) {
    unsigned long hash = hashtable->hash_fn(key, hashtable->bucket_count);
    if (hash >= hashtable->bucket_count) {
        return NULL;
    }

    struct hashtable_item *item = hashtable->buckets[hash];

    while (item != NULL) {
        if (hashtable->key_compare_fn(item->key, key) == 0) {
            return item->value;
        }
        item = item->next;
    }

    return NULL;
}

bool hashtable_remove(struct hashtable *hashtable, void *key) {
    unsigned long hash = hashtable->hash_fn(key, hashtable->bucket_count);
    if (hash >= hashtable->bucket_count) {
        return false;
    }

    struct hashtable_item *item = hashtable->buckets[hash];
    struct hashtable_item *prev_item = NULL;

    while (item != NULL) {
        if (hashtable->key_compare_fn(item->key, key) == 0) {
            if (prev_item == NULL) {
                hashtable->buckets[hash] = item->next;
            } else {
                prev_item->next = item->next;
            }
            hashtable->destroy_fn(item->key, item->value);
            free(item);
            hashtable->size--;
            return true;
        }
        prev_item = item;
        item = item->next;
    }

    return false;
}

void hashtable_iterator_init(struct hashtable_iterator *iterator, struct hashtable *hashtable) {
    iterator->hashtable = hashtable;
    iterator->bucket_index = 0;
    iterator->item = hashtable->buckets[0];

    // Find first item
    while (iterator->item == NULL) {
        iterator->bucket_index++;

        // Terminate when we reach the end of the list
        if (iterator->bucket_index >= hashtable->bucket_count) {
            iterator->item = NULL;
            break;
        }

        iterator->item = hashtable->buckets[iterator->bucket_index];
    }
}

void hashtable_iterator_next(struct hashtable_iterator *iterator) {
    struct hashtable *hashtable = iterator->hashtable;

    if (iterator->item == NULL) {
        return;
    }

    // Advance iterator
    iterator->item = iterator->item->next;

    // Find next item
    while (iterator->item == NULL) {
        iterator->bucket_index++;

        // Terminate when we reach the end of the list
        if (iterator->bucket_index >= hashtable->bucket_count) {
            iterator->item = NULL;
            break;
        }

        iterator->item = hashtable->buckets[iterator->bucket_index];
    }
}

bool hashtable_iterator_finished(struct hashtable_iterator *iterator) {
    return iterator->item == NULL;
}

void *hashtable_iterator_value(struct hashtable_iterator *iterator) {
    if (iterator->item == NULL) {
        return NULL;
    }

    return iterator->item->value;
}

unsigned long hashtable_hash_string(void *key, unsigned long bucket_count) {
    char *key_str = key;

    // Jenkins one-at-a-time hash
    uint32_t hash = 0;
    for (uint32_t i = 0; key_str[i] != '\0'; i++) {
        hash += key_str[i];
        hash += hash << 10;
        hash ^= hash >> 6;
    }
    hash += hash << 3;
    hash ^= hash >> 11;
    hash += hash << 15;

    return hash % bucket_count;
}

int hashtable_compare_string(void *a, void *b) {
    return strcmp(a, b);
}
