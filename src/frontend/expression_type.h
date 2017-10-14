
#pragma once

#include <stdint.h>
#include <stdbool.h>

enum expression_type_access_type {
	EXPRESSION_TYPE_READ,
	EXPRESSION_TYPE_WRITE
};

struct expression_type {
	enum expression_type_access_type access_type;
	uint64_t width;
};

bool expression_type_matches(struct expression_type *a, struct expression_type *b);
void expression_type_copy(struct expression_type *target, struct expression_type *source);
