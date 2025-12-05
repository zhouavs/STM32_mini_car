#pragma once

#include <stdbool.h>
#include <stdint.h>

bool str_to_uint32(char *str, uint32_t *rt_num_ptr, char **str_num_end_ptr);
