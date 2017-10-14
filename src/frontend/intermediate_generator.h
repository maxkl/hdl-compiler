
#pragma once

#include "ast.h"
#include <shared/intermediate.h>

int intermediate_generator_generate(struct ast_node *ast_root, struct intermediate_block ***blocks, size_t *block_count);
