
#include "expression_type.h"

bool expression_type_matches(struct expression_type *a, struct expression_type *b) {
	return a->access_type == b->access_type && a->width == b->width;
}

void expression_type_copy(struct expression_type *target, struct expression_type *source) {
	target->access_type = source->access_type;
	target->width = source->width;
}
