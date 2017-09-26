
#include "mem.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

static void die_oom() {
    fprintf(stderr, "Out of memory\n");
    fflush(stderr);
    abort();
}

static void handle_oom() {
    die_oom();
}

void *xmalloc(size_t size) {
    assert(size > 0);
    void *ptr = malloc(size);
    if (ptr == NULL) {
        handle_oom();
        ptr = malloc(size);
        if (ptr == NULL) {
            die_oom();
            // die_oom() will never return
        }
    }
    return ptr;
}

void *xcalloc(size_t count, size_t size) {
    assert(count > 0);
    assert(size > 0);
    void *ptr = calloc(count, size);
    if (ptr == NULL) {
        handle_oom();
        ptr = calloc(count, size);
        if (ptr == NULL) {
            die_oom();
            // die_oom() will never return
        }
    }
    return ptr;
}

void *xrealloc(void *old_ptr, size_t size) {
    void *ptr = realloc(old_ptr, size);
    if (size != 0 && ptr == NULL) {
        handle_oom();
        ptr = realloc(old_ptr, size);
        if (ptr == NULL) {
            die_oom();
            // die_oom() will never return
        }
    }
    return ptr;
}

void xfree(void *ptr) {
    free(ptr);
}

char *xstrdup(const char *str) {
    assert(str != NULL);
    char *new_str = strdup(str);
    if (new_str == NULL) {
        handle_oom();
        new_str = strdup(str);
        if (new_str == NULL) {
            die_oom();
            // die_oom() will never return
        }
    }
    return new_str;
}