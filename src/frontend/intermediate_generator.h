
#pragma once

#include "ast.h"
#include <shared/intermediate.h>
#include <shared/intermediate_file.h>

int intermediate_generator_generate(struct ast_node *ast_root, struct intermediate_file *intermediate_file);
