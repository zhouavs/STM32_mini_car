#pragma once

#include <stdint.h>
#include "common/errno/errno.h"

errno_t delay_s(uint32_t s);
errno_t delay_ms(uint32_t ms);
errno_t delay_us(uint32_t us);
