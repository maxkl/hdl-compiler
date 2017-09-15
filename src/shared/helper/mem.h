
#pragma once

#include <stdlib.h>

void *xmalloc(size_t size);
void *xcalloc(size_t count, size_t size);
void *xrealloc(void *ptr, size_t size);
void xfree(void *ptr);
