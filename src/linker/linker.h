
#pragma once

#include <stdlib.h>

#include <shared/intermediate.h>
#include <shared/intermediate_file.h>

int linker_link(struct intermediate_file *output, struct intermediate_file *inputs, size_t input_count);
